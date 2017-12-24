// Copyright Pagansoft.com, All rights reserved.

#include "WSystemManager.h"

WSystemManager* WSystemManager::ManagerInstance = nullptr;

bool WSystemManager::bSystemStarted = false;
bool WSystemManager::StartSystem()
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new WSystemManager();
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
    int32 Process_CPU_Utilization;

    int64 Total_Memory_Utilization;
    int64 Process_Memory_Utilization;
    while (bSystemStarted)
    {
        Process_CPU_Utilization = CPUMonitor.GetUsage(&Total_CPU_Utilization);
        Process_Memory_Utilization = MemoryMonitor.GetUsage(&Total_Memory_Utilization);

        WUtilities::Print(EWLogType::Log,
                           FString("Process CPU: ") +
                           FString::FromInt(Process_CPU_Utilization) +
                           FString("\t Total CPU: ") +
                           FString::FromInt(Total_CPU_Utilization) +
                           FString("\t Process Memory: ") +
                           FString::FromInt(Process_Memory_Utilization) +
                           FString("\t Total Memory: ") +
                           FString::FromInt(Total_Memory_Utilization));

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