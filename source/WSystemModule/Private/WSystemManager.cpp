// Copyright Pagansoft.com, All rights reserved.

#include "WSystemManager.h"

UWSystemManager* UWSystemManager::ManagerInstance = nullptr;

bool UWSystemManager::bSystemStarted = false;
bool UWSystemManager::StartSystem()
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new UWSystemManager();
    if (!ManagerInstance->StartSystem_Internal())
    {
        EndSystem();
        return false;
    }

    return true;
}
bool UWSystemManager::StartSystem_Internal()
{
    SystemManagerThread = new WThread(std::bind(&UWSystemManager::SystemThreadsDen, this), std::bind(&UWSystemManager::SystemThreadStopped, this));
    return true;
}

void UWSystemManager::EndSystem()
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
void UWSystemManager::EndSystem_Internal()
{
    if (SystemManagerThread != nullptr)
    {
        if (SystemManagerThread->IsJoinable())
        {
            SystemManagerThread->Join();
        }
        delete (SystemManagerThread);
    }
}

void UWSystemManager::SystemThreadsDen()
{
    int32 Total_CPU_Utilization;
    int32 Process_CPU_Utilization;

    int64 Total_Memory_Utilization;
    int64 Process_Memory_Utilization;
    while (bSystemStarted)
    {
        Process_CPU_Utilization = CPUMonitor.GetUsage(&Total_CPU_Utilization);
        Process_Memory_Utilization = MemoryMonitor.GetUsage(&Total_Memory_Utilization);

        UWUtilities::Print(EWLogType::Log,
                           FString(L"Process CPU: ") +
                           FString::FromInt(Process_CPU_Utilization) +
                           FString(L"\t Total CPU: ") +
                           FString::FromInt(Total_CPU_Utilization) +
                           FString(L"\t Process Memory: ") +
                           FString::FromInt(Process_Memory_Utilization) +
                           FString(L"\t Total Memory: ") +
                           FString::FromInt(Total_Memory_Utilization));

        WThread::SleepThread(200);
    }
}
uint32 UWSystemManager::SystemThreadStopped()
{
    if (!bSystemStarted) return 0;
    if (SystemManagerThread) delete (SystemManagerThread);
    SystemManagerThread = new WThread(std::bind(&UWSystemManager::SystemThreadsDen, this), std::bind(&UWSystemManager::SystemThreadStopped, this));
    return 0;
}