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
}

void UWAsyncTaskManager::PushFreeWorker(FWAsyncWorker* Worker)
{
    if (!bSystemStarted || ManagerInstance == nullptr || Worker == nullptr) return;
    ManagerInstance->FreeWorkers.Push(Worker);
}

void UWAsyncTaskManager::NewAsyncTask(WFutureAsyncTask& NewTask)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;

    FWAsyncWorker* PossibleFreeWorker = nullptr;
    if (ManagerInstance->FreeWorkers.Pop(PossibleFreeWorker) && PossibleFreeWorker != nullptr)
    {
        PossibleFreeWorker->SetData(NewTask);
    }
    else
    {
        ManagerInstance->AwaitingTasks.Push(NewTask);
    }
}
bool UWAsyncTaskManager::TryToGetAwaitingTask(WFutureAsyncTask& Destination)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return false;
    return ManagerInstance->AwaitingTasks.Pop(Destination);
}

void FWAsyncWorker::SetData(WFutureAsyncTask& NewData)
{
    WScopeGuard Lock(&Mutex);
    CurrentData = NewData;
    DataReady = true;
    Condition.signal();
}
void FWAsyncWorker::WorkersDen()
{
    while (UWAsyncTaskManager::IsSystemStarted())
    {
        {
            WScopeGuard Lock(&Mutex);
            while (!DataReady)
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
        CurrentData();
    }
    DataReady = false;

    WFutureAsyncTask PossibleAwaitingTask;
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