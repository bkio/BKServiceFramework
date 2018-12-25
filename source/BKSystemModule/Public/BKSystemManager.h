// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKSystemManager
#define Pragma_Once_BKSystemManager

#include "BKEngine.h"
#include "BKThread.h"
#include "../Private/BKCPUMonitor.h"
#include "../Private/BKMemoryMonitor.h"
#include "BKJson.h"
#include "BKElementWrapper.h"

class BKSystemInfo
{
private:
    int32 Total_CPU_Utilization;
    int32 Total_Memory_Utilization;

public:
    BKSystemInfo(int32 _Total_CPU_Utilization, int32 _Total_Memory_Utilization)
    {
        Total_CPU_Utilization = _Total_CPU_Utilization;
        Total_Memory_Utilization = _Total_Memory_Utilization;
    }

    BKJson::Node ToJsonObject()
    {
        BKJson::Node ResultObject = BKJson::Node(BKJson::Node::T_OBJECT);
        ResultObject.Add(FString("Total_CPU_Utilization"), BKJson::Node(Total_CPU_Utilization));
        ResultObject.Add(FString("Total_Memory_Utilization"), BKJson::Node(Total_Memory_Utilization));
        return ResultObject;
    }
    FString ToJsonString()
    {
        BKJson::Writer Writer;
        return Writer.WriteString(ToJsonObject());
    }

    int32 GetTotalCPUUtilization() { return Total_CPU_Utilization; }
    int32 GetTotalMemoryUtilization() { return Total_Memory_Utilization; }
};

typedef std::function<void(class BKSystemInfo*)> WSystemInfoCallback;

class BKSystemManager
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

    BKThread* SystemManagerThread{};

    static BKSystemManager* ManagerInstance;
    BKSystemManager() = default;

    void SystemThreadsDen();
    uint32 SystemThreadStopped();

    BKCPUMonitor CPUMonitor;
    BKMemoryMonitor MemoryMonitor;

    BKSystemInfo* LastSystemInfo = nullptr;

    TArray<BKNonComparable_ElementWrapper<uint32, WSystemInfoCallback>> Callbacks;
    BKMutex Callbacks_Mutex;
    uint32 CurrentCallbackUniqueIx = 1;
};

#endif //Pragma_Once_BKSystemManager