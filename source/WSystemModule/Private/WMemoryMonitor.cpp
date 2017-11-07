// Copyright Pagansoft.com, All rights reserved.

#include "WMemoryMonitor.h"
#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <psapi.h>
#else
#endif

int64 WMemoryMonitor::GetUsage(int64* pSystemUsage)
{
#if PLATFORM_WINDOWS
    MEMORYSTATUSEX Status{};

    Status.dwLength = sizeof (Status);

    GlobalMemoryStatusEx (&Status);
    *pSystemUsage = Status.dwMemoryLoad;

    PROCESS_MEMORY_COUNTERS MemoryCounter{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &MemoryCounter, sizeof(MemoryCounter)))
    {
        double UsedRamByProcess = ((double)(MemoryCounter.WorkingSetSize)) / Status.ullTotalPhys;
        return static_cast<int64>(UsedRamByProcess);
    }
    return *pSystemUsage;
#else
    return 0;
#endif
}