// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMemoryMonitor
#define Pragma_Once_WMemoryMonitor

#include "WEngine.h"
#include "WMutex.h"

class WMemoryMonitor
{

private:
    WMutex m_lock;

public:
    int32 GetUsage();
};

#endif //Pragma_Once_WMemoryMonitor