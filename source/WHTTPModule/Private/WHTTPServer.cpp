// Copyright Pagansoft.com, All rights reserved.

#include <WAsyncTaskManager.h>
#include <WMemory.h>
#include "WHTTPServer.h"
#include "WMath.h"

bool UWHTTPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPServer: WSAStartup() failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    HTTPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if PLATFORM_WINDOWS
    if (HTTPSocket == INVALID_SOCKET)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPServer: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (HTTPSocket == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPServer: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        return false;
    }
#endif

    int32 On = 1;
    setsockopt(HTTPSocket, SOL_SOCKET, SO_REUSEADDR, (const ANSICHAR*)&On, sizeof(int32));
    setsockopt(HTTPSocket, IPPROTO_TCP, TCP_NODELAY, (const ANSICHAR*)&On, sizeof(int32));

    FMemory::Memzero((ANSICHAR*)&HTTPServer, sizeof(HTTPServer));
    HTTPServer.sin_family = AF_INET;
    HTTPServer.sin_addr.s_addr = INADDR_ANY;
    HTTPServer.sin_port = htons(Port);
    HTTPPort = Port;

    int32 ret = bind(HTTPSocket, (struct sockaddr*)&HTTPServer, sizeof(HTTPServer));
    if (ret == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPServer: Socket binding failed with error: ") + UWUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void UWHTTPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(HTTPSocket);
    WSACleanup();
#else
    shutdown(HTTPSocket, SHUT_RDWR);
    close(HTTPSocket);
#endif
}

void UWHTTPServer::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Client = new sockaddr;
#if PLATFORM_WINDOWS
        int32 ClientLen = sizeof(*Client);
#else
        socklen_t ClientLen = sizeof(*Client);
#endif

        auto ListenResult = listen(HTTPSocket, SOMAXCONN);
#if PLATFORM_WINDOWS
        if (ListenResult == SOCKET_ERROR)
#else
        if (ListenResult == -1)
#endif
        {
            if (!bSystemStarted) return;

            EndSystem();
            StartSystem(HTTPPort, TimeoutInMs, HTTPListenCallback);
            return;
        }

        auto ClientSocket = accept(HTTPSocket, Client, &ClientLen);
#if PLATFORM_WINDOWS
        if (ClientSocket == INVALID_SOCKET)
#else
        if (ClientSocket == -1)
#endif
        {
            delete (Client);
            if (!bSystemStarted) return;
            continue;
        }

        auto TaskParameter = new FWHTTPAcceptedClient(ClientSocket, Client, TimeoutInMs);
        TArray<FWAsyncTaskParameter*> TaskParameterAsArray(TaskParameter);

        WFutureAsyncTask TaskLambda = [](TArray<FWAsyncTaskParameter*> TaskParameters)
        {
            if (!bSystemStarted || !ManagerInstance || !ManagerInstance->HTTPListenCallback) return;

            if (TaskParameters.Num() > 0)
            {
                if (auto Parameter = dynamic_cast<FWHTTPAcceptedClient*>(TaskParameters[0]))
                {
                    ManagerInstance->HTTPListenCallback(Parameter);
                }
            }
        };
        UWAsyncTaskManager::NewAsyncTask(TaskLambda, TaskParameterAsArray);
    }
}
uint32 UWHTTPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (HTTPSystemThread) delete (HTTPSystemThread);
    HTTPSystemThread = new WThread(std::bind(&UWHTTPServer::ListenSocket, this), std::bind(&UWHTTPServer::ListenerStopped, this));
    return 0;
}

UWHTTPServer* UWHTTPServer::ManagerInstance = nullptr;

bool UWHTTPServer::bSystemStarted = false;
bool UWHTTPServer::StartSystem(uint16 Port, uint32 TimeoutMs, std::function<void(FWHTTPAcceptedClient*)> Callback)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new UWHTTPServer(std::move(Callback));

    if (!ManagerInstance->StartSystem_Internal(Port, TimeoutMs))
    {
        EndSystem();
        return false;
    }

    return true;
}
bool UWHTTPServer::StartSystem_Internal(uint16 Port, uint32 TimeoutMs)
{
    TimeoutInMs = TimeoutMs == 0 ? 2500 : TimeoutMs;
    if (InitializeSocket(Port))
    {
        HTTPSystemThread = new WThread(std::bind(&UWHTTPServer::ListenSocket, this), std::bind(&UWHTTPServer::ListenerStopped, this));
        return true;
    }
    return false;
}

void UWHTTPServer::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    if (ManagerInstance != nullptr)
    {
        ManagerInstance->EndSystem_Internal();
        delete (ManagerInstance);
        ManagerInstance = nullptr;
    }
}
void UWHTTPServer::EndSystem_Internal()
{
    CloseSocket();
    HTTPListenCallback = nullptr;
    if (HTTPSystemThread != nullptr)
    {
        if (HTTPSystemThread->IsJoinable())
        {
            HTTPSystemThread->Join();
        }
        delete (HTTPSystemThread);
    }
}