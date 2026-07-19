#pragma once

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

// (نفس الهيكل من MapManager لضمان عدم وجود أخطاء في الاستدعاء)
#ifndef MAP_MANAGER_H
enum class ColliderType { STATIC_WALL, STATIC_GROUND, STATIC_PROP, TRIGGER_ZONE };
struct EnvironmentCollider { BoundingBox bounds; ColliderType type; std::string tag; };
#endif

// حالات الذكاء الاصطناعي
enum class NPCState {
    PATROL,         // الدورية العادية بين النقاط
    INVESTIGATE,    // الذهاب لمكان صوت أو لمحة سريعة للاعب
    CHASE,          // مطاردة اللاعب
    ATTACK,         // التوقف وإطلاق النار
    TAKE_COVER,     // البحث عن جدار للاحتماء
    RETREAT         // الهرب إذا كانت الصحة منخفضة
};

class NPCBrain {
public:
    Vector3 position;
    Vector3 velocity;
    float rotationAngle; // الدوران حول محور Y (للنظر)

    // إعدادات الذكاء الاصطناعي
    NPCState currentState;
    float moveSpeed = 2.5f;
    float runSpeed = 5.5f;
    float rotationSpeed = 10.0f;
    float health = 100.0f;

    // 1. نظام الرؤية (Vision System)
    float visionRange = 30.0f;           // أقصى مسافة للرؤية (متر)
    float visionAngle = 90.0f * DEG2RAD; // زاوية الرؤية (مخروط 90 درجة)
    
    // 2. نظام الذاكرة والانتباه (Memory & Alert System)
    float alertLevel = 0.0f;             // من 0 (طبيعي) إلى 100 (هجوم)
    Vector3 lastKnownTargetPos;          // آخر مكان تم رؤية اللاعب فيه
    bool hasTarget = false;              // هل العدو يمتلك هدفاً حالياً؟
    float timeSinceLastSeen = 999.0f;    // الوقت منذ آخر مرة رأى فيها اللاعب

    // 3. نظام الدورية (Patrol System)
    std::vector<Vector3> patrolWaypoints;
    int currentWaypointIndex = 0;
    float waitTimer = 0.0f;

    // متغيرات القتال
    float attackCooldown = 0.0f;
    float attackRate = 0.5f; // رصاصتان في الثانية

    NPCBrain(Vector3 startPos) {
        position = startPos;
        velocity = { 0.0f, 0.0f, 0.0f };
        rotationAngle = 0.0f;
        currentState = NPCState::PATROL;
        lastKnownTargetPos = startPos;

        // توليد نقاط دورية عشوائية حول نقطة البداية كمثال
        patrolWaypoints.push_back(startPos);
        patrolWaypoints.push_back({ startPos.x + 10.0f, startPos.y, startPos.z + 5.0f });
        patrolWaypoints.push_back({ startPos.x - 5.0f, startPos.y, startPos.z + 10.0f });
    }

    // دالة التحديث الرئيسية (تُستدعى في كل إطار)
    void Update(float dt, Vector3 targetPos, const std::vector<EnvironmentCollider>& colliders) {
        if (health <= 0.0f) return; // ميت

        UpdateVision(dt, targetPos, colliders);
        UpdateAlertSystem(dt);
        ProcessState(dt, targetPos);

        // تطبيق الحركة مع الجاذبية
        velocity.y -= 9.81f * dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        position.z += velocity.z * dt;

        // منع السقوط تحت الأرض (تصادم سريع مع الأرضية)
        if (position.y < 1.0f) { // افتراض أن 1.0f هو ارتفاع الأرض+العدو
            position.y = 1.0f;
            velocity.y = 0.0f;
        }
    }

    // لتلقي الأصوات (مثال: اللاعب يركض أو يطلق نار)
    void HearSound(Vector3 soundPos, float intensity) {
        float dist = Vector3Distance(position, soundPos);
        if (dist < intensity) {
            alertLevel += (intensity - dist); // زيادة الانتباه بناءً على قوة ومسافة الصوت
            lastKnownTargetPos = soundPos;
            hasTarget = true;
            if (currentState == NPCState::PATROL) {
                ChangeState(NPCState::INVESTIGATE);
            }
        }
    }

    // تلقي الضرر
    void TakeDamage(float amount, Vector3 attackerPos) {
        health -= amount;
        alertLevel = 100.0f; // تنبيه فوري أقصى
        lastKnownTargetPos = attackerPos;
        hasTarget = true;
        
        if (health < 30.0f) ChangeState(NPCState::RETREAT);
        else ChangeState(NPCState::ATTACK);
    }

private:
    // ==========================================
    // 1. نظام الرؤية وخط النظر (Line of Sight)
    // ==========================================
    void UpdateVision(float dt, Vector3 targetPos, const std::vector<EnvironmentCollider>& colliders) {
        Vector3 dirToTarget = Vector3Subtract(targetPos, position);
        float distance = Vector3Length(dirToTarget);

        if (distance < visionRange) {
            Vector3 normDirToTarget = Vector3Normalize(dirToTarget);
            Vector3 forwardVec = { sinf(rotationAngle), 0.0f, cosf(rotationAngle) };

            // فحص مخروط الرؤية (Vision Cone)
            float dotProduct = Vector3DotProduct(forwardVec, normDirToTarget);
            float angleToTarget = acosf(dotProduct);

            if (angleToTarget < visionAngle / 2.0f) {
                // اللاعب داخل المخروط، الآن نفحص هل يوجد جدار يمنع الرؤية؟ (Raycast)
                if (CheckLineOfSight(targetPos, colliders)) {
                    // اللاعب مرئي تماماً!
                    lastKnownTargetPos = targetPos;
                    timeSinceLastSeen = 0.0f;
                    hasTarget = true;
                    
                    // زيادة الانتباه بسرعة بناءً على المسافة
                    alertLevel += (50.0f / distance) * dt * 100.0f; 
                    return;
                }
            }
        }

        // إذا لم يره، يبدأ الوقت بالمرور لنسيان مكان اللاعب
        timeSinceLastSeen += dt;
    }

    bool CheckLineOfSight(Vector3 targetPos, const std::vector<EnvironmentCollider>& colliders) {
        Vector3 eyePos = { position.x, position.y + 1.5f, position.z }; // مستوى عين العدو
        Vector3 targetEyePos = { targetPos.x, targetPos.y + 1.5f, targetPos.z };
        
        Vector3 dir = Vector3Normalize(Vector3Subtract(targetEyePos, eyePos));
        float maxDist = Vector3Distance(eyePos, targetEyePos);
        Ray ray = { eyePos, dir };

        for (const auto& col : colliders) {
            // تجاهل المجسمات الصغيرة أو الـ Triggers
            if (col.type == ColliderType::TRIGGER_ZONE) continue;

            RayCollision hit = GetRayCollisionBox(ray, col.bounds);
            if (hit.hit && hit.distance < maxDist) {
                return false; // الرؤية محجوبة بجدار
            }
        }
        return true; // لا يوجد حواجز
    }

    // ==========================================
    // 2. نظام إدارة الوعي والانتباه (Alert System)
    // ==========================================
    void UpdateAlertSystem(float dt) {
        alertLevel = Clamp(alertLevel, 0.0f, 100.0f);

        // تهدئة الانتباه بمرور الوقت إذا لم يرَ اللاعب
        if (timeSinceLastSeen > 5.0f) {
            alertLevel -= 5.0f * dt;
        }

        // تغيير الحالات بذكاء بناءً على مستوى الانتباه
        if (currentState != NPCState::RETREAT) {
            if (alertLevel > 80.0f && hasTarget) {
                if (Vector3Distance(position, lastKnownTargetPos) < 15.0f) ChangeState(NPCState::ATTACK);
                else ChangeState(NPCState::CHASE);
            } 
            else if (alertLevel > 30.0f && hasTarget) {
                ChangeState(NPCState::INVESTIGATE);
            } 
            else if (alertLevel < 10.0f) {
                hasTarget = false;
                ChangeState(NPCState::PATROL);
            }
        }
    }

    // ==========================================
    // 3. آلة الحالات (State Machine Processing)
    // ==========================================
    void ProcessState(float dt, Vector3 targetPos) {
        switch (currentState) {
            case NPCState::PATROL:
                if (patrolWaypoints.empty()) return;

                if (waitTimer > 0.0f) {
                    waitTimer -= dt;
                    velocity.x = 0; velocity.z = 0;
                } else {
                    Vector3 wp = patrolWaypoints[currentWaypointIndex];
                    if (MoveTowards(wp, moveSpeed, dt)) {
                        // وصل للنقطة، ينتظر قليلاً ثم ينتقل للتالية
                        waitTimer = 2.0f;
                        currentWaypointIndex = (currentWaypointIndex + 1) % patrolWaypoints.size();
                    }
                }
                break;

            case NPCState::INVESTIGATE:
                // الذهاب لآخر مكان سمع/رأى فيه اللاعب
                if (MoveTowards(lastKnownTargetPos, moveSpeed, dt)) {
                    // وصل للمكان ولم يجد أحداً؟ يدور حول نفسه للبحث
                    rotationAngle += rotationSpeed * 0.5f * dt;
                    if (timeSinceLastSeen > 10.0f) {
                        alertLevel = 0.0f; // ييأس ويعود للدورية
                    }
                }
                break;

            case NPCState::CHASE:
                MoveTowards(lastKnownTargetPos, runSpeed, dt);
                break;

            case NPCState::ATTACK:
                velocity.x = 0; velocity.z = 0; // يتوقف ليطلق النار
                SmoothRotateTowards(lastKnownTargetPos, dt);
                
                attackCooldown -= dt;
                if (attackCooldown <= 0.0f && timeSinceLastSeen < 0.5f) {
                    // إطلاق الرصاصة (يجب دمجها مع ObjectPool لاحقاً)
                    // FireBullet();
                    attackCooldown = attackRate;
                }
                
                // إذا ابتعد اللاعب، ارجع للمطاردة
                if (Vector3Distance(position, lastKnownTargetPos) > 15.0f) {
                    ChangeState(NPCState::CHASE);
                }
                break;

            case NPCState::RETREAT:
                // الركض في الاتجاه المعاكس للاعب
                Vector3 retreatDir = Vector3Normalize(Vector3Subtract(position, lastKnownTargetPos));
                Vector3 retreatPos = Vector3Add(position, Vector3Scale(retreatDir, 10.0f));
                MoveTowards(retreatPos, runSpeed, dt);
                break;
                
            case NPCState::TAKE_COVER:
                // (سيتم تطويرها بربطها مع خوارزمية بحث عن الجدران في البيئة)
                break;
        }

        // الحفاظ على الزاوية بين -PI و PI
        rotationAngle = fmodf(rotationAngle, PI * 2.0f);
    }

    void ChangeState(NPCState newState) {
        if (currentState == newState) return;
        currentState = newState;
        waitTimer = 0.0f;
    }

    // ==========================================
    // دوال الحركة المساعدة (Locomotion)
    // ==========================================
    bool MoveTowards(Vector3 target, float speed, float dt) {
        Vector3 dir = Vector3Subtract(target, position);
        dir.y = 0.0f; // منع الطيران
        
        float dist = Vector3Length(dir);
        if (dist < 0.5f) {
            velocity.x = 0; velocity.z = 0;
            return true; // وصل للهدف
        }

        Vector3 normDir = Vector3Normalize(dir);
        velocity.x = normDir.x * speed;
        velocity.z = normDir.z * speed;

        SmoothRotateTowards(target, dt);
        return false;
    }

    void SmoothRotateTowards(Vector3 target, float dt) {
        Vector3 dir = Vector3Subtract(target, position);
        float targetAngle = atan2f(dir.x, dir.z); // Z هو الأمام في أنظمة 3D
        
        // حساب فرق الزاوية لاختيار أقصر طريق للدوران
        float diff = targetAngle - rotationAngle;
        while (diff < -PI) diff += 2.0f * PI;
        while (diff >  PI) diff -= 2.0f * PI;

        rotationAngle += diff * rotationSpeed * dt;
    }
};
