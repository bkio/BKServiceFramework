// Copyright Pagansoft.com, All rights reserved.

#include "WAsyncTaskManager.h"
#include "WUDPServer.h"
#include "WUDPHandler.h"

bool WUDPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        WUtilities::Print(EWLogType::Error, FString("WUDPServer: WSAStartup() failed with error: ") + WUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#if PLATFORM_WINDOWS
    if (UDPSocket == INVALID_SOCKET)
    {
        WUtilities::Print(EWLogType::Error, FString("WUDPServer: Socket initialization failed with error: ") + WUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (UDPSocket == -1)
    {
        WUtilities::Print(EWLogType::Error, FString("WUDPServer: Socket initialization failed with error: ") + WUtilities::WGetSafeErrorMessage());
        return false;
    }
#endif

    int32 optval = 1;
    setsockopt(UDPSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int32));
#if PLATFORM_WINDOWS
    setsockopt(UDPSocket, SOL_SOCKET, UDP_NOCHECKSUM, (const char*)&optval, sizeof(int32));
#else
    setsockopt(UDPSocket, SOL_SOCKET, SO_NO_CHECK, (const char*)&optval, sizeof(int32));
#endif

    FMemory::Memzero((ANSICHAR*)&UDPServer, sizeof(UDPServer));
    UDPServer.sin_family = AF_INET;
    UDPServer.sin_addr.s_addr = INADDR_ANY;
    UDPServer.sin_port = htons(Port);

    int32 ret = bind(UDPSocket, (struct sockaddr*)&UDPServer, sizeof(UDPServer));
    if (ret == -1)
    {
        WUtilities::Print(EWLogType::Error, FString("WUDPServer: Socket binding failed with error: ") + WUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void WUDPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
    close(UDPSocket);
#endif
}

void WUDPServer::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Buffer = new ANSICHAR[UDP_BUFFER_SIZE];
        auto Client = new sockaddr;
#if PLATFORM_WINDOWS
        int32 ClientLen = sizeof(*Client);
#else
        socklen_t ClientLen = sizeof(*Client);
#endif

        auto RetrievedSize = static_cast<int32>(recvfrom(UDPSocket, Buffer, UDP_BUFFER_SIZE, 0, Client, &ClientLen));
        if (RetrievedSize < 0 || !bSystemStarted)
        {
            delete[] Buffer;
            delete (Client);
            if (!bSystemStarted) return;
            continue;
        }
        if (RetrievedSize == 0) continue;

        TArray<WAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new WUDPTaskParameter(RetrievedSize, Buffer, Client, true));

        WFutureAsyncTask Lambda = [](TArray<WAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ServerInstance = reinterpret_cast<WUDPServer*>(TaskParameters[0]);
                auto Parameter = reinterpret_cast<WUDPTaskParameter*>(TaskParameters[1]);

                if (!ServerInstance || !ServerInstance->bSystemStarted || !ServerInstance->UDPListenCallback)
                {
                    if (Parameter)
                    {
                        delete (Parameter);
                    }
                    return;
                }

                if (Parameter)
                {
                    ServerInstance->UDPListenCallback(ServerInstance->UDPHandler, Parameter);
                    delete (Parameter);
                }
            }
        };
        WAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 WUDPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (UDPSystemThread) delete (UDPSystemThread);
    UDPSystemThread = new WThread(std::bind(&WUDPServer::ListenSocket, this), std::bind(&WUDPServer::ListenerStopped, this));
    return 0;
}

bool WUDPServer::StartSystem(uint16 Port)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    if (InitializeSocket(Port))
    {
        UDPSystemThread = new WThread(std::bind(&WUDPServer::ListenSocket, this), std::bind(&WUDPServer::ListenerStopped, this));
        if (UDPHandler)
        {
            delete (UDPHandler);
        }
        UDPHandler = new WUDPHandler(UDPSocket);
        UDPHandler->StartSystem();
        return true;
    }
    return false;
}

void WUDPServer::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    if (UDPHandler)
    {
        UDPHandler->EndSystem();
        delete (UDPHandler);
    }
    CloseSocket();
    UDPListenCallback = nullptr;
    if (UDPSystemThread)
    {
        if (UDPSystemThread->IsJoinable())
        {
            UDPSystemThread->Join();
        }
        delete (UDPSystemThread);
    }
}