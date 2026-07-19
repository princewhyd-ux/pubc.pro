#pragma once

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h" // ضروري للتحكم في Backface Culling للبيوت والخريطة
#include <vector>
#include <string>
#include <iostream>

// أنواع مجسمات التصادم في العالم
enum class ColliderType {
    STATIC_WALL,
    STATIC_GROUND,
    STATIC_PROP,
    TRIGGER_ZONE
};

// هيكل يحمل بيانات التصادم ليستخدمه اللاعب والسيارة والـ NPC
struct EnvironmentCollider {
    BoundingBox bounds;
    ColliderType type;
    std::string tag;
};

// هيكل لتمثيل العناصر الثابتة (مثل المنازل، الأشجار، الصناديق)
struct StaticProp {
    Model* model;
    Vector3 position;
    float scale;
    Color tint;
};

class MapManager {
private:
    Model mapModel;
    Model groundModel;
    Model houseModel; // يمكنك لاحقاً تحويلها إلى مصفوفة نماذج (Array of Models)
    Texture2D groundTex;

    std::vector<EnvironmentCollider> colliders;
    std::vector<StaticProp> worldProps;

    bool isInitialized = false;

    // دالة داخلية لتوليد الأرضية اللانهائية
    void GenerateGround() {
        Image groundImg = LoadImage("ground.png");
        if (groundImg.data != nullptr) {
            groundTex = LoadTextureFromImage(groundImg);
            UnloadImage(groundImg);
        } else {
            // في حال لم يجد الصورة، نولد صورة مؤقتة لمنع الانهيار
            Image tempImg = GenImageChecked(512, 512, 64, 64, DARKGRAY, LIGHTGRAY);
            groundTex = LoadTextureFromImage(tempImg);
            UnloadImage(tempImg);
        }

        // إنشاء شبكة أرضية مساحتها 2000x2000 متر
        Mesh groundMesh = GenMeshPlane(2000.0f, 2000.0f, 1, 1);
        
        // تكرار الصورة 500 مرة حتى لا تبدو ممطوطة (UV Mapping)
        for (int i = 0; i < groundMesh.vertexCount; i++) {
            groundMesh.texcoords[i * 2] *= 500.0f;     
            groundMesh.texcoords[i * 2 + 1] *= 500.0f; 
        }
        
        UpdateMeshBuffer(groundMesh, 1, groundMesh.texcoords, groundMesh.vertexCount * 2 * sizeof(float), 0);
        groundModel = LoadModelFromMesh(groundMesh);
        groundModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTex;
    }

    // بناء جدران الحماية حول الخريطة لمنع السقوط
    void BuildWorldBoundaries() {
        colliders.push_back({ { {-276.0f, -10.0f, -276.0f}, {-275.0f, 50.0f, 276.0f} }, ColliderType::STATIC_WALL, "Wall_Left" });
        colliders.push_back({ { {275.0f, -10.0f, -276.0f}, {276.0f, 50.0f, 276.0f} }, ColliderType::STATIC_WALL, "Wall_Right" });
        colliders.push_back({ { {-276.0f, -10.0f, -276.0f}, {276.0f, 50.0f, -275.0f} }, ColliderType::STATIC_WALL, "Wall_Back" });
        colliders.push_back({ { {-276.0f, -10.0f, 275.0f}, {276.0f, 50.0f, 276.0f} }, ColliderType::STATIC_WALL, "Wall_Front" });
        
        // يمكن إضافة صناديق التصادم الخاصة بالمنازل هنا لاحقاً
    }

public:
    MapManager() = default;
    ~MapManager() { Unload(); }

    void Init() {
        if (isInitialized) return;

        // 1. تحميل الخريطة وتطبيق التكبير الدقيق الخاص بك
        mapModel = LoadModel("map.glb");
        mapModel.transform = MatrixScale(3.0f, 3.0f, 3.0f);

        // 2. تحميل المنزل، وإصلاح دورانه (-90 درجة على محور X) وحجمه
        houseModel = LoadModel("house1.glb");
        Matrix houseScale = MatrixScale(0.01f, 0.01f, 0.01f);
        Matrix charRot = MatrixRotateX(-PI / 2.0f);
        houseModel.transform = MatrixMultiply(houseScale, charRot);

        // إضافة المنزل الأول إلى قائمة عناصر العالم (هذا يسهل إضافة منازل أخرى)
        worldProps.push_back({ &houseModel, { -15.0f, 0.0f, 15.0f }, 1.0f, WHITE });

        // 3. بناء الأرضية
        GenerateGround();

        // 4. بناء الجدران الفيزيائية
        BuildWorldBoundaries();

        isInitialized = true;
        std::cout << "[MapManager] World loaded successfully!" << std::endl;
    }

    void Draw() {
        if (!isInitialized) return;

        // 1. رسم الأرضية (لا تحتاج لإلغاء الـ Culling لأنها مسطحة)
        DrawModel(groundModel, { 0.0f, -0.1f, 0.0f }, 1.0f, WHITE);

        // تعطيل الـ Backface Culling حتى تظهر الخريطة والبيوت من الداخل إذا دخلها اللاعب
        rlDisableBackfaceCulling();

        // 2. رسم الخريطة الرئيسية
        DrawModel(mapModel, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

        // 3. رسم جميع المنازل والعناصر الثابتة في العالم
        for (const auto& prop : worldProps) {
            DrawModel(*(prop.model), prop.position, prop.scale, prop.tint);
        }

        // إعادة تفعيل الـ Backface Culling فوراً للحفاظ على الـ FPS للكيانات الأخرى (اللاعب، السيارات)
        rlEnableBackfaceCulling();
    }

    // دالة تستدعيها أنظمة اللاعب والسيارة للحصول على الجدران وإجراء فحص التصادم
    const std::vector<EnvironmentCollider>& GetColliders() const {
        return colliders;
    }

    // تفريغ الذاكرة الآمن (VRAM)
    void Unload() {
        if (isInitialized) {
            UnloadModel(mapModel);
            UnloadModel(houseModel);
            
            // groundModel تحتوي على الـ Mesh، ويجب تفريغ الـ Texture بشكل منفصل
            UnloadModel(groundModel);
            UnloadTexture(groundTex);

            colliders.clear();
            worldProps.clear();

            isInitialized = false;
            std::cout << "[MapManager] World unloaded successfully!" << std::endl;
        }
    }
};
