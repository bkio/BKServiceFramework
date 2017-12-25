// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSystemManager
#define Pragma_Once_WSystemManager

#include "WEngine.h"
#include "WThread.h"
#include "../Private/WCPUMonitor.h"
#include "../Private/WMemoryMonitor.h"
#include "WJson.h"
#include "WElementWrapper.h"

class WSystemInfo
{
private:
    int32 Total_CPU_Utilization;
    int32 Total_Memory_Utilization;

public:
    WSystemInfo(int32 _Total_CPU_Utilization, int32 _Total_Memory_Utilization)
    {
        Total_CPU_Utilization = _Total_CPU_Utilization;
        Total_Memory_Utilization = _Total_Memory_Utilization;
    }

    WJson::Node ToJsonObject()
    {
        WJson::Node ResultObject = WJson::Node(WJson::Node::T_OBJECT);
        ResultObject.Add(FString("Total_CPU_Utilization"), WJson::Node(Total_CPU_Utilization));
        ResultObject.Add(FString("Total_Memory_Utilization"), WJson::Node(Total_Memory_Utilization));
        return ResultObject;
    }
    FString ToJsonString()
    {
        WJson::Writer Writer;
        return Writer.WriteString(ToJsonObject());
    }

    int32 GetTotalCPUUtilization() { return Total_CPU_Utilization; }
    int32 GetTotalMemoryUtilization() { return Total_Memory_Utilization; }
};

typedef std::function<void(class WSystemInfo*)> WSystemInfoCallback;

class WSystemManager
{

public:
    static bool StartSystem(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback);
    static void EndSystem();

    static void AddCallback(uint32& _UniqueCallbackID, WSystemInfoCallback _Callback);
    static void RemoveCallback(uint32 _UniqueCallbackID, WSystemInfoCallback _Callback);

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

    TArray<WNonComparable_ElementWrapper<uint32, WSystemInfoCallback>> Callbacks;
    WMutex Callbacks_Mutex;
    uint32 CurrentCallbackUniqueIx = 1;
};

#endif //Pragma_Once_WSystemManager