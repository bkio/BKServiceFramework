// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMemoryMonitor
#define Pragma_Once_WMemoryMonitor

#include "WEngine.h"
#include "WMutex.h"

class WMemoryMonitor
{

private:
    WMutex m_lock;

#if PLATFORM_WINDOWS
#else
    int32 ParseLine(ANSICHAR* line)
    {
        auto i = static_cast<int32>(strlen(line));
        const ANSICHAR* p = line;
        while (*p <'0' || *p > '9') p++;
        line[i - 3] = '\0';
        i = atoi(p);
        return i;
    }
#endif

public:
    int64 GetUsage(int64* pSystemUsage);
};

#endif //Pragma_Once_WMemoryMonitor