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
    WConditionVariable();
    ~WConditionVariable();

    void signal();
    void wait(WScopeGuard& guard);

private:
#if PLATFORM_WINDOWS
    CONDITION_VARIABLE      m_cond;
#else
    pthread_cond_t  m_cond;
#endif
};

WConditionVariable::WConditionVariable()
{
#if PLATFORM_WINDOWS
    InitializeConditionVariable(&m_cond);
#else
    memset(&m_cond, 0, sizeof(pthread_cond_t));

    pthread_cond_init(&m_cond, NULL);
#endif
}

WConditionVariable::~WConditionVariable()
{
#if PLATFORM_WINDOWS
#else
    pthread_cond_destroy(&m_cond);
#endif
}

void WConditionVariable::signal()
{
#if PLATFORM_WINDOWS
    WakeConditionVariable(&m_cond);
#else
    pthread_cond_signal(&m_cond);
#endif
}

void WConditionVariable::wait(WScopeGuard& guard)
{
#if PLATFORM_WINDOWS
    SleepConditionVariableCS(&m_cond, guard.handle()->handle(), INFINITE);
#else
    pthread_cond_wait(&m_cond, guard.handle()->handle());
#endif
}

#endif //Pragma_Once_WConditionVariable
