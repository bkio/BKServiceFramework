// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WThread
#define Pragma_Once_WThread

#include "WEngine.h"
#include "WUtilities.h"
#include "WString.h"
#include <functional>
#if PLATFORM_WINDOWS
    #include <windows.h>
#else
    #include <pthread.h>
    #include <sched.h>
#endif

typedef std::function<void()> WThreadCallback;

class WThread
{

private:
    WThread(const WThread&);
    const WThread& operator=(const WThread&);

#if PLATFORM_WINDOWS
    HANDLE hThread;

    static DWORD WINAPI Run(LPVOID pVoid);
#else
    pthread_t hThread;

    static void* Run(void* pVoid);
#endif

    WThreadCallback Callback;

    bool bThreadJoinable = false;

public:
    WThread(WThreadCallback ThreadCallback)
    {
        Callback = ThreadCallback;

#if PLATFORM_WINDOWS
        DWORD threadID;
        hThread = CreateThread(nullptr, 0, &WThread::Run, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &threadID);
        if (hThread)
        {
            SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
        }
#else
        pthread_attr_t hThreadAttribute;
        pthread_attr_init (&hThreadAttribute);

        sched_param hScheduleParameter;
        pthread_attr_getschedparam (&hThreadAttribute, &hScheduleParameter);
        hScheduleParameter.sched_priority = sched_get_priority_max(SCHED_FIFO);

        pthread_create(&hThread, hScheduleParameter, &WThread::Run, this);
        pthread_mutexattr_destroy(&hThreadAttribute);
#endif
    }

    bool IsJoinable()
    {
        return bThreadJoinable;
    }
    void Join()
    {
        if (!bThreadJoinable) return;

#if PLATFORM_WINDOWS
        if (hThread)
        {
            WaitForSingleObject(hThread, INFINITE);
        }
#else
        pthread_join(hThread, nullptr);
#endif
    }
};

#if PLATFORM_WINDOWS
DWORD WINAPI WThread::Run(LPVOID pVoid)
#else
void* WThread::Run(void* pVoid)
#endif
{
    WThread* wThread = static_cast<WThread*>(pVoid);
    if (wThread && wThread->Callback)
    {
        wThread->bThreadJoinable = true;
        wThread->Callback();
        wThread->bThreadJoinable = false;
#if PLATFORM_WINDOWS
        if (wThread->hThread)
        {
            CloseHandle(wThread->hThread);
        }
#endif
    }
    return 0;
}

#endif //Pragma_Once_WThread