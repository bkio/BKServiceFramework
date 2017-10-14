// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WConditionVariable
#define Pragma_Once_WConditionVariable

#include "WEngine.h"
#if PLATFORM_WINDOWS
    #include <windows.h>
#else
    #include <pthread.h>
#endif
#include "WMutex.h"

class WConditionVariable
{

public:
    WConditionVariable()
    {
#if PLATFORM_WINDOWS
        InitializeConditionVariable(&m_cond);
#else
        memset(&m_cond, 0, sizeof(pthread_cond_t));

        pthread_cond_init(&m_cond, NULL);
#endif
    }

    ~WConditionVariable()
    {
        signal();
#if PLATFORM_WINDOWS
#else
        pthread_cond_destroy(&m_cond);
#endif
    }

    void signal()
    {
#if PLATFORM_WINDOWS
        WakeAllConditionVariable(&m_cond);
#else
        pthread_cond_broadcast(&m_cond);
#endif
    }
    void wait(WScopeGuard& guard)
    {
#if PLATFORM_WINDOWS
        SleepConditionVariableCS(&m_cond, guard.handle()->handle(), INFINITE);
#else
        pthread_cond_wait(&m_cond, guard.handle()->handle());
#endif
    }

private:
#if PLATFORM_WINDOWS
    CONDITION_VARIABLE      m_cond;
#else
    pthread_cond_t  m_cond;
#endif
};

#endif //Pragma_Once_WConditionVariable
