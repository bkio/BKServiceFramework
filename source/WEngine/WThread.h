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

typedef std::function<void()> WThreadRunCallback;
typedef std::function<uint32()> WThreadStopCallback;

class WThread
{

private:
    WThread(const WThread&);
    const WThread& operator=(const WThread&);

#if PLATFORM_WINDOWS
    HANDLE hThread;
    static DWORD WINAPI Run(LPVOID pVoid)
#else
    pthread_t hThread{};
    static void* Run(void* pVoid)
#endif
    {
        auto wThread = static_cast<WThread*>(pVoid);
        if (wThread)
        {
            if (wThread->RunCallback)
            {
                wThread->bThreadJoinable = true;
                try
                {
                    wThread->RunCallback();
                }
                catch (std::bad_alloc& ba)
                {
                    UWUtilities::Print(EWLogType::Error, FString("Error: Thread out-of-memory: ") + FString(ba.what()));
                    CleanThread(wThread);
                    if (wThread->StopCallback)
                    {
#if PLATFORM_WINDOWS
                        return wThread->StopCallback();
#else
                        return reinterpret_cast<void *>(wThread->StopCallback());
#endif
                    }
                }
                wThread->bThreadJoinable = false;
                CleanThread(wThread);
            }
            if (wThread->StopCallback)
            {
#if PLATFORM_WINDOWS
                return wThread->StopCallback();
#else
                return reinterpret_cast<void *>(wThread->StopCallback());
#endif
            }
        }

#if PLATFORM_WINDOWS
        return 0;
#else
        return nullptr;
#endif
    }

    static void CleanThread(WThread* wThread)
    {
#if PLATFORM_WINDOWS
        if (wThread->hThread)
        {
            CloseHandle(wThread->hThread);
        }
#else
        pthread_exit(nullptr);
#endif
    }

    WThreadRunCallback RunCallback;
    WThreadStopCallback StopCallback;

    bool bThreadJoinable = false;

public:
    explicit WThread(WThreadRunCallback _RunCallback, WThreadStopCallback _StopCallback)
    {
        RunCallback = std::move(_RunCallback);
        StopCallback = std::move(_StopCallback);

#if PLATFORM_WINDOWS
        DWORD threadID;
        hThread = CreateThread(nullptr, 67108863, &WThread::Run, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &threadID);
        if (hThread)
        {
            SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
        }
#else
        pthread_attr_t hThreadAttribute{};
        pthread_attr_init (&hThreadAttribute);

        sched_param hScheduleParameter{};
        pthread_attr_getschedparam (&hThreadAttribute, &hScheduleParameter);
        hScheduleParameter.sched_priority = sched_get_priority_max(SCHED_FIFO);
        pthread_attr_setschedparam (&hThreadAttribute, &hScheduleParameter);

		pthread_attr_setstacksize(&hThreadAttribute, 67108863);

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

    static void SleepThread(uint32 DurationMs)
    {
#if PLATFORM_WINDOWS
        Sleep(DurationMs);
#else
        usleep(DurationMs * 1000);
#endif
    }
};

#endif //Pragma_Once_WThread