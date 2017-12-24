// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSystemManager
#define Pragma_Once_WSystemManager

#include "WEngine.h"
#include "WThread.h"
#include "../Private/WCPUMonitor.h"
#include "../Private/WMemoryMonitor.h"

class WSystemInfo
{
private:
    int32 Total_CPU_Utilization;
    int64 Total_Memory_Utilization;

public:
    WSystemInfo(int32 _Total_CPU_Utilization, int64 _Total_Memory_Utilization)
    {
        Total_CPU_Utilization = _Total_CPU_Utilization;
        Total_Memory_Utilization = _Total_Memory_Utilization;
    }

    int32 GetTotalCPUUtilization() { return Total_CPU_Utilization; }
    int64 GetTotalMemoryUtilization() { return Total_Memory_Utilization; }
};

typedef std::function<void(class WSystemInfo*)> WSystemInfoCallback;

class WSystemManager
{

public:
    static bool StartSystem(WSystemInfoCallback _Callback);
    static void EndSystem();

private:
    static bool bSystemStarted;

    bool StartSystem_Internal();
    void EndSystem_Internal();

    WThread* SystemManagerThread{};

    static WSystemManager* ManagerInstance;
    WSystemManager() = default;

    void SystemThreadsDen();
    uint32 SystemThreadStopped();

    WCPUMonitor CPUMonitor;
    WMemoryMonitor MemoryMonitor;

    WSystemInfo* LastSystemInfo = nullptr;

    std::function<void(WSystemInfo* CurrentSystemInfo)> Callback;
};

#endif //Pragma_Once_WSystemManager