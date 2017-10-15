// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WUDPManager.h"

void Start()
{
    WThread::StartSystem();
    UWAsyncTaskManager::StartSystem(4);
    UWUDPManager::StartSystem(45000);
}
void Stop()
{
    UWUDPManager::EndSystem();
    UWAsyncTaskManager::EndSystem();
    WThread::EndSystem();
}
void Restart()
{
    Stop();
    Start();
}
void Quit()
{
    Stop();
}
void ClearUDPClientRecords()
{
    UWUDPManager::ClearClientRecords();
}

int main()
{
    setlocale(LC_CTYPE, "");

    UWUtilities::Print(EWLogType::Log, L"Application has started.");

    UWUtilities::Print(EWLogType::Log, L"Commands:");
    UWUtilities::Print(EWLogType::Log, L"__________________");
    UWUtilities::Print(EWLogType::Log, L"0: Exit");
    UWUtilities::Print(EWLogType::Log, L"1: Start");
    UWUtilities::Print(EWLogType::Log, L"2: Stop");
    UWUtilities::Print(EWLogType::Log, L"3: Restart");
    UWUtilities::Print(EWLogType::Log, L"4: Clear UDP Client Records");
    UWUtilities::Print(EWLogType::Log, L"__________________");

    UWUtilities::Print(EWLogType::Log, L"Auto-start...");
    Start();

    int32 Signal;
    while (true)
    {
        std::cin >> Signal;
        if (Signal == 0)
        {
            Quit();
            return 0;
        }
        else if (Signal == 1)
        {
            Start();
            UWUtilities::Print(EWLogType::Log, L"System started.");
        }
        else if (Signal == 2)
        {
            Stop();
            UWUtilities::Print(EWLogType::Log, L"System stopped.");
        }
        else if (Signal == 3)
        {
            Restart();
            UWUtilities::Print(EWLogType::Log, L"System restarted.");
        }
        else if (Signal == 4)
        {
            ClearUDPClientRecords();
        }
    }
}