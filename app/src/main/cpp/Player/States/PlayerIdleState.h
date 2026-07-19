#pragma once

#include "../IPlayerState.h"
#include "../Player.h"
// استدعاء الحالات الأخرى التي يمكن الانتقال إليها (سيتم إنشاؤها لاحقاً)
// #include "PlayerWalkState.h" 
// #include "PlayerJumpState.h"

class PlayerIdleState : public IPlayerState {
public:
    void Enter(Player* player) override {
        // عند الدخول في حالة الوقوف، نصفر السرعة المستهدفة
        // (لا نلمس السرعة الحالية لكي يتوقف اللاعب بنعومة وليس فجأة)
    }

    void Update(Player* player, float dt) override {
        // 1. قراءة الإدخال من اللاعب
        Vector2 inputDir = player->GetInputDirection();
        
        // 2. تحديث الفيزياء (تمرير سرعة 0.0f لأننا في حالة وقوف)
        // يتم استخدام زاوية الكاميرا هنا بصفر مؤقتاً (لأن الكاميرا ستديرها من الخارج)
        player->movement.CalculateMovement(inputDir, 0.0f, player->cameraYaw, dt);

        // 3. شروط الانتقال إلى حالات أخرى
        if (player->IsJumpPressed() && player->movement.isGrounded) {
            player->movement.Jump();
            // player->ChangeState(std::make_unique<PlayerJumpState>());
        }
        else if (Vector2Length(inputDir) > 0.1f) {
            // إذا حرك اللاعب الجويستيك، انتقل لحالة المشي أو الركض
            if (player->IsSprintPressed() && player->stamina > 0.0f) {
                // player->ChangeState(std::make_unique<PlayerRunState>());
            } else {
                // player->ChangeState(std::make_unique<PlayerWalkState>());
            }
        }
        
        // استرجاع الطاقة (Stamina) أثناء الوقوف
        if (player->stamina < player->maxStamina) {
            player->stamina += 15.0f * dt;
        }
    }

    void Exit(Player* player) override {
        // لا نحتاج لتنظيف شيء عند الخروج من الوقوف
    }

    const char* GetName() const override {
        return "IDLE";
    }
};
