// Copyright Pagansoft.com, All rights reserved.

#include "WMemoryMonitor.h"
#if PLATFORM_WINDOWS
    #include <windows.h>
    #include <psapi.h>
#else
    #include "sys/sysinfo.h"
#endif

int64 WMemoryMonitor::GetUsage(int64* pSystemUsage)
{
#if PLATFORM_WINDOWS
    WScopeGuard Guard(&m_lock);

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
    WScopeGuard Guard(&m_lock);

    struct sysinfo MemInfo{};
    if (sysinfo (&MemInfo) != -1)
    {
        auto PhysMemUsed = static_cast<int64>(MemInfo.totalram - MemInfo.freeram);
        PhysMemUsed *= MemInfo.mem_unit;

        auto TotalPhysMem = static_cast<int64>(MemInfo.totalram);
        TotalPhysMem *= MemInfo.mem_unit;

        double SystemPercent = ((double)PhysMemUsed) / TotalPhysMem;
        *pSystemUsage = static_cast<int64>(SystemPercent);

        FILE* StatusFile = fopen("/proc/self/status", "r");
        int32 WorkingSetSize = 0;
        ANSICHAR Line[128];

        while (fgets(Line, 128, StatusFile) != nullptr)
        {
            if (strncmp(Line, "VmRSS:", 6) == 0)
            {
                WorkingSetSize = ParseLine(Line);
                break;
            }
        }
        fclose(StatusFile);

        double UsedRamByProcess = ((double)(WorkingSetSize * 1024)) / TotalPhysMem;
        return static_cast<int64>(UsedRamByProcess);
    }

    *pSystemUsage = 0;
    return 0;
#endif
}