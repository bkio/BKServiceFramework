// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WScheduledAsyncTaskManager
#define Pragma_Once_WScheduledAsyncTaskManager

#include "WEngine.h"
#include "WTaskDefines.h"
#include "WSafeQueue.h"

class UWScheduledAsyncTaskManager
{

public:
    static void StartSystem(uint32 SleepDurationMs);
    static void EndSystem();

    static bool IsSystemStarted();

    static void NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters = false);

private:
    static bool bSystemStarted;

    void TickerRun();
    uint32 TickerStop();

    WThread* TickThread = nullptr;

    uint32 SleepMsBetweenCheck = 50;

    WSafeQueue<FWAwaitingTask*> AwaitingScheduledTasks;

    static UWScheduledAsyncTaskManager* ManagerInstance;

    UWScheduledAsyncTaskManager() = default;
    ~UWScheduledAsyncTaskManager() = default;
    UWScheduledAsyncTaskManager(const UWScheduledAsyncTaskManager& Other);
    UWScheduledAsyncTaskManager& operator=(const UWScheduledAsyncTaskManager& Other)
    {
        return *this;
    }

    void StartSystem_Internal(uint32 SleepDurationMs);
    void EndSystem_Internal();
};

#endif //Pragma_Once_WScheduledAsyncTaskManager