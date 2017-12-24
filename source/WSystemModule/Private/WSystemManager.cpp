// Copyright Pagansoft.com, All rights reserved.

#include "WSystemManager.h"

WSystemManager* WSystemManager::ManagerInstance = nullptr;

bool WSystemManager::bSystemStarted = false;
bool WSystemManager::StartSystem(WSystemInfoCallback _Callback)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new WSystemManager();
    ManagerInstance->Callback = std::move(_Callback);

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
    int64 Total_Memory_Utilization;

    while (bSystemStarted)
    {
        Total_CPU_Utilization = CPUMonitor.GetUsage();
        Total_Memory_Utilization = MemoryMonitor.GetUsage();

        if (LastSystemInfo)
        {
            delete (LastSystemInfo);
        }
        LastSystemInfo = new WSystemInfo(Total_CPU_Utilization, Total_Memory_Utilization);

        if (Callback)
        {
            Callback(LastSystemInfo);
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