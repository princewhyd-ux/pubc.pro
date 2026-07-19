#pragma once

#include "raylib.h"
#include "raymath.h"
#include "IPlayerState.h"
#include "PlayerMovement.h"
#include "../Animation/AnimationController.h"
#include "../UI/HUDManager.h"
#include "../UI/TouchJoystick.h"
#include <memory>
#include <vector>

class Player {
public:
    Model* model;
    
    // المكونات الأساسية (Components)
    PlayerMovement movement;
    AnimationController animController;
    std::unique_ptr<IPlayerState> currentState;
    
    // واجهة التحكم (Touch UI)
    HUDManager* hud;
    TouchJoystick* joystick;

    // خصائص اللاعب (Stats)
    float health = 100.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    bool isDead = false;

    // زاوية الكاميرا الحالية لربط الحركة بالرؤية
    float cameraYaw = 0.0f;

    // مصفوفة الأساس لإصلاح النموذج (.glb)
    Matrix baseTransform;

    Player(Model* playerModel, Vector3 startPos, HUDManager* playerHud, TouchJoystick* playerJoystick) 
        : movement(startPos), hud(playerHud), joystick(playerJoystick) 
    {
        model = playerModel;
        
        // ==========================================
        // تطبيق مصفوفاتك الأصلية لإصلاح نموذج اللاعب
        // ==========================================
        Matrix offset = MatrixTranslate(0.0f, -26.78f, 30.55f);
        Matrix scale  = MatrixScale(0.01f, 0.01f, 0.01f);
        Matrix rot    = MatrixRotateX(PI / 2.0f); // تعديل نوم الشخصية على الأرض
        
        baseTransform = MatrixMultiply(MatrixMultiply(offset, scale), rot);
    }

    // تفريغ الذاكرة للأنيميشن
    ~Player() {
        animController.Unload();
    }

    // ربط الحركات بمحرك الدمج (Blend Tree)
    void SetAnimations(ModelAnimation* anims, int animCount) {
        animController.Init(*model, anims, animCount);
    }

    // تغيير الحالة (State Machine)
    void ChangeState(std::unique_ptr<IPlayerState> newState) {
        if (currentState) currentState->Exit(this);
        currentState = std::move(newState);
        currentState->Enter(this);
    }

    // حلقة التحديث الرئيسية
    void Update(float dt, float camYaw, const std::vector<EnvironmentCollider>& colliders) {
        if (isDead) return;

        cameraYaw = camYaw; // حفظ زاوية الكاميرا للحالات

        // 1. تحديث آلة الحالات (والتي ستستدعي movement.CalculateMovement)
        if (currentState) {
            currentState->Update(this, dt);
        }

        // 2. تحديث الفيزياء والتصادمات (دائماً تعمل حتى لو كان اللاعب في الهواء)
        movement.UpdatePhysics(dt, colliders);

        // 3. حساب السرعة الحالية على المحورين X و Z لتشغيل الأنيميشن الصحيح
        float currentSpeed = Vector3Length({ movement.velocity.x, 0.0f, movement.velocity.z });

        // 4. تحديث الرسوم المتحركة بنعومة (Blend Tree)
        animController.UpdateLocomotion(*model, currentSpeed, movement.walkSpeed, movement.runSpeed, dt);
        
        // 5. تقييد الطاقة (Stamina)
        stamina = Clamp(stamina, 0.0f, maxStamina);
    }

    // رسم اللاعب وتطبيق المصفوفات
    void Draw() {
        if (isDead) return;

        // دمج دوران جسد اللاعب (Yaw) مع مصفوفة الإصلاح
        Matrix yawRot = MatrixRotateY(movement.rotationAngle * DEG2RAD);
        
        // تحريك اللاعب لمكانه الفيزيائي
        Matrix translation = MatrixTranslate(movement.position.x, movement.position.y, movement.position.z);
        
        // الترتيب الصارم لضرب المصفوفات: (الأساس -> الدوران -> النقل)
        model->transform = MatrixMultiply(MatrixMultiply(baseTransform, yawRot), translation);

        // رسم النموذج النهائي
        DrawModel(*model, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    }

    // ==========================================
    // دوال الإدخال الموحدة (أندرويد + كمبيوتر)
    // ==========================================
    Vector2 GetInputDirection() const {
        Vector2 dir = { 0.0f, 0.0f };
        
        // أندرويد (Touch Joystick)
        if (joystick && joystick->IsActive()) {
            dir = joystick->GetDirection();
            dir.y = -dir.y; // عكس الـ Y ليتوافق مع نظام 3D
        } 
        // كمبيوتر (Keyboard)
        else {
            if (IsKeyDown(KEY_W)) dir.y += 1.0f;
            if (IsKeyDown(KEY_S)) dir.y -= 1.0f;
            if (IsKeyDown(KEY_A)) dir.x += 1.0f;
            if (IsKeyDown(KEY_D)) dir.x -= 1.0f;
            
            if (Vector2LengthSqr(dir) > 0.01f) {
                dir = Vector2Normalize(dir);
            }
        }
        return dir;
    }

    bool IsJumpPressed() const {
        return IsKeyPressed(KEY_SPACE) || (hud && hud->IsButtonPressed("jump"));
    }

    bool IsSprintPressed() const {
        // في الأندرويد، نعتبر اللاعب يركض إذا دفع الجويستيك لأقصى حافة (أكثر من 80%)
        bool touchSprint = (joystick && joystick->GetStrength() > 0.8f);
        return IsKeyDown(KEY_LEFT_SHIFT) || touchSprint;
    }

    bool IsCrouchPressed() const {
        return IsKeyPressed(KEY_C) || (hud && hud->IsButtonPressed("crouch"));
    }
};
