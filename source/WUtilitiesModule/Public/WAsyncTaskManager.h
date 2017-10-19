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

    FWAwaitingTask* CurrentData = nullptr;
    bool DataReady = false;

    WConditionVariable Condition;
    WMutex Mutex;

    WThread* WorkerThread;

public:
    FWAsyncWorker();

    void StartWorker();
    void EndWorker();

    void SetData(FWAwaitingTask* Task);
    void WorkersDen();
};

class UWAsyncTaskManager
{

public:
    static void StartSystem(int32 WorkerThreadNo);
    static void EndSystem();

    static bool IsSystemStarted();

    static void PushFreeWorker(FWAsyncWorker* Worker);
    static bool TryToGetAwaitingTask(FWAwaitingTask* Destination);

    static void NewAsyncTask(WFutureAsyncTask& NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters);

private:
    static bool bSystemStarted;

    static UWAsyncTaskManager* ManagerInstance;

    UWAsyncTaskManager() = default;
    ~UWAsyncTaskManager() = default;
    UWAsyncTaskManager(const UWAsyncTaskManager& Other);
    UWAsyncTaskManager& operator=(const UWAsyncTaskManager& Other)
    {
        return *this;
    }

    void StartSystem_Internal(int32 WorkerThreadNo);
    void EndSystem_Internal();

    FWAsyncWorker* AsyncWorkers = nullptr;
    int32 WorkerThreadCount = 0;

    WSafeQueue<FWAsyncWorker*> FreeWorkers;
    WSafeQueue<FWAwaitingTask*> AwaitingTasks;
};

#endif //Pragma_Once_WAsyncTaskManager