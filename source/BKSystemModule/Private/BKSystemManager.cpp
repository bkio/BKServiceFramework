// Copyright Burak Kara, All rights reserved.

#include "BKSystemManager.h"

BKSystemManager* BKSystemManager::ManagerInstance = nullptr;

bool BKSystemManager::bSystemStarted = false;
bool BKSystemManager::StartSystem(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new BKSystemManager();
    {
        BKScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);
        ManagerInstance->Callbacks.Reset();

        _UniqueCallbackID = ManagerInstance->CurrentCallbackUniqueIx++;
        if (ManagerInstance->CurrentCallbackUniqueIx >= 32767)
        {
            ManagerInstance->CurrentCallbackUniqueIx = 1;
        }

        ManagerInstance->Callbacks.Add(BKNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
    }

    if (!ManagerInstance->StartSystem_Internal())
    {
        EndSystem();
        return false;
    }

    return true;
}
bool BKSystemManager::StartSystem_Internal()
{
    SystemManagerThread = new BKThread(std::bind(&BKSystemManager::SystemThreadsDen, this), std::bind(&BKSystemManager::SystemThreadStopped, this));
    return true;
}

void BKSystemManager::EndSystem()
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
void BKSystemManager::EndSystem_Internal()
{
    if (SystemManagerThread)
    {
        if (SystemManagerThread->IsJoinable())
        {
            SystemManagerThread->Join();
        }
        delete (SystemManagerThread);
    }
}

void BKSystemManager::SystemThreadsDen()
{
    int32 Total_CPU_Utilization;
    int32 Total_Memory_Utilization;

    while (bSystemStarted)
    {
        Total_CPU_Utilization = CPUMonitor.GetUsage();
        Total_Memory_Utilization = MemoryMonitor.GetUsage();

        if (LastSystemInfo)
        {
            delete (LastSystemInfo);
        }
        LastSystemInfo = new BKSystemInfo(Total_CPU_Utilization, Total_Memory_Utilization);

        for (int32 i = Callbacks.Num() - 1; i >=0; i--)
        {
            if (Callbacks[i].NonComparable)
            {
                Callbacks[i].NonComparable(LastSystemInfo);
            }
            else
            {
                Callbacks.RemoveAt(i);
            }
        }

        BKThread::SleepThread(1000);
    }
}
uint32 BKSystemManager::SystemThreadStopped()
{
    if (!bSystemStarted) return 0;
    if (SystemManagerThread) delete (SystemManagerThread);
    SystemManagerThread = new BKThread(std::bind(&BKSystemManager::SystemThreadsDen, this), std::bind(&BKSystemManager::SystemThreadStopped, this));
    return 0;
}

void BKSystemManager::AddCallback(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (!bSystemStarted || !ManagerInstance || !_Callback) return;

    BKScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);

    _UniqueCallbackID = ManagerInstance->CurrentCallbackUniqueIx++;
    if (ManagerInstance->CurrentCallbackUniqueIx >= 32767)
    {
        ManagerInstance->CurrentCallbackUniqueIx = 1;
    }

    ManagerInstance->Callbacks.AddUnique(BKNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
}
void BKSystemManager::RemoveCallback(uint32 _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (!bSystemStarted || !ManagerInstance || !_Callback) return;

    BKScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);
    ManagerInstance->Callbacks.Remove(BKNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
}