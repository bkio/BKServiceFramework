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
    for (int32 i = AwaitingScheduledTasks.Num() - 1; i >=0; i--)
    {
        if ((AwaitingScheduledTask = AwaitingScheduledTasks[i]))
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

void UWScheduledAsyncTaskManager::NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<FWAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocate)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;
    if (WaitFor == 0)
    {
        UWAsyncTaskManager::NewAsyncTask(NewTask, TaskParameters, bDoNotDeallocate);
        return;
    }

    auto AsTask = new FWAwaitingTask(NewTask, TaskParameters, WaitFor, bLoop, bDoNotDeallocate);
    ManagerInstance->AwaitingScheduledTasks.Add(AsTask);
}

void UWScheduledAsyncTaskManager::TickerRun()
{
    while (bSystemStarted)
    {
        WThread::SleepThread(SleepMsBetweenCheck);
        if (!bSystemStarted) return;

        FWAwaitingTask* PossibleAwaitingTask = nullptr;
        for (int32 i = AwaitingScheduledTasks.Num() - 1; i >= 0; i--)
        {
            PossibleAwaitingTask = AwaitingScheduledTasks[i];
            if (PossibleAwaitingTask)
            {
                PossibleAwaitingTask->PassedTimeMs += SleepMsBetweenCheck;
                if (PossibleAwaitingTask->PassedTimeMs >= PossibleAwaitingTask->WaitTimeMs)
                {
                    UWAsyncTaskManager::NewAsyncTask(PossibleAwaitingTask->FunctionPtr, PossibleAwaitingTask->Parameters, PossibleAwaitingTask->bDoNotDeallocateParameters);
                    if (!PossibleAwaitingTask->bLoop)
                    {
                        AwaitingScheduledTasks.RemoveAt(i);
                        delete (PossibleAwaitingTask);
                    }
                    else
                    {
                        PossibleAwaitingTask->PassedTimeMs = 0;
                    }
                }
            }
            else
            {
                AwaitingScheduledTasks.RemoveAt(i);
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