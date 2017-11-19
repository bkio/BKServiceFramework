// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WAsyncTaskManager
#define Pragma_Once_WAsyncTaskManager

#include "WEngine.h"
#include "WSafeQueue.h"
#include "WTaskDefines.h"

struct FWAsyncWorker
{

private:
    void ProcessData();
    void ProcessData_CriticalPart();

    FWAwaitingTask* CurrentData = nullptr;
    bool DataReady = false;

    WConditionVariable Condition;
    WMutex Mutex;

    WThread* WorkerThread;

public:
    FWAsyncWorker();

    void StartWorker();
    void EndWorker();

    void SetData(FWAwaitingTask* Task, bool bSendSignal);
    void WorkersDen();
    uint32 WorkersStopCallback();
};

class WAsyncTaskManager
{

public:
    static void StartSystem(int32 WorkerThreadNo);
    static void EndSystem();

    static bool IsSystemStarted();

    static void PushFreeWorker(FWAsyncWorker* Worker);
    static FWAwaitingTask* TryToGetAwaitingTask();

    static void NewAsyncTask(WFutureAsyncTask& NewTask, TArray<WAsyncTaskParameter*>& TaskParameters, bool bDoNotDeallocateParameters = false);

private:
    static bool bSystemStarted;

    static WAsyncTaskManager* ManagerInstance;

    WAsyncTaskManager() = default;
    ~WAsyncTaskManager() = default;
    WAsyncTaskManager(const WAsyncTaskManager& Other);
    WAsyncTaskManager& operator=(const WAsyncTaskManager& Other)
    {
        return *this;
    }

    friend class FWAsyncWorker;
    static uint32 AsyncWorkerStopped(FWAsyncWorker* StoppedWorker);

    void StartSystem_Internal(int32 WorkerThreadNo);
    void EndSystem_Internal();

    void StartWorkers(int32 WorkerThreadNo);

    FWAsyncWorker** AsyncWorkers = nullptr;
    int32 WorkerThreadCount = 0;

    WSafeQueue<FWAsyncWorker*> FreeWorkers;
    WSafeQueue<FWAwaitingTask*> AwaitingTasks;
};

#endif //Pragma_Once_WAsyncTaskManager