#pragma once

#include "raylib.h"
#include "raymath.h"

/**
 * @brief واجهة الإدخال الموحدة.
 * أي جهاز إدخال (شاشة لمس، كيبورد/ماوس، أو حتى يد تحكم PlayStation/Xbox) 
 * يجب أن يرث من هذا الكلاس ويقوم ببرمجة هذه الدوال.
 */
class IInputDevice {
public:
    // مدمر افتراضي (Virtual Destructor) ضروري لمنع تسريب الذاكرة عند التبديل بين الأجهزة
    virtual ~IInputDevice() = default;

    /**
     * @brief التحديث اليومي للإدخال (يُستدعى كل إطار)
     * مفيد لتحديث حالة الجويستيك أو تفريغ الذاكرة المؤقتة للمس
     */
    virtual void Update() = 0;

    // ==========================================
    // 1. دوال الحركة (Locomotion & Camera)
    // ==========================================

    /**
     * @brief جلب اتجاه حركة اللاعب (X و Y)
     * - في الكمبيوتر: أزرار WASD
     * - في الأندرويد: الجويستيك الأيسر
     * @return متجه (Vector2) يمثل الاتجاه، ويجب أن يكون Normalized لتجنب الحركة السريعة المائلة
     */
    virtual Vector2 GetMovementDirection() const = 0;

    /**
     * @brief جلب مقدار حركة الكاميرا في هذا الإطار
     * - في الكمبيوتر: حركة الماوس (Mouse Delta)
     * - في الأندرويد: سحب الإصبع على النصف الأيمن من الشاشة
     */
    virtual Vector2 GetCameraDelta() const = 0;


    // ==========================================
    // 2. دوال الأزرار والأفعال (Actions)
    // ==========================================

    /**
     * @brief هل زر القفز مضغوط حالياً؟
     */
    virtual bool IsJumpPressed() const = 0;

    /**
     * @brief هل زر الركض السريع (Sprint) مضغوط؟
     * - في الأندرويد: دفع الجويستيك لأقصى حافة
     * - في الكمبيوتر: الضغط على زر Shift
     */
    virtual bool IsSprintPressed() const = 0;

    /**
     * @brief هل زر الانحناء (Crouch) مضغوط؟
     */
    virtual bool IsCrouchPressed() const = 0;

    /**
     * @brief هل زر الانبطاح (Prone) مضغوط؟
     */
    virtual bool IsPronePressed() const = 0;

    /**
     * @brief هل زر إطلاق النار (Fire) مضغوط؟
     */
    virtual bool IsFirePressed() const = 0;

    /**
     * @brief هل زر التصويب الدقيق (Aim / ADS) مضغوط؟
     */
    virtual bool IsAimPressed() const = 0;

    /**
     * @brief هل زر التفاعل (Interact / ركوب السيارة / فتح الباب) مضغوط؟
     */
    virtual bool IsInteractPressed() const = 0;
};
