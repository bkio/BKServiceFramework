// Copyright Burak Kara, All rights reserved.

#include "BKAsyncTaskManager.h"

BKAsyncTaskManager* BKAsyncTaskManager::ManagerInstance = nullptr;

bool BKAsyncTaskManager::bSystemStarted = false;
void BKAsyncTaskManager::StartSystem(int32 WorkerThreadNo)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new BKAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(WorkerThreadNo);
}
void BKAsyncTaskManager::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    if (ManagerInstance)
    {
        ManagerInstance->EndSystem_Internal();
        delete (ManagerInstance);
        ManagerInstance = nullptr;
    }
}
bool BKAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void BKAsyncTaskManager::StartSystem_Internal(int32 WorkerThreadNo)
{
    StartWorkers(WorkerThreadNo);
}
void BKAsyncTaskManager::StartWorkers(int32 WorkerThreadNo)
{
    if (WorkerThreadNo <= 0) WorkerThreadNo = 1;
    WorkerThreadCount = WorkerThreadNo;

    AsyncWorkers = new FBKAsyncWorker*[WorkerThreadCount];
    for (int32 i = 0; i < WorkerThreadCount; i++)
    {
        AsyncWorkers[i] = new FBKAsyncWorker;
        AsyncWorkers[i]->StartWorker();
    }
}
void BKAsyncTaskManager::EndSystem_Internal()
{
    for (int32 i = 0; i < WorkerThreadCount; i++)
    {
        if (AsyncWorkers[i])
        {
            AsyncWorkers[i]->EndWorker();
            delete (AsyncWorkers[i]);
        }
    }
    delete[] AsyncWorkers;

    FBKAwaitingTask* AwaitingTask = nullptr;
    while (AwaitingTasks.Pop(AwaitingTask))
    {
        if (AwaitingTask)
        {
            for (BKAsyncTaskParameter* Param : AwaitingTask->Parameters)
            {
                if (Param)
                {
                    delete (Param);
                }
            }
            AwaitingTask->Parameters.Empty();
            delete (AwaitingTask);
        }
    }
}

void BKAsyncTaskManager::PushFreeWorker(FBKAsyncWorker* Worker)
{
    if (!bSystemStarted || !ManagerInstance || !Worker) return;
    ManagerInstance->FreeWorkers.Push(Worker);
}

void BKAsyncTaskManager::NewAsyncTask(BKFutureAsyncTask& NewTask, TArray<BKAsyncTaskParameter*>& TaskParameters, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || !ManagerInstance) return;

    auto AsTask = new FBKAwaitingTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
    FBKAsyncWorker* PossibleFreeWorker = nullptr;
    if (ManagerInstance->FreeWorkers.Pop(PossibleFreeWorker) && PossibleFreeWorker)
    {
        PossibleFreeWorker->SetData(AsTask, true);
    }
    else
    {
        AsTask->QueuedTimestamp = BKUtilities::GetTimeStampInMSDetailed();
        AsTask->bQueued = true;
        ManagerInstance->AwaitingTasks.Push(AsTask);
    }
}
FBKAwaitingTask* BKAsyncTaskManager::TryToGetAwaitingTask()
{
    if (!bSystemStarted || !ManagerInstance) return nullptr;

    FBKAwaitingTask* Destination = nullptr;
    ManagerInstance->AwaitingTasks.Pop(Destination);

    return Destination;
}
uint32 BKAsyncTaskManager::AsyncWorkerStopped(FBKAsyncWorker* StoppedWorker)
{
    if (!bSystemStarted || !ManagerInstance || !StoppedWorker) return 0;

    if (!ManagerInstance->AsyncWorkers)
    {
        ManagerInstance->StartWorkers(ManagerInstance->WorkerThreadCount);
    }

    for (int32 i = 0; i < ManagerInstance->WorkerThreadCount; i++)
    {
        if (ManagerInstance->AsyncWorkers[i] == StoppedWorker)
        {
            delete (ManagerInstance->AsyncWorkers[i]);
            ManagerInstance->AsyncWorkers[i] = new FBKAsyncWorker;
            ManagerInstance->AsyncWorkers[i]->StartWorker();
            BKUtilities::Print(EBKLogType::Warning, FString("An AsyncWorker has stopped. Another worker has just been started."));
            return 0;
        }
    }

    return 0;
}

FBKAsyncWorker::FBKAsyncWorker()
{
    BKAsyncTaskManager::PushFreeWorker(this);
}
void FBKAsyncWorker::SetData(FBKAwaitingTask* Task, bool bSendSignal)
{
    if (!Task) return;

    BKScopeGuard Lock(&Mutex);
    CurrentData = Task;
    DataReady = true;
    if (bSendSignal)
    {
        Condition.signal();
    }
}
void FBKAsyncWorker::WorkersDen()
{
    while (BKAsyncTaskManager::IsSystemStarted())
    {
        {
            BKScopeGuard Lock(&Mutex);
            while (!DataReady && BKAsyncTaskManager::IsSystemStarted())
            {
                Condition.wait(Lock);
            }
        }
        if (!BKAsyncTaskManager::IsSystemStarted()) return;
        ProcessData();
    }
}
uint32 FBKAsyncWorker::WorkersStopCallback()
{
    return BKAsyncTaskManager::AsyncWorkerStopped(this);
}
void FBKAsyncWorker::ProcessData_CriticalPart()
{
    if (CurrentData)
    {
        if (CurrentData->FunctionPtr)
        {
            CurrentData->FunctionPtr(CurrentData->Parameters);
        }
        if (!CurrentData->bDoNotDeallocateParameters)
        {
            for (BKAsyncTaskParameter* Parameter : CurrentData->Parameters)
            {
                if (Parameter)
                {
                    delete (Parameter);
                }
            }
            CurrentData->Parameters.Empty();
        }
        delete (CurrentData);
        CurrentData = nullptr;
    }
}
void FBKAsyncWorker::ProcessData()
{
    ProcessData_CriticalPart();
    DataReady = false;

    FBKAwaitingTask* PossibleAwaitingTask = BKAsyncTaskManager::TryToGetAwaitingTask();
    if (PossibleAwaitingTask)
    {
        if (PossibleAwaitingTask->bQueued)
        {
            double DiffMs = BKUtilities::GetTimeStampInMSDetailed() - PossibleAwaitingTask->QueuedTimestamp;
            if (DiffMs > 1000)
            {
                BKUtilities::Print(EBKLogType::Warning, FString("WAsyncTask was in queue for ") + FString::FromFloat(DiffMs));
            }

            PossibleAwaitingTask->bQueued = false;
            PossibleAwaitingTask->QueuedTimestamp = 0;
        }
        SetData(PossibleAwaitingTask, false);
        ProcessData();
    }
    else
    {
        BKAsyncTaskManager::PushFreeWorker(this);
    }
}
void FBKAsyncWorker::StartWorker()
{
    WorkerThread = new BKThread(std::bind(&FBKAsyncWorker::WorkersDen, this), std::bind(&FBKAsyncWorker::WorkersStopCallback, this));
}
void FBKAsyncWorker::EndWorker()
{
    if (WorkerThread)
    {
        if (!DataReady)
        {
            BKScopeGuard Lock(&Mutex);
            Condition.signal();
        }
        if (WorkerThread->IsJoinable())
        {
            WorkerThread->Join();
        }
    }
    delete (WorkerThread);
}