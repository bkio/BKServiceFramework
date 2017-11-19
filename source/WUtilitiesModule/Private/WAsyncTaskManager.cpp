// Copyright Pagansoft.com, All rights reserved.

#include "WAsyncTaskManager.h"

WAsyncTaskManager* WAsyncTaskManager::ManagerInstance = nullptr;

bool WAsyncTaskManager::bSystemStarted = false;
void WAsyncTaskManager::StartSystem(int32 WorkerThreadNo)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new WAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(WorkerThreadNo);
}
void WAsyncTaskManager::EndSystem()
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
bool WAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void WAsyncTaskManager::StartSystem_Internal(int32 WorkerThreadNo)
{
    StartWorkers(WorkerThreadNo);
}
void WAsyncTaskManager::StartWorkers(int32 WorkerThreadNo)
{
    if (WorkerThreadNo <= 0) WorkerThreadNo = 1;
    WorkerThreadCount = WorkerThreadNo;

    AsyncWorkers = new FWAsyncWorker*[WorkerThreadCount];
    for (int32 i = 0; i < WorkerThreadCount; i++)
    {
        AsyncWorkers[i] = new FWAsyncWorker;
        AsyncWorkers[i]->StartWorker();
    }
}
void WAsyncTaskManager::EndSystem_Internal()
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

    FWAwaitingTask* AwaitingTask = nullptr;
    while (AwaitingTasks.Pop(AwaitingTask))
    {
        if (AwaitingTask)
        {
            for (WAsyncTaskParameter* Param : AwaitingTask->Parameters)
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

void WAsyncTaskManager::PushFreeWorker(FWAsyncWorker* Worker)
{
    if (!bSystemStarted || !ManagerInstance || !Worker) return;
    ManagerInstance->FreeWorkers.Push(Worker);
}

void WAsyncTaskManager::NewAsyncTask(WFutureAsyncTask& NewTask, TArray<WAsyncTaskParameter*>& TaskParameters, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || !ManagerInstance) return;

    auto AsTask = new FWAwaitingTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
    FWAsyncWorker* PossibleFreeWorker = nullptr;
    if (ManagerInstance->FreeWorkers.Pop(PossibleFreeWorker) && PossibleFreeWorker)
    {
        PossibleFreeWorker->SetData(AsTask, true);
    }
    else
    {
        AsTask->QueuedTimestamp = WUtilities::GetTimeStampInMSDetailed();
        AsTask->bQueued = true;
        ManagerInstance->AwaitingTasks.Push(AsTask);
    }
}
FWAwaitingTask* WAsyncTaskManager::TryToGetAwaitingTask()
{
    if (!bSystemStarted || !ManagerInstance) return nullptr;

    FWAwaitingTask* Destination = nullptr;
    ManagerInstance->AwaitingTasks.Pop(Destination);

    return Destination;
}
uint32 WAsyncTaskManager::AsyncWorkerStopped(FWAsyncWorker* StoppedWorker)
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
            ManagerInstance->AsyncWorkers[i] = new FWAsyncWorker;
            ManagerInstance->AsyncWorkers[i]->StartWorker();
            WUtilities::Print(EWLogType::Warning, FString("An AsyncWorker has stopped. Another worker has just been started."));
            return 0;
        }
    }

    return 0;
}

FWAsyncWorker::FWAsyncWorker()
{
    WAsyncTaskManager::PushFreeWorker(this);
}
void FWAsyncWorker::SetData(FWAwaitingTask* Task, bool bSendSignal)
{
    if (!Task) return;

    WScopeGuard Lock(&Mutex);
    CurrentData = Task;
    DataReady = true;
    if (bSendSignal)
    {
        Condition.signal();
    }
}
void FWAsyncWorker::WorkersDen()
{
    while (WAsyncTaskManager::IsSystemStarted())
    {
        {
            WScopeGuard Lock(&Mutex);
            while (!DataReady && WAsyncTaskManager::IsSystemStarted())
            {
                Condition.wait(Lock);
            }
        }
        if (!WAsyncTaskManager::IsSystemStarted()) return;
        ProcessData();
    }
}
uint32 FWAsyncWorker::WorkersStopCallback()
{
    return WAsyncTaskManager::AsyncWorkerStopped(this);
}
void FWAsyncWorker::ProcessData_CriticalPart()
{
    if (CurrentData)
    {
        if (CurrentData->FunctionPtr)
        {
            CurrentData->FunctionPtr(CurrentData->Parameters);
        }
        if (!CurrentData->bDoNotDeallocateParameters)
        {
            for (WAsyncTaskParameter* Parameter : CurrentData->Parameters)
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
void FWAsyncWorker::ProcessData()
{
    ProcessData_CriticalPart();
    DataReady = false;

    FWAwaitingTask* PossibleAwaitingTask = WAsyncTaskManager::TryToGetAwaitingTask();
    if (PossibleAwaitingTask)
    {
        if (PossibleAwaitingTask->bQueued)
        {
            double DiffMs = WUtilities::GetTimeStampInMSDetailed() - PossibleAwaitingTask->QueuedTimestamp;
            if (DiffMs > 1000)
            {
                WUtilities::Print(EWLogType::Warning, FString("WAsyncTask was in queue for ") + FString::FromFloat(DiffMs));
            }

            PossibleAwaitingTask->bQueued = false;
            PossibleAwaitingTask->QueuedTimestamp = 0;
        }
        SetData(PossibleAwaitingTask, false);
        ProcessData();
    }
    else
    {
        WAsyncTaskManager::PushFreeWorker(this);
    }
}
void FWAsyncWorker::StartWorker()
{
    WorkerThread = new WThread(std::bind(&FWAsyncWorker::WorkersDen, this), std::bind(&FWAsyncWorker::WorkersStopCallback, this));
}
void FWAsyncWorker::EndWorker()
{
    if (WorkerThread)
    {
        if (!DataReady)
        {
            WScopeGuard Lock(&Mutex);
            Condition.signal();
        }
        if (WorkerThread->IsJoinable())
        {
            WorkerThread->Join();
        }
    }
    delete (WorkerThread);
}