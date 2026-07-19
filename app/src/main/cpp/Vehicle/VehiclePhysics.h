#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

// (إعادة تعريف مبسطة لضمان استقلالية الملف، نفس الموجود في MapManager)
#ifndef MAP_MANAGER_H
enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };
struct EnvironmentCollider { BoundingBox bounds; ColliderType type; std::string tag; };
#endif

// بيانات العجلة الواحدة (لتحريك المجسمات ثلاثية الأبعاد لاحقاً)
struct Wheel {
    Vector3 localPosition; // موقع العجلة بالنسبة لمركز السيارة
    float radius;          // نصف القطر (لحساب سرعة الدوران)
    float rotationAngle;   // زاوية الدوران حول نفسها (تتأثر بالسرعة)
    float steeringAngle;   // زاوية الانعطاف (للعجلات الأمامية فقط)
    float suspensionCompression; // مقدار ضغط المساعدين (التعليق)
    bool isGrounded;       // هل العجلة تلامس الأرض؟
};

class VehiclePhysics {
public:
    Vector3 position;
    Vector3 velocity;
    
    // زوايا دوران السيارة (Yaw: يمين/يسار, Pitch: فوق/تحت, Roll: ميلان جانبي)
    float yaw, pitch, roll; 

    // إعدادات المحرك والأداء
    float maxSpeed = 30.0f;        // أقصى سرعة (متر/ثانية)
    float enginePower = 20.0f;     // قوة التسارع
    float reversePower = 10.0f;    // قوة الرجوع للخلف
    float brakePower = 40.0f;      // قوة الفرامل
    float drag = 0.5f;             // احتكاك الهواء والشارع (يوقف السيارة تدريجياً)
    float mass = 1500.0f;          // الكتلة
    
    // نظام التوجيه (Steering)
    float maxSteerAngle = 35.0f * DEG2RAD; 
    float steerSpeed = 5.0f;
    float currentSteer = 0.0f;

    // نظام التعليق (Suspension) والأبعاد
    float suspensionRestLength = 0.6f;
    float carWidth = 1.8f;
    float carLength = 4.0f;

    // نظام التروس (Gearbox) و RPM
    int currentGear = 1; // 0 = Reverse, 1 = Neutral, 2-6 = Forward Gears
    float currentRPM = 1000.0f;
    float maxRPM = 7000.0f;

    // حالة السيارة
    bool isEngineOn = false;
    float currentSpeed = 0.0f;

    // العجلات الأربع
    Wheel wheels[4];

    VehiclePhysics(Vector3 startPos) {
        position = startPos;
        velocity = { 0.0f, 0.0f, 0.0f };
        yaw = 0.0f; pitch = 0.0f; roll = 0.0f;
        isEngineOn = true; // يمكن ربطها بزر التشغيل لاحقاً

        // إعداد مواقع العجلات الأربع بالنسبة لمركز السيارة (يجب تعديلها حسب مجسمك .glb)
        // [0] أمامي يسار, [1] أمامي يمين, [2] خلفي يسار, [3] خلفي يمين
        float hw = carWidth / 2.0f;
        float hl = carLength / 2.0f;
        float wheelRadius = 0.35f;

        wheels[0] = { {-hw, 0.0f,  hl}, wheelRadius, 0.0f, 0.0f, 0.0f, false };
        wheels[1] = { { hw, 0.0f,  hl}, wheelRadius, 0.0f, 0.0f, 0.0f, false };
        wheels[2] = { {-hw, 0.0f, -hl}, wheelRadius, 0.0f, 0.0f, 0.0f, false };
        wheels[3] = { { hw, 0.0f, -hl}, wheelRadius, 0.0f, 0.0f, 0.0f, false };
    }

    // دالة التحديث الرئيسية (يتم استدعاؤها كل إطار)
    // inputDirection.y = البنزين/الفرامل (1.0 إلى -1.0)
    // inputDirection.x = التوجيه يمين/يسار (1.0 إلى -1.0)
    void Update(float dt, Vector2 inputDirection, bool handbrake, const std::vector<EnvironmentCollider>& colliders) {
        if (!isEngineOn) return;

        CalculateSuspensionAndTerrain(dt, colliders);
        ApplySteering(inputDirection.x, dt);
        ApplyAccelerationAndBraking(inputDirection.y, handbrake, dt);
        UpdateGearAndRPM();
        UpdateWheelsVisuals(dt);

        // تطبيق الحركة النهائية
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        position.z += velocity.z * dt;
    }

    // الحصول على مصفوفة الدوران الكاملة لرسم مجسم السيارة بشكل صحيح
    Matrix GetTransformMatrix() const {
        Matrix matYaw   = MatrixRotateY(yaw);
        Matrix matPitch = MatrixRotateX(pitch);
        Matrix matRoll  = MatrixRotateZ(roll);
        
        // دمج الدورانات (Y ثم X ثم Z)
        Matrix rotation = MatrixMultiply(MatrixMultiply(matPitch, matYaw), matRoll);
        Matrix translation = MatrixTranslate(position.x, position.y, position.z);
        
        return MatrixMultiply(rotation, translation);
    }

private:
    // 1. نظام التعليق وقراءة تضاريس الأرض (Raycast Suspension)
    void CalculateSuspensionAndTerrain(float dt, const std::vector<EnvironmentCollider>& colliders) {
        float heights[4] = { position.y, position.y, position.y, position.y };
        bool groundedCount = 0;

        Matrix rotation = MatrixRotateY(yaw); // نأخذ الـ Yaw فقط لحساب أماكن العجلات

        // إطلاق شعاع وهمي من كل عجلة لمعرفة ارتفاع الأرض تحتها
        for (int i = 0; i < 4; i++) {
            Vector3 worldWheelPos = Vector3Transform(wheels[i].localPosition, rotation);
            worldWheelPos = Vector3Add(worldWheelPos, position);
            
            float groundHeight = GetGroundHeight(worldWheelPos, colliders);
            
            // إذا كانت العجلة قريبة من الأرض، نعتبرها ملامسة
            if (position.y - groundHeight < suspensionRestLength * 2.0f) {
                heights[i] = groundHeight;
                wheels[i].isGrounded = true;
                wheels[i].suspensionCompression = (position.y - groundHeight) / suspensionRestLength;
                groundedCount++;
            } else {
                wheels[i].isGrounded = false;
                wheels[i].suspensionCompression = 1.0f; // العجلة في الهواء
            }
        }

        // إذا كانت السيارة على الأرض، نحسب زاوية الميلان (Pitch & Roll)
        if (groundedCount >= 3) {
            // حساب الارتفاع المتوسط
            float avgHeight = (heights[0] + heights[1] + heights[2] + heights[3]) / 4.0f;
            
            // جذب السيارة نحو الأرض بنعومة (Suspension Spring)
            float targetY = avgHeight + suspensionRestLength;
            position.y = Lerp(position.y, targetY, 10.0f * dt);
            velocity.y = 0.0f; // إيقاف الجاذبية

            // حساب Roll (ميلان جانبي)
            float leftHeight  = (heights[0] + heights[2]) / 2.0f;
            float rightHeight = (heights[1] + heights[3]) / 2.0f;
            float targetRoll = atan2f(leftHeight - rightHeight, carWidth);

            // حساب Pitch (ميلان أمامي/خلفي)
            float frontHeight = (heights[0] + heights[1]) / 2.0f;
            float rearHeight  = (heights[2] + heights[3]) / 2.0f;
            float targetPitch = atan2f(rearHeight - frontHeight, carLength);

            // تطبيق الدوران بنعومة شديدة لعدم اهتزاز الكاميرا
            roll = Lerp(roll, targetRoll, 15.0f * dt);
            pitch = Lerp(pitch, targetPitch, 15.0f * dt);
        } else {
            // السقوط الحر (الجاذبية) إذا كانت السيارة في الهواء
            velocity.y -= 9.81f * dt;
            pitch = Lerp(pitch, 0.0f, 2.0f * dt);
            roll = Lerp(roll, 0.0f, 2.0f * dt);
        }
    }

    // 2. تطبيق التوجيه (الدركسيون)
    void ApplySteering(float steerInput, float dt) {
        // لا يمكن الانعطاف إذا كانت السيارة متوقفة تماماً (محاكاة واقعية)
        float speedFactor = fminf(currentSpeed / 5.0f, 1.0f);
        
        float targetSteer = steerInput * maxSteerAngle;
        currentSteer = Lerp(currentSteer, targetSteer, steerSpeed * dt);
        
        // العجلات الأمامية [0] و [1] تأخذ زاوية الانعطاف للرسم
        wheels[0].steeringAngle = currentSteer;
        wheels[1].steeringAngle = currentSteer;

        // تطبيق الالتفاف على جسم السيارة (يعتمد على سرعة السيارة)
        // الاتجاه يتغير إذا كانت السيارة ترجع للخلف (محاكاة عكس الدركسيون)
        float turnDirection = (Vector3DotProduct(velocity, GetForwardVector()) >= 0.0f) ? 1.0f : -1.0f;
        
        if (currentSpeed > 0.5f) {
            // كلما زادت السرعة يقل تأثير الدركسيون لمنع الانقلاب (Understeer)
            float highSpeedNerf = 1.0f - (currentSpeed / maxSpeed) * 0.5f; 
            yaw -= currentSteer * (currentSpeed * 0.5f) * turnDirection * highSpeedNerf * dt;
        }
    }

    // 3. تطبيق المحرك، الفرامل، وفرامل اليد
    void ApplyAccelerationAndBraking(float accelInput, bool handbrake, float dt) {
        Vector3 forward = GetForwardVector();
        
        // حساب السرعة الحالية كقيمة مفردة (Scalar)
        currentSpeed = Vector3Length(velocity);

        // 1. قوة المحرك أو الفرامل
        if (accelInput > 0.1f) {
            // بنزين للأمام
            if (currentGear == 0) currentGear = 2; // التبديل التلقائي من الرجوع إلى الأول
            
            float accelRatio = 1.0f - (currentSpeed / maxSpeed); // يقل التسارع كلما اقتربنا من السرعة القصوى
            Vector3 force = Vector3Scale(forward, enginePower * accelInput * accelRatio * dt);
            velocity = Vector3Add(velocity, force);
        } 
        else if (accelInput < -0.1f) {
            // إذا كانت السيارة تمشي للأمام -> فرامل، إذا كانت متوقفة -> رجوع للخلف
            float forwardVel = Vector3DotProduct(velocity, forward);
            
            if (forwardVel > 1.0f) {
                // فرامل عادية
                Vector3 brakeForce = Vector3Scale(Vector3Normalize(velocity), -brakePower * dt);
                velocity = Vector3Add(velocity, brakeForce);
            } else {
                // رجوع للخلف
                currentGear = 0; // Reverse gear
                Vector3 force = Vector3Scale(forward, -reversePower * fabsf(accelInput) * dt);
                velocity = Vector3Add(velocity, force);
            }
        }

        // 2. فرامل اليد (Handbrake)
        if (handbrake) {
            Vector3 brakeForce = Vector3Scale(Vector3Normalize(velocity), -brakePower * 1.5f * dt);
            velocity = Vector3Add(velocity, brakeForce);
        }

        // 3. الاحتكاك الطبيعي (Drag/Friction) لإيقاف السيارة عند رفع الإصبع عن البنزين
        if (currentSpeed > 0.01f) {
            Vector3 dragForce = Vector3Scale(velocity, -drag * dt);
            velocity = Vector3Add(velocity, dragForce);
            
            // منع السرعة من الانزلاق السالب العشوائي
            if (Vector3Length(velocity) < 0.1f && fabsf(accelInput) < 0.1f && !handbrake) {
                velocity = {0,0,0};
            }
        }

        // 4. جعل الحركة تتبع زاوية السيارة (يمنع السيارة من الانزلاق كالجليد)
        if (currentSpeed > 0.1f && !handbrake) {
            Vector3 fixedVelocity = Vector3Scale(forward, Vector3DotProduct(velocity, forward));
            // دمج السرعة الحقيقية مع السرعة الموجهة لمحاكاة الانزلاق الخفيف (Drift)
            velocity = Vector3Lerp(velocity, fixedVelocity, 5.0f * dt); 
        }
    }

    // 4. تحديث التروس (Gears) وصوت المحرك (RPM)
    void UpdateGearAndRPM() {
        if (currentGear == 0) {
            // Reverse
            currentRPM = Lerp(1000.0f, maxRPM, currentSpeed / 15.0f);
        } else {
            // Auto Transmission Logic
            float speedRatio = currentSpeed / maxSpeed;
            
            if (speedRatio < 0.15f) currentGear = 2; // Gear 1
            else if (speedRatio < 0.35f) currentGear = 3; // Gear 2
            else if (speedRatio < 0.55f) currentGear = 4; // Gear 3
            else if (speedRatio < 0.75f) currentGear = 5; // Gear 4
            else currentGear = 6; // Gear 5

            // حساب الـ RPM لعمل صوت واقعي لاحقاً
            float gearMinSpeed = (currentGear - 2) * 0.2f * maxSpeed;
            float gearMaxSpeed = (currentGear - 1) * 0.2f * maxSpeed;
            float rpmRatio = (currentSpeed - gearMinSpeed) / (gearMaxSpeed - gearMinSpeed);
            currentRPM = Lerp(2000.0f, maxRPM, Clamp(rpmRatio, 0.0f, 1.0f));
        }
    }

    // 5. دوران العجلات البصري (للرسم)
    void UpdateWheelsVisuals(float dt) {
        float forwardVel = Vector3DotProduct(velocity, GetForwardVector());
        
        for (int i = 0; i < 4; i++) {
            // السرعة = المسافة / الزمن. الزاوية = السرعة / نصف القطر
            float rotationDelta = (forwardVel * dt) / wheels[i].radius;
            wheels[i].rotationAngle -= rotationDelta; // دوران حول محور X للنماذج
            
            // الحفاظ على الزاوية بين -PI و PI
            if (wheels[i].rotationAngle > PI * 2) wheels[i].rotationAngle -= PI * 2;
            if (wheels[i].rotationAngle < -PI * 2) wheels[i].rotationAngle += PI * 2;
        }
    }

    // --- دوال مساعدة ---

    // الحصول على اتجاه السيارة الأمامي بناءً على زاويتها
    Vector3 GetForwardVector() const {
        return { sinf(yaw), 0.0f, cosf(yaw) }; 
    }

    // محاكاة سريعة لاكتشاف الأرض (بديلاً عن Raycaster ثقيل لكل عجلة)
    float GetGroundHeight(Vector3 checkPos, const std::vector<EnvironmentCollider>& colliders) const {
        float highestGround = 0.0f;
        for (const auto& col : colliders) {
            // إذا كان نوع المربع هو جدار، لا نحسبه كأرض
            if (col.type == ColliderType::STATIC_WALL) continue; 
            
            // فحص هل العجلة تقع ضمن محيط المجسم (X و Z)
            if (checkPos.x >= col.bounds.min.x && checkPos.x <= col.bounds.max.x &&
                checkPos.z >= col.bounds.min.z && checkPos.z <= col.bounds.max.z) {
                // أخذ أعلى نقطة أرضية
                if (col.bounds.max.y > highestGround) {
                    highestGround = col.bounds.max.y;
                }
            }
        }
        return highestGround;
    }
};
