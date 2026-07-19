#pragma once

#include "raylib.h"
#include "raymath.h"
#include "VehiclePhysics.h"
#include "../Player/Player.h" // نحتاج اللاعب لنقل موقعه وإخفائه عند الركوب
#include <vector>
#include <string>

class Car {
private:
    Model* carModel;
    
    // إعدادات واجهة اللمس للأندرويد (Touch UI)
    Rectangle btnGas;
    Rectangle btnBrake;
    Rectangle btnLeft;
    Rectangle btnRight;
    Rectangle btnExit;
    Rectangle btnEnter; // زر الركوب عندما يكون اللاعب قريباً

    // نظام محاكاة الدخول والخروج
    float interactionRadius = 4.0f; // المسافة المسموحة لركوب السيارة
    Vector3 exitOffset = { -2.0f, 0.0f, 0.0f }; // المكان الذي ينزل فيه اللاعب (يسار السيارة)

    // دالة مساعدة لإنشاء أزرار الواجهة وتحديد أماكنها بناءً على حجم الشاشة
    void SetupUI() {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        // أزرار القيادة (يمين الشاشة)
        btnGas   = { (float)sw - 150, (float)sh - 200, 100, 150 };
        btnBrake = { (float)sw - 280, (float)sh - 120, 100, 70 };

        // أزرار التوجيه (يسار الشاشة)
        btnLeft  = { 50,  (float)sh - 150, 100, 100 };
        btnRight = { 200, (float)sh - 150, 100, 100 };

        // زر الخروج (أعلى اليمين)
        btnExit  = { (float)sw - 120, 20, 100, 50 };

        // زر الركوب (يظهر بجانب السيارة)
        btnEnter = { (float)sw / 2 + 100, (float)sh / 2, 120, 60 };
    }

public:
    VehiclePhysics physics;
    bool isPlayerInside = false;

    Car(Model* model, Vector3 startPos) : physics(startPos) {
        carModel = model;
        SetupUI();
    }

    // تحديث منطق السيارة والمدخلات
    void Update(float dt, Player* player, const std::vector<EnvironmentCollider>& colliders) {
        // تحديث مقاسات الشاشة في حال تم تدوير الهاتف أو تغيير حجم النافذة
        SetupUI();

        Vector2 inputDirection = { 0.0f, 0.0f };
        bool handbrake = false;

        // 1. حالة اللاعب خارج السيارة (فحص مسافة الركوب)
        if (!isPlayerInside) {
            float distToPlayer = Vector3Distance(physics.position, player->position);
            
            // إذا كان قريباً، نسمح له بالركوب (عن طريق حرف F للكمبيوتر أو زر اللمس)
            if (distToPlayer <= interactionRadius) {
                bool enterPressed = IsKeyPressed(KEY_F);
                
                // فحص لمس زر الركوب
                for (int i = 0; i < GetTouchPointCount(); i++) {
                    if (CheckCollisionPointRec(GetTouchPosition(i), btnEnter)) {
                        enterPressed = true;
                    }
                }

                if (enterPressed) {
                    isPlayerInside = true;
                    // إخفاء اللاعب ونقله لداخل السيارة (ستتم معالجتها في الرسم)
                }
            }

            // تطبيق فيزياء السيارة حتى وهي فارغة (لتتوقف إذا كانت تتدحرج)
            physics.Update(dt, {0.0f, 0.0f}, true, colliders); 
            return;
        }

        // 2. حالة اللاعب داخل السيارة
        if (isPlayerInside) {
            // تثبيت موقع اللاعب ليكون مع السيارة دائماً
            player->position = physics.position;

            // --- أ) نظام الإدخال للكمبيوتر (Keyboard) ---
            #if !defined(PLATFORM_ANDROID)
            if (IsKeyDown(KEY_W)) inputDirection.y += 1.0f;
            if (IsKeyDown(KEY_S)) inputDirection.y -= 1.0f;
            if (IsKeyDown(KEY_A)) inputDirection.x += 1.0f; // يسار (يعكس الزاوية)
            if (IsKeyDown(KEY_D)) inputDirection.x -= 1.0f; // يمين
            if (IsKeyDown(KEY_SPACE)) handbrake = true;
            #endif

            // --- ب) نظام الإدخال للأندرويد (Touch) ---
            bool exitPressed = IsKeyPressed(KEY_F); // خروج بالكمبيوتر

            for (int i = 0; i < GetTouchPointCount(); i++) {
                Vector2 touchPos = GetTouchPosition(i);
                
                if (CheckCollisionPointRec(touchPos, btnGas))   inputDirection.y += 1.0f;
                if (CheckCollisionPointRec(touchPos, btnBrake)) inputDirection.y -= 1.0f;
                if (CheckCollisionPointRec(touchPos, btnLeft))  inputDirection.x += 1.0f;
                if (CheckCollisionPointRec(touchPos, btnRight)) inputDirection.x -= 1.0f;
                if (CheckCollisionPointRec(touchPos, btnExit))  exitPressed = true;
            }

            // تنفيذ الخروج من السيارة
            if (exitPressed) {
                isPlayerInside = false;
                
                // نقل اللاعب لخارج السيارة (حسب زاوية السيارة لكي لا يظهر داخل الجدار)
                Matrix carRot = MatrixRotateY(physics.yaw);
                Vector3 worldExitOffset = Vector3Transform(exitOffset, carRot);
                player->position = Vector3Add(physics.position, worldExitOffset);
                
                // إضافة قفزة خفيفة للاعب لتجنب تعليقه في الأرض
                player->velocity.y = 5.0f; 
            }

            // 3. تحديث الفيزياء بناءً على الإدخالات
            physics.Update(dt, inputDirection, handbrake, colliders);
        }
    }

    // رسم السيارة في العالم ثلاثي الأبعاد
    void Draw() {
        // 1. جلب مصفوفة الموقع والدوران (Pitch, Yaw, Roll) من نظام الفيزياء
        Matrix physTransform = physics.GetTransformMatrix();

        // 2. تطبيق حجم السيارة الخاص بك بدقة (Scale = 0.01) الذي طلبته
        Matrix scaleMat = MatrixScale(0.01f, 0.01f, 0.01f);

        // 3. دمج المصفوفات وتطبيقها على النموذج
        // ملاحظة: ضرب المصفوفات غير تبادلي، الحجم أولاً ثم الموقع/الدوران
        carModel->transform = MatrixMultiply(scaleMat, physTransform);

        // 4. رسم السيارة
        // نمرر {0,0,0} كموقع لأن المصفوفة `transform` تحتوي بالفعل على الموقع النهائي
        DrawModel(*carModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

        // ملاحظة للمستقبل: إذا قمت بتفصيل ملف الـ GLB ليكون جسم السيارة منفصلاً عن العجلات، 
        // يمكنك هنا رسم العجلات بشكل مستقل بتطبيق زاوية الدوران من `physics.wheels[i].rotationAngle`
    }

    // رسم واجهة المستخدم 2D (HUD)
    void DrawUI(const Player* player) {
        // 1. واجهة "خارج السيارة" (زر الركوب)
        if (!isPlayerInside) {
            float distToPlayer = Vector3Distance(physics.position, player->position);
            if (distToPlayer <= interactionRadius) {
                DrawRectangleRounded(btnEnter, 0.3f, 10, Fade(BLACK, 0.7f));
                DrawRectangleRoundedLines(btnEnter, 0.3f, 10, 2.0f, YELLOW);
                DrawText("DRIVE", (int)btnEnter.x + 25, (int)btnEnter.y + 20, 20, WHITE);
            }
            return;
        }

        // 2. واجهة "داخل السيارة"
        // رسم أزرار التحكم للأندرويد
        #if defined(PLATFORM_ANDROID) || true // يمكنك إزالة || true لاحقاً (موضوعة للتجربة بالماوس)
            // أزرار الدواسات
            DrawRectangleRounded(btnGas, 0.2f, 10, Fade(GREEN, 0.3f));
            DrawText("GAS", (int)btnGas.x + 25, (int)btnGas.y + 65, 20, WHITE);

            DrawRectangleRounded(btnBrake, 0.2f, 10, Fade(RED, 0.3f));
            DrawText("BRAKE", (int)btnBrake.x + 15, (int)btnBrake.y + 25, 20, WHITE);

            // أزرار التوجيه
            DrawRectangleRounded(btnLeft, 0.5f, 10, Fade(DARKGRAY, 0.5f));
            DrawText("<", (int)btnLeft.x + 40, (int)btnLeft.y + 35, 30, WHITE);

            DrawRectangleRounded(btnRight, 0.5f, 10, Fade(DARKGRAY, 0.5f));
            DrawText(">", (int)btnRight.x + 40, (int)btnRight.y + 35, 30, WHITE);

            // زر الخروج
            DrawRectangleRounded(btnExit, 0.3f, 10, Fade(RED, 0.7f));
            DrawText("EXIT", (int)btnExit.x + 25, (int)btnExit.y + 15, 20, WHITE);
        #endif

        // 3. رسم عداد السرعة (Speedometer) والغيارات
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        
        // تحويل السرعة من (متر/ثانية) إلى (كم/ساعة)
        int speedKmh = (int)(physics.currentSpeed * 3.6f); 
        
        // مربع العداد
        DrawRectangleRounded({ (float)sw / 2 - 100, (float)sh - 100, 200, 80 }, 0.5f, 10, Fade(BLACK, 0.6f));
        
        // النص (السرعة)
        DrawText(TextFormat("%03d", speedKmh), sw / 2 - 40, sh - 90, 40, WHITE);
        DrawText("KM/H", sw / 2 + 45, sh - 75, 20, LIGHTGRAY);
        
        // الترس الحالي (Gear)
        const char* gearText = "N";
        if (physics.currentGear == 0) gearText = "R";
        else if (physics.currentGear >= 2) gearText = TextFormat("D%d", physics.currentGear - 1);
        
        DrawText(gearText, sw / 2 - 80, sh - 80, 30, (physics.currentGear == 0) ? RED : GREEN);

        // شريط الـ RPM (دوران المحرك)
        float rpmRatio = physics.currentRPM / physics.maxRPM;
        Color rpmColor = (rpmRatio > 0.8f) ? RED : (rpmRatio > 0.5f ? YELLOW : GREEN);
        
        DrawRectangle(sw / 2 - 80, sh - 30, 160, 10, Fade(DARKGRAY, 0.5f)); // الخلفية
        DrawRectangle(sw / 2 - 80, sh - 30, (int)(160 * rpmRatio), 10, rpmColor); // الشريط المتلئ
    }
};
