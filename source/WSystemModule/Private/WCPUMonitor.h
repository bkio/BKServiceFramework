// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WCPUMonitor
#define Pragma_Once_WCPUMonitor

#include "WEngine.h"
#include "WMutex.h"
#if PLATFORM_WINDOWS
    #include "windows.h"
    #include "tchar.h"
#else
    #include "sys/times.h"
    #include "sys/vtimes.h"
#endif

#if PLATFORM_WINDOWS

typedef BOOL ( __stdcall * pfnGetSystemTimes)( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );

struct WTKTimeStruct
{
    int32     m_hour;
};

class WTKTime
{
public:
    WTKTime();
    explicit WTKTime( LPCTSTR szGetSystemTimeDummy );  // szGetSystemTimeDummy is not used it's just to Get the system time in the constructor
    WTKTime( const WTKTime& time );
    explicit WTKTime( const FILETIME& fileTime );
    explicit WTKTime( const SYSTEMTIME& sysTime );
    WTKTime( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD millisseconds );
    WTKTime( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second );
    explicit WTKTime( int64 time );

    ~WTKTime() = default;

    WTKTime& operator =( int64 time );
    WTKTime& operator +=( int64 time );
    WTKTime& operator -=( int64 time );
    WTKTime& operator /=( int64 time );
    WTKTime& operator *=( int64 time );

    explicit operator WTKTimeStruct()
    {
        int64 time = (m_time < 0) ? -m_time : m_time;

        time /= 10000;

        WTKTimeStruct tkTime{};
        tkTime.m_hour           = int32(time / 3600000);

        return tkTime;
    }

    explicit operator FILETIME();
    explicit operator SYSTEMTIME();
    explicit operator int64();

    WTKTime& Set( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds );
    WTKTime& Set( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second );

private:
    int64 m_time;
};

class TKDelay
{
public:
    void    Mark(){ m_mark = ::GetTickCount(); };
    int32     MSec(){ return static_cast<int32>((::GetTickCount() - m_mark) & 0x7FFFFFFF); };
private:
    DWORD   m_mark;
};

#endif

class WCPUMonitor
{

public:
    WCPUMonitor();
#if PLATFORM_WINDOWS
    ~WCPUMonitor();
#else
    ~WCPUMonitor() = default;
#endif

    int32 GetUsage();

private:

    WMutex m_lock;

    bool bFirstTime = true;

#if PLATFORM_WINDOWS
    static TKDelay s_delay;

    static int64 s_time;

    static int64 s_idleTime;
    static int64 s_kernelTime;
    static int64 s_userTime;
    static int32 s_lastCpu;
    static int32 s_cpu[5];

    static int32 s_count;
    static int32 s_index;

    static int64 s_lastUpTime;

    static HINSTANCE s_hKernel;
    static pfnGetSystemTimes s_pfnGetSystemTimes;
#else
    uint64 lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

    int32 numProcessors;
#endif
};

#endif //Pragma_Once_WCPUMonitor