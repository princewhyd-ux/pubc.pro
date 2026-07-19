#pragma once

#include "raylib.h"
#include "raymath.h"
#include <unordered_map>
#include <vector>
#include <iostream>

// (نستخدم نفس هيكل StaticProp و EnvironmentCollider من MapManager إذا كان متاحاً)
// تم إعادة تعريفها هنا بشكل مبسط لضمان استقلالية الملف إذا أردت فصله تماماً.
#ifndef MAP_MANAGER_H
struct StaticProp {
    Model* model;
    Vector3 position;
    float scale;
    Color tint;
};

enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };

struct EnvironmentCollider {
    BoundingBox bounds;
    ColliderType type;
    std::string tag;
};
#endif

// 1. نظام إحداثيات المربعات (Grid Coordinates)
struct ChunkID {
    int x, z;

    // معامل المقارنة ليعمل مع std::unordered_map
    bool operator==(const ChunkID& other) const {
        return x == other.x && z == other.z;
    }
};

// 2. دالة تشفير (Hash Function) سريعة جداً لتحويل الإحداثيات إلى رقم فريد
struct ChunkIDHash {
    std::size_t operator()(const ChunkID& k) const {
        // دمج إحداثيات X و Z في رقم Hash واحد بكفاءة عالية (Bitwise XOR & Shift)
        return ((std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 1)) >> 1);
    }
};

// 3. هيكل المربع الواحد (Chunk) الذي يحتوي على العناصر
struct Chunk {
    std::vector<StaticProp> props;
    std::vector<EnvironmentCollider> colliders;
    bool isActive = false;
};

// ==========================================
// مدير التحميل التدريجي (Level Streamer)
// ==========================================
class LevelStreamer {
private:
    // قاموس (Hash Map) يخزن كل المربعات في العالم بكفاءة (O(1) Access)
    std::unordered_map<ChunkID, Chunk, ChunkIDHash> worldGrid;
    
    // قائمة بالمربعات النشطة حالياً (القريبة من اللاعب) لسرعة الرسم
    std::vector<Chunk*> activeChunks;

    float chunkSize;       // حجم المربع الواحد (مثلاً 50.0 متر)
    int renderDistance;    // كم مربعاً حول اللاعب يجب رسمه؟ (مثلاً 2 = شبكة 5x5)

    // كاش لتتبع مكان اللاعب لمنع الحسابات المكررة في كل إطار
    int lastViewerChunkX = -999999;
    int lastViewerChunkZ = -999999;

    // دالة داخلية لتحويل إحداثيات العالم (3D) إلى إحداثيات الشبكة (2D Grid)
    ChunkID GetChunkIDFromPosition(Vector3 pos) const {
        int cx = static_cast<int>(std::floor(pos.x / chunkSize));
        int cz = static_cast<int>(std::floor(pos.z / chunkSize));
        return { cx, cz };
    }

public:
    // التهيئة الافتراضية: حجم المربع 50 متر، ونرسم مربعين في كل اتجاه
    LevelStreamer(float chunkSize = 50.0f, int renderDistance = 2) 
        : chunkSize(chunkSize), renderDistance(renderDistance) {}

    // تفريغ الذاكرة
    ~LevelStreamer() {
        ClearWorld();
    }

    /**
     * @brief إضافة عنصر (منزل، شجرة) إلى العالم. النظام سيضعه في المربع الصحيح تلقائياً.
     */
    void AddProp(Model* model, Vector3 pos, float scale, Color tint = WHITE) {
        ChunkID id = GetChunkIDFromPosition(pos);
        worldGrid[id].props.push_back({ model, pos, scale, tint });
    }

    /**
     * @brief إضافة صندوق تصادم (Collision) إلى مربع معين.
     */
    void AddCollider(Vector3 pos, BoundingBox box, ColliderType type, const std::string& tag) {
        ChunkID id = GetChunkIDFromPosition(pos);
        worldGrid[id].colliders.push_back({ box, type, tag });
    }

    /**
     * @brief تحديث حالة العالم بناءً على موقع اللاعب أو الكاميرا.
     * @param viewerPosition موقع الكاميرا أو اللاعب الحالي.
     */
    void Update(Vector3 viewerPosition) {
        ChunkID viewerChunk = GetChunkIDFromPosition(viewerPosition);

        // التحسين (Optimization): لا تفعل شيئاً إذا لم يغادر اللاعب المربع الحالي!
        if (viewerChunk.x == lastViewerChunkX && viewerChunk.z == lastViewerChunkZ) {
            return; 
        }

        // تحديث الكاش
        lastViewerChunkX = viewerChunk.x;
        lastViewerChunkZ = viewerChunk.z;

        // 1. إيقاف تفعيل جميع المربعات النشطة حالياً
        for (Chunk* chunk : activeChunks) {
            chunk->isActive = false;
        }
        activeChunks.clear();

        // 2. البحث عن المربعات القريبة ضمن نطاق الرؤية (Render Distance) وتفعيلها
        for (int x = -renderDistance; x <= renderDistance; ++x) {
            for (int z = -renderDistance; z <= renderDistance; ++z) {
                
                ChunkID targetID = { viewerChunk.x + x, viewerChunk.z + z };
                
                // التأكد من أن المربع موجود فعلاً في القاموس لتجنب حجز ذاكرة فارغة
                auto it = worldGrid.find(targetID);
                if (it != worldGrid.end()) {
                    it->second.isActive = true;
                    activeChunks.push_back(&(it->second));
                }
            }
        }
    }

    /**
     * @brief رسم العناصر القريبة فقط (تُستدعى داخل BeginMode3D).
     */
    void Draw() const {
        for (const Chunk* chunk : activeChunks) {
            for (const auto& prop : chunk->props) {
                DrawModel(*(prop.model), prop.position, prop.scale, prop.tint);
            }
        }
    }

    /**
     * @brief استرجاع صناديق التصادم القريبة فقط ليتم حسابها في الفيزياء (يمنع اللاج).
     */
    std::vector<EnvironmentCollider> GetActiveColliders() const {
        std::vector<EnvironmentCollider> activeCols;
        for (const Chunk* chunk : activeChunks) {
            // ندمج تصادمات المربع النشط مع القائمة المرجعة
            activeCols.insert(activeCols.end(), chunk->colliders.begin(), chunk->colliders.end());
        }
        return activeCols;
    }

    // تنظيف العالم بالكامل عند الخروج أو تغيير الخريطة
    void ClearWorld() {
        worldGrid.clear();
        activeChunks.clear();
        lastViewerChunkX = -999999;
        lastViewerChunkZ = -999999;
    }

    // مراقبة الأداء
    int GetActiveChunkCount() const { return static_cast<int>(activeChunks.size()); }
    int GetTotalChunkCount() const { return static_cast<int>(worldGrid.size()); }
};
