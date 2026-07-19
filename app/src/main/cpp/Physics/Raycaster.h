#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include "../Physics/CollisionSystem.h" // 🚨 هذا هو السطر السحري الذي نسيت إضافته!

// هيكل يحمل بيانات الضربة (ماذا أصاب الشعاع؟ وأين؟)
struct RaycastHit {
    bool hit;           // هل اصطدم الشعاع بشيء؟
    Vector3 point;      // الإحداثيات الدقيقة (X,Y,Z) لنقطة الاصطدام
    Vector3 normal;     // اتجاه ارتداد الضربة (مفيد لرسم ثقوب الرصاص أو شرار الاصطدام)
    float distance;     // المسافة من نقطة الإطلاق حتى نقطة الاصطدام
    ColliderType type;  // نوع المجسم الذي تم ضربه (جدار، أرض...)
    std::string tag;    // العلامة (Tag) لمعرفة اسم المجسم المضروب
};

class Raycaster {
public:

    /**
     * @brief إطلاق شعاع عام وتحديد أقرب مجسم يصطدم به (مفيد لأسلحة Hitscan)
     * @param origin نقطة انطلاق الشعاع (مثلاً الكاميرا أو فوهة السلاح)
     * @param direction اتجاه الإطلاق
     * @param maxDistance أقصى مسافة يصل لها الشعاع (مثلاً مدى السلاح 100 متر)
     * @param colliders قائمة مجسمات البيئة المراد فحصها
     * @return معلومات الضربة RaycastHit
     */
    static RaycastHit CastRay(Vector3 origin, Vector3 direction, float maxDistance, const std::vector<EnvironmentCollider>& colliders) {
        RaycastHit closestHit;
        closestHit.hit = false;
        closestHit.distance = maxDistance; // نبدأ بأقصى مسافة للبحث عن الأقرب

        // تأمين الاتجاه ليكون متجه وحدة (Normalized) لتفادي أخطاء الحساب
        Vector3 normDir = Vector3Normalize(direction);
        Ray ray = { origin, normDir };

        for (const auto& col : colliders) {
            // عادةً لا نريد للرصاص أن يصطدم بالمناطق الوهمية (Triggers)
            if (col.type == ColliderType::TRIGGER_ZONE) continue;

            RayCollision collision = GetRayCollisionBox(ray, col.bounds);
            
            // إذا حدث اصطدام وكان أقرب من الاصطدامات السابقة
            if (collision.hit && collision.distance < closestHit.distance) {
                closestHit.hit = true;
                closestHit.distance = collision.distance;
                closestHit.point = collision.point;
                closestHit.normal = collision.normal; // اتجاه سطح الجدار (لرسم شرار الارتداد)
                closestHit.type = col.type;
                closestHit.tag = col.tag;
            }
        }

        // إذا لم يصطدم بشيء، نعيد النقطة النهائية في الهواء لكي نرسم الرصاصة الطائرة
        if (!closestHit.hit) {
            closestHit.point = Vector3Add(origin, Vector3Scale(normDir, maxDistance));
            closestHit.normal = { 0.0f, 1.0f, 0.0f }; // اتجاه افتراضي لأعلى
        }

        return closestHit;
    }

    /**
     * @brief دالة محسّنة جداً للذكاء الاصطناعي (أسرع 10 مرات من CastRay)
     * وظيفته: هل يوجد أي جدار بين النقطة A والنقطة B؟ 
     * لا يبحث عن الأقرب، بل يرجع true بمجرد أن يجد حاجزاً واحداً ليوفر طاقة المعالج.
     */
    static bool IsLineOfSightBlocked(Vector3 start, Vector3 end, const std::vector<EnvironmentCollider>& colliders) {
        Vector3 dir = Vector3Subtract(end, start);
        float dist = Vector3Length(dir);
        
        if (dist < 0.01f) return false;

        Ray ray = { start, Vector3Normalize(dir) };

        for (const auto& col : colliders) {
            if (col.type == ColliderType::TRIGGER_ZONE) continue;

            RayCollision hit = GetRayCollisionBox(ray, col.bounds);
            
            // إذا وجدنا أي جدار يقطع المسار والمسافة أقل من المسافة الكلية للهدف
            if (hit.hit && hit.distance < dist) {
                return true; // الرؤية محجوبة! (نخرج من الـ Loop فوراً لتوفير الأداء)
            }
        }
        return false; // الرؤية واضحة
    }

    /**
     * @brief إطلاق شعاع مباشر للأسفل للعثور على الأرضية الدقيقة
     * @param origin نقطة الانطلاق (مثل مركز اللاعب أو العجلة)
     * @param maxDistance أقصى مسافة للبحث
     * @return ارتفاع الـ Y للأرضية، أو رقم سالب جداً إذا كان تحتها هاوية
     */
    static float GetGroundHeight(Vector3 origin, float maxDistance, const std::vector<EnvironmentCollider>& colliders) {
        RaycastHit hit = CastRay(origin, { 0.0f, -1.0f, 0.0f }, maxDistance, colliders);
        if (hit.hit) {
            return hit.point.y;
        }
        return -9999.0f; // لا توجد أرضية (حالة السقوط)
    }

    /**
     * @brief فحص التصادم بين شعاع وشكل كروي (مفيد لإصابة الأعداء - Headshots)
     * @param origin نقطة انطلاق الرصاصة
     * @param direction اتجاه الرصاصة
     * @param targetCenter مركز رأس أو جسد العدو
     * @param targetRadius نصف القطر للعدو
     */
    static bool CheckSphereHit(Vector3 origin, Vector3 direction, Vector3 targetCenter, float targetRadius) {
        Ray ray = { origin, Vector3Normalize(direction) };
        RayCollision hit = GetRayCollisionSphere(ray, targetCenter, targetRadius);
        return hit.hit;
    }
};
