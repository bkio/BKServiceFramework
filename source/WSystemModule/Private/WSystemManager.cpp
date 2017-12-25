// Copyright Pagansoft.com, All rights reserved.

#include "WSystemManager.h"

WSystemManager* WSystemManager::ManagerInstance = nullptr;

bool WSystemManager::bSystemStarted = false;
bool WSystemManager::StartSystem(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new WSystemManager();
    {
        WScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);
        ManagerInstance->Callbacks.Reset();

        _UniqueCallbackID = ManagerInstance->CurrentCallbackUniqueIx++;
        if (ManagerInstance->CurrentCallbackUniqueIx >= 32767)
        {
            ManagerInstance->CurrentCallbackUniqueIx = 1;
        }

        ManagerInstance->Callbacks.Add(WNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
    }

    if (!ManagerInstance->StartSystem_Internal())
    {
        EndSystem();
        return false;
    }

    return true;
}
bool WSystemManager::StartSystem_Internal()
{
    SystemManagerThread = new WThread(std::bind(&WSystemManager::SystemThreadsDen, this), std::bind(&WSystemManager::SystemThreadStopped, this));
    return true;
}

void WSystemManager::EndSystem()
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
void WSystemManager::EndSystem_Internal()
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

void WSystemManager::SystemThreadsDen()
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
        LastSystemInfo = new WSystemInfo(Total_CPU_Utilization, Total_Memory_Utilization);

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

        WThread::SleepThread(1000);
    }
}
uint32 WSystemManager::SystemThreadStopped()
{
    if (!bSystemStarted) return 0;
    if (SystemManagerThread) delete (SystemManagerThread);
    SystemManagerThread = new WThread(std::bind(&WSystemManager::SystemThreadsDen, this), std::bind(&WSystemManager::SystemThreadStopped, this));
    return 0;
}

void WSystemManager::AddCallback(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (!bSystemStarted || !ManagerInstance || !_Callback) return;

    WScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);

    _UniqueCallbackID = ManagerInstance->CurrentCallbackUniqueIx++;
    if (ManagerInstance->CurrentCallbackUniqueIx >= 32767)
    {
        ManagerInstance->CurrentCallbackUniqueIx = 1;
    }

    ManagerInstance->Callbacks.AddUnique(WNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
}
void WSystemManager::RemoveCallback(uint32 _UniqueCallbackID, WSystemInfoCallback _Callback)
{
    if (!bSystemStarted || !ManagerInstance || !_Callback) return;

    WScopeGuard LocalGuard(&ManagerInstance->Callbacks_Mutex);
    ManagerInstance->Callbacks.Remove(WNonComparable_ElementWrapper<uint32, WSystemInfoCallback>(_UniqueCallbackID, _Callback));
}