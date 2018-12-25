// Copyright Burak Kara, All rights reserved.

#include "BKAsyncTaskManager.h"
#include "BKUDPServer.h"
#include "BKUDPHandler.h"

bool BKUDPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        BKUtilities::Print(EBKLogType::Error, FString("BKUDPServer: WSAStartup() failed with error: ") + BKUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#if PLATFORM_WINDOWS
    if (UDPSocket == INVALID_SOCKET)
    {
        BKUtilities::Print(EBKLogType::Error, FString("BKUDPServer: Socket initialization failed with error: ") + BKUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (UDPSocket == -1)
    {
        BKUtilities::Print(EBKLogType::Error, FString("WUDPServer: Socket initialization failed with error: ") + BKUtilities::WGetSafeErrorMessage());
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
        BKUtilities::Print(EBKLogType::Error, FString("BKUDPServer: Socket binding failed with error: ") + BKUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void BKUDPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
    close(UDPSocket);
#endif
}

void BKUDPServer::ListenSocket()
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

        TArray<BKAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new WUDPTaskParameter(RetrievedSize, Buffer, Client, true));

        BKFutureAsyncTask Lambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ServerInstance = reinterpret_cast<BKUDPServer*>(TaskParameters[0]);
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
        BKAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 BKUDPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (UDPSystemThread) delete (UDPSystemThread);
    UDPSystemThread = new BKThread(std::bind(&BKUDPServer::ListenSocket, this), std::bind(&BKUDPServer::ListenerStopped, this));
    return 0;
}

bool BKUDPServer::StartSystem(uint16 Port)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    if (InitializeSocket(Port))
    {
        UDPSystemThread = new BKThread(std::bind(&BKUDPServer::ListenSocket, this), std::bind(&BKUDPServer::ListenerStopped, this));
        if (UDPHandler)
        {
            delete (UDPHandler);
        }
        UDPHandler = new BKUDPHandler(UDPSocket);
        UDPHandler->StartSystem();
        return true;
    }
    return false;
}

void BKUDPServer::EndSystem()
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