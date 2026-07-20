#include "raylib.h"
#include "raymath.h"

// 1. استدعاء أنظمة المحرك الأساسية
#include "Rendering/Renderer.h"
#include "Rendering/PlayerCamera.h"
#include "Physics/CollisionSystem.h"
#include "Physics/Raycaster.h"

// 2. استدعاء واجهات اللمس الخاصة بك
#include "UI/HUDManager.h"
#include "UI/TouchJoystick.h"

// 3. استدعاء كيانات اللعبة
#include "Player/Player.h"
#include "NPC/NPC.h"
#include "Player/States/PlayerIdleState.h"

#include <iostream>
#include <memory>

int main() {
    // 1. تهيئة شاشة الهاتف (الوضع العرضي والكامل)
    InitWindow(0, 0, "Sufi Engine - Mobile");
    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 2. تهيئة أنظمة المحرك
    Renderer renderer; 
    renderer.Init(1.0f); 
    
    CollisionSystem collisionSystem;
    PlayerCamera camera;

    // ==========================================
    // تهيئة واجهة المستخدم (اللمس) الخاصة بك
    // ==========================================
    HUDManager hud;
    // نضع عصا التحكم في الجزء السفلي الأيسر من الشاشة
    TouchJoystick joystick({ 150.0f, (float)screenHeight - 150.0f }, 100.0f);

    // 3. تحميل المجسمات
    Model playerModel = LoadModel("player.glb");
    int playerAnimCount = 0;
    ModelAnimation* playerAnims = LoadModelAnimations("player.glb", &playerAnimCount);
    // 💡 ملاحظة: لم نضف MatrixRotateX هنا لأنك قمت ببرمجتها مسبقاً داخل Player.h (baseTransform) ببراعة!

    Model npcModel = LoadModel("woman1.glb");
    npcModel.transform = MatrixRotateX(PI / 2.0f);
    int npcAnimCount = 0;
    ModelAnimation* npcAnims = LoadModelAnimations("woman1.glb", &npcAnimCount);

    // ==========================================
    // إعداد الأرضية الحقيقية بالصورة
    // ==========================================
    Texture2D groundTex = LoadTexture("ground.png");
    SetTextureWrap(groundTex, TEXTURE_WRAP_REPEAT); 
    Mesh planeMesh = GenMeshPlane(200.0f, 200.0f, 20, 20); 
    Model groundModel = LoadModelFromMesh(planeMesh);
    groundModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = groundTex;

    // بناء جدران 
    collisionSystem.AddStaticCollider({ {-50, -1, -50}, {50, 0, 50} }, ColliderType::STATIC_GROUND, "Ground");
    collisionSystem.AddStaticCollider({ {5, 0, 5}, {10, 5, 10} }, ColliderType::STATIC_WALL, "Obstacle1");

    // 4. تهيئة اللاعب وتمرير الـ Joystick والـ HUD
    Player player(&playerModel, { 0.0f, 0.0f, 0.0f }, &hud, &joystick);
    player.SetAnimations(playerAnims, playerAnimCount);
    player.ChangeState(std::make_unique<PlayerIdleState>()); 

    NPC enemy(&npcModel, { -5.0f, 1.0f, -8.0f }, 1.8f);
    enemy.SetAnimations(npcAnims, npcAnimCount);

    collisionSystem.RegisterDynamicCollider(&player.movement.position, player.movement.radius);
    collisionSystem.RegisterDynamicCollider(&enemy.brain.position, 0.4f);

    // ==========================================
    // حلقة اللعبة (Game Loop)
    // ==========================================
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // 1. تحديث الإدخال (عصا التحكم والأزرار)
        joystick.Update();
        hud.Update(); // افتح هذا التعليق إذا كانت الـ HUD تحتاج تحديث

        // 2. تحديث الكاميرا (السحب بإصبعك على النصف الأيمن من الشاشة)
        Vector2 camDelta = {0.0f, 0.0f};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && GetMousePosition().x > screenWidth / 2.0f) {
            camDelta = GetMouseDelta();
        }
        camera.AddYawPitch(camDelta.x, camDelta.y);

        bool isCrouching = (player.currentState->GetName() == std::string("CROUCH"));
        
        // 3. تحديث الكيانات
        player.Update(dt, camera.yaw, collisionSystem.GetAllStaticColliders());
        enemy.Update(dt, player.movement.position, collisionSystem.GetAllStaticColliders());

        camera.Update(player.movement.position, isCrouching, dt, collisionSystem.GetAllStaticColliders());
        collisionSystem.ResolveDynamicOverlaps();

        // 4. الرسم
        renderer.BeginFrame();
            
            renderer.BeginScene3D(camera.camera);
                // رسم الأرضية
                DrawModel(groundModel, {0.0f, -0.5f, 0.0f}, 1.0f, WHITE);
                DrawCube({7.5f, 2.5f, 7.5f}, 5, 5, 5, GRAY);

                player.Draw();
                enemy.Draw();
            renderer.EndScene3D();

            renderer.BeginUI();
                // رسم واجهة المستخدم واللمس
                joystick.Draw(); 
                hud.Draw(); 

                DrawCircle(screenWidth / 2, screenHeight / 2, 2, GREEN);
                DrawFPS(10, 10);
            renderer.EndFrame();
    }

    // ==========================================
    // تفريغ الذاكرة
    // ==========================================
    UnloadModelAnimations(playerAnims, playerAnimCount);
    UnloadModel(playerModel);
    UnloadModelAnimations(npcAnims, npcAnimCount);
    UnloadModel(npcModel);
    UnloadModel(groundModel); 
    UnloadTexture(groundTex); 
    
    renderer.Unload();
    CloseWindow();

    return 0;
}
