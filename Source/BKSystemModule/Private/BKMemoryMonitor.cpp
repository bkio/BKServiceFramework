// Copyright Burak Kara, All rights reserved.

#include "BKMemoryMonitor.h"
#if PLATFORM_WINDOWS
#else
    #include "sys/sysinfo.h"
#endif

int32 BKMemoryMonitor::GetUsage()
{
#if PLATFORM_WINDOWS
    BKScopeGuard Guard(&m_lock);

    MEMORYSTATUSEX Status{};

    Status.dwLength = sizeof (Status);

    GlobalMemoryStatusEx (&Status);
    return (int32)Status.dwMemoryLoad;
#else
    BKScopeGuard Guard(&m_lock);

    struct sysinfo MemInfo{};
    if (sysinfo (&MemInfo) != -1)
    {
        auto PhysMemUsed = static_cast<int64>(MemInfo.totalram - MemInfo.freeram);
        PhysMemUsed *= MemInfo.mem_unit;

        auto TotalPhysMem = static_cast<int64>(MemInfo.totalram);
        TotalPhysMem *= MemInfo.mem_unit;

        double SystemPercent = ((double)PhysMemUsed) / TotalPhysMem;
        return static_cast<int32>(SystemPercent * 100);
    }
    return 0;
#endif
}