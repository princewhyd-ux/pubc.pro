#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>

// هيكل يمثل "مستوى" (Plane) رياضي مكون من متجه اتجاه (Normal) ومسافة (Distance)
struct FrustumPlane {
    Vector3 normal;
    float d;

    // تسوية المستوى رياضيًا (Normalization) لضمان دقة الحسابات
    void Normalize() {
        float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (length > 0.0001f) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
            d /= length;
        }
    }
};

class FrustumCuller {
private:
    FrustumPlane planes[6]; // مستويات الكاميرا الستة (أمام، خلف، يمين، يسار، أعلى، أسفل)

public:
    FrustumCuller() = default;

    /**
     * @brief تحديث هرم الرؤية (يجب استدعاؤها في كل إطار بعد تحديث الكاميرا)
     * @param camera كاميرا اللعبة الحالية
     * @param aspect نسبة أبعاد الشاشة (العرض / الطول)
     * @param zNear مسافة أقرب نقطة للرؤية (عادة 0.01f)
     * @param zFar مسافة أبعد نقطة للرؤية (مثلاً 1000.0f)
     */
    void Update(const Camera3D& camera, float aspect, float zNear = 0.1f, float zFar = 1000.0f) {
        // 1. حساب مصفوفة الرؤية (View Matrix) بناءً على موقع الكاميرا وهدفها
        Matrix view = GetCameraMatrix(camera);

        // 2. حساب مصفوفة الإسقاط (Projection Matrix)
        Matrix proj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, zNear, zFar);

        // 3. دمج المصفوفتين للحصول على المصفوفة النهائية التي تحدد هرم الرؤية
        Matrix vp = MatrixMultiply(view, proj);

        // 4. استخراج المستويات الستة من المصفوفة (تقنية رياضية سريعة جداً)
        
        // المستوى الأيسر (Left)
        planes[0].normal.x = vp.m3 + vp.m0;
        planes[0].normal.y = vp.m7 + vp.m4;
        planes[0].normal.z = vp.m11 + vp.m8;
        planes[0].d        = vp.m15 + vp.m12;

        // المستوى الأيمن (Right)
        planes[1].normal.x = vp.m3 - vp.m0;
        planes[1].normal.y = vp.m7 - vp.m4;
        planes[1].normal.z = vp.m11 - vp.m8;
        planes[1].d        = vp.m15 - vp.m12;

        // المستوى السفلي (Bottom)
        planes[2].normal.x = vp.m3 + vp.m1;
        planes[2].normal.y = vp.m7 + vp.m5;
        planes[2].normal.z = vp.m11 + vp.m9;
        planes[2].d        = vp.m15 + vp.m13;

        // المستوى العلوي (Top)
        planes[3].normal.x = vp.m3 - vp.m1;
        planes[3].normal.y = vp.m7 - vp.m5;
        planes[3].normal.z = vp.m11 - vp.m9;
        planes[3].d        = vp.m15 - vp.m13;

        // المستوى القريب (Near)
        planes[4].normal.x = vp.m3 + vp.m2;
        planes[4].normal.y = vp.m7 + vp.m6;
        planes[4].normal.z = vp.m11 + vp.m10;
        planes[4].d        = vp.m15 + vp.m14;

        // المستوى البعيد (Far)
        planes[5].normal.x = vp.m3 - vp.m2;
        planes[5].normal.y = vp.m7 - vp.m6;
        planes[5].normal.z = vp.m11 - vp.m10;
        planes[5].d        = vp.m15 - vp.m14;

        // 5. تسوية المستويات لضمان دقة حساب المسافات
        for (int i = 0; i < 6; i++) {
            planes[i].Normalize();
        }
    }

    /**
     * @brief فحص ما إذا كان صندوق التصادم (AABB) مرئياً للكاميرا.
     * طريقة محسنة تعتمد على النقطة الإيجابية (P-Vertex) لرفع الأداء.
     */
    bool IsBoxVisible(const BoundingBox& box) const {
        // نمر على مستويات الكاميرا الستة
        for (int i = 0; i < 6; i++) {
            // العثور على النقطة الأقرب في الصندوق لتحديد ما إذا كان يقع خلف المستوى
            Vector3 p;
            p.x = (planes[i].normal.x > 0) ? box.max.x : box.min.x;
            p.y = (planes[i].normal.y > 0) ? box.max.y : box.min.y;
            p.z = (planes[i].normal.z > 0) ? box.max.z : box.min.z;

            // إذا كانت هذه النقطة تقع خلف المستوى (قيمة سالبة)، فالصندوق بأكمله غير مرئي!
            if (Vector3DotProduct(planes[i].normal, p) + planes[i].d < 0.0f) {
                return false; 
            }
        }
        return true; // الصندوق مرئي أو يتقاطع مع حدود الرؤية
    }

    /**
     * @brief فحص ما إذا كان الشكل الكروي مرئياً (مفيد لانفجارات الجزيئات والأعداء)
     */
    bool IsSphereVisible(Vector3 center, float radius) const {
        for (int i = 0; i < 6; i++) {
            float distance = Vector3DotProduct(planes[i].normal, center) + planes[i].d;
            // إذا كانت المسافة أصغر من سالب نصف القطر، فالكرة خارج الشاشة
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief فحص نقطة واحدة (مفيد للإضاءة أو جزيئات صغيرة جداً)
     */
    bool IsPointVisible(Vector3 point) const {
        for (int i = 0; i < 6; i++) {
            if (Vector3DotProduct(planes[i].normal, point) + planes[i].d < 0.0f) {
                return false;
            }
        }
        return true;
    }
};
