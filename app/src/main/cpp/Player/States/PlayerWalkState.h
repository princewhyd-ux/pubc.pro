#pragma once

#include "../IPlayerState.h"
#include "../Player.h"
#include "PlayerIdleState.h"
// استدعاء الحالات الأخرى للانتقال إليها
// #include "PlayerRunState.h"
// #include "PlayerAirState.h"
// #include "PlayerCrouchState.h"

class PlayerWalkState : public IPlayerState {
public:
    void Enter(Player* player) override {
        // تُستدعى لحظة انتقال اللاعب من الوقوف أو الركض إلى المشي
        // يمكننا هنا تجهيز أي متغيرات، مثل ضبط وتيرة صوت الخطوات (Footsteps) لتكون أبطأ من الركض
    }

    void Update(Player* player, float dt) override {
        // 1. قراءة الإدخال
        Vector2 inputDir = player->GetInputDirection();
        float inputLength = Vector2Length(inputDir);

        // 2. التحقق من السقوط (إذا مشى اللاعب من فوق حافة الهاوية)
        if (!player->movement.isGrounded) {
            // player->ChangeState(std::make_unique<PlayerAirState>());
            return;
        }

        // 3. التحقق من القفز
        if (player->IsJumpPressed()) {
            player->movement.Jump();
            // player->ChangeState(std::make_unique<PlayerAirState>());
            return;
        }

        // 4. التحقق من الانحناء (Crouch)
        if (player->IsCrouchPressed()) {
            // player->ChangeState(std::make_unique<PlayerCrouchState>());
            return;
        }

        // 5. التحقق من الركض (Sprint)
        // إذا ضغط زر الركض وكان لديه طاقة متبقية، ينتقل لحالة الركض فوراً
        if (player->IsSprintPressed() && player->stamina > 0.0f) {
            // player->ChangeState(std::make_unique<PlayerRunState>());
            return;
        }

        // 6. استرجاع الطاقة (Stamina)
        // المشي يسمح باسترجاع الطاقة، لكن بسرعة أبطأ من الوقوف التام (الوقوف = 15.0f)
        if (player->stamina < player->maxStamina) {
            player->stamina += 10.0f * dt; 
        }

        // 7. شروط الخروج والاستمرار
        if (inputLength < 0.01f) {
            // اللاعب توقف تماماً عن دفع الجويستيك أو رفع يده عن الكيبورد
            player->ChangeState(std::make_unique<PlayerIdleState>());
        } 
        else {
            // الاستمرار في المشي وتحديث حركة الفيزياء
            player->movement.CalculateMovement(inputDir, player->movement.walkSpeed, player->cameraYaw, dt);
        }
    }

    void Exit(Player* player) override {
        // تُستدعى لحظة التوقف عن المشي أو الانتقال لحالة أخرى
    }

    const char* GetName() const override {
        return "WALK";
    }
};
