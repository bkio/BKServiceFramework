// Copyright Burak Kara, All rights reserved.

#include "BKScheduledTaskManager.h"
#include "BKAsyncTaskManager.h"

BKScheduledAsyncTaskManager* BKScheduledAsyncTaskManager::ManagerInstance = nullptr;

bool BKScheduledAsyncTaskManager::bSystemStarted = false;
void BKScheduledAsyncTaskManager::StartSystem(uint32 SleepDurationMs)
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    ManagerInstance = new BKScheduledAsyncTaskManager;
    ManagerInstance->StartSystem_Internal(SleepDurationMs > 0 ? SleepDurationMs : 50);
}
void BKScheduledAsyncTaskManager::EndSystem()
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
bool BKScheduledAsyncTaskManager::IsSystemStarted()
{
    return bSystemStarted;
}

void BKScheduledAsyncTaskManager::StartSystem_Internal(uint32 SleepDurationMs)
{
    SleepMsBetweenCheck = SleepDurationMs;
    TickThread = new BKThread(std::bind(&BKScheduledAsyncTaskManager::TickerRun, this), std::bind(&BKScheduledAsyncTaskManager::TickerStop, this));
}
void BKScheduledAsyncTaskManager::EndSystem_Internal()
{
    if (TickThread)
    {
        if (TickThread->IsJoinable())
        {
            TickThread->Join();
        }
        delete (TickThread);
    }

    FBKAwaitingTask* AwaitingScheduledTask = nullptr;
    while (AwaitingScheduledTasks.Pop(AwaitingScheduledTask))
    {
        if (AwaitingScheduledTask)
        {
            if (!AwaitingScheduledTask->bDoNotDeallocateParameters)
            {
                for (BKAsyncTaskParameter* Param : AwaitingScheduledTask->Parameters)
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

uint32 BKScheduledAsyncTaskManager::NewScheduledAsyncTask(BKFutureAsyncTask NewTask, TArray<BKAsyncTaskParameter*>& TaskParameters, uint32 WaitFor, bool bLoop, bool bDoNotDeallocateParameters)
{
    if (!bSystemStarted || !ManagerInstance) return 0;

    uint32 TaskUniqueIx;
    {
        BKScopeGuard LocalGuard(&ManagerInstance->CurrentTaskIx_Mutex);
        TaskUniqueIx = ManagerInstance->CurrentTaskIx++;
        if (TaskUniqueIx >= 16383) TaskUniqueIx = 1;
    }
    if (WaitFor == 0)
    {
        BKAsyncTaskManager::NewAsyncTask(NewTask, TaskParameters, bDoNotDeallocateParameters);
        return TaskUniqueIx;
    }

    auto AsTask = new FBKAwaitingTask(TaskUniqueIx, NewTask, TaskParameters, WaitFor, bLoop, bDoNotDeallocateParameters);
    ManagerInstance->AwaitingScheduledTasks.Push(AsTask);
    return TaskUniqueIx;
}
void BKScheduledAsyncTaskManager::CancelScheduledAsyncTask(uint32 TaskUniqueIx)
{
    if (!bSystemStarted || !ManagerInstance) return;

    BKScopeGuard LocalGuard(&ManagerInstance->Ticker_Mutex);
    ManagerInstance->CancelledScheduledTasks.AddUnique(TaskUniqueIx);
}

void BKScheduledAsyncTaskManager::TickerRun()
{
    while (bSystemStarted)
    {
        BKThread::SleepThread(SleepMsBetweenCheck);
        if (!bSystemStarted) return;

        BKSafeQueue<FBKAwaitingTask*> NewQueueAfterTick;

        FBKAwaitingTask* PossibleAwaitingTask = nullptr;

        BKScopeGuard LocalGuard(&Ticker_Mutex);

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
                            for (BKAsyncTaskParameter* Parameter : PossibleAwaitingTask->Parameters)
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
uint32 BKScheduledAsyncTaskManager::TickerStop()
{
    if (!bSystemStarted) return 0;
    if (TickThread) delete (TickThread);
    TickThread = new BKThread(std::bind(&BKScheduledAsyncTaskManager::TickerRun, this), std::bind(&BKScheduledAsyncTaskManager::TickerStop, this));
}