#pragma once

#include <vector>
#include <memory>
#include <cassert>

template <class T>
class ObjectPool {
private:
    // storage يمتلك الذاكرة الفعلية للكائنات ويضمن حذفها بأمان عند إغلاق اللعبة
    std::vector<std::unique_ptr<T>> storage; 
    
    // available يخزن المؤشرات الجاهزة للاستخدام السريع (بدون حجز ذاكرة جديد)
    std::vector<T*> available;               

    // دالة داخلية لتوسيع المسبح في حال نفاد الكائنات أثناء اللعب
    void Expand(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            storage.push_back(std::make_unique<T>());
            available.push_back(storage.back().get());
        }
    }

public:
    // التهيئة مع حجز مسبق (Pre-allocation) للذاكرة لتجنب التقطيع
    explicit ObjectPool(size_t initialSize = 50) {
        storage.reserve(initialSize);
        available.reserve(initialSize);
        Expand(initialSize);
    }

    // جلب كائن لاستخدامه في اللعبة (O(1) Performance)
    T* Acquire() {
        if (available.empty()) {
            // التوسيع التلقائي إذا نفدت الكائنات (نضيف 50% من الحجم الحالي)
            Expand(storage.size() / 2 + 1); 
        }

        T* obj = available.back();
        available.pop_back();
        return obj;
    }

    // إعادة الكائن للمسبح بعد الانتهاء منه (مثل اختفاء الرصاصة)
    void Release(T* obj) {
        assert(obj != nullptr && "Cannot release a null pointer into the pool!");
        
        // تنبيه للمبرمج: يجب تصفير قيم الكائن (Reset) من الملف الذي استدعاه 
        // قبل أو بعد إرجاعه لضمان عدم تداخل البيانات القديمة
        available.push_back(obj);
    }

    // تفريغ الذاكرة بالكامل (يستخدم عند الانتقال من خريطة إلى أخرى)
    void Clear() {
        available.clear();
        storage.clear();
    }

    // دوال مساعدة لمراقبة الأداء والاستهلاك (مفيدة لـ Debugging UI)
    size_t GetAvailableCount() const { 
        return available.size(); 
    }
    
    size_t GetTotalCount() const { 
        return storage.size(); 
    }
    
    size_t GetActiveCount() const { 
        return storage.size() - available.size(); 
    }
};
