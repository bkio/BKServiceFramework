// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WScheduledAsyncTaskManager
#define Pragma_Once_WScheduledAsyncTaskManager

#include "WEngine.h"
#include "WTaskDefines.h"
#include "WSafeQueue.h"

class WScheduledAsyncTaskManager
{

public:
    static void StartSystem(uint32 SleepDurationMs);
    static void EndSystem();

    static bool IsSystemStarted();

    static uint32 NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<WAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters = false);
    static void CancelScheduledAsyncTask(uint32 TaskUniqueIx);

private:
    static bool bSystemStarted;

    void TickerRun();
    uint32 TickerStop();
    WMutex Ticker_Mutex;

    WThread* TickThread = nullptr;

    uint32 SleepMsBetweenCheck = 50;

    WSafeQueue<FWAwaitingTask*> AwaitingScheduledTasks;
    TArray<uint32> CancelledScheduledTasks;

    static WScheduledAsyncTaskManager* ManagerInstance;

    WScheduledAsyncTaskManager() = default;
    ~WScheduledAsyncTaskManager() = default;
    WScheduledAsyncTaskManager(const WScheduledAsyncTaskManager& Other);
    WScheduledAsyncTaskManager& operator=(const WScheduledAsyncTaskManager& Other)
    {
        return *this;
    }

    uint32 CurrentTaskIx = 1;
    WMutex CurrentTaskIx_Mutex;

    void StartSystem_Internal(uint32 SleepDurationMs);
    void EndSystem_Internal();
};

#endif //Pragma_Once_WScheduledAsyncTaskManager