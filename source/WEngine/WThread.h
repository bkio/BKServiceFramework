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
    #include <unistd.h>
#endif

typedef std::function<void()> WThreadCallback;

class WThread
{

private:
    WThread(const WThread&);
    const WThread& operator=(const WThread&);

    int32 CurrentThreadNumber = 0;

#if PLATFORM_WINDOWS
    HANDLE hThread;
    static DWORD WINAPI Run(LPVOID pVoid)
#else
    pthread_t hThread;
    static void* Run(void* pVoid)
#endif
    {
        auto wThread = static_cast<WThread*>(pVoid);
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
#else
            pthread_exit(nullptr);
#endif
        }
        return 0;
    }

    WThreadCallback Callback;

    bool bThreadJoinable = false;

public:
    explicit WThread(WThreadCallback ThreadCallback)
    {
        Callback = std::move(ThreadCallback);

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
        pthread_attr_setschedparam (&hThreadAttribute, &hScheduleParameter);

        pthread_create(&hThread, &hThreadAttribute, &WThread::Run, this);
        pthread_attr_destroy(&hThreadAttribute);
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
        WaitForSingleObject(hThread, INFINITE);
#else
        pthread_join(hThread, nullptr);
#endif
    }

    int32 GetCurrentThreadNo()
    {
        return CurrentThreadNumber;
    }

    static void SleepThread(uint32 DurationMs)
    {
#if PLATFORM_WINDOWS
        Sleep(DurationMs);
#else
        usleep(DurationMs * 1000);
#endif
    }

    static void StartSystem()
    {
    }
    static void EndSystem()
    {
    }
};

#endif //Pragma_Once_WThread