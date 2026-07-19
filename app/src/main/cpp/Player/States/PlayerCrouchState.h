#pragma once

#include "../IPlayerState.h"
#include "../Player.h"
#include "PlayerIdleState.h"
// استدعاء حالة الانبطاح وحالة الهواء (سيتم ربطها لاحقاً)
// #include "PlayerProneState.h"
// #include "PlayerAirState.h"

class PlayerCrouchState : public IPlayerState {
public:
    void Enter(Player* player) override {
        // 1. تقليل ارتفاع كبسولة التصادم الفيزيائية (اللاعب يصبح أقصر)
        // الارتفاع الطبيعي كان 1.74f، نجعله 1.0f للمرور تحت العوائق
        player->movement.currentHeight = 1.0f;

        // 2. هنا يمكن استدعاء الأنيميشن الخاص بالنزول
        // player->animController.PlayOverrideAnimation("crouch_idle");
    }

    void Update(Player* player, float dt) override {
        // 1. قراءة الإدخال لتوجيه اللاعب
        Vector2 inputDir = player->GetInputDirection();
        
        // 2. تحديث حركة الفيزياء بسرعة الانحناء (crouchSpeed = 2.0f)
        player->movement.CalculateMovement(inputDir, player->movement.crouchSpeed, player->cameraYaw, dt);

        // 3. استرجاع الطاقة (Stamina) أثناء الانحناء (يسترجعها بشكل أسرع لأنه مرتاح)
        if (player->stamina < player->maxStamina) {
            player->stamina += 20.0f * dt; 
        }

        // 4. التحقق من السقوط (إذا انحنى ومشى نحو حافة الهاوية)
        if (!player->movement.isGrounded) {
            // player->ChangeState(std::make_unique<PlayerAirState>());
            return; // خروج من الدالة لمنع تنفيذ أوامر أخرى
        }

        // 5. شروط الانتقال والخروج من الحالة
        
        // أ) الانتقال للانبطاح (Prone)
        // if (player->hud && player->hud->IsButtonPressed("prone")) {
        //     player->ChangeState(std::make_unique<PlayerProneState>());
        // }
        
        // ب) العودة للوقوف (عن طريق الضغط على زر الانحناء مجدداً أو زر القفز)
        else if (player->IsCrouchPressed() || player->IsJumpPressed()) {
            
            // [ملاحظة احترافية مدمجة]: في الألعاب الكبرى (AAA)، نرسل شعاعاً (Raycast) للأعلى 
            // للتأكد من عدم وجود سقف منخفض فوق رأس اللاعب قبل السماح له بالوقوف.
            // إذا كان السقف منخفضاً، نتجاهل طلب الوقوف.
            bool hasCeilingClearance = true; 
            /* 
            // كود مستقبلي لفحص السقف:
            Vector3 headPos = player->movement.position;
            headPos.y += 1.74f; // الارتفاع الطبيعي
            if (player->movement.CheckCapsuleCollision(headPos, colliders)) hasCeilingClearance = false;
            */

            if (hasCeilingClearance) {
                // إذا ضغط قفز، نعطيه دفعة للأعلى فوراً
                if (player->IsJumpPressed()) {
                    player->movement.Jump();
                    // player->ChangeState(std::make_unique<PlayerAirState>());
                } else {
                    // عودة طبيعية لوضع الوقوف أو المشي
                    player->ChangeState(std::make_unique<PlayerIdleState>());
                }
            }
        }
    }

    void Exit(Player* player) override {
        // إعادة ارتفاع اللاعب الطبيعي لحظة الخروج من حالة الانحناء
        player->movement.currentHeight = 1.74f;
        
        // إصلاح الموقع قليلاً للأعلى لمنع انغراس قدم اللاعب في الأرض عند تكبير الكبسولة
        player->movement.position.y += 0.05f; 
    }

    const char* GetName() const override {
        return "CROUCH";
    }
};
