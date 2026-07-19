#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

// هيكل زر الواجهة
struct HUDButton {
    std::string name;
    Texture2D texture;
    Rectangle rect;
    bool isPressed;
    bool isSelected; // يُستخدم فقط في وضع التعديل
};

class HUDManager {
public:
    std::vector<HUDButton> buttons;
    bool isEditMode = false;
    Rectangle gearRect;

    HUDManager() = default;
    ~HUDManager() { Unload(); }

    void Init() {
        // زر الإعدادات (الترس) لفتح وضع التعديل
        gearRect = { 20, 20, 60, 60 }; 

        // تحديد مسار الصور بناءً على المنصة
        #if defined(PLATFORM_ANDROID)
            const char* path = "hud/"; 
        #else
            const char* path = "assets/hud/"; 
        #endif

        // إضافة الأزرار الافتراضية بأحجام ومواقع مبدئية
        AddButton("scope",  TextFormat("%sscope.png", path),  { 1000, 300, 80, 80 });
        AddButton("fire",   TextFormat("%sfire.png", path),   { 1100, 400, 90, 90 });
        AddButton("jump",   TextFormat("%sjump.png", path),   { 1150, 550, 80, 80 });
        AddButton("crouch", TextFormat("%scrouch.png", path), { 1000, 550, 70, 70 });
        AddButton("prone",  TextFormat("%sprone.png", path),  { 850,  550, 70, 70 });
        AddButton("kick",   TextFormat("%skick.png", path),   { 1100, 200, 70, 70 });

        // محاولة تحميل التخطيط المحفوظ (إن وُجد)
        LoadLayout(); 
    }

    void Update() {
        Vector2 primaryTouchPos = {0, 0}; 
        bool isTouching = false;

        // التحقق من اللمس أو الماوس
        if (GetTouchPointCount() > 0) { 
            primaryTouchPos = GetTouchPosition(0); 
            isTouching = true; 
        } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { 
            primaryTouchPos = GetMousePosition(); 
            isTouching = true; 
        }

        // فحص الضغط على زر الترس (Gear Button) للتبديل بين وضع اللعب والتعديل
        bool gearPressed = false;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) gearPressed = true;
        
        for (int i = 0; i < GetTouchPointCount(); i++) {
            // نستخدم Tap Gesture في الأندرويد لضمان الدقة في الضغطات السريعة
            if (IsGestureDetected(GESTURE_TAP) || GetTouchPointCount() > 0) {
                // للتبسيط، نعتبر اللمسة الأولى هي المسؤولة عن زر الإعدادات
                if (CheckCollisionPointRec(GetTouchPosition(i), gearRect)) {
                    gearPressed = true;
                }
            }
        }

        if (gearPressed && isTouching && CheckCollisionPointRec(primaryTouchPos, gearRect)) {
            isEditMode = !isEditMode;
            if (!isEditMode) {
                SaveLayout(); // حفظ الأماكن تلقائياً عند الخروج من وضع التعديل
            }
            WaitTime(0.2f); // منع التكرار السريع للضغطة (Debounce)
        }

        // توجيه التحديث إما لوضع التعديل أو وضع اللعب
        if (isEditMode) {
            HandleEditMode(primaryTouchPos, isTouching);
        } else {
            HandlePlayMode();
        }
    }

    void Draw() {
        // رسم زر الترس (أحمر في وضع التعديل، وشفاف في وضع اللعب)
        DrawRectangleRounded(gearRect, 0.5f, 10, isEditMode ? Fade(RED, 0.8f) : Fade(DARKGRAY, 0.5f));
        DrawText("SET", (int)gearRect.x + 10, (int)gearRect.y + 20, 20, WHITE);

        // رسم جميع الأزرار
        for (auto& btn : buttons) {
            // رسم الأزرار بنصف شفافية حتى لا تحجب رؤية اللعبة
            DrawTexturePro(btn.texture, 
                           { 0, 0, (float)btn.texture.width, (float)btn.texture.height }, 
                           btn.rect, 
                           { 0, 0 }, 0.0f, Fade(WHITE, 0.5f));

            if (isEditMode) {
                // إظهار إطار حول الزر إذا كان محدداً للتعديل
                DrawRectangleLinesEx(btn.rect, 2.0f, btn.isSelected ? GREEN : YELLOW);
                
                if (btn.isSelected) {
                    // رسم مربع صغير في الزاوية السفلية اليمنى لتغيير الحجم (Scale Handle)
                    DrawRectangle((int)(btn.rect.x + btn.rect.width - 20), 
                                  (int)(btn.rect.y + btn.rect.height - 20), 
                                  20, 20, Fade(GREEN, 0.8f));
                }
            } else if (btn.isPressed) {
                // وميض خفيف (Highlight) عند الضغط على الزر أثناء اللعب
                DrawRectangleRounded(btn.rect, 0.2f, 10, Fade(WHITE, 0.3f));
            }
        }
        
        if (isEditMode) {
            DrawText("EDIT MODE: Drag to move, use bottom-right corner to scale.", 
                     GetScreenWidth() / 2 - 200, 20, 20, GREEN);
        }
    }

    // دالة يستخدمها اللاعب (PlayerState) لمعرفة هل الزر مضغوط
    bool IsButtonPressed(const std::string& name) const {
        for (const auto& btn : buttons) {
            if (btn.name == name) return btn.isPressed;
        }
        return false;
    }

private:
    void AddButton(const std::string& name, const std::string& path, Rectangle defaultRect) {
        Texture2D tex = LoadTexture(path.c_str());
        // في حال لم يجد الصورة، نستخدم صورة احتياطية لمنع الانهيار
        if (tex.id == 0) {
            Image tempImg = GenImageColor((int)defaultRect.width, (int)defaultRect.height, LIGHTGRAY);
            tex = LoadTextureFromImage(tempImg);
            UnloadImage(tempImg);
            std::cout << "[HUDManager] WARNING: Failed to load " << path << std::endl;
        }
        buttons.push_back({name, tex, defaultRect, false, false});
    }

    void HandleEditMode(Vector2 touchPos, bool isTouching) {
        static HUDButton* selectedBtn = nullptr;
        static bool isScaling = false;

        // عند بداية الضغطة، حدد الزر
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isTouching) {
            selectedBtn = nullptr; 
            isScaling = false;

            for (auto& btn : buttons) {
                btn.isSelected = false; // إلغاء تحديد الكل
                
                // منطقة تغيير الحجم (الزاوية السفلية اليمنى)
                Rectangle scaleHandle = { btn.rect.x + btn.rect.width - 30, btn.rect.y + btn.rect.height - 30, 40, 40 };
                
                if (CheckCollisionPointRec(touchPos, scaleHandle)) { 
                    selectedBtn = &btn; 
                    btn.isSelected = true; 
                    isScaling = true; 
                    break; 
                } 
                else if (CheckCollisionPointRec(touchPos, btn.rect)) { 
                    selectedBtn = &btn; 
                    btn.isSelected = true; 
                    break; 
                }
            }
        }

        // إذا كان هناك زر محدد واللاعب لا يزال يلمس الشاشة (سحب وإفلات)
        if (isTouching && selectedBtn != nullptr) {
            if (isScaling) {
                float newWidth = touchPos.x - selectedBtn->rect.x; 
                float newHeight = touchPos.y - selectedBtn->rect.y;
                
                // وضع حد أدنى لحجم الزر حتى لا يختفي
                if (newWidth > 40.0f) selectedBtn->rect.width = newWidth; 
                if (newHeight > 40.0f) selectedBtn->rect.height = newHeight;
            } else {
                // تحريك الزر مع جعل نقطة اللمس في منتصفه
                selectedBtn->rect.x = touchPos.x - (selectedBtn->rect.width / 2.0f);
                selectedBtn->rect.y = touchPos.y - (selectedBtn->rect.height / 2.0f);
            }
        }
    }

    void HandlePlayMode() {
        // تصفير حالة كل الأزرار أولاً
        for (auto& btn : buttons) {
            btn.isPressed = false;
        }

        // 1. نظام اللمس المتعدد (دعم أزرار متعددة في نفس الوقت، مثلاً: ركض + إطلاق + قفز)
        int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; i++) {
            Vector2 tPos = GetTouchPosition(i);
            for (auto& btn : buttons) {
                if (CheckCollisionPointRec(tPos, btn.rect)) {
                    btn.isPressed = true;
                }
            }
        }

        // 2. نظام الماوس (لتجربة اللعبة على الكمبيوتر)
        #if !defined(PLATFORM_ANDROID)
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mPos = GetMousePosition();
            for (auto& btn : buttons) {
                if (CheckCollisionPointRec(mPos, btn.rect)) {
                    btn.isPressed = true;
                }
            }
        }
        #endif
    }

    void SaveLayout() const {
        std::string data = "";
        for (const auto& btn : buttons) {
            data += btn.name + "," + 
                    std::to_string(btn.rect.x) + "," + 
                    std::to_string(btn.rect.y) + "," + 
                    std::to_string(btn.rect.width) + "," + 
                    std::to_string(btn.rect.height) + "\n";
        }
        
        // حفظ الملف في مسار عمل اللعبة
        const char* filePath = TextFormat("%s/hud_layout.txt", GetWorkingDirectory());
        SaveFileText(filePath, (char*)data.c_str());
        std::cout << "[HUDManager] Layout saved to " << filePath << std::endl;
    }

    void LoadLayout() {
        const char* filePath = TextFormat("%s/hud_layout.txt", GetWorkingDirectory());
        if (!FileExists(filePath)) return; // لا يوجد ملف محفوظ مسبقاً، نستخدم الافتراضي

        char* fileText = LoadFileText(filePath);
        if (fileText == nullptr) return; 
        
        std::string content(fileText); 
        std::stringstream ss(content); 
        std::string line;

        // قراءة الملف سطر بسطر وتحديث الإحداثيات
        while (std::getline(ss, line, '\n')) {
            std::stringstream lineStream(line); 
            std::string name, xStr, yStr, wStr, hStr;
            
            if (std::getline(lineStream, name, ',') && 
                std::getline(lineStream, xStr, ',') && 
                std::getline(lineStream, yStr, ',') && 
                std::getline(lineStream, wStr, ',') && 
                std::getline(lineStream, hStr, ',')) {
                
                for (auto& btn : buttons) {
                    if (btn.name == name) { 
                        btn.rect.x = std::stof(xStr); 
                        btn.rect.y = std::stof(yStr); 
                        btn.rect.width = std::stof(wStr); 
                        btn.rect.height = std::stof(hStr); 
                    }
                }
            }
        }
        
        UnloadFileText(fileText); // تحرير الذاكرة لمنع التسريب
        std::cout << "[HUDManager] Layout loaded successfully." << std::endl;
    }

    void Unload() {
        for (auto& btn : buttons) {
            UnloadTexture(btn.texture);
        }
        buttons.clear();
    }
};
