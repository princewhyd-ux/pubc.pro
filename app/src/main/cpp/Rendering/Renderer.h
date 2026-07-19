#pragma once

#include "raylib.h"
#include "raymath.h"
#include "FrustumCulling.h"
#include <iostream>

class Renderer {
private:
    FrustumCuller culler;
    RenderTexture2D mainRenderTarget; // الشاشة الوهمية لدعم الدقة الديناميكية
    float currentResolutionScale = 1.0f;
    bool isInitialized = false;

public:
    Renderer() = default;
    ~Renderer() { Unload(); }

    /**
     * @brief تهيئة محرك الرسوميات (يستدعى مرة واحدة في البداية)
     * @param resolutionScale دقة العرض (1.0 = دقة الشاشة الكاملة، 0.7 = 70% لرفع الأداء على الأندرويد)
     */
    void Init(float resolutionScale = 1.0f) {
        if (isInitialized) return;

        currentResolutionScale = Clamp(resolutionScale, 0.1f, 1.0f);
        
        int renderWidth = (int)(GetScreenWidth() * currentResolutionScale);
        int renderHeight = (int)(GetScreenHeight() * currentResolutionScale);

        // إنشاء لوحة الرسم الوهمية
        mainRenderTarget = LoadRenderTexture(renderWidth, renderHeight);
        
        // تحسين مظهر التكبير (Bilinear Filtering) لكي لا تبدو اللعبة "مبكسلة" عند خفض الدقة
        SetTextureFilter(mainRenderTarget.texture, TEXTURE_FILTER_BILINEAR);

        isInitialized = true;
        std::cout << "[Renderer] Initialized at " << renderWidth << "x" << renderHeight 
                  << " (" << (currentResolutionScale * 100) << "% scale)." << std::endl;
    }

    /**
     * @brief يغير الدقة أثناء اللعب (مفيد لتوفير البطارية أو إذا انخفض الـ FPS)
     */
    void SetResolutionScale(float newScale) {
        if (!isInitialized || newScale == currentResolutionScale) return;
        
        UnloadRenderTexture(mainRenderTarget);
        Init(newScale);
    }

    /**
     * @brief بداية رسم الإطار (يجب استدعاؤه أولاً في الـ Game Loop)
     */
    void BeginFrame() {
        // توجيه الرسم إلى اللوحة الوهمية بدلاً من الشاشة المباشرة
        BeginTextureMode(mainRenderTarget);
        ClearBackground(SKYBLUE); // لون السماء الافتراضي
    }

    /**
     * @brief الدخول في وضع العالم ثلاثي الأبعاد وتفعيل الكاميرا
     */
    void BeginScene3D(const Camera3D& camera) {
        // تحديث هرم الرؤية (Frustum) بناءً على الكاميرا الحالية لقص المجسمات غير المرئية
        float aspect = (float)mainRenderTarget.texture.width / (float)mainRenderTarget.texture.height;
        culler.Update(camera, aspect);

        BeginMode3D(camera);
    }

    /**
     * @brief دالة ذكية للرسم: لا ترسم المجسم إلا إذا كان داخل مجال رؤية الكاميرا
     * @param model النموذج المراد رسمه
     * @param position موقعه في العالم
     * @param transform مصفوفة الدوران والحجم الخاصة به
     * @param localBounds الصندوق المحيط بالنموذج (لتحديد حجمه)
     * @return true إذا تم رسمه، false إذا تم إخفاؤه لرفع الأداء
     */
    bool DrawModelOptimized(Model& model, Vector3 position, Matrix transform, const BoundingBox& localBounds) const {
        // حساب الصندوق المحيط في العالم الحقيقي
        BoundingBox worldBounds = {
            Vector3Add(localBounds.min, position),
            Vector3Add(localBounds.max, position)
        };

        // التحقق مما إذا كان الكائن مرئياً للكاميرا
        if (culler.IsBoxVisible(worldBounds)) {
            model.transform = transform;
            DrawModel(model, position, 1.0f, WHITE);
            return true; 
        }
        
        return false; // الكائن خلف الكاميرا أو بعيد، تم توفير طاقة المعالج!
    }

    /**
     * @brief الخروج من وضع العالم ثلاثي الأبعاد
     */
    void EndScene3D() {
        EndMode3D();
    }

    /**
     * @brief نقل ما تم رسمه في اللوحة الوهمية إلى الشاشة الحقيقية وبدء رسم واجهة المستخدم 2D
     */
    void BeginUI() {
        EndTextureMode(); // التوقف عن الرسم في اللوحة الوهمية

        BeginDrawing();   // بدء الرسم على شاشة الهاتف الحقيقية
        ClearBackground(BLACK);

        // رسم اللوحة الوهمية (بحجمها المخفض) وتكبيرها لتملأ شاشة الهاتف بالكامل
        Rectangle sourceRec = { 
            0.0f, 0.0f, 
            (float)mainRenderTarget.texture.width, 
            -(float)mainRenderTarget.texture.height // القيمة السالبة لقلب الصورة (Raylib OpenGL quirk)
        };
        
        Rectangle destRec = { 
            0.0f, 0.0f, 
            (float)GetScreenWidth(), 
            (float)GetScreenHeight() 
        };

        // هنا يتم دمج الـ Shaders (Post-Processing) لاحقاً إذا أردت استخدام BeginShaderMode
        DrawTexturePro(mainRenderTarget.texture, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    /**
     * @brief إنهاء رسم الإطار بالكامل (يرسله لبطاقة الشاشة ليعرضه للاعب)
     */
    void EndFrame() {
        EndDrawing();
    }

    // تنظيف الذاكرة (VRAM)
    void Unload() {
        if (isInitialized) {
            UnloadRenderTexture(mainRenderTarget);
            isInitialized = false;
            std::cout << "[Renderer] Unloaded successfully." << std::endl;
        }
    }
    
    // دالة مساعدة لتمرير الـ Culler لمن يحتاجه (مثل الظلال المستقبلية)
    const FrustumCuller& GetCuller() const {
        return culler;
    }
};
