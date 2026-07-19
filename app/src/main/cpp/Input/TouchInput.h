#pragma once

#include "raylib.h"
#include "raymath.h"
#include <map>

// حالة كل إصبع يلمس الشاشة
struct TouchState {
    Vector2 startPos;       // النقطة التي بدأ فيها اللمس
    Vector2 currentPos;     // النقطة الحالية
    Vector2 delta;          // مقدار الحركة في هذا الإطار (مهم جداً للكاميرا)
    bool isUIClaimed;       // هل هذا الإصبع محجوز لزر من أزرار الشاشة؟
    bool isActive;          // هل الإصبع لا يزال على الشاشة؟
};

class TouchInput {
private:
    std::map<int, TouchState> activeTouches; // قاموس يحفظ كل إصبع بناءً على الـ ID الخاص به
    Vector2 mouseDelta; // حفظ حركة الماوس للكمبيوتر

public:
    TouchInput() = default;

    /**
     * @brief يجب استدعاء هذه الدالة مرة واحدة بداية كل إطار (في حلقة Update)
     */
    void Update() {
        // 1. تحديد جميع اللمسات كـ "غير نشطة" مبدئياً
        for (auto& pair : activeTouches) {
            pair.second.isActive = false;
            pair.second.delta = { 0.0f, 0.0f };
        }

        // 2. تحديث حركة الماوس للكمبيوتر (PC Fallback)
        mouseDelta = GetMouseDelta();

        // 3. قراءة كل الأصابع الموجودة على الشاشة حالياً
        int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; i++) {
            int id = GetTouchPointId(i);
            Vector2 pos = GetTouchPosition(i);

            // إذا كان هذا الإصبع جديداً (เพิ่ง نزل على الشاشة)
            if (activeTouches.find(id) == activeTouches.end()) {
                activeTouches[id] = { pos, pos, {0.0f, 0.0f}, false, true };
            } 
            // إذا كان الإصبع موجوداً مسبقاً (يتحرك)
            else {
                activeTouches[id].delta = Vector2Subtract(pos, activeTouches[id].currentPos);
                activeTouches[id].currentPos = pos;
                activeTouches[id].isActive = true;
            }
        }

        // 4. حذف الأصابع التي رُفعت عن الشاشة
        for (auto it = activeTouches.begin(); it != activeTouches.end(); ) {
            if (!it->second.isActive) {
                it = activeTouches.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief فحص ما إذا كان هناك أي إصبع يلمس منطقة معينة (للأزرار)
     * وإذا وجد، يقوم بـ "حجز" الإصبع لكي لا تستخدمه الكاميرا.
     * @param area المربع الذي يمثل الزر
     * @return true إذا تم اللمس
     */
    bool CheckAndClaimButton(Rectangle area) {
        // فحص اللمس للأندرويد
        for (auto& pair : activeTouches) {
            if (CheckCollisionPointRec(pair.second.currentPos, area)) {
                pair.second.isUIClaimed = true; // حجز الإصبع
                return true;
            }
        }

        // فحص الماوس للكمبيوتر
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), area)) {
            return true;
        }

        return false;
    }

    /**
     * @brief جلب حركة الكاميرا الصافية (Drag Delta) من الجزء الأيمن من الشاشة.
     * يتجاهل الأصابع المحجوزة لأزرار الواجهة (UI).
     * @param screenSplitX نقطة تقسيم الشاشة (عادة منتصف الشاشة GetScreenWidth() / 2)
     */
    Vector2 GetCameraDelta(float screenSplitX) const {
        Vector2 totalDelta = { 0.0f, 0.0f };

        // 1. حساب حركة الأصابع (Android)
        bool hasTouch = false;
        for (const auto& pair : activeTouches) {
            const TouchState& touch = pair.second;
            
            // نأخذ الحركة فقط إذا كان الإصبع في الجهة اليمنى للرؤية، ولم يحجزه زر (مثل زر الضرب)
            if (touch.currentPos.x > screenSplitX && !touch.isUIClaimed) {
                totalDelta = Vector2Add(totalDelta, touch.delta);
                hasTouch = true;
            }
        }

        // 2. إذا لم يكن هناك لمس (نحن على الكمبيوتر)، نقرأ حركة الماوس
        if (!hasTouch && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            // كخيار للكمبيوتر: نتجاهل الـ UI إذا أردت، أو نستخدم الماوس بحرية
            totalDelta = mouseDelta; 
        }

        return totalDelta;
    }

    /**
     * @brief الحصول على حالة إصبع معين إذا احتجته لبرمجة جويستيك (Joystick) مخصص
     */
    bool GetTouchState(int id, TouchState& outState) const {
        auto it = activeTouches.find(id);
        if (it != activeTouches.end()) {
            outState = it->second;
            return true;
        }
        return false;
    }

    /**
     * @brief إرجاع عدد الأصابع النشطة حالياً
     */
    int GetActiveTouchCount() const {
        return activeTouches.size();
    }
};
