#pragma once

#include "../IPlayerState.h"
#include "../Player.h"
#include "PlayerIdleState.h"
// استدعاء الحالات الأخرى للانتقال إليها
// #include "PlayerWalkState.h"
// #include "PlayerAirState.h"
// #include "PlayerCrouchState.h"

class PlayerRunState : public IPlayerState {
private:
    float staminaDrainRate = 25.0f; // سرعة استهلاك الطاقة (يفقد 25 نقطة في الثانية)

public:
    void Enter(Player* player) override {
        // يمكن هنا إضافة تأثيرات بصرية مثل:
        // - زيادة زاوية الكاميرا (FOV) تدريجياً لإعطاء إحساس بالسرعة
        // - تشغيل تأثير غبار خفيف تحت الأقدام (Particles)
    }

    void Update(Player* player, float dt) override {
        // 1. قراءة الإدخال
        Vector2 inputDir = player->GetInputDirection();
        float inputLength = Vector2Length(inputDir);

        // 2. التحقق من السقوط أولاً (إذا ركض من فوق حافة)
        if (!player->movement.isGrounded) {
            // player->ChangeState(std::make_unique<PlayerAirState>());
            return;
        }

        // 3. التحقق من القفز أثناء الركض (قفزة أطول وأسرع لأن الزخم عالي)
        if (player->IsJumpPressed()) {
            player->movement.Jump();
            // player->ChangeState(std::make_unique<PlayerAirState>());
            return;
        }

        // 4. التحقق من الانزلاق أو الانحناء (Slide / Crouch)
        if (player->IsCrouchPressed()) {
            // إذا كان يركض وضغط انحناء، يمكن أن نرسله لاحقاً لحالة "الانزلاق" (PlayerSlideState)
            // مؤقتاً سنرسله للانحناء العادي
            // player->ChangeState(std::make_unique<PlayerCrouchState>());
            return;
        }

        // 5. نظام استهلاك الطاقة (Stamina System)
        player->stamina -= staminaDrainRate * dt;
        player->stamina = fmaxf(player->stamina, 0.0f); // التأكد من أن الطاقة لا تنزل تحت الصفر

        // 6. شروط الخروج من حالة الركض
        if (inputLength < 0.01f) {
            // اللاعب توقف عن الضغط على الجويستيك أو الكيبورد
            player->ChangeState(std::make_unique<PlayerIdleState>());
        } 
        else if (!player->IsSprintPressed() || player->stamina <= 0.0f) {
            // اللاعب رفع إصبعه عن زر الركض، أو أن طاقته نفدت تماماً
            // نجبره على الانتقال لحالة المشي ليتمكن من استرجاع طاقته
            // (مؤقتاً نرسله لـ Idle لو لم نبرمج Walk، لكن الصحيح هو WalkState)
            
            // player->ChangeState(std::make_unique<PlayerWalkState>());
            
            // سطر مؤقت حتى نكتب WalkState:
            player->ChangeState(std::make_unique<PlayerIdleState>()); 
        } 
        else {
            // 7. الاستمرار في الركض (تحديث الفيزياء)
            player->movement.CalculateMovement(inputDir, player->movement.runSpeed, player->cameraYaw, dt);
        }
    }

    void Exit(Player* player) override {
        // عند التوقف عن الركض، يمكن إعادة زاوية الكاميرا (FOV) لوضعها الطبيعي
    }

    const char* GetName() const override {
        return "RUN";
    }
};
