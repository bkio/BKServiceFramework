// Copyright Pagansoft.com, All rights reserved.

#include "WCPUMonitor.h"

WCPUMonitor::WCPUMonitor()
{
#if PLATFORM_WINDOWS
    if( !s_hKernel )
    {
        s_hKernel = LoadLibrary( _T("Kernel32.dll") );
        if( s_hKernel )
        {
            s_pfnGetSystemTimes = (pfnGetSystemTimes)GetProcAddress( s_hKernel, "GetSystemTimes" );
            if( !s_pfnGetSystemTimes )
            {
                FreeLibrary( s_hKernel ); s_hKernel = nullptr;
            }
        }
    }
    s_delay.Mark();
#else
    FILE* StatFile = fopen("/proc/stat", "r");
    fscanf(StatFile, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow, &lastTotalSys, &lastTotalIdle);
    fclose(StatFile);

    struct tms timeSample{};
    ANSICHAR line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    FILE* CpuFile = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while (fgets(line, 128, CpuFile))
    {
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(CpuFile);
#endif
}
#if PLATFORM_WINDOWS
WCPUMonitor::~WCPUMonitor()
{
    if( !s_hKernel )
    {
        FreeLibrary( s_hKernel ); s_hKernel = nullptr;
    }
}
#endif

int32 WCPUMonitor::GetUsage()
{
#if PLATFORM_WINDOWS
    int64 sTime;
    int32 sLastCpu;
    WTKTime sLastUpTime;

    {
        WScopeGuard Guard(&m_lock);

        sTime           = s_time;
        sLastCpu        = s_lastCpu;
        sLastUpTime     = s_lastUpTime;
    }

    if( s_delay.MSec() <= 200 )
    {
        if (bFirstTime)
        {
            bFirstTime = false;
            return 0;
        }
        return sLastCpu;
    }

    int64 time;

    int64 idleTime;
    int64 kernelTime;
    int64 userTime;
    int64 kernelTimeProcess;
    int64 userTimeProcess;

    GetSystemTimeAsFileTime( (LPFILETIME)&time );

    if( sTime == 0 )
    {
        if( s_pfnGetSystemTimes )
        {
            s_pfnGetSystemTimes( (LPFILETIME)&idleTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime );
        }
        else
        {
            idleTime    = 0;
            kernelTime  = 0;
            userTime    = 0;
        }

        {
            FILETIME createTime{};
            FILETIME exitTime{};
            GetProcessTimes( GetCurrentProcess(), &createTime, &exitTime,
                             (LPFILETIME)&kernelTimeProcess,
                             (LPFILETIME)&userTimeProcess );
        }

        {
            WScopeGuard Guard(&m_lock);

            s_time              = time;

            s_idleTime          = idleTime;
            s_kernelTime        = kernelTime;
            s_userTime          = userTime;

            s_lastCpu           = 0;

            s_lastUpTime        = kernelTime + userTime;

            sLastCpu        = s_lastCpu;
            sLastUpTime     = s_lastUpTime;
        }

        s_delay.Mark();

        if (bFirstTime)
        {
            bFirstTime = false;
            return 0;
        }
        return sLastCpu;
    }

    if( s_pfnGetSystemTimes )
    {
        s_pfnGetSystemTimes( (LPFILETIME)&idleTime, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime );
    }
    else
    {
        idleTime    = 0;
        kernelTime  = 0;
        userTime    = 0;
    }

    {
        FILETIME createTime{};
        FILETIME exitTime{};
        GetProcessTimes( GetCurrentProcess(), &createTime, &exitTime,
                         (LPFILETIME)&kernelTimeProcess,
                         (LPFILETIME)&userTimeProcess );
    }

    int32 cpu;
    {
        WScopeGuard Guard(&m_lock);

        int64 usr = userTime   - s_userTime;
        int64 ker = kernelTime - s_kernelTime;
        int64 idl = idleTime   - s_idleTime;

        int64 sys = (usr + ker);

        if( sys == 0 )
            cpu = 0;
        else
            cpu = int32( (sys - idl) *100 / sys );

        s_time              = time;

        s_idleTime          = idleTime;
        s_kernelTime        = kernelTime;
        s_userTime          = userTime;

        s_cpu[(s_index++) %5] = cpu;
        s_count ++;
        if( s_count > 5 )
            s_count = 5;

        int32 i;
        cpu = 0;
        for( i = 0; i < s_count; i++ )
            cpu += s_cpu[i];

        cpu         /= s_count;

        s_lastCpu        = cpu;

        s_lastUpTime     = kernelTime + userTime;

        sLastCpu        = s_lastCpu;
        sLastUpTime     = s_lastUpTime;
    }

    s_delay.Mark();

    if (bFirstTime)
    {
        bFirstTime = false;
        return 0;
    }
    return sLastCpu;
#else
    WScopeGuard Guard(&m_lock);

    double SystemPercent;
    FILE* StatFile;
    uint64 totalUser, totalUserLow, totalSys, totalIdle, total;

    StatFile = fopen("/proc/stat", "r");
    fscanf(StatFile, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow, &totalSys, &totalIdle);
    fclose(StatFile);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow || totalSys < lastTotalSys || totalIdle < lastTotalIdle)
    {
        //Overflow detection. Just skip this value.
        SystemPercent = 0.0f;
    }
    else
    {
        total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) + (totalSys - lastTotalSys);
        SystemPercent = total;
        total += (totalIdle - lastTotalIdle);
        SystemPercent /= total;
        SystemPercent *= 100;
    }

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return static_cast<int32>(SystemPercent);
#endif
}

#if PLATFORM_WINDOWS
inline WTKTime::WTKTime()
{
    m_time = 0;
}

inline WTKTime::WTKTime( LPCTSTR )
{
    FILETIME ft{};
    ::GetSystemTimeAsFileTime( &ft );
    (*(LARGE_INTEGER*)&m_time).HighPart = ft.dwHighDateTime;
    (*(LARGE_INTEGER*)&m_time).LowPart  = ft.dwLowDateTime;
}

inline WTKTime::WTKTime( const WTKTime &time )
{
    m_time = time.m_time;
}

inline WTKTime::WTKTime( const FILETIME &ft )
{
    (*(LARGE_INTEGER*)&m_time).HighPart = ft.dwHighDateTime;
    (*(LARGE_INTEGER*)&m_time).LowPart  = ft.dwLowDateTime;
}

inline WTKTime::WTKTime( const SYSTEMTIME &sysTime )
{
    FILETIME ft{};
    if ( ! SystemTimeToFileTime( &sysTime, &ft ) )
    {
        m_time = 0;
    }
    else
    {
        (*(LARGE_INTEGER*)&m_time).HighPart = ft.dwHighDateTime;
        (*(LARGE_INTEGER*)&m_time).LowPart  = ft.dwLowDateTime;
    }
}

inline WTKTime::WTKTime( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds )
{
    Set( days, hours, minutes, seconds, milliseconds );
}

inline WTKTime::WTKTime( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second )
{
    Set( year, month, day, hour, minute, second );
}

inline WTKTime::WTKTime( int64 time )
{
    m_time = time;
}

inline WTKTime& WTKTime::operator =( int64 time )
{
    m_time = time;
    return *this;
}

inline WTKTime& WTKTime::operator +=( int64 time )
{
    m_time = m_time + time;
    return *this;
}

inline WTKTime& WTKTime::operator -=( int64 time )
{
    m_time = m_time - time;
    return *this;
}

inline WTKTime& WTKTime::operator /=( int64 time )
{
    m_time = m_time / time;
    return *this;
}

inline WTKTime& WTKTime::operator *=( int64 time )
{
    m_time = m_time * time;
    return *this;
}

inline WTKTime::operator FILETIME()
{
    int64 time = (m_time < 0) ? -m_time : m_time;
    FILETIME ft{};
    ft.dwHighDateTime = static_cast<DWORD>((*(LARGE_INTEGER*)&time).HighPart);
    ft.dwLowDateTime  = (*(LARGE_INTEGER*)&time).LowPart;
    return ft;
}

inline WTKTime::operator int64()
{
    return m_time;
}

inline WTKTime::operator SYSTEMTIME( )
{
    int64 time = (m_time < 0) ? -m_time : m_time;
    SYSTEMTIME sysTime{};
    FILETIME ft{};
    ft.dwHighDateTime = static_cast<DWORD>((*(LARGE_INTEGER*)&time).HighPart);
    ft.dwLowDateTime  = (*(LARGE_INTEGER*)&time).LowPart;
    if ( ! FileTimeToSystemTime( &ft, &sysTime))
        ZeroMemory (&sysTime, sizeof(sysTime));

    return sysTime;
}

inline WTKTime& WTKTime::Set( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds )
{
    m_time = UInt32x32To64( days, 24 *60 *60 ) *10000000 +
             UInt32x32To64( hours, 60 *60 ) *10000000 +
             UInt32x32To64( minutes, 60 ) *10000000 +
             UInt32x32To64( seconds, 10000000 ) +
             UInt32x32To64( milliseconds, 10000 );
    return *this;
}

inline WTKTime& WTKTime::Set( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second )
{
    SYSTEMTIME sys{};
    memset( &sys, 0, sizeof(sys) );
    sys.wYear   = year;
    sys.wMonth  = month;
    sys.wDay    = day;
    sys.wHour   = hour;
    sys.wMinute = minute;
    sys.wSecond = second;

    FILETIME ft{};
    if ( ! SystemTimeToFileTime( &sys, &ft ) )
    {
        m_time = 0;
    }
    else
    {
        (*(LARGE_INTEGER*)&m_time).HighPart = ft.dwHighDateTime;
        (*(LARGE_INTEGER*)&m_time).LowPart  = ft.dwLowDateTime;
    }

    return *this;
}

int64  WCPUMonitor::s_time;
TKDelay WCPUMonitor::s_delay;

int32 WCPUMonitor::s_count = 0;
int32 WCPUMonitor::s_index = 0;

int64 WCPUMonitor::s_idleTime;
int64 WCPUMonitor::s_kernelTime;
int64 WCPUMonitor::s_userTime;
int32 WCPUMonitor::s_lastCpu = 0;
int32 WCPUMonitor::s_cpu[];

int64 WCPUMonitor::s_lastUpTime = 0;

HINSTANCE WCPUMonitor::s_hKernel = nullptr;

pfnGetSystemTimes WCPUMonitor::s_pfnGetSystemTimes = nullptr;
#endif