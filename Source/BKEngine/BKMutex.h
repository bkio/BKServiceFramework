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
    CRITICAL_SECTION MutexValue{};
#else
    pthread_mutex_t MutexValue{};
#endif

    bool bLocked = false;

    void Initialize()
    {
#if PLATFORM_WINDOWS
        InitializeCriticalSection(&MutexValue);
#else
        pthread_mutexattr_t Attr{};
        pthread_mutexattr_init(&Attr);
        pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&MutexValue, &Attr);
        pthread_mutexattr_destroy(&Attr);
#endif
        bLocked = false;
    }


public:
    BKMutex()
    {
        Initialize();
    }

    BKMutex(const BKMutex& _MutexValue)
    {
        Initialize();

        if (_MutexValue.bLocked && !bLocked)
        {
            Lock();
        }
        else if (!_MutexValue.bLocked && bLocked)
        {
            Unlock();
        }
    }

    BKMutex& operator=(const BKMutex& _MutexValue)
    {
        if (_MutexValue.bLocked && !bLocked)
        {
            Lock();
        }
        else if (!_MutexValue.bLocked && bLocked)
        {
            Unlock();
        }
        return *this;
    }

#if PLATFORM_WINDOWS
    CRITICAL_SECTION* Handle()
    {
        return &MutexValue;
    };
#else
    pthread_mutex_t* Handle()
    {
        return &MutexValue;
    };
#endif

    //Destructor
    ~BKMutex()
    {
#if PLATFORM_WINDOWS
        DeleteCriticalSection(&MutexValue);
#else
        pthread_mutex_unlock(&MutexValue);
        pthread_mutex_destroy(&MutexValue);
#endif
    }

    bool Lock()
    {
        bLocked = true;
#if PLATFORM_WINDOWS
        EnterCriticalSection(&MutexValue);
        return true;
#else
        return pthread_mutex_lock(&MutexValue) == 0;
#endif
    }

    bool Unlock()
    {
        bLocked = false;
#if PLATFORM_WINDOWS
        LeaveCriticalSection(&MutexValue);
        return true;
#else
        return pthread_mutex_unlock(&MutexValue) == 0;
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
            RelativeMutex->Lock();
        }
    }
    volatile BKScopeGuard_Internal& operator=(BKMutex* Mutex) volatile noexcept
    {
        RelativeMutex = Mutex;
        if (RelativeMutex)
        {
            RelativeMutex->Lock();
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
            RelativeMutex->Unlock();
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
        SleepConditionVariableCS(Condition, RelativeMutex->Handle(), INFINITE);
#else
        pthread_cond_wait(Condition, RelativeMutex->Handle());
#endif
    }
};

#endif //Pragma_Once_BKMutex