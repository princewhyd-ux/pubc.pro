#pragma once

#include "../IPlayerState.h"
#include "../Player.h"
#include "PlayerIdleState.h"
// استدعاء حالات الحركة الأرضية إذا أردت الانتقال لها مباشرة عند الهبوط (سيتم إنشاؤها لاحقاً)
// #include "PlayerWalkState.h"
// #include "PlayerRunState.h"

class PlayerAirState : public IPlayerState {
public:
    void Enter(Player* player) override {
        // تُستدعى لحظة القفز أو بداية السقوط
        // هنا يمكنك لاحقاً إخبار الـ AnimationController بتشغيل حركة القفز (Jump Animation)
        // player->animController.PlayOverrideAnimation("jump_stand");
    }

    void Update(Player* player, float dt) override {
        // 1. قراءة إدخال اللاعب لتوجيه نفسه في الهواء (Air Control)
        Vector2 inputDir = player->GetInputDirection();
        
        // 2. تحديد السرعة المطلوبة (لو كان يركض قبل القفز، سيحتفظ بزخمه وسرعته)
        float targetSpeed = player->IsSprintPressed() ? player->movement.runSpeed : player->movement.walkSpeed;

        // 3. تحديث الفيزياء
        // ملاحظة: نظام PlayerMovement يقلل التسارع تلقائياً في الهواء (multiplier = 0.3f) 
        // لكي لا يغير اللاعب اتجاهه في الهواء وكأنه على الأرض!
        player->movement.CalculateMovement(inputDir, targetSpeed, player->cameraYaw, dt);

        // 4. التحقق من الهبوط (Landing)
        if (player->movement.isGrounded) {
            
            // هنا يمكنك إضافة مؤثرات الهبوط:
            // - تشغيل صوت (Footstep / Landing Sound)
            // - اهتزاز طفيف للكاميرا (playerCamera.AddTrauma(0.2f))
            // - تشغيل غبار من تحت قدميه (Particles)

            // الانتقال إلى الحالة المناسبة بناءً على الإدخال لحظة الهبوط
            if (Vector2LengthSqr(inputDir) > 0.01f) {
                // إذا كان اللاعب يضغط على الجويستيك لحظة الهبوط، يكمل مشي/ركض
                // (مؤقتاً سنعيده لـ Idle لأننا لم ننشئ ملف Walk بعد، لكن Idle سينقله تلقائياً)
                player->ChangeState(std::make_unique<PlayerIdleState>());
                
                /* الكود المستقبلي:
                if (player->IsSprintPressed() && player->stamina > 0.0f)
                    player->ChangeState(std::make_unique<PlayerRunState>());
                else
                    player->ChangeState(std::make_unique<PlayerWalkState>());
                */
            } else {
                // هبط وهو لا يضغط على أي زر
                player->ChangeState(std::make_unique<PlayerIdleState>());
            }
        }
    }

    void Exit(Player* player) override {
        // تُستدعى لحظة الهبوط وملامسة الأرض
        // يمكنك هنا إخبار الأنيميشن بإنهاء حركة السقوط وتشغيل حركة (Land) الناعمة
    }

    const char* GetName() const override {
        return "AIR (JUMP/FALL)";
    }
};
