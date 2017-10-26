// Copyright Pagansoft.com, All rights reserved.

#include "WAsyncTaskManager.h"

UWAsyncTaskManager* UWAsyncTaskManager::ManagerInstance = nullptr;

bool UWAsyncTaskManager::bSystemStarted = false;
void UWAsyncTaskManager::StartSystem(int32 WorkerThreadNo)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new UWAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(WorkerThreadNo);
}
void UWAsyncTaskManager::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    if (ManagerInstance != nullptr)
    {
        ManagerInstance->EndSystem_Internal();
        delete (ManagerInstance);
        ManagerInstance = nullptr;
    }
}
bool UWAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void UWAsyncTaskManager::StartSystem_Internal(int32 WorkerThreadNo)
{
    StartWorkers(WorkerThreadNo);
}
void UWAsyncTaskManager::StartWorkers(int32 WorkerThreadNo)
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
void UWAsyncTaskManager::EndSystem_Internal()
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
        if (AwaitingTask != nullptr)
        {
            for (FWAsyncTaskParameter* Param : AwaitingTask->Parameters)
            {
                if (Param != nullptr)
                {
                    delete (Param);
                }
            }
            AwaitingTask->Parameters.Empty();
            delete (AwaitingTask);
        }
    }
}

void UWAsyncTaskManager::PushFreeWorker(FWAsyncWorker* Worker)
{
    if (!bSystemStarted || ManagerInstance == nullptr || Worker == nullptr) return;
    ManagerInstance->FreeWorkers.Push(Worker);
}

void UWAsyncTaskManager::NewAsyncTask(WFutureAsyncTask& NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;

    auto AsTask = new FWAwaitingTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
    FWAsyncWorker* PossibleFreeWorker = nullptr;
    if (ManagerInstance->FreeWorkers.Pop(PossibleFreeWorker) && PossibleFreeWorker != nullptr)
    {
        PossibleFreeWorker->SetData(AsTask, true);
    }
    else
    {
        AsTask->QueuedTimestamp = UWUtilities::GetTimeStampInMSDetailed();
        AsTask->bQueued = true;
        ManagerInstance->AwaitingTasks.Push(AsTask);
    }
}
FWAwaitingTask* UWAsyncTaskManager::TryToGetAwaitingTask()
{
    if (!bSystemStarted || ManagerInstance == nullptr) return nullptr;

    FWAwaitingTask* Destination = nullptr;
    ManagerInstance->AwaitingTasks.Pop(Destination);

    return Destination;
}
uint32 UWAsyncTaskManager::AsyncWorkerStopped(FWAsyncWorker* StoppedWorker)
{
    if (!bSystemStarted || ManagerInstance == nullptr || StoppedWorker == nullptr) return 0;

    if (ManagerInstance->AsyncWorkers == nullptr)
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
            return 0;
        }
    }

    return 0;
}

FWAsyncWorker::FWAsyncWorker()
{
    UWAsyncTaskManager::PushFreeWorker(this);
}
void FWAsyncWorker::SetData(FWAwaitingTask* Task, bool bSendSignal)
{
    if (Task == nullptr) return;

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
    while (UWAsyncTaskManager::IsSystemStarted())
    {
        {
            WScopeGuard Lock(&Mutex);
            while (!DataReady && UWAsyncTaskManager::IsSystemStarted())
            {
                Condition.wait(Lock);
            }
        }
        if (!UWAsyncTaskManager::IsSystemStarted()) return;
        ProcessData();
    }
}
uint32 FWAsyncWorker::WorkersStopCallback()
{
    return UWAsyncTaskManager::AsyncWorkerStopped(this);
}
void FWAsyncWorker::ProcessData()
{
    if (CurrentData)
    {
        if (CurrentData->FunctionPtr)
        {
            CurrentData->FunctionPtr(CurrentData->Parameters);
        }
        if (!CurrentData->bDoNotDeallocateParameters)
        {
            for (FWAsyncTaskParameter* Parameter : CurrentData->Parameters)
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
    DataReady = false;

    FWAwaitingTask* PossibleAwaitingTask = UWAsyncTaskManager::TryToGetAwaitingTask();
    if (PossibleAwaitingTask)
    {
        if (PossibleAwaitingTask->bQueued)
        {
            double DiffMs = UWUtilities::GetTimeStampInMSDetailed() - PossibleAwaitingTask->QueuedTimestamp;
            UWUtilities::Print(EWLogType::Warning, L"WAsyncTask was in queue for " + FString::FromFloat(DiffMs));

            PossibleAwaitingTask->bQueued = false;
            PossibleAwaitingTask->QueuedTimestamp = 0;
        }
        SetData(PossibleAwaitingTask, false);
        ProcessData();
    }
    else
    {
        UWAsyncTaskManager::PushFreeWorker(this);
    }
}
void FWAsyncWorker::StartWorker()
{
    WorkerThread = new WThread(std::bind(&FWAsyncWorker::WorkersDen, this), std::bind(&FWAsyncWorker::WorkersStopCallback, this));
}
void FWAsyncWorker::EndWorker()
{
    if (WorkerThread != nullptr)
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