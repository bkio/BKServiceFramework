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
    CRITICAL_SECTION _mutex; /**< Windows mutex */
#else
    pthread_mutex_t _mutex; /**< Posix mutex */
#endif

    volatile bool _locked;

    void init();

public:
    WMutex()
    {
        init();
    }

    //Copy Constructor
    WMutex(const WMutex &in_mutex)
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

    //Copy Constructor gibi = operator yuklemesi.
    WMutex& operator=(const WMutex &in_mutex)
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

    //Lock - Eger lock edildiyse true, degilse false
    bool lock();

    //Windows her durumda true doner, posix sonucu doner.
    bool unlock();

    bool isLocked() const;
};

class WScopeGuard
{

private:
    WMutex* RelativeMutex;

    //Copy Constructor
    WScopeGuard(const WScopeGuard &InGuard)
    {
        RelativeMutex = InGuard.RelativeMutex;
    }

    //Copy Constructor gibi = operator yuklemesi.
    WScopeGuard& operator=(const WScopeGuard &InGuard)
    {
        RelativeMutex = InGuard.RelativeMutex;
        return *this;
    }

public:
    WScopeGuard(WMutex* Mutex)
    {
        RelativeMutex = Mutex;
        if (RelativeMutex != nullptr)
        {
            RelativeMutex->lock();
        }
    }
    ~WScopeGuard()
    {
        if (RelativeMutex != nullptr)
        {
            RelativeMutex->unlock();
        }
    }

    WMutex* handle()
    {
        return RelativeMutex;
    };
};

void WMutex::init()
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

bool WMutex::lock()
{
    _locked = true;
#if PLATFORM_WINDOWS
    TryEnterCriticalSection(&_mutex);
	return true;
#else
    return pthread_mutex_lock(&_mutex) == 0;
#endif
}

//Windows her durumda true doner, posix sonucu doner.
bool WMutex::unlock()
{
    _locked = false;
#if PLATFORM_WINDOWS
    LeaveCriticalSection(&_mutex);
	return true;
#else
    return pthread_mutex_unlock(&_mutex) == 0;
#endif
}

bool WMutex::isLocked() const
{
    return _locked;
}

#endif //Pragma_Once_WMutex