// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMemoryMonitor
#define Pragma_Once_WMemoryMonitor

#include "WEngine.h"

class WMemoryMonitor
{

public:
    int64 GetUsage(int64* pSystemUsage);
};

#endif //Pragma_Once_WMemoryMonitor