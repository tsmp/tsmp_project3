// Критическая секция - класс для защиты общих данных 
// от параллельного изменения разными потоками

#pragma once

// раскомментировать если нужно найти узкие места производительности
// из за критических секций
//#define PROFILE_CRITICAL_SECTIONS

#ifdef PROFILE_CRITICAL_SECTIONS
typedef void (*add_profile_portion_callback)(LPCSTR id, const u64 &time);
void XRCORE_API set_add_profile_portion(add_profile_portion_callback callback);

#define STRINGIZER_HELPER(a) #a
#define STRINGIZER(a) STRINGIZER_HELPER(a)
#define CONCATENIZE_HELPER(a, b) a##b
#define CONCATENIZE(a, b) CONCATENIZE_HELPER(a, b)
#define MUTEX_PROFILE_PREFIX_ID #mutexes /
#define MUTEX_PROFILE_ID(a) STRINGIZER(CONCATENIZE(MUTEX_PROFILE_PREFIX_ID, a))
#endif // PROFILE_CRITICAL_SECTIONS

// Простая обертка над виндовой критической секцией
class XRCORE_API xrCriticalSection
{
private:

    // виндовая критическая секция
    CRITICAL_SECTION* m_pCritSection;

#ifdef PROFILE_CRITICAL_SECTIONS
    // id секции
    LPCSTR m_szId;
#endif // PROFILE_CRITICAL_SECTIONS

public:

#ifdef PROFILE_CRITICAL_SECTIONS
    xrCriticalSection(LPCSTR id);
#else  // PROFILE_CRITICAL_SECTIONS
    xrCriticalSection();
#endif // PROFILE_CRITICAL_SECTIONS

    ~xrCriticalSection();

    // Войти в критическую секцию. Код после этого будет защищен от 
    // выполнения в нескольких потоках параллельно. Будет выполняться
    // только в одном потоке
    void Enter();

    // Покинуть крит секцию. Разрешает выполнение кода после этого
    // параллельно в разных потоках
    void Leave();

    // Попробовать занять крит секцию. Если уже кем то занята то вернет
    // false. Если была свободна то займет ее и вернет true.
    bool TryEnter();
};
