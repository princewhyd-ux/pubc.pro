#pragma once

#include "raylib.h"
#include "raymath.h"
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace Utils {

    // ==========================================
    // 1. أدوات الرياضيات المتقدمة (Math Utils)
    // ==========================================
    
    constexpr float FLOAT_EPSILON = 0.0001f;

    // مقارنة آمنة للأرقام العشرية (لأن 0.1 + 0.2 في البرمجة لا تساوي 0.3 تماماً)
    inline bool IsEqual(float a, float b, float epsilon = FLOAT_EPSILON) {
        return std::fabs(a - b) <= epsilon;
    }

    // مقارنة آمنة لمتجهات 3D
    inline bool IsVector3Equal(Vector3 v1, Vector3 v2, float epsilon = FLOAT_EPSILON) {
        return IsEqual(v1.x, v2.x, epsilon) && 
               IsEqual(v1.y, v2.y, epsilon) && 
               IsEqual(v1.z, v2.z, epsilon);
    }

    // تطبيع آمن للمتجه (يمنع قسمة على الصفر التي تسبب انهيار اللعبة NaN)
    inline Vector3 SafeNormalize(Vector3 v) {
        float lengthSq = Vector3LengthSqr(v);
        if (lengthSq < FLOAT_EPSILON) {
            return {0.0f, 0.0f, 0.0f};
        }
        float length = std::sqrt(lengthSq);
        return {v.x / length, v.y / length, v.z / length};
    }

    // انتقال سلس احترافي (Smooth Damp) - المعيار في ألعاب AAA للكاميرا وحركة واجهة المستخدم
    inline float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {
        smoothTime = std::fmax(0.0001f, smoothTime);
        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
        float change = current - target;
        float originalTo = target;
        
        float maxChange = maxSpeed * smoothTime;
        change = Clamp(change, -maxChange, maxChange);
        target = current - change;
        
        float temp = (currentVelocity + omega * change) * deltaTime;
        currentVelocity = (currentVelocity - omega * temp) * exp;
        float output = target + (change + temp) * exp;
        
        if (originalTo - current > 0.0f == output > originalTo) {
            output = originalTo;
            currentVelocity = (output - originalTo) / deltaTime;
        }
        return output;
    }

    // Smooth Damp لمتجهات 3D (ممتاز لكاميرا الكتف Shoulder Camera)
    inline Vector3 SmoothDampVector3(Vector3 current, Vector3 target, Vector3& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {
        return {
            SmoothDamp(current.x, target.x, currentVelocity.x, smoothTime, maxSpeed, deltaTime),
            SmoothDamp(current.y, target.y, currentVelocity.y, smoothTime, maxSpeed, deltaTime),
            SmoothDamp(current.z, target.z, currentVelocity.z, smoothTime, maxSpeed, deltaTime)
        };
    }


    // ==========================================
    // 2. أدوات العشوائية (Random Utilities)
    // ==========================================

    // توليد رقم عشري عشوائي بسرعة فائقة باستخدام محرك Raylib (بدون مكتبات ثقيلة)
    inline float RandomFloat(float min, float max) {
        int randomInt = GetRandomValue(0, 10000);
        return min + (max - min) * (randomInt / 10000.0f);
    }

    // توليد متجه عشوائي (مفيد لانفجارات الجزيئات Particles واهتزاز الكاميرا)
    inline Vector3 RandomVector3(float min, float max) {
        return { RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max) };
    }


    // ==========================================
    // 3. أدوات النصوص والمعرفات (String Utilities)
    // ==========================================

    // تحويل Vector3 إلى نص (مفيد جداً في عملية الـ Debugging)
    inline std::string ToString(Vector3 v, int precision = 2) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) 
           << "X: " << v.x << ", Y: " << v.y << ", Z: " << v.z;
        return ss.str();
    }

    // خوارزمية FNV-1a Hashing (constexpr)
    // تقوم بتحويل أي نص مثل "idle_anim" إلى رقم Integer وقت الترجمة (Compile-time).
    // هذا يعني أن المقارنة بين الأسماء ستأخذ 0% من المعالج! (أسرع من std::string بأضعاف).
    constexpr uint32_t HashString(const char* str, uint32_t value = 2166136261u) {
        return *str ? HashString(str + 1, (value ^ static_cast<uint32_t>(*str)) * 16777619u) : value;
    }

    // نسخة لدعم std::string (تعمل وقت التشغيل Runtime)
    inline uint32_t HashString(const std::string& str) {
        uint32_t hash = 2166136261u;
        for (char c : str) {
            hash ^= static_cast<uint32_t>(c);
            hash *= 16777619u;
        }
        return hash;
    }
}
