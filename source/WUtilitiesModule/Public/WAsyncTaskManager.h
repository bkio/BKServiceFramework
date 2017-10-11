// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WAsyncTaskManager
#define Pragma_Once_WAsyncTaskManager

#include "WEngine.h"
#include "WSafeQueue.h"
#include "WThread.h"
#include "WConditionVariable.h"

typedef std::function<void()> WFutureAsyncTask;

struct FWAsyncWorker
{

private:
    void ProcessData();

    WFutureAsyncTask CurrentData;
    bool DataReady = false;

    WConditionVariable Condition;
    WMutex Mutex;

    WThread* WorkerThread;

public:
    void StartWorker();
    void EndWorker();

    void SetData(WFutureAsyncTask& NewData);
    void WorkersDen();
};

class UWAsyncTaskManager
{

public:
    static void StartSystem(int32 WorkerThreadNo);
    static void EndSystem();

    static bool IsSystemStarted();

    static void PushFreeWorker(FWAsyncWorker* Worker);
    static bool TryToGetAwaitingTask(WFutureAsyncTask& Destination);

    static void NewAsyncTask(WFutureAsyncTask& NewTask);

private:
    static bool bSystemStarted;

    static UWAsyncTaskManager* ManagerInstance;

    UWAsyncTaskManager() {}
    ~UWAsyncTaskManager() {}
    UWAsyncTaskManager(const UWAsyncTaskManager& Other) {}
    UWAsyncTaskManager(UWAsyncTaskManager&& Other) {}
    UWAsyncTaskManager& operator=(const UWAsyncTaskManager& Other)
    {
        return *this;
    }

    void StartSystem_Internal(int32 WorkerThreadNo);
    void EndSystem_Internal();

    FWAsyncWorker* AsyncWorkers = nullptr;
    int32 WorkerThreadCount = 0;

    WSafeQueue<FWAsyncWorker*> FreeWorkers;
    WSafeQueue<WFutureAsyncTask> AwaitingTasks;
};

#endif //Pragma_Once_WAsyncTaskManager