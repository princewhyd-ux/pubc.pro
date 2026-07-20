#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "../Physics/CollisionSystem.h" // 🚨 الاستدعاء الصحيح لنظام التصادم بدلاً من التكرار

class PlayerCamera {
public:
    Camera3D camera;
    
    // إعدادات موقع الكاميرا بالنسبة للاعب (Shoulder Offset)
    // X: يمين/يسار، Y: أعلى/أسفل (مستوى الرأس)، Z: المسافة للخلف
    Vector3 shoulderOffset = { 0.6f, 1.5f, 2.5f }; 
    
    // موقع الكاميرا الفعلي (يستخدم للنعومة Smooth Follow)
    Vector3 currentPosition;
    Vector3 currentHeadPos;

    // زوايا الدوران (Pitch للرأس، Yaw للالتفاف)
    float pitch = 0.0f;
    float yaw = 0.0f;
    
    // حدود دوران الكاميرا لمنع انقلابها (Gimbal Lock)
    float minPitch = -80.0f * DEG2RAD;
    float maxPitch =  80.0f * DEG2RAD;
    
    // إعدادات النعومة (Lag/Smoothness)
    float positionSmoothness = 15.0f;
    float headFollowSmoothness = 20.0f;

    // نظام الاهتزاز (Trauma Model) للانفجارات والارتداد (Recoil)
    float trauma = 0.0f;
    float traumaDecay = 1.5f; // سرعة اختفاء الاهتزاز
    float maxShakeOffset = 0.2f; // أقصى إزاحة للاهتزاز
    float maxShakeAngle = 5.0f * DEG2RAD; // أقصى زاوية اهتزاز

    // مجال الرؤية (Dynamic FOV)
    float currentFOV = 75.0f;
    float targetFOV = 75.0f;
    float fovSmoothness = 8.0f;

    PlayerCamera() {
        camera = { 0 };
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = currentFOV;
        camera.projection = CAMERA_PERSPECTIVE;
        currentPosition = { 0.0f, 0.0f, 0.0f };
        currentHeadPos = { 0.0f, 0.0f, 0.0f };
    }

    /**
     * @brief إضافة صدمة للكاميرا (لصنع اهتزاز واقعي)
     * @param amount مقدار الصدمة (مثال: 0.2 لرصاصة، 1.0 لانفجار)
     */
    void AddTrauma(float amount) {
        trauma = Clamp(trauma + amount, 0.0f, 1.0f);
    }

    /**
     * @brief استقبال حركة الماوس أو اللمس من نظام الإدخال الرئيسي
     */
    void AddYawPitch(float deltaYaw, float deltaPitch, float sensitivity = 0.003f) {
        yaw -= deltaYaw * sensitivity;
        pitch -= deltaPitch * sensitivity;
        
        // تقييد زاوية النظر لأعلى وأسفل
        pitch = Clamp(pitch, minPitch, maxPitch);
        
        // الحفاظ على زاوية Yaw بين -PI و PI
        yaw = fmodf(yaw, PI * 2.0f);
    }

    /**
     * @brief التحديث الرئيسي لفيزياء وموقع الكاميرا
     * @param playerPos موقع اللاعب الحالي
     * @param isCrouching هل اللاعب منحني؟ (لخفض الكاميرا)
     */
    void Update(Vector3 playerPos, bool isCrouching, float dt, const std::vector<EnvironmentCollider>& colliders) {
        
        // 1. حساب مستوى رأس اللاعب بناءً على حالته (انحناء أو وقوف)
        float targetHeadHeight = isCrouching ? (shoulderOffset.y * 0.6f) : shoulderOffset.y;
        Vector3 targetHeadPos = { playerPos.x, playerPos.y + targetHeadHeight, playerPos.z };
        
        // 🚨 [تم التعديل هنا] تتبع سلس لحركة رأس اللاعب وحمايته من بطء الإطار الأول
        float headLerpAmount = fminf(headFollowSmoothness * dt, 1.0f);
        currentHeadPos = Vector3Lerp(currentHeadPos, targetHeadPos, headLerpAmount);

        // 2. حساب مصفوفة الدوران (Rotation Matrix)
        Matrix rotation = MatrixRotateXYZ({ pitch, yaw, 0.0f });
        
        // 3. حساب متجهات الاتجاه (Forward, Right, Up) من مصفوفة الدوران
        Vector3 forward = Vector3Transform({ 0.0f, 0.0f, -1.0f }, rotation);
        Vector3 right   = Vector3Transform({ 1.0f, 0.0f,  0.0f }, rotation);
        Vector3 up      = Vector3Transform({ 0.0f, 1.0f,  0.0f }, rotation);

        // 4. حساب الموقع المطلوب للكاميرا (خلف ويمين اللاعب)
        Vector3 offset = Vector3Add(Vector3Scale(forward, -shoulderOffset.z), Vector3Scale(right, shoulderOffset.x));
        Vector3 desiredPosition = Vector3Add(currentHeadPos, offset);

        // 5. نظام الاصطدام (Camera Collision) لمنع اختراق الجدران
        Vector3 safePosition = HandleCollision(currentHeadPos, desiredPosition, colliders);

        // 🚨 [تم التعديل هنا] النعومة في الحركة (Camera Lag) وحمايتها
        float posLerpAmount = fminf(positionSmoothness * dt, 1.0f);
        currentPosition = Vector3Lerp(currentPosition, safePosition, posLerpAmount);

        // 7. تطبيق نظام الاهتزاز (Trauma / Shake)
        Vector3 finalPos = currentPosition;
        Vector3 targetLookPos = Vector3Add(currentHeadPos, Vector3Scale(forward, 10.0f)); // ننظر للأمام دائماً
        
        if (trauma > 0.01f) {
            // استخدام القوة التربيعية للصدمة يجعلها قوية جداً في البداية وتتلاشى بنعومة (Trauma^2)
            float shake = trauma * trauma; 
            
            // إزاحة عشوائية للموقع (Translation Shake)
            finalPos.x += maxShakeOffset * shake * GetRandomFloat(-1.0f, 1.0f);
            finalPos.y += maxShakeOffset * shake * GetRandomFloat(-1.0f, 1.0f);
            finalPos.z += maxShakeOffset * shake * GetRandomFloat(-1.0f, 1.0f);
            
            // إزاحة عشوائية لنقطة النظر (Rotation Shake)
            targetLookPos.x += maxShakeOffset * 2.0f * shake * GetRandomFloat(-1.0f, 1.0f);
            targetLookPos.y += maxShakeOffset * 2.0f * shake * GetRandomFloat(-1.0f, 1.0f);
            
            // تبريد الاهتزاز مع الوقت
            trauma -= traumaDecay * dt;
            if (trauma < 0.0f) trauma = 0.0f;
        }

        // 🚨 [تم التعديل هنا] تحديث زاوية الرؤية الديناميكية (Dynamic FOV) وحمايتها
        float fovLerpAmount = fminf(fovSmoothness * dt, 1.0f);
        currentFOV = Lerp(currentFOV, targetFOV, fovLerpAmount);
        camera.fovy = currentFOV;

        // 9. تطبيق الإحداثيات النهائية على الكاميرا
        camera.position = finalPos;
        camera.target = targetLookPos;
        camera.up = up;
    }

private:
    /**
     * @brief يرسل شعاعاً من رأس اللاعب نحو الكاميرا لتقريبها عند وجود جدار (Camera Clipping Prevention)
     */
    Vector3 HandleCollision(Vector3 headPos, Vector3 targetCamPos, const std::vector<EnvironmentCollider>& colliders) {
        Vector3 direction = Vector3Subtract(targetCamPos, headPos);
        float maxDistance = Vector3Length(direction);
        
        if (maxDistance < 0.001f) return targetCamPos;

        direction = Vector3Normalize(direction);
        Ray ray = { headPos, direction };
        
        float closestHitDistance = maxDistance;

        for (const auto& col : colliders) {
            if (col.type == ColliderType::TRIGGER_ZONE) continue; // نتجاهل الزناد (مثل منطقة فتح الباب)

            RayCollision hit = GetRayCollisionBox(ray, col.bounds);
            
            if (hit.hit && hit.distance < closestHitDistance) {
                closestHitDistance = hit.distance;
            }
        }

        // إذا اصطدمنا بجدار، نضع الكاميرا قبل نقطة الاصطدام بقليل (Padding)
        if (closestHitDistance < maxDistance) {
            float padding = 0.2f; // هامش أمان لمنع الكاميرا من رؤية ما بداخل الجدار
            float safeDistance = std::max(0.0f, closestHitDistance - padding);
            return Vector3Add(headPos, Vector3Scale(direction, safeDistance));
        }

        return targetCamPos; // المسار آمن ولا يوجد جدران
    }

    // دالة مساعدة لتوليد رقم عشري عشوائي (للاهتزاز)
    float GetRandomFloat(float min, float max) {
        // نستخدم rand() البسيط لأن اهتزاز الكاميرا لا يحتاج لدقة تشفيرية عالية
        return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) / (max - min));
    }
};
