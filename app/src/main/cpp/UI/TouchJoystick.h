#pragma once

#include "raylib.h"
#include "raymath.h"

class TouchJoystick {
private:
    Vector2 anchorPosition;   // الموقع الافتراضي للجويستيك عند عدم اللمس
    Vector2 basePosition;     // موقع القاعدة (يتغير إذا كان الجويستيك ديناميكياً)
    Vector2 knobPosition;     // موقع المقبض (الزر الذي يتحرك)
    
    float baseRadius;         // نصف قطر الدائرة الكبيرة (نطاق الحركة)
    float knobRadius;         // نصف قطر المقبض الداخلي
    
    bool isDynamic;           // هل يظهر الجويستيك مكان لمس الإصبع؟
    bool isActive;            // هل اللاعب يلمس الجويستيك حالياً؟

    Vector2 direction;        // اتجاه الحركة (Normalized 0 to 1)
    float strength;           // قوة الضغطة (0.0 للوقوف، 1.0 للركض السريع)

public:
    // التهيئة: نحدد موقعه الافتراضي، حجمه، وما إذا كان عائماً
    TouchJoystick(Vector2 defaultPos, float bRadius = 80.0f, float kRadius = 35.0f, bool dynamic = true) {
        anchorPosition = defaultPos;
        basePosition = defaultPos;
        knobPosition = defaultPos;
        baseRadius = bRadius;
        knobRadius = kRadius;
        isDynamic = dynamic;
        isActive = false;
        direction = { 0.0f, 0.0f };
        strength = 0.0f;
    }

    void Update() {
        direction = { 0.0f, 0.0f };
        strength = 0.0f;
        bool touchDetected = false;
        Vector2 currentTouchPos = { 0.0f, 0.0f };

        // 1. نظام الأندرويد (تعدد اللمس Multi-Touch)
        int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; i++) {
            Vector2 touchPos = GetTouchPosition(i);
            
            // نأخذ أول لمسة تقع في النصف الأيسر من الشاشة للتحكم بالحركة
            if (touchPos.x < GetScreenWidth() / 2.0f) {
                currentTouchPos = touchPos;
                touchDetected = true;
                break; // وجدنا الإصبع المسؤول عن الحركة، نتجاهل الباقي
            }
        }

        // 2. محاكاة الماوس لتسهيل التطوير على الكمبيوتر
        #if !defined(PLATFORM_ANDROID)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mousePos = GetMousePosition();
            if (mousePos.x < GetScreenWidth() / 2.0f) {
                currentTouchPos = mousePos;
                touchDetected = true;
            }
        }
        #endif

        // 3. معالجة الحركة
        if (touchDetected) {
            // إذا كانت هذه بداية اللمسة والجويستيك ديناميكي، انقله لمكان الإصبع
            if (!isActive && isDynamic) {
                basePosition = currentTouchPos;
            }
            
            isActive = true;

            // حساب المسافة والاتجاه بين مركز الجويستيك والإصبع
            Vector2 diff = Vector2Subtract(currentTouchPos, basePosition);
            float distance = Vector2Length(diff);

            if (distance > 0.001f) {
                direction = Vector2Normalize(diff);
                // القوة تتراوح بين 0 (مركز) و 1 (حافة الجويستيك)
                strength = fminf(distance / baseRadius, 1.0f);

                // منع المقبض من الخروج عن الدائرة الرئيسية
                if (distance > baseRadius) {
                    knobPosition = Vector2Add(basePosition, Vector2Scale(direction, baseRadius));
                } else {
                    knobPosition = currentTouchPos;
                }
            }
        } else {
            // إعادة الجويستيك لوضعه الطبيعي عند ترك الشاشة
            isActive = false;
            basePosition = anchorPosition; // يعود لمكانه الافتراضي
            
            // عودة المقبض للمركز بنعومة (Spring effect)
            knobPosition.x = Lerp(knobPosition.x, basePosition.x, 15.0f * GetFrameTime());
            knobPosition.y = Lerp(knobPosition.y, basePosition.y, 15.0f * GetFrameTime());
        }
    }

    void Draw() const {
        // إذا كان ديناميكياً ولا يتم لمسه، يمكننا جعله شبه مخفي تماماً
        float alphaMult = (isDynamic && !isActive) ? 0.3f : 1.0f;

        // رسم القاعدة (خلفية الجويستيك)
        DrawCircleLines((int)basePosition.x, (int)basePosition.y, baseRadius, Fade(DARKGRAY, 0.5f * alphaMult));
        DrawCircle((int)basePosition.x, (int)basePosition.y, baseRadius, Fade(GRAY, 0.2f * alphaMult));

        // رسم المقبض المتحرك
        DrawCircle((int)knobPosition.x, (int)knobPosition.y, knobRadius, Fade(LIGHTGRAY, 0.8f * alphaMult));
    }

    // دوال لجلب البيانات إلى نظام حركة اللاعب (PlayerMovement)
    Vector2 GetDirection() const { return direction; }
    float GetStrength() const { return strength; }
    bool IsActive() const { return isActive; }
};
