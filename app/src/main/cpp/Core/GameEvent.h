#pragma once

#include <functional>
#include <unordered_map>
#include <utility>

template<typename... Args>
class GameEvent {
public:
    // تعريف نوع الدالة (Callback) بناءً على المعاملات الممررة
    using Callback = std::function<void(Args...)>;

private:
    // قاموس لتخزين المستمعين (Listeners) مع رقم تعريفي (ID) فريد لكل مستمع
    std::unordered_map<int, Callback> listeners;
    
    // عداد لإنشاء أرقام تعريفية فريدة
    int nextListenerId = 0;

public:
    GameEvent() = default;
    ~GameEvent() { Clear(); }

    // منع النسخ لتجنب الأخطاء المنطقية في الذاكرة
    GameEvent(const GameEvent&) = delete;
    GameEvent& operator=(const GameEvent&) = delete;

    /**
     * @brief إضافة مستمع (Listener) للحدث
     * @param callback الدالة المراد تنفيذها عند إطلاق الحدث
     * @return رقم تعريفي (ID) يجب الاحتفاظ به لإلغاء الاشتراك لاحقاً
     */
    int Subscribe(Callback callback) {
        int id = nextListenerId++;
        listeners[id] = std::move(callback);
        return id;
    }

    /**
     * @brief إلغاء اشتراك مستمع باستخدام رقمه التعريفي
     * @param id الرقم التعريفي الذي تم الحصول عليه عند الاشتراك
     */
    void Unsubscribe(int id) {
        auto it = listeners.find(id);
        if (it != listeners.end()) {
            listeners.erase(it);
        }
    }

    /**
     * @brief إطلاق الحدث لتنفيذ جميع الدوال المشتركة
     * @param args البيانات المراد إرسالها للمستمعين
     */
    void Invoke(Args... args) const {
        // نأخذ نسخة من القاموس أثناء التنفيذ لتجنب أخطاء التعديل المتزامن
        // (في حال قام أحد المستمعين بإلغاء اشتراكه أثناء تشغيل الحدث)
        auto currentListeners = listeners; 
        
        for (const auto& pair : currentListeners) {
            if (pair.second) {
                // استخدام std::forward للحفاظ على نوع البيانات (L-value أو R-value) لضمان أعلى أداء
                pair.second(std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief إزالة جميع المستمعين (مفيد جداً عند الانتقال من مرحلة لأخرى)
     */
    void Clear() {
        listeners.clear();
        nextListenerId = 0;
    }

    /**
     * @brief معرفة عدد المستمعين الحاليين (لأغراض المراقبة - Debugging)
     */
    size_t GetListenerCount() const {
        return listeners.size();
    }
};

/* 
=================================================
 أمثلة على كيفية الاستخدام (لا تقم بنسخها للمشروع):
=================================================

 1. حدث بدون بيانات (مثال: قفز اللاعب)
 GameEvent<> OnPlayerJump;
 int jumpID = OnPlayerJump.Subscribe([]() { printf("Player Jumped!"); });
 OnPlayerJump.Invoke();

 2. حدث مع بيانات (مثال: تلقي ضرر ومكان الإصابة)
 GameEvent<float, Vector3> OnPlayerTakeDamage;
 int dmgID = OnPlayerTakeDamage.Subscribe([](float dmg, Vector3 hitPos) {
     printf("Took %f damage at X:%f", dmg, hitPos.x);
 });
 OnPlayerTakeDamage.Invoke(25.5f, {1.0f, 2.0f, 3.0f});
 
 3. إلغاء الاشتراك (مهم جداً عند تدمير الكائن)
 OnPlayerTakeDamage.Unsubscribe(dmgID);
*/
