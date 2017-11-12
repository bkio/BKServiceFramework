// Copyright Pagansoft.com, All rights reserved.

#include "WAsyncTaskManager.h"
#include "WUDPServer.h"

bool UWUDPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: WSAStartup() failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#if PLATFORM_WINDOWS
    if (UDPSocket == INVALID_SOCKET)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (UDPSocket == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
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
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: Socket binding failed with error: ") + UWUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void UWUDPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
    close(UDPSocket);
#endif
}

void UWUDPServer::ListenSocket()
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

        TArray<UWAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new UWUDPTaskParameter(RetrievedSize, Buffer, Client, true));

        WFutureAsyncTask Lambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ServerInstance = reinterpret_cast<UWUDPServer*>(TaskParameters[0]);
                auto Parameter = reinterpret_cast<UWUDPTaskParameter*>(TaskParameters[1]);

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
        UWAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 UWUDPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (UDPSystemThread) delete (UDPSystemThread);
    UDPSystemThread = new WThread(std::bind(&UWUDPServer::ListenSocket, this), std::bind(&UWUDPServer::ListenerStopped, this));
    return 0;
}

bool UWUDPServer::StartSystem(uint16 Port)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    if (InitializeSocket(Port))
    {
        UDPSystemThread = new WThread(std::bind(&UWUDPServer::ListenSocket, this), std::bind(&UWUDPServer::ListenerStopped, this));
        if (UDPHandler)
        {
            delete (UDPHandler);
        }
        UDPHandler = new UWUDPHandler(UDPSocket);
        UDPHandler->StartSystem();
        return true;
    }
    return false;
}

void UWUDPServer::EndSystem()
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