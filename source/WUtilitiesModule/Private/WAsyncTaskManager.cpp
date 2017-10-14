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
    if (WorkerThreadNo <= 0) WorkerThreadNo = 1;
    WorkerThreadCount = WorkerThreadNo;

    AsyncWorkers = new FWAsyncWorker[WorkerThreadCount];
    for (int32 i = 0; i < WorkerThreadCount; i++)
    {
        AsyncWorkers[i].StartWorker();
    }
}
void UWAsyncTaskManager::EndSystem_Internal()
{
    for (int32 i = 0; i < WorkerThreadCount; i++)
    {
        AsyncWorkers[i].EndWorker();
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

void UWAsyncTaskManager::NewAsyncTask(WFutureAsyncTask& NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;

    auto AsTask = new FWAwaitingTask(NewTask, TaskParameters);
    FWAsyncWorker* PossibleFreeWorker = nullptr;
    if (ManagerInstance->FreeWorkers.Pop(PossibleFreeWorker) && PossibleFreeWorker != nullptr)
    {
        PossibleFreeWorker->SetData(AsTask);
    }
    else
    {
        ManagerInstance->AwaitingTasks.Push(AsTask);
    }
}
bool UWAsyncTaskManager::TryToGetAwaitingTask(FWAwaitingTask* Destination)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return false;
    return ManagerInstance->AwaitingTasks.Pop(Destination);
}

FWAsyncWorker::FWAsyncWorker()
{
    UWAsyncTaskManager::PushFreeWorker(this);
}
void FWAsyncWorker::SetData(FWAwaitingTask* Task)
{
    if (Task == nullptr) return;

    WScopeGuard Lock(&Mutex);
    CurrentData = Task;
    DataReady = true;
    Condition.signal();
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
void FWAsyncWorker::ProcessData()
{
    if (CurrentData)
    {
        if (CurrentData->FunctionPtr)
        {
            CurrentData->FunctionPtr(CurrentData->Parameters);
            for (FWAsyncTaskParameter* Param : CurrentData->Parameters)
            {
                if (Param != nullptr)
                {
                    delete (Param);
                }
            }
            CurrentData->Parameters.Empty();
        }
        delete (CurrentData);
    }
    DataReady = false;

    FWAwaitingTask *PossibleAwaitingTask = nullptr;
    if (UWAsyncTaskManager::TryToGetAwaitingTask(PossibleAwaitingTask))
    {
        SetData(PossibleAwaitingTask);
    }
    else
    {
        UWAsyncTaskManager::PushFreeWorker(this);
    }
}
void FWAsyncWorker::StartWorker()
{
    WorkerThread = new WThread(std::bind(&FWAsyncWorker::WorkersDen, this));
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