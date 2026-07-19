#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

// (لضمان الاستقلالية في الاستدعاء)
#ifndef MAP_MANAGER_H
enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };
struct EnvironmentCollider { BoundingBox bounds; ColliderType type; std::string tag; };
#endif

class PlayerMovement {
public:
    // الإحداثيات الأساسية
    Vector3 position;
    Vector3 velocity;
    float rotationAngle;
    bool isGrounded;

    // إعدادات السرعة (متر/ثانية)
    float walkSpeed = 3.5f;
    float runSpeed = 7.0f;
    float crouchSpeed = 2.0f;
    float proneSpeed = 1.0f;

    // إعدادات التسارع والتحكم
    float acceleration = 25.0f;
    float deceleration = 30.0f;
    float rotationSpeed = 15.0f;
    
    // إعدادات الفيزياء
    float jumpForce = 7.0f;
    float gravity = 20.0f;
    float maxFallSpeed = -45.0f;
    
    // إعدادات الكبسولة (Capsule Collision) وصعود الدرج
    float radius = 0.4f;       // عرض اللاعب
    float currentHeight = 1.74f; // ارتفاع اللاعب (يتغير عند الانحناء)
    float stepOffset = 0.3f;   // أقصى ارتفاع لدرجة السلم يمكن صعودها بدون قفز

    PlayerMovement(Vector3 startPos) {
        position = startPos;
        velocity = { 0.0f, 0.0f, 0.0f };
        rotationAngle = 0.0f;
        isGrounded = false;
    }

    /**
     * @brief حساب الحركة بناءً على إدخال اللاعب وزاوية الكاميرا
     * @param inputDir اتجاه الإدخال (X يمين/يسار, Y أمام/خلف) من الجويستيك أو الكيبورد
     * @param targetSpeed السرعة المطلوبة بناءً على حالة اللاعب (مشي، ركض...)
     * @param camYaw زاوية الكاميرا الحالية لضمان أن "الأمام" هو ما تنظر إليه الكاميرا
     */
    void CalculateMovement(Vector2 inputDir, float targetSpeed, float camYaw, float dt) {
        // إذا كان اللاعب في الهواء، نقلل من سيطرته على الحركة (Air Control)
        float controlMultiplier = isGrounded ? 1.0f : 0.3f;
        float currentAccel = acceleration * controlMultiplier;
        float currentDecel = deceleration * controlMultiplier;

        float targetSpeedX = 0.0f;
        float targetSpeedZ = 0.0f;

        // التحقق مما إذا كان هناك إدخال حقيقي
        float inputLength = Vector2Length(inputDir);
        
        if (inputLength > 0.01f) {
            // 1. حساب الاتجاه بالنسبة للكاميرا (Camera-Relative Movement)
            Vector3 forward = Vector3Normalize({ sinf(camYaw), 0.0f, cosf(camYaw) });
            Vector3 right = Vector3Normalize({ forward.z, 0.0f, -forward.x });

            // 2. دمج الإدخال مع اتجاه الكاميرا
            Vector3 worldMove = Vector3Normalize(Vector3Add(
                Vector3Scale(right, inputDir.x), 
                Vector3Scale(forward, inputDir.y)
            ));

            targetSpeedX = worldMove.x * targetSpeed * inputLength;
            targetSpeedZ = worldMove.z * targetSpeed * inputLength;

            // 3. تدوير جسد اللاعب بسلاسة نحو اتجاه الحركة
            float targetAngle = atan2f(-worldMove.x, -worldMove.z) * RAD2DEG + 180.0f;
            
            // حساب أقصر طريق للدوران (يمنع اللاعب من الدوران دورة كاملة بشكل غريب)
            float diff = targetAngle - rotationAngle;
            while (diff < -180.0f) diff += 360.0f;
            while (diff >  180.0f) diff -= 360.0f;
            
            rotationAngle += diff * rotationSpeed * dt;
            rotationAngle = fmodf(rotationAngle, 360.0f);
        }

        // 4. تطبيق التسارع (Lerp) للوصول للسرعة المطلوبة بنعومة
        float rate = (inputLength > 0.01f) ? currentAccel : currentDecel;
        velocity.x = Lerp(velocity.x, targetSpeedX, rate * dt);
        velocity.z = Lerp(velocity.z, targetSpeedZ, rate * dt);
    }

    /**
     * @brief تطبيق الجاذبية وحساب التصادم المستمر (Kinematic Collision Resolution)
     */
    void UpdatePhysics(float dt, const std::vector<EnvironmentCollider>& colliders) {
        // تطبيق الجاذبية
        velocity.y -= gravity * dt;
        if (velocity.y < maxFallSpeed) velocity.y = maxFallSpeed;

        Vector3 nextPos = position;

        // 1. معالجة التصادم في محور X (تسمح بالانزلاق على الجدران)
        nextPos.x += velocity.x * dt;
        if (CheckCapsuleCollision(nextPos, colliders)) {
            // محاولة صعود درجة سلم (Step Offset)
            Vector3 stepTestPos = position;
            stepTestPos.y += stepOffset;
            stepTestPos.x += velocity.x * dt;
            
            if (!CheckCapsuleCollision(stepTestPos, colliders)) {
                nextPos = stepTestPos; // صعود السلم ناجح
            } else {
                nextPos.x = position.x; // اصطدام بجدار صلب، إيقاف الحركة في X
                velocity.x = 0.0f;
            }
        }

        // 2. معالجة التصادم في محور Z (تسمح بالانزلاق على الجدران)
        nextPos.z += velocity.z * dt;
        if (CheckCapsuleCollision(nextPos, colliders)) {
            // محاولة صعود درجة سلم
            Vector3 stepTestPos = position;
            stepTestPos.y += stepOffset;
            stepTestPos.z += velocity.z * dt;
            
            if (!CheckCapsuleCollision(stepTestPos, colliders)) {
                nextPos = stepTestPos;
            } else {
                nextPos.z = position.z; // اصطدام بجدار، إيقاف الحركة في Z
                velocity.z = 0.0f;
            }
        }

        // 3. معالجة محور Y وتحديد الأرضية (Grounded Check)
        nextPos.y += velocity.y * dt;
        
        // فحص مخصص للأرضية أسفل اللاعب مباشرة بدقة متناهية
        float groundHeight = GetGroundHeight(nextPos, colliders);

        if (nextPos.y <= groundHeight + 0.01f) { // 0.01 هامش خطأ بسيط
            nextPos.y = groundHeight;
            if (velocity.y < 0.0f) velocity.y = 0.0f; // إيقاف الجاذبية
            isGrounded = true;
        } else {
            // التأكد من عدم اختراق السقف عند القفز
            if (velocity.y > 0.0f && CheckCapsuleCollision(nextPos, colliders)) {
                nextPos.y = position.y;
                velocity.y = 0.0f; // ضرب رأسه بالسقف
            }
            isGrounded = false;
        }

        // تحديث الموقع النهائي
        position = nextPos;
    }

    // دالة القفز
    void Jump() {
        if (isGrounded) {
            velocity.y = jumpForce;
            isGrounded = false;
        }
    }

private:
    /**
     * @brief محاكاة اصطدام الكبسولة (Capsule Collision) عبر 3 دوائر وهمية (سفلى، وسطى، عليا)
     * تضمن هذه الطريقة عدم اختراق رأس أو قدم أو وسط اللاعب للجدران.
     */
    bool CheckCapsuleCollision(Vector3 checkPos, const std::vector<EnvironmentCollider>& colliders) const {
        Vector3 bottom = { checkPos.x, checkPos.y + radius, checkPos.z };
        Vector3 middle = { checkPos.x, checkPos.y + (currentHeight / 2.0f), checkPos.z };
        Vector3 top    = { checkPos.x, checkPos.y + currentHeight - radius, checkPos.z };

        for (const auto& col : colliders) {
            if (col.type == ColliderType::TRIGGER_ZONE) continue; // نتجاهل مناطق الزناد

            if (CheckCollisionBoxSphere(col.bounds, bottom, radius) || 
                CheckCollisionBoxSphere(col.bounds, middle, radius) || 
                CheckCollisionBoxSphere(col.bounds, top, radius)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief العثور على ارتفاع الأرض تحت اللاعب بدقة لدعم المنحدرات (Slopes)
     */
    float GetGroundHeight(Vector3 checkPos, const std::vector<EnvironmentCollider>& colliders) const {
        float highestGround = -9999.0f; // رقم صغير جداً للبدء

        for (const auto& col : colliders) {
            if (col.type == ColliderType::STATIC_WALL) continue;

            // إذا كان اللاعب يقف ضمن حدود هذا المجسم (على المحورين X و Z)
            if (checkPos.x >= col.bounds.min.x && checkPos.x <= col.bounds.max.x &&
                checkPos.z >= col.bounds.min.z && checkPos.z <= col.bounds.max.z) {
                
                // نأخذ أعلى نقطة للمجسم أسفل اللاعب كأرضية
                if (col.bounds.max.y > highestGround && col.bounds.max.y <= checkPos.y + stepOffset) {
                    highestGround = col.bounds.max.y;
                }
            }
        }
        
        // إذا لم يجد أرضية (حافة الهاوية)، نرجع رقم صغير جداً ليسقط اللاعب
        return (highestGround == -9999.0f) ? -100.0f : highestGround;
    }
};
