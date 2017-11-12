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

    if (ManagerInstance)
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
    if (TickThread)
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
        if (AwaitingScheduledTask)
        {
            if (!AwaitingScheduledTask->bDoNotDeallocateParameters)
            {
                for (UWAsyncTaskParameter* Param : AwaitingScheduledTask->Parameters)
                {
                    if (Param)
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

void UWScheduledAsyncTaskManager::NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<UWAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || !ManagerInstance) return;
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
                            for (UWAsyncTaskParameter* Parameter : PossibleAwaitingTask->Parameters)
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
                else
                {
                    NewQueueAfterTick.Push(PossibleAwaitingTask);
                }
            }
        }
        AwaitingScheduledTasks.AddAll_NotTSTemporaryQueue(NewQueueAfterTick);
    }
}
uint32 UWScheduledAsyncTaskManager::TickerStop()
{
    if (!bSystemStarted) return 0;
    if (TickThread) delete (TickThread);
    TickThread = new WThread(std::bind(&UWScheduledAsyncTaskManager::TickerRun, this), std::bind(&UWScheduledAsyncTaskManager::TickerStop, this));
}