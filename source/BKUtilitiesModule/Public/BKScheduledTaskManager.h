// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKScheduledAsyncTaskManager
#define Pragma_Once_BKScheduledAsyncTaskManager

#include "BKEngine.h"
#include "BKTaskDefines.h"
#include "BKSafeQueue.h"

class BKScheduledAsyncTaskManager
{

public:
    static void StartSystem(uint32 SleepDurationMs);
    static void EndSystem();

    static bool IsSystemStarted();

    static uint32 NewScheduledAsyncTask(BKFutureAsyncTask NewTask, TArray<BKAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters = false);
    static void CancelScheduledAsyncTask(uint32 TaskUniqueIx);

private:
    static bool bSystemStarted;

    void TickerRun();
    uint32 TickerStop();
    BKMutex Ticker_Mutex;

    BKThread* TickThread = nullptr;

    uint32 SleepMsBetweenCheck = 50;

    BKSafeQueue<FBKAwaitingTask*> AwaitingScheduledTasks;
    TArray<uint32> CancelledScheduledTasks;

    static BKScheduledAsyncTaskManager* ManagerInstance;

    BKScheduledAsyncTaskManager() = default;
    ~BKScheduledAsyncTaskManager() = default;
    BKScheduledAsyncTaskManager(const BKScheduledAsyncTaskManager& Other);
    BKScheduledAsyncTaskManager& operator=(const BKScheduledAsyncTaskManager& Other)
    {
        return *this;
    }

    uint32 CurrentTaskIx = 1;
    BKMutex CurrentTaskIx_Mutex;

    void StartSystem_Internal(uint32 SleepDurationMs);
    void EndSystem_Internal();
};

#endif //Pragma_Once_BKScheduledAsyncTaskManager