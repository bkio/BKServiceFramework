// Copyright Pagansoft.com, All rights reserved.

#include "WScheduledTaskManager.h"
#include "WAsyncTaskManager.h"

UWScheduledAsyncTaskManager* UWScheduledAsyncTaskManager::ManagerInstance = nullptr;

bool UWScheduledAsyncTaskManager::bSystemStarted = false;
void UWScheduledAsyncTaskManager::StartSystem(uint32 SleepDurationMs)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new UWScheduledAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(SleepDurationMs > 0 ? SleepDurationMs : 50);
}
void UWScheduledAsyncTaskManager::EndSystem()
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
bool UWScheduledAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void UWScheduledAsyncTaskManager::StartSystem_Internal(uint32 SleepDurationMs)
{
    SleepMsBetweenCheck = SleepDurationMs;
    TickThread = new WThread(std::bind(&UWScheduledAsyncTaskManager::TickerRun, this), std::bind(&UWScheduledAsyncTaskManager::TickerStop, this));
}
void UWScheduledAsyncTaskManager::EndSystem_Internal()
{
    if (TickThread != nullptr)
    {
        if (TickThread->IsJoinable())
        {
            TickThread->Join();
        }
        delete (TickThread);
    }

    FWAwaitingTask* AwaitingScheduledTask = nullptr;
    while (AwaitingScheduledTasks.Pop(AwaitingScheduledTask))
    {
        if (AwaitingScheduledTask != nullptr)
        {
            if (!AwaitingScheduledTask->bDoNotDeallocateParameters)
            {
                for (FWAsyncTaskParameter* Param : AwaitingScheduledTask->Parameters)
                {
                    if (Param != nullptr)
                    {
                        delete (Param);
                    }
                }
                AwaitingScheduledTask->Parameters.Empty();
            }
            delete (AwaitingScheduledTask);
        }
    }
}

void UWScheduledAsyncTaskManager::NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;
    if (WaitFor == 0)
    {
        UWAsyncTaskManager::NewAsyncTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
        return;
    }

    auto AsTask = new FWAwaitingTask(NewTask, TaskParameters, WaitFor, bLoop, bDoNotDeallocateParameters);
    ManagerInstance->AwaitingScheduledTasks.Push(AsTask);
}

void UWScheduledAsyncTaskManager::TickerRun()
{
    while (bSystemStarted)
    {
        WThread::SleepThread(SleepMsBetweenCheck);
        if (!bSystemStarted) return;

        WSafeQueue<FWAwaitingTask*> NewQueueAfterTick;

        FWAwaitingTask* PossibleAwaitingTask = nullptr;
        while (AwaitingScheduledTasks.Pop(PossibleAwaitingTask))
        {
            if (PossibleAwaitingTask)
            {
                PossibleAwaitingTask->PassedTimeMs += SleepMsBetweenCheck;
                if (PossibleAwaitingTask->PassedTimeMs >= PossibleAwaitingTask->WaitTimeMs)
                {
                    if (PossibleAwaitingTask->FunctionPtr)
                    {
                        PossibleAwaitingTask->FunctionPtr(PossibleAwaitingTask->Parameters);
                    }
                    if (!PossibleAwaitingTask->bLoop)
                    {
                        if (!PossibleAwaitingTask->bDoNotDeallocateParameters)
                        {
                            for (FWAsyncTaskParameter* Parameter : PossibleAwaitingTask->Parameters)
                            {
                                if (Parameter)
                                {
                                    delete (Parameter);
                                }
                            }
                            PossibleAwaitingTask->Parameters.Empty();
                        }
                        delete (PossibleAwaitingTask);
                    }
                    else
                    {
                        PossibleAwaitingTask->PassedTimeMs = 0;
                        NewQueueAfterTick.Push(PossibleAwaitingTask);
                    }
                }
            }
        }

        FWAwaitingTask* LoopingTask = nullptr;
        while (NewQueueAfterTick.Pop(LoopingTask))
        {
            if (LoopingTask)
            {
                AwaitingScheduledTasks.Push(LoopingTask);
            }
        }
    }
}
uint32 UWScheduledAsyncTaskManager::TickerStop()
{
    if (!bSystemStarted) return 0;
    if (TickThread) delete (TickThread);
    TickThread = new WThread(std::bind(&UWScheduledAsyncTaskManager::TickerRun, this), std::bind(&UWScheduledAsyncTaskManager::TickerStop, this));
}