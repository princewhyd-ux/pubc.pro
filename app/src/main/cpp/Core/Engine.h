#pragma once

#include "raylib.h"
#include <string>

// هيكل يحتوي على إعدادات المحرك الأولية
struct EngineConfig {
    int screenWidth = 1280;
    int screenHeight = 720;
    std::string title = "My 3D Engine";
    int targetFPS = 60;          // 60 أو 120 حسب رغبتك
    bool useVSync = true;        // يمنع تمزق الشاشة (Screen Tearing)
    bool useMSAA = true;         // تنعيم الحواف (Anti-Aliasing)
    bool fullscreen = false;     // وضع ملء الشاشة للكمبيوتر
};

class Engine {
private:
    EngineConfig config;
    bool isInitialized;

public:
    Engine() : isInitialized(false) {}

    // يتم استدعاء Shutdown تلقائياً عند تدمير الكائن لضمان عدم بقاء النافذة معلقة
    ~Engine() {
        if (isInitialized) {
            Shutdown();
        }
    }

    // دالة التهيئة الرئيسية للمحرك
    void Init(const EngineConfig& cfg) {
        config = cfg;

        // 1. تجميع أعلام التهيئة (Flags) بناءً على الإعدادات
        unsigned int flags = 0;
        
        if (config.useVSync) flags |= FLAG_VSYNC_HINT;
        if (config.useMSAA)  flags |= FLAG_MSAA_4X_HINT;
        if (config.fullscreen) flags |= FLAG_FULLSCREEN_MODE;

        // 2. إعدادات خاصة بمنصة الأندرويد
        #if defined(PLATFORM_ANDROID)
            flags |= FLAG_WINDOW_RESIZABLE; // ضروري ليتوافق مع مختلف شاشات الهواتف
            // في الأندرويد، نمرر 0, 0 ليقوم Raylib بأخذ الدقة الأصلية للشاشة تلقائياً
            config.screenWidth = 0;
            config.screenHeight = 0;
        #endif

        // تطبيق الأعلام قبل فتح النافذة
        SetConfigFlags(flags);

        // 3. إنشاء النافذة
        InitWindow(config.screenWidth, config.screenHeight, config.title.c_str());

        // 4. ضبط معدل الإطارات المستهدف
        SetTargetFPS(config.targetFPS);

        isInitialized = true;
    }

    // للتحقق مما إذا كان المستخدم قد طلب إغلاق اللعبة
    bool IsRunning() const {
        return !WindowShouldClose();
    }

    // بداية رسم الإطار الجديد
    void BeginFrame() const {
        BeginDrawing();
        ClearBackground(BLACK); // اللون الافتراضي قبل رسم أي شيء
    }

    // نهاية رسم الإطار وعرضه على الشاشة
    void EndFrame() const {
        EndDrawing();
    }

    // إغلاق المحرك وتفريغ الذاكرة
    void Shutdown() {
        if (isInitialized) {
            CloseWindow();
            isInitialized = false;
        }
    }

    // دوال مساعدة للوصول السريع لمعلومات المحرك
    float GetDeltaTime() const {
        return GetFrameTime();
    }
    
    int GetCurrentFPS() const {
        return GetFPS(); // استخدام دالة Raylib الأصلية
    }

    int GetScreenWidth() const {
        return ::GetScreenWidth();
    }

    int GetScreenHeight() const {
        return ::GetScreenHeight();
    }
};
