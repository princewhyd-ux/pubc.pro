#pragma once

#include "raylib.h"
#include "raymath.h"
#include "BlendTree.h" // محرك الدمج الذي بنيناه سابقاً
#include <string>
#include <unordered_map>
#include <iostream>

// قائمة بكل الحركات الممكنة لضمان عدم وجود أخطاء إملائية في الكود
enum class AnimID {
    IDLE, 
    WALK, 
    RUN, 
    CROUCH_IDLE, 
    CROUCH_TO_PRONE,
    PRONE_IDLE, 
    PRONE_TO_CROUCH,
    JUMP, 
    LAND, 
    FIRE_STAND, 
    FIRE_RUN, 
    FIRE_CROUCH, 
    KICK, 
    DRIVE
};

class AnimationController {
private:
    BlendTree blendTree;
    std::unordered_map<AnimID, ModelAnimation*> animMap;

    // متغيرات التزامن لدمج الحركات (Locomotion)
    float normalizedTime = 0.0f;
    
    // متغيرات تشغيل الحركات الفردية (Single-shot) مثل القفز والضرب
    float overrideAnimTime = 0.0f;
    bool isPlayingOverride = false;
    AnimID currentOverrideAnim;

public:
    AnimationController() = default;

    /**
     * @brief تهيئة المحرك وربط الحركات من ملف الـ GLB بشكل ذكي
     */
    void Init(Model& model, ModelAnimation* anims, int animCount) {
        // تهيئة محرك الدمج (BlendTree) لحجز الذاكرة مرة واحدة فقط للأندرويد
        blendTree.Init(model.boneCount, model.bones);

        // محرك البحث الذكي لربط الأسماء بالأنيميشنات الحقيقية
        for (int i = 0; i < animCount; i++) {
            std::string name = anims[i].name;
            
            if (name.find("idle_25") != std::string::npos || name.find("IDLE") != std::string::npos) 
                animMap[AnimID::IDLE] = &anims[i];
            else if (name.find("med_run_30") != std::string::npos || name.find("run_30") != std::string::npos) 
                animMap[AnimID::WALK] = &anims[i];
            else if (name.find("fast_run_18") != std::string::npos) 
                animMap[AnimID::RUN] = &anims[i];
            else if (name.find("c_idle_7") != std::string::npos) 
                animMap[AnimID::CROUCH_IDLE] = &anims[i];
            else if (name.find("c_to_p_8") != std::string::npos) 
                animMap[AnimID::CROUCH_TO_PRONE] = &anims[i];
            else if (name.find("p_idle_34") != std::string::npos) 
                animMap[AnimID::PRONE_IDLE] = &anims[i];
            else if (name.find("p_to_c_35") != std::string::npos) 
                animMap[AnimID::PRONE_TO_CROUCH] = &anims[i];
            else if (name.find("26") != std::string::npos || name.find("Jump_27") != std::string::npos) 
                animMap[AnimID::JUMP] = &anims[i];
            else if (name.find("land_29") != std::string::npos) 
                animMap[AnimID::LAND] = &anims[i];
            else if (name.find("fire_2_19") != std::string::npos) 
                animMap[AnimID::FIRE_STAND] = &anims[i];
            else if (name.find("fire_20") != std::string::npos) 
                animMap[AnimID::FIRE_RUN] = &anims[i];
            else if (name.find("c_fire_5") != std::string::npos) 
                animMap[AnimID::FIRE_CROUCH] = &anims[i];
            else if (name.find("kick_28") != std::string::npos) 
                animMap[AnimID::KICK] = &anims[i];
            else if (name.find("drive_13") != std::string::npos) 
                animMap[AnimID::DRIVE] = &anims[i];
        }

        // حماية المحرك من الانهيار إذا لم يجد أنيميشن معين في الـ GLB
        // سيقوم بوضع حركة الوقوف (IDLE) كحركة افتراضية (Fallback)
        for (int i = 0; i <= (int)AnimID::DRIVE; i++) {
            if (animMap.find((AnimID)i) == animMap.end()) {
                animMap[(AnimID)i] = animMap[AnimID::IDLE] ? animMap[AnimID::IDLE] : &anims[0];
            }
        }
    }

    /**
     * @brief دمج حركات المشي والركض والوقوف بناءً على سرعة اللاعب الحقيقية (1D Blend Space)
     */
    void UpdateLocomotion(Model& model, float currentSpeed, float walkSpeed, float runSpeed, float dt) {
        // إذا كان هناك حركة إجبارية تعمل (مثل القفز أو الضربة)، لا تقم بدمج حركة المشي
        if (isPlayingOverride) return;

        ModelAnimation* idleAnim = animMap[AnimID::IDLE];
        ModelAnimation* walkAnim = animMap[AnimID::WALK];
        ModelAnimation* runAnim  = animMap[AnimID::RUN];

        // زيادة سرعة التشغيل إذا كان يركض
        float speedRatio = currentSpeed / runSpeed;
        float animSpeedMultiplier = 1.0f + (speedRatio * 0.5f);
        
        normalizedTime += dt * animSpeedMultiplier;
        if (normalizedTime > 1.0f) normalizedTime -= 1.0f; // إعادة الدورة (Loop)

        // دمج ناعم للحالات بناءً على السرعة
        if (currentSpeed <= 0.01f) {
            // اللاعب واقف تماماً
            blendTree.UpdateBlend1D(model, *idleAnim, normalizedTime, *idleAnim, normalizedTime, 0.0f);
        }
        else if (currentSpeed <= walkSpeed) {
            // دمج بين الوقوف والمشي
            float blendWeight = currentSpeed / walkSpeed;
            blendTree.UpdateBlend1D(model, *idleAnim, normalizedTime, *walkAnim, normalizedTime, blendWeight);
        }
        else {
            // دمج بين المشي والركض
            float blendWeight = (currentSpeed - walkSpeed) / (runSpeed - walkSpeed);
            blendWeight = Clamp(blendWeight, 0.0f, 1.0f);
            blendTree.UpdateBlend1D(model, *walkAnim, normalizedTime, *runAnim, normalizedTime, blendWeight);
        }
    }

    /**
     * @brief تشغيل أنيميشن فردي (مثل الانحناء، القفز، الهبوط) يلغي دمج المشي مؤقتاً
     * @param loop إذا كان true، يستمر الأنيميشن (مثل وضعية الانحناء). إذا كان false، ينتهي ويعود للمشي.
     */
    void PlayOverrideAnimation(Model& model, AnimID animID, float dt, bool loop = true) {
        // إذا تم تغيير الحركة المطلوبة، نبدأ من الإطار الأول
        if (!isPlayingOverride || currentOverrideAnim != animID) {
            isPlayingOverride = true;
            currentOverrideAnim = animID;
            overrideAnimTime = 0.0f;
        }

        ModelAnimation* targetAnim = animMap[animID];
        
        // افتراض أن الأنيميشن يعمل بـ 30 إطار في الثانية
        float fps = 30.0f;
        overrideAnimTime += dt * fps;
        
        int currentFrame = (int)overrideAnimTime;

        // التحقق من انتهاء الحركة
        if (currentFrame >= targetAnim->frameCount) {
            if (loop) {
                // إعادة الحركة من البداية
                overrideAnimTime = 0.0f;
                currentFrame = 0;
            } else {
                // تجميد الحركة عند الإطار الأخير وإلغاء حالة الاستبدال
                currentFrame = targetAnim->frameCount - 1;
                isPlayingOverride = false; 
            }
        }

        // تطبيق الحركة على النموذج
        UpdateModelAnimation(model, *targetAnim, currentFrame);
    }

    /**
     * @brief يجب استدعاؤها في الـ Exit الخاصة بأي State لإعادة التحكم لنظام الـ Locomotion
     */
    void StopOverrideAnimation() {
        isPlayingOverride = false;
        overrideAnimTime = 0.0f;
    }

    // تفريغ الذاكرة
    void Unload() {
        blendTree.Unload();
    }
};
