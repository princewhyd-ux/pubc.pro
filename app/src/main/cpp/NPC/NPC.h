#pragma once

#include "raylib.h"
#include "raymath.h"
#include "NPCBrain.h"
#include "../Animation/AnimationController.h" // نظام دمج الحركات الذي بنيناه
#include <vector>
#include <string>

class NPC {
private:
    Model* model;
    AnimationController animController;
    
    // إعدادات المجسم (مأخوذة من الكود الأصلي الخاص بك لإصلاح ملفات الـ GLB)
    float modelScale;
    Matrix baseRotation;

public:
    NPCBrain brain;
    bool isDead = false;

    // التهيئة (نحتاج المجسم، نقطة البداية، وحجم المجسم لأن شخصيات الـ Women لديك أحجامها مختلفة 1.0 و 1.8)
    NPC(Model* npcModel, Vector3 startPos, float scale = 1.0f) 
        : brain(startPos), modelScale(scale) 
    {
        model = npcModel;
        
        // إصلاح دوران الشخصيات (النماذج مسطحة على الوجه في ملف الـ GLB)
        baseRotation = MatrixRotateX(-PI / 2.0f);
    }

    // ربط ملفات الأنيميشن بالمجسم
    void SetAnimations(ModelAnimation* anims, int animCount) {
        animController.Init(*model, anims, animCount);
    }

    // التحديث (تُستدعى كل إطار)
    void Update(float dt, Vector3 targetPos, const std::vector<EnvironmentCollider>& colliders) {
        if (isDead) return;

        // 1. تحديث تفكير الذكاء الاصطناعي
        brain.Update(dt, targetPos, colliders);

        // إذا وصل الدم للصفر، يموت
        if (brain.health <= 0.0f) {
            isDead = true;
            return;
        }

        // 2. حساب السرعة الحالية بناءً على حركة المخ
        float currentSpeed = Vector3Length({ brain.velocity.x, 0.0f, brain.velocity.z });

        // 3. تحديث الأنيميشن بناءً على السرعة والحالة
        if (brain.currentState == NPCState::ATTACK) {
            // إذا كان يهاجم، يمكننا تشغيل أنيميشن الإطلاق (السرعة = 0 ليقف ويثبت)
            animController.UpdateLocomotion(*model, 0.0f, brain.moveSpeed, brain.runSpeed, dt);
            
            // ملاحظة: لاحقاً يمكنك استدعاء دالة خاصة بالهجوم هنا مثل:
            // animController.PlayOverrideAnimation("fire_stand");
        } else {
            // دمج الحركات (وقوف -> مشي -> ركض) بنعومة تامة
            animController.UpdateLocomotion(*model, currentSpeed, brain.moveSpeed, brain.runSpeed, dt);
        }
    }

    // رسم المجسم في العالم ثلاثي الأبعاد
    void Draw() {
        if (isDead) return; // لا ترسمه إذا مات (أو يمكنك إبقاؤه على الأرض بتشغيل أنيميشن الموت)

        // 1. إعداد مصفوفة الدوران بناءً على زاوية نظر الـ Brain
        Matrix matYaw = MatrixRotateY(brain.rotationAngle);
        
        // 2. إعداد مصفوفة الموقع
        Matrix matTranslation = MatrixTranslate(brain.position.x, brain.position.y, brain.position.z);
        
        // 3. إعداد مصفوفة الحجم
        Matrix matScale = MatrixScale(modelScale, modelScale, modelScale);

        // 4. دمج المصفوفات بالترتيب الصحيح: الحجم -> دوران إصلاح الـ GLB -> دوران التوجيه -> الموقع
        model->transform = MatrixMultiply(MatrixMultiply(MatrixMultiply(matScale, baseRotation), matYaw), matTranslation);

        // 5. رسم المجسم النهائي
        DrawModel(*model, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
    }

    // رسم واجهة المستخدم 2D فوق رأس العدو (مفيدة جداً للمطورين)
    void DrawUI(Camera3D camera) {
        if (isDead) return;

        // تحديد موقع رأس العدو (مثلاً أعلى من مركزه بـ 2 متر)
        Vector3 headPos = { brain.position.x, brain.position.y + 2.0f, brain.position.z };
        
        // تحويل الإحداثيات من 3D إلى 2D لتظهر على الشاشة
        Vector2 screenPos = GetWorldToScreen(headPos, camera);

        // التأكد من أن العدو أمام الكاميرا وليس خلفها
        if (screenPos.x > 0 && screenPos.y > 0 && screenPos.x < GetScreenWidth() && screenPos.y < GetScreenHeight()) {
            
            // 1. رسم شريط الصحة (Health Bar)
            float healthRatio = brain.health / 100.0f;
            int barWidth = 50;
            int barHeight = 6;
            int posX = (int)screenPos.x - (barWidth / 2);
            int posY = (int)screenPos.y;

            DrawRectangle(posX, posY, barWidth, barHeight, Fade(BLACK, 0.6f));
            DrawRectangle(posX, posY, (int)(barWidth * healthRatio), barHeight, (healthRatio > 0.5f) ? GREEN : RED);
            DrawRectangleLines(posX, posY, barWidth, barHeight, DARKGRAY);

            // 2. رسم نص يوضح حالة الذكاء الاصطناعي (يساعدك على فهم ماذا يفعل العدو)
            const char* stateText = "IDLE";
            Color stateColor = WHITE;

            switch (brain.currentState) {
                case NPCState::PATROL:      stateText = "PATROL";      stateColor = GREEN; break;
                case NPCState::INVESTIGATE: stateText = "? INVESTIGATE"; stateColor = YELLOW; break;
                case NPCState::CHASE:       stateText = "! CHASE";     stateColor = ORANGE; break;
                case NPCState::ATTACK:      stateText = "!!! ATTACK";  stateColor = RED; break;
                case NPCState::RETREAT:     stateText = "RETREAT";     stateColor = BLUE; break;
                case NPCState::TAKE_COVER:  stateText = "COVER";       stateColor = GRAY; break;
            }

            // رسم النص فوق شريط الصحة بقليل
            DrawText(stateText, posX - 10, posY - 15, 10, stateColor);
        }
    }
};
