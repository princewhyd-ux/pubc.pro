#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>

// (لضمان الاستقلالية في الاستدعاء إذا لم يتم استدعاء MapManager مسبقاً)
#ifndef MAP_MANAGER_H
enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };
struct EnvironmentCollider { BoundingBox bounds; ColliderType type; std::string tag; };
#endif

class CapsuleCollider {
public:
    Vector3 position; // موقع قاعدة الكبسولة (القدمين)
    float height;     // الارتفاع الإجمالي
    float radius;     // نصف القطر (عرض الكائن)

    CapsuleCollider(Vector3 basePosition = {0,0,0}, float totalHeight = 1.74f, float capRadius = 0.4f) {
        position = basePosition;
        height = totalHeight;
        radius = capRadius;
    }

    /**
     * @brief تحديث أبعاد الكبسولة ديناميكياً (مفيد جداً عند الانحناء Crouch أو الانبطاح Prone)
     */
    void UpdateDimensions(float newHeight, float newRadius) {
        height = newHeight;
        radius = newRadius;
    }

    /**
     * @brief تحديث موقع الكبسولة ليتطابق مع اللاعب أو العدو
     */
    void UpdatePosition(Vector3 newPosition) {
        position = newPosition;
    }

    /**
     * @brief فحص اصطدام الكبسولة مع بيئة اللعبة بشكل محكم وبدون فراغات
     * @param testPosition الموقع المراد فحصه (نمرر موقعاً مستقبلياً لمعرفة هل سيصطدم قبل أن يتحرك فعلياً)
     * @param colliders قائمة جدران وأرضيات العالم
     */
    bool CollidesWith(Vector3 testPosition, const std::vector<EnvironmentCollider>& colliders) const {
        // 1. حساب المساحة الفعالة للكبسولة (نطرح نصف القطر من الأعلى والأسفل)
        float effectiveHeight = std::max(0.0f, height - (radius * 2.0f));
        
        // 2. الحساب الذكي لعدد الكرات: 
        // نحسب كم كرة نحتاج لتغطية الكبسولة بالكامل بناءً على نصف القطر (لن يمر جدار من بين الكرات أبداً)
        int sphereCount = std::max(2, (int)std::ceil(effectiveHeight / radius) + 1);
        
        // 3. مسافة التباعد بين كل كرة وأخرى
        float step = effectiveHeight / std::max(1, sphereCount - 1);

        // 4. فحص التصادم
        for (const auto& col : colliders) {
            // نتجاهل المناطق الوهمية (مثل دائرة تفعيل الباب)
            if (col.type == ColliderType::TRIGGER_ZONE) continue;

            for (int i = 0; i < sphereCount; i++) {
                // مركز الكرة الحالية متجهة للأعلى من القدمين وحتى الرأس
                Vector3 sphereCenter = { 
                    testPosition.x, 
                    testPosition.y + radius + (i * step), 
                    testPosition.z 
                };
                
                // فحص التصادم باستخدام دالة Raylib المدمجة السريعة جداً
                if (CheckCollisionBoxSphere(col.bounds, sphereCenter, radius)) {
                    return true; // وجدنا تصادم! (نخرج فوراً لتوفير الأداء)
                }
            }
        }
        return false; // المسار آمن
    }

    /**
     * @brief إيجاد ارتفاع الأرضية بدقة تحت الكبسولة (لمحاكاة السلالم والمنحدرات)
     * @param stepOffset أقصى ارتفاع يمكن للكبسولة صعوده بدون قفز (مثل رصيف الشارع)
     */
    float GetGroundHeight(const std::vector<EnvironmentCollider>& colliders, float stepOffset = 0.3f) const {
        float highestGround = -9999.0f; // قيمة ابتدائية تمثل الهاوية

        for (const auto& col : colliders) {
            if (col.type == ColliderType::STATIC_WALL) continue; // الجدار لا يعتبر أرضية

            // فحص ما إذا كانت الكبسولة تقع فوق هذا المجسم (في محوري X و Z)
            // قمنا بتوسيع الفحص بمقدار نصف القطر (radius) حتى لا يسقط اللاعب إذا وقف على حافة الهاوية بنصف قدمه!
            if (position.x + radius >= col.bounds.min.x && position.x - radius <= col.bounds.max.x &&
                position.z + radius >= col.bounds.min.z && position.z - radius <= col.bounds.max.z) {
                
                // نأخذ أعلى نقطة للمجسم بشرط ألا تكون أعلى بكثير من قدم اللاعب (أعلى من stepOffset)
                if (col.bounds.max.y > highestGround && col.bounds.max.y <= position.y + stepOffset + 0.1f) {
                    highestGround = col.bounds.max.y;
                }
            }
        }
        
        // إذا لم يجد أرضية، نرجع رقم سالب ليسقط الكائن بفعل الجاذبية
        return (highestGround == -9999.0f) ? -100.0f : highestGround;
    }
};
