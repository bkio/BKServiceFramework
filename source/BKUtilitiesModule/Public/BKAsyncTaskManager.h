// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKAsyncTaskManager
#define Pragma_Once_BKAsyncTaskManager

#include "BKEngine.h"
#include "BKSafeQueue.h"
#include "BKTaskDefines.h"

struct FBKAsyncWorker
{

private:
    void ProcessData();
    void ProcessData_CriticalPart();

    FBKAwaitingTask* CurrentData = nullptr;
    bool DataReady = false;

    BKConditionVariable Condition;
    BKMutex Mutex;

    BKThread* WorkerThread;

public:
    FBKAsyncWorker();

    void StartWorker();
    void EndWorker();

    void SetData(FBKAwaitingTask* Task, bool bSendSignal);
    void WorkersDen();
    uint32 WorkersStopCallback();
};

class BKAsyncTaskManager
{

public:
    static void StartSystem(int32 WorkerThreadNo);
    static void EndSystem();

    static bool IsSystemStarted();

    static void PushFreeWorker(FBKAsyncWorker* Worker);
    static FBKAwaitingTask* TryToGetAwaitingTask();

    static void NewAsyncTask(BKFutureAsyncTask& NewTask, TArray<BKAsyncTaskParameter*>& TaskParameters, bool bDoNotDeallocateParameters = false);

private:
    static bool bSystemStarted;

    static BKAsyncTaskManager* ManagerInstance;

    BKAsyncTaskManager() = default;
    ~BKAsyncTaskManager() = default;
    BKAsyncTaskManager(const BKAsyncTaskManager& Other);
    BKAsyncTaskManager& operator=(const BKAsyncTaskManager& Other)
    {
        return *this;
    }

    friend class FBKAsyncWorker;
    static uint32 AsyncWorkerStopped(FBKAsyncWorker* StoppedWorker);

    void StartSystem_Internal(int32 WorkerThreadNo);
    void EndSystem_Internal();

    void StartWorkers(int32 WorkerThreadNo);

    FBKAsyncWorker** AsyncWorkers = nullptr;
    int32 WorkerThreadCount = 0;

    BKSafeQueue<FBKAsyncWorker*> FreeWorkers;
    BKSafeQueue<FBKAwaitingTask*> AwaitingTasks;
};

#endif //Pragma_Once_BKAsyncTaskManager