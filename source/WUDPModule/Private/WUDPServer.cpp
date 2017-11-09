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
        auto Buffer = new ANSICHAR[1024];
        auto Client = new sockaddr;
#if PLATFORM_WINDOWS
        int32 ClientLen = sizeof(*Client);
#else
        socklen_t ClientLen = sizeof(*Client);
#endif

        auto RetrievedSize = static_cast<int32>(recvfrom(UDPSocket, Buffer, 1024, 0, Client, &ClientLen));
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
        PassParameters.Add(new UWUDPTaskParameter(RetrievedSize, Buffer, Client));

        WFutureAsyncTask Lambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2)
            {
                auto ServerInstance = dynamic_cast<UWUDPServer*>(TaskParameters[0]);
                auto Parameter = dynamic_cast<UWUDPTaskParameter*>(TaskParameters[1]);

                if (!ServerInstance || !ServerInstance->bSystemStarted || !ServerInstance->UDPListenCallback) return;

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

void UWUDPServer::Send(sockaddr* Client, const FWCHARWrapper& SendBuffer)
{
    if (!bSystemStarted) return;

    if (Client == nullptr) return;
    if (SendBuffer.GetSize() == 0) return;

#if PLATFORM_WINDOWS
    int32 ClientLen = sizeof(*Client);
#else
    socklen_t ClientLen = sizeof(*Client);
#endif
    int32 SentLength;
    WScopeGuard SendGuard(&SendMutex);
    {
#if PLATFORM_WINDOWS
        SentLength = static_cast<int32>(sendto(UDPSocket, SendBuffer.GetValue(), static_cast<size_t>(SendBuffer.GetSize()), 0, Client, ClientLen));
#else
        SentLength = static_cast<int32>(sendto(UDPSocket, SendBuffer.GetValue(), static_cast<size_t>(SendBuffer.GetSize()), MSG_NOSIGNAL, Client, ClientLen));
#endif
    }

#if PLATFORM_WINDOWS
    if (SentLength == SOCKET_ERROR)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#else
    if (SentLength == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPServer: Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#endif
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
        UDPHandler = new UWUDPHandler(std::bind(&UWUDPServer::Send, this, std::placeholders::_1, std::placeholders::_2));
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
    if (UDPSystemThread != nullptr)
    {
        if (UDPSystemThread->IsJoinable())
        {
            UDPSystemThread->Join();
        }
        delete (UDPSystemThread);
    }
}
bool WReliableConnectionRecord::ResetterFunction()
{
    if (ResponsibleHandler == nullptr) return true;
    if (GetBuffer() == nullptr || !GetBuffer()->IsValid()) return true;

    if (GetHandshakingStatus() == 3) return true;
    if (++FailureTrialCount >= 5)
    {
        if (!bAsSender) return true;
        FailureTrialCount = 0;
        SetHandshakingStatus(0);
    }

    UpdateLastInteraction();
    ResponsibleHandler->SendFunction(GetClient(), *GetBuffer());

    return false;
}