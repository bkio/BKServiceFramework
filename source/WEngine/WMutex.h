// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMutex
#define Pragma_Once_WMutex

#include "WEngine.h"
#if PLATFORM_WINDOWS
	#include <windows.h>
#else
    #include <pthread.h>
#endif

class WMutex
{

private:

#if PLATFORM_WINDOWS
    CRITICAL_SECTION _mutex{};
#else
    pthread_mutex_t _mutex{};
#endif

    bool _locked = false;

    void init()
    {
#if PLATFORM_WINDOWS
        InitializeCriticalSection(&_mutex);
#else
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
#endif
        _locked = false;
    }


public:
    WMutex()
    {
        init();
    }

    WMutex(const WMutex& in_mutex)
    {
        init();

        if (in_mutex._locked && !_locked)
        {
            lock();
        }
        else if (!in_mutex._locked && _locked)
        {
            unlock();
        }
    }

    WMutex& operator=(const WMutex& in_mutex)
    {
        if (in_mutex._locked && !_locked)
        {
            lock();
        }
        else if (!in_mutex._locked && _locked)
        {
            unlock();
        }
        return *this;
    }

#if PLATFORM_WINDOWS
    CRITICAL_SECTION* handle()
    {
        return &_mutex;
    };
#else
    pthread_mutex_t* handle()
    {
        return &_mutex;
    };
#endif

    //Destructor
    virtual ~WMutex()
    {
#if PLATFORM_WINDOWS
        DeleteCriticalSection(&_mutex);
#else
        pthread_mutex_unlock(&_mutex);
        pthread_mutex_destroy(&_mutex);
#endif
    }

    bool lock()
    {
        _locked = true;
#if PLATFORM_WINDOWS
        EnterCriticalSection(&_mutex);
        return true;
#else
        return pthread_mutex_lock(&_mutex) == 0;
#endif
    }

    bool unlock()
    {
        _locked = false;
#if PLATFORM_WINDOWS
        LeaveCriticalSection(&_mutex);
        return true;
#else
        return pthread_mutex_unlock(&_mutex) == 0;
#endif
    }
};

#define WScopeGuard volatile WScopeGuard_Internal
class WScopeGuard_Internal
{

private:
    WMutex* RelativeMutex = nullptr;
    bool bRedirected = false;

public:
    explicit WScopeGuard_Internal(WMutex* Mutex, bool bDoNoLock = false)
    {
        RelativeMutex = Mutex;
        if (!bDoNoLock && RelativeMutex != nullptr)
        {
            RelativeMutex->lock();
        }
    }
    volatile WScopeGuard_Internal& operator=(WMutex* Mutex) volatile noexcept
    {
        RelativeMutex = Mutex;
        if (RelativeMutex != nullptr)
        {
            RelativeMutex->lock();
        }
        return *this;
    }
    WScopeGuard_Internal(WScopeGuard_Internal&& InGuard) noexcept
    {
        RelativeMutex = InGuard.RelativeMutex;
    }
    WScopeGuard_Internal& operator=(WScopeGuard_Internal&& InGuard) noexcept
    {
        RelativeMutex = InGuard.RelativeMutex;
        return *this;
    }

    WScopeGuard_Internal() = default;

    ~WScopeGuard_Internal()
    {
        if (RelativeMutex != nullptr && !bRedirected)
        {
            RelativeMutex->unlock();
        }
    }

#if PLATFORM_WINDOWS
    void SleepWithCondition(CONDITION_VARIABLE* Condition) volatile
#else
    void SleepWithCondition(pthread_cond_t* Condition) volatile
#endif
    {
        if (Condition == nullptr) return;

#if PLATFORM_WINDOWS
        SleepConditionVariableCS(Condition, RelativeMutex->handle(), INFINITE);
#else
        pthread_cond_wait(Condition, RelativeMutex->handle());
#endif
    }
};

#endif //Pragma_Once_WMutex