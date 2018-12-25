// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKMutex
#define Pragma_Once_BKMutex

#include "BKEngine.h"
#if PLATFORM_WINDOWS
	#include <windows.h>
#else
    #include <pthread.h>
#endif

class BKMutex
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
        pthread_mutexattr_t attr{};
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
#endif
        _locked = false;
    }


public:
    BKMutex()
    {
        init();
    }

    BKMutex(const BKMutex& in_mutex)
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

    BKMutex& operator=(const BKMutex& in_mutex)
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
    virtual ~BKMutex()
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

#define BKScopeGuard volatile BKScopeGuard_Internal
class BKScopeGuard_Internal
{

private:
    BKMutex* RelativeMutex = nullptr;
    bool bRedirected = false;

public:
    explicit BKScopeGuard_Internal(BKMutex* Mutex, bool bDoNoLock = false)
    {
        RelativeMutex = Mutex;
        if (!bDoNoLock && RelativeMutex)
        {
            RelativeMutex->lock();
        }
    }
    volatile BKScopeGuard_Internal& operator=(BKMutex* Mutex) volatile noexcept
    {
        RelativeMutex = Mutex;
        if (RelativeMutex)
        {
            RelativeMutex->lock();
        }
        return *this;
    }
    BKScopeGuard_Internal(BKScopeGuard_Internal&& InGuard) noexcept
    {
        RelativeMutex = InGuard.RelativeMutex;
    }
    BKScopeGuard_Internal& operator=(BKScopeGuard_Internal&& InGuard) noexcept
    {
        RelativeMutex = InGuard.RelativeMutex;
        return *this;
    }

    BKScopeGuard_Internal() = default;

    ~BKScopeGuard_Internal()
    {
        if (RelativeMutex && !bRedirected)
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
        if (!Condition) return;

#if PLATFORM_WINDOWS
        SleepConditionVariableCS(Condition, RelativeMutex->handle(), INFINITE);
#else
        pthread_cond_wait(Condition, RelativeMutex->handle());
#endif
    }
};

#endif //Pragma_Once_BKMutex