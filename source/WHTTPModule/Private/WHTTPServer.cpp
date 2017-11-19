// Copyright Pagansoft.com, All rights reserved.

#include <WAsyncTaskManager.h>
#include <WMemory.h>
#include "WHTTPServer.h"
#include "WMath.h"

bool WHTTPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        WUtilities::Print(EWLogType::Error, FString("WHTTPServer: WSAStartup() failed with error: ") + WUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    HTTPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if PLATFORM_WINDOWS
    if (HTTPSocket == INVALID_SOCKET)
    {
        WUtilities::Print(EWLogType::Error, FString("WHTTPServer: Socket initialization failed with error: ") + WUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (HTTPSocket == -1)
    {
        WUtilities::Print(EWLogType::Error, FString("WHTTPServer: Socket initialization failed with error: ") + WUtilities::WGetSafeErrorMessage());
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
        WUtilities::Print(EWLogType::Error, FString("WHTTPServer: Socket binding failed with error: ") + WUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void WHTTPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(HTTPSocket);
    WSACleanup();
#else
    shutdown(HTTPSocket, SHUT_RDWR);
    close(HTTPSocket);
#endif
}

void WHTTPServer::ListenSocket()
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
            StartSystem(HTTPPort, TimeoutInMs);
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

        TArray<WAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new WHTTPAcceptedClient(ClientSocket, Client, TimeoutInMs));

        WFutureAsyncTask TaskLambda = [](TArray<WAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ServerInstance = reinterpret_cast<WHTTPServer*>(TaskParameters[0]);
                auto Parameter = reinterpret_cast<WHTTPAcceptedClient*>(TaskParameters[1]);
                if (!ServerInstance || !ServerInstance->bSystemStarted || !ServerInstance->HTTPListenCallback)
                {
                    if (Parameter)
                    {
                        delete (Parameter);
                    }
                    return;
                }

                if (Parameter)
                {
                    ServerInstance->HTTPListenCallback(Parameter);
                    delete (Parameter);
                }
            }
        };
        WAsyncTaskManager::NewAsyncTask(TaskLambda, PassParameters, true);
    }
}
uint32 WHTTPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (HTTPSystemThread) delete (HTTPSystemThread);
    HTTPSystemThread = new WThread(std::bind(&WHTTPServer::ListenSocket, this), std::bind(&WHTTPServer::ListenerStopped, this));
    return 0;
}

bool WHTTPServer::StartSystem(uint16 Port, uint32 TimeoutMs)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    TimeoutInMs = TimeoutMs == 0 ? 2500 : TimeoutMs;
    if (InitializeSocket(Port))
    {
        HTTPSystemThread = new WThread(std::bind(&WHTTPServer::ListenSocket, this), std::bind(&WHTTPServer::ListenerStopped, this));
        return true;
    }
    return false;
}

void WHTTPServer::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    CloseSocket();
    HTTPListenCallback = nullptr;
    if (HTTPSystemThread)
    {
        if (HTTPSystemThread->IsJoinable())
        {
            HTTPSystemThread->Join();
        }
        delete (HTTPSystemThread);
    }
}