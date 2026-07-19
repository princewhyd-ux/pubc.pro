#pragma once

// إعلان مبكر (Forward Declaration) لمنع مشكلة الاستدعاء الدائري (Circular Dependency)
// لأن اللاعب (Player) يحتاج لمعرفة الحالات، والحالات تحتاج لمعرفة اللاعب.
class Player;

class IPlayerState {
public:
    // مدمر افتراضي (Virtual Destructor) ضروري جداً في الـ Interfaces
    // بدونه، سيحدث تسريب في الذاكرة (Memory Leak) عند تدمير الحالات أثناء اللعب.
    virtual ~IPlayerState() = default;

    /**
     * @brief تُستدعى مرة واحدة فقط عند الدخول إلى هذه الحالة.
     * مفيدة لتهيئة المتغيرات أو تشغيل مؤثر صوتي (مثل صوت الهبوط على الأرض).
     */
    virtual void Enter(Player* player) = 0;

    /**
     * @brief تُستدعى في كل إطار (Frame) طوال فترة وجود اللاعب في هذه الحالة.
     * هنا نكتب منطق الحركة، الإدخال، والانتقال لحالات أخرى.
     */
    virtual void Update(Player* player, float dt) = 0;

    /**
     * @brief تُستدعى مرة واحدة فقط لحظة الخروج من هذه الحالة.
     * مفيدة لتنظيف المتغيرات أو إيقاف تأثير معين قبل الانتقال للحالة التالية.
     */
    virtual void Exit(Player* player) = 0;

    /**
     * @brief دالة مساعدة ترجع اسم الحالة.
     * مفيدة جداً لطباعتها على الشاشة أثناء التطوير (Debugging) لمعرفة ماذا يفعل اللاعب حالياً.
     */
    virtual const char* GetName() const = 0;
};
