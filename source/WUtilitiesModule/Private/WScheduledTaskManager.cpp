// Copyright Pagansoft.com, All rights reserved.

#include "WScheduledTaskManager.h"
#include "WAsyncTaskManager.h"

WScheduledAsyncTaskManager* WScheduledAsyncTaskManager::ManagerInstance = nullptr;

bool WScheduledAsyncTaskManager::bSystemStarted = false;
void WScheduledAsyncTaskManager::StartSystem(uint32 SleepDurationMs)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new WScheduledAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(SleepDurationMs > 0 ? SleepDurationMs : 50);
}
void WScheduledAsyncTaskManager::EndSystem()
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
bool WScheduledAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void WScheduledAsyncTaskManager::StartSystem_Internal(uint32 SleepDurationMs)
{
    SleepMsBetweenCheck = SleepDurationMs;
    TickThread = new WThread(std::bind(&WScheduledAsyncTaskManager::TickerRun, this), std::bind(&WScheduledAsyncTaskManager::TickerStop, this));
}
void WScheduledAsyncTaskManager::EndSystem_Internal()
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
                for (WAsyncTaskParameter* Param : AwaitingScheduledTask->Parameters)
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

uint32 WScheduledAsyncTaskManager::NewScheduledAsyncTask(WFutureAsyncTask NewTask, TArray<WAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || !ManagerInstance) return 0;

    uint32 TaskUniqueIx;
    {
        WScopeGuard LocalGuard(&ManagerInstance->CurrentTaskIx_Mutex);
        TaskUniqueIx = ManagerInstance->CurrentTaskIx++;
        if (TaskUniqueIx >= 16383) TaskUniqueIx = 1;
    }
    if (WaitFor == 0)
    {
        WAsyncTaskManager::NewAsyncTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
        return TaskUniqueIx;
    }

    auto AsTask = new FWAwaitingTask(TaskUniqueIx, NewTask, TaskParameters, WaitFor, bLoop, bDoNotDeallocateParameters);
    ManagerInstance->AwaitingScheduledTasks.Push(AsTask);
    return TaskUniqueIx;
}
void WScheduledAsyncTaskManager::CancelScheduledAsyncTask(uint32 TaskUniqueIx)
{
    if (!bSystemStarted || !ManagerInstance) return;

    WScopeGuard LocalGuard(&ManagerInstance->Ticker_Mutex);
    ManagerInstance->CancelledScheduledTasks.AddUnique(TaskUniqueIx);
}

void WScheduledAsyncTaskManager::TickerRun()
{
    while (bSystemStarted)
    {
        WThread::SleepThread(SleepMsBetweenCheck);
        if (!bSystemStarted) return;

        WSafeQueue<FWAwaitingTask*> NewQueueAfterTick;

        FWAwaitingTask* PossibleAwaitingTask = nullptr;

        WScopeGuard LocalGuard(&Ticker_Mutex);

        while (AwaitingScheduledTasks.Pop(PossibleAwaitingTask))
        {
            if (PossibleAwaitingTask && !CancelledScheduledTasks.Contains(PossibleAwaitingTask->TaskUniqueIx))
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
                            for (WAsyncTaskParameter* Parameter : PossibleAwaitingTask->Parameters)
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
uint32 WScheduledAsyncTaskManager::TickerStop()
{
    if (!bSystemStarted) return 0;
    if (TickThread) delete (TickThread);
    TickThread = new WThread(std::bind(&WScheduledAsyncTaskManager::TickerRun, this), std::bind(&WScheduledAsyncTaskManager::TickerStop, this));
}