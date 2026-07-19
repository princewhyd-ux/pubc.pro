#pragma once

#include "raylib.h"
#include "raymath.h"
#include <iostream>

class BlendTree {
private:
    ModelAnimation blendedAnim; // الأنيميشن الوهمي الذي سنخزن فيه نتيجة الدمج
    bool isInitialized = false;

public:
    BlendTree() = default;
    ~BlendTree() { Unload(); }

    /**
     * @brief تهيئة الأنيميشن الوهمي وحجز الذاكرة. 
     * تُستدعى مرة واحدة فقط عند تحميل اللعبة لحماية الأندرويد من التقطيع (FPS Drop).
     * @param boneCount عدد عظام نموذج اللاعب
     * @param bones مصفوفة العظام من النموذج
     */
    void Init(int boneCount, BoneInfo* bones) {
        if (isInitialized) return;

        blendedAnim.boneCount = boneCount;
        blendedAnim.frameCount = 1;  // نحتاج إطاراً واحداً فقط للدمج اللحظي
        blendedAnim.bones = bones;   // نربط الأنيميشن الوهمي بعظام اللاعب الحقيقية
        blendedAnim.name[0] = '\0';  // اسم فارغ

        // حجز ذاكرة للإطار المدمج باستخدام دوال C الأساسية للحصول على أقصى سرعة
        // framePoses هو مصفوفة ثنائية الأبعاد [رقم الإطار][رقم العظمة]
        blendedAnim.framePoses = (Transform**)MemAlloc(sizeof(Transform*));
        blendedAnim.framePoses[0] = (Transform*)MemAlloc(boneCount * sizeof(Transform));
        
        isInitialized = true;
        std::cout << "[BlendTree] Initialized successfully with " << boneCount << " bones." << std::endl;
    }

    /**
     * @brief دمج حركتين بناءً على عامل الدمج (Blend Factor) من 0.0 إلى 1.0
     * @param animA الحركة الأولى (مثلاً: المشي)
     * @param timeA الوقت المرجعي للحركة الأولى (من 0.0 إلى 1.0)
     * @param animB الحركة الثانية (مثلاً: الركض)
     * @param timeB الوقت المرجعي للحركة الثانية (من 0.0 إلى 1.0)
     * @param blendFactor نسبة الدمج (0.0 = الحركة الأولى 100%، 1.0 = الحركة الثانية 100%)
     */
    void UpdateBlend1D(Model& model, const ModelAnimation& animA, float timeA, const ModelAnimation& animB, float timeB, float blendFactor) {
        if (!isInitialized || animA.frameCount == 0 || animB.frameCount == 0) return;

        // 1. تحديد الإطار الحالي لكل حركة بناءً على الوقت (Normalized Time)
        // هذا يمنع انزلاق الأقدام (Foot Sliding) لأننا نزامن دورة الخطوة
        int frameA = (int)(timeA * animA.frameCount) % animA.frameCount;
        int frameB = (int)(timeB * animB.frameCount) % animB.frameCount;

        // 2. ضمان بقاء نسبة الدمج في حدود الأمان
        blendFactor = Clamp(blendFactor, 0.0f, 1.0f);

        // 3. دمج كل عظمة في الجسم على حدة
        for (int i = 0; i < blendedAnim.boneCount; i++) {
            Transform transformA = animA.framePoses[frameA][i];
            Transform transformB = animB.framePoses[frameB][i];

            // أ) دمج الموقع (Translation) بخط مستقيم
            blendedAnim.framePoses[0][i].translation = Vector3Lerp(transformA.translation, transformB.translation, blendFactor);
            
            // ب) دمج الدوران (Rotation - Quaternion) باستخدام Slerp 
            // (الـ Lerp العادي يشوه العظام عند الدوران، الـ Slerp يدورها بشكل كروي ناعم)
            blendedAnim.framePoses[0][i].rotation = QuaternionSlerp(transformA.rotation, transformB.rotation, blendFactor);
            
            // ج) دمج الحجم (Scale) بخط مستقيم
            blendedAnim.framePoses[0][i].scale = Vector3Lerp(transformA.scale, transformB.scale, blendFactor);
        }

        // 4. تطبيق الأنيميشن الوهمي على المجسم ليتم رسمه
        UpdateModelAnimation(model, blendedAnim, 0);
    }

    /**
     * @brief تفريغ الذاكرة بأمان عند إغلاق اللعبة أو تدمير الكائن
     */
    void Unload() {
        if (isInitialized) {
            // يجب تحرير الذاكرة من الداخل للخارج لتجنب أخطاء المؤشرات (Dangling Pointers)
            if (blendedAnim.framePoses) {
                if (blendedAnim.framePoses[0]) {
                    MemFree(blendedAnim.framePoses[0]);
                }
                MemFree(blendedAnim.framePoses);
            }
            isInitialized = false;
        }
    }
};
