// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WSystemManager
#define Pragma_Once_WSystemManager

#include "WEngine.h"
#include "WThread.h"
#include "../Private/WCPUMonitor.h"
#include "../Private/WMemoryMonitor.h"

class WSystemManager
{

public:
    static bool StartSystem();
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
};

#endif //Pragma_Once_WSystemManager