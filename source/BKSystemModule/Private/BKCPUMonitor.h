// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKCPUMonitor
#define Pragma_Once_BKCPUMonitor

#include "BKEngine.h"
#include "BKMutex.h"
#if PLATFORM_WINDOWS
    #include "windows.h"
    #include "tchar.h"
#else
    #include "sys/times.h"
    #include "sys/vtimes.h"
#endif

#if PLATFORM_WINDOWS

typedef BOOL ( __stdcall * pfnGetSystemTimes)( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );

struct BKTKTimeStruct
{
    int32     m_hour;
};

class BKTKTime
{
public:
    BKTKTime();
    explicit BKTKTime( LPCTSTR szGetSystemTimeDummy );  // szGetSystemTimeDummy is not used it's just to Get the system time in the constructor
    BKTKTime( const BKTKTime& time );
    explicit BKTKTime( const FILETIME& fileTime );
    explicit BKTKTime( const SYSTEMTIME& sysTime );
    BKTKTime( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD millisseconds );
    BKTKTime( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second );
    explicit BKTKTime( int64 time );

    ~BKTKTime() = default;

    BKTKTime& operator =( int64 time );
    BKTKTime& operator +=( int64 time );
    BKTKTime& operator -=( int64 time );
    BKTKTime& operator /=( int64 time );
    BKTKTime& operator *=( int64 time );

    explicit operator BKTKTimeStruct()
    {
        int64 time = (m_time < 0) ? -m_time : m_time;

        time /= 10000;

        BKTKTimeStruct tkTime{};
        tkTime.m_hour           = int32(time / 3600000);

        return tkTime;
    }

    explicit operator FILETIME();
    explicit operator SYSTEMTIME();
    explicit operator int64();

    BKTKTime& Set( DWORD days, DWORD hours, DWORD minutes, DWORD seconds, DWORD milliseconds );
    BKTKTime& Set( WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second );

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

class BKCPUMonitor
{

public:
    BKCPUMonitor();
#if PLATFORM_WINDOWS
    ~BKCPUMonitor();
#else
    ~WCPUMonitor() = default;
#endif

    int32 GetUsage();

private:

    BKMutex m_lock;

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

#endif //Pragma_Once_BKCPUMonitor