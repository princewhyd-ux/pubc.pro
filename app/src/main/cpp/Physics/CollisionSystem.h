#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>

// تعريف أنواع المجسمات لتسهيل التعامل معها
#ifndef MAP_MANAGER_H
enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };
struct EnvironmentCollider { BoundingBox bounds; ColliderType type; std::string tag; };
#endif

// هيكل يمثل الكائنات المتحركة (اللاعب، الأعداء، السيارات)
struct DynamicCollider {
    Vector3* position; // مؤشر لموقع الكائن الأصلي لكي نتمكن من تعديله مباشرة (دفعه)
    float radius;      // حجم الكائن
    bool isStatic;     // هل هو ثابت حالياً؟ (مثل سيارة متوقفة لا تتأثر بالدفع)
};

class CollisionSystem {
private:
    std::vector<EnvironmentCollider> staticColliders;  // جدران الخريطة (لا تتحرك أبداً)
    std::vector<DynamicCollider> dynamicColliders;     // الكائنات المتحركة (الأعداء واللاعبين)

public:
    CollisionSystem() = default;

    // ==========================================
    // 1. إدارة الخريطة (Static Environment)
    // ==========================================

    // إضافة جدار أو أرضية جديدة للنظام
    void AddStaticCollider(BoundingBox bounds, ColliderType type, std::string tag = "") {
        staticColliders.push_back({ bounds, type, tag });
    }

    // استرجاع كل الجدران (للتوافق مع الدوال القديمة مثل Raycaster)
    const std::vector<EnvironmentCollider>& GetAllStaticColliders() const {
        return staticColliders;
    }

    // مسح الخريطة عند الانتقال لمرحلة أخرى
    void ClearStaticColliders() {
        staticColliders.clear();
    }

    /**
     * @brief [أهم دالة للأداء] الحصول على المجسمات القريبة فقط بدلاً من فحص الخريطة كلها
     * @param center موقع اللاعب أو الكاميرا
     * @param range دائرة البحث (مثلاً 10 أمتار)
     */
    std::vector<EnvironmentCollider> GetNearbyColliders(Vector3 center, float range) const {
        std::vector<EnvironmentCollider> nearby;
        // احتياطي لتجنب إعادة حجز الذاكرة بشكل مستمر
        nearby.reserve(20); 

        for (const auto& col : staticColliders) {
            // نتحقق إذا كان الصندوق المحيط يقع ضمن دائرة البحث
            if (CheckCollisionBoxSphere(col.bounds, center, range)) {
                nearby.push_back(col);
            }
        }
        return nearby; // إرجاع 5 جدران أفضل بكثير من إرجاع 1000!
    }

    // ==========================================
    // 2. إدارة الكائنات المتحركة (Dynamic Entities)
    // ==========================================

    // تسجيل لاعب أو عدو أو سيارة في النظام الفيزيائي
    void RegisterDynamicCollider(Vector3* posRef, float radius, bool isStatic = false) {
        dynamicColliders.push_back({ posRef, radius, isStatic });
    }

    // تنظيف الكائنات المتحركة (مثلاً عند موت الأعداء)
    void ClearDynamicColliders() {
        dynamicColliders.clear();
    }

    /**
     * @brief تحديث فيزياء التدافع (يُستدعى كل إطار لمنع الكائنات من الدخول في بعضها)
     */
    void ResolveDynamicOverlaps() {
        int count = dynamicColliders.size();
        if (count < 2) return; // لا يوجد كائنان ليصطدما

        // مقارنة كل كائن بباقي الكائنات (O(N^2) لكن العدد عادة صغير جداً < 50)
        for (int i = 0; i < count; i++) {
            for (int j = i + 1; j < count; j++) {
                
                DynamicCollider& a = dynamicColliders[i];
                DynamicCollider& b = dynamicColliders[j];

                // حساب المسافة بين مركز الكائنين (نتجاهل الـ Y لكي لا يتدافعوا في السلالم)
                Vector2 posA = { a.position->x, a.position->z };
                Vector2 posB = { b.position->x, b.position->z };
                
                float distance = Vector2Distance(posA, posB);
                float minSafeDistance = a.radius + b.radius;

                // إذا كانت المسافة أصغر من مجموع أنصاف الأقطار، فهما متداخلان (Overlap)!
                if (distance < minSafeDistance) {
                    
                    // حساب مقدار التداخل (كم متر دخلا في بعضهما؟)
                    float overlap = minSafeDistance - distance;
                    
                    // حساب اتجاه الدفع
                    Vector2 pushDir = { 1.0f, 0.0f }; // اتجاه افتراضي إذا كانا في نفس النقطة تماماً
                    if (distance > 0.001f) {
                        pushDir = Vector2Normalize(Vector2Subtract(posA, posB));
                    }

                    // تطبيق الدفع لإبعاد الكائنين عن بعضهما
                    if (!a.isStatic && !b.isStatic) {
                        // كلاهما متحرك: ندفع كل واحد منهما نصف المسافة للخلف
                        Vector2 pushForce = Vector2Scale(pushDir, overlap * 0.5f);
                        
                        a.position->x += pushForce.x;
                        a.position->z += pushForce.y;
                        
                        b.position->x -= pushForce.x;
                        b.position->z -= pushForce.y;
                    } 
                    else if (!a.isStatic && b.isStatic) {
                        // b ثابت (مثل سيارة متوقفة)، ندفع a فقط بالمسافة كاملة
                        Vector2 pushForce = Vector2Scale(pushDir, overlap);
                        a.position->x += pushForce.x;
                        a.position->z += pushForce.y;
                    }
                    else if (a.isStatic && !b.isStatic) {
                        // a ثابت، ندفع b فقط بالمسافة كاملة
                        Vector2 pushForce = Vector2Scale(pushDir, overlap);
                        b.position->x -= pushForce.x;
                        b.position->z -= pushForce.y;
                    }
                }
            }
        }
    }
};
