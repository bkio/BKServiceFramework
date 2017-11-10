// Copyright Pagansoft.com, All rights reserved.

#include "WUDPClient.h"
#include "WUDPHandler.h"
#include "WUDPHelper.h"
#include "WAsyncTaskManager.h"

UWUDPClient* UWUDPClient::NewUDPClient(std::string _ServerAddress, uint16 _ServerPort, WUDPClient_DataReceived& _DataReceivedCallback)
{
    auto NewClient = new UWUDPClient();

    NewClient->UDPListenCallback = _DataReceivedCallback;

    if (!NewClient->StartUDPClient(_ServerAddress, _ServerPort))
    {
        delete (NewClient);
        return nullptr;
    }

    return NewClient;
}

UWUDPHandler* UWUDPClient::GetUDPHandler()
{
    return UDPHandler;
}

bool UWUDPClient::StartUDPClient(std::string& _ServerAddress, uint16 _ServerPort)
{
    if (bClientStarted) return false;
    bClientStarted = true;

    ServerAddress = _ServerAddress;
    ServerPort = _ServerPort;

    if (InitializeClient())
    {
        UDPClientThread = new WThread(std::bind(&UWUDPClient::ListenServer, this), std::bind(&UWUDPClient::ServerListenerStopped, this));
        if (UDPHandler)
        {
            delete (UDPHandler);
        }
        UDPHandler = new UWUDPHandler(UDPSocket);
        UDPHandler->StartSystem();
    }
}

void UWUDPClient::EndUDPClient()
{
    if (!bClientStarted) return;
    bClientStarted = false;

    if (UDPHandler)
    {
        UDPHandler->EndSystem();
        delete (UDPHandler);
    }
    CloseSocket();
    if (SocketAddress != nullptr)
    {
        FMemory::Free(SocketAddress);
    }
    UDPListenCallback = nullptr;
    if (UDPClientThread != nullptr)
    {
        if (UDPClientThread->IsJoinable())
        {
            UDPClientThread->Join();
        }
        delete (UDPClientThread);
    }
}

bool UWUDPClient::InitializeClient()
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    DWORD AddrInfo;

    int32 RetVal = WSAStartup(0x202, &WSAData);
    if (RetVal != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
        return false;
    }
#else
    int32 AddrInfo;
#endif

    FString PortString = FString::FromInt(ServerPort);

    struct addrinfo Hint{};
    FMemory::Memset(&Hint, 0, sizeof(struct addrinfo));
    Hint.ai_family = AF_INET;
    Hint.ai_socktype = SOCK_DGRAM;
    Hint.ai_protocol = IPPROTO_UDP;

    struct addrinfo* Result;

#if PLATFORM_WINDOWS
    AddrInfo = static_cast<DWORD>(getaddrinfo(ServerAddress.c_str(), PortString.GetAnsiCharArray().c_str(), &Hint, &Result));
#else
    AddrInfo = getaddrinfo(ServerAddress.c_str(), PortString.GetAnsiCharArray().c_str(), &Hint, &Result);
#endif
    if (AddrInfo != 0)
    {
#if PLATFORM_WINDOWS
        UWUtilities::Print(EWLogType::Error, FString(L"Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
        WSACleanup();
#else
        UWUtilities::Print(EWLogType::Error, FString(L"Cannot resolve address."));
#endif
        return false;
    }

    UDPSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
    if (UDPSocket < 0)
    {
        freeaddrinfo(Result);

#if PLATFORM_WINDOWS
        UWUtilities::Print(EWLogType::Error, FString(L"Cannot create socket. Error: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        UWUtilities::Print(EWLogType::Error, FString(L"Cannot create socket."));
#endif
        return false;
    }

    int32 optval = 1;
    setsockopt(UDPSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int32));
#if PLATFORM_WINDOWS
    setsockopt(UDPSocket, SOL_SOCKET, UDP_NOCHECKSUM, (const char*)&optval, sizeof(int32));
#else
    setsockopt(UDPSocket, SOL_SOCKET, SO_NO_CHECK, (const char*)&optval, sizeof(int32));
#endif

    SocketAddress = (struct sockaddr*)FMemory::Malloc(Result->ai_addrlen);
    FMemory::Memcpy(SocketAddress, Result->ai_addr, Result->ai_addrlen);
    SocketAddressLength = Result->ai_addrlen;

    freeaddrinfo(Result);

    if (UDPSocket < 0)
    {
#if PLATFORM_WINDOWS
        UWUtilities::Print(EWLogType::Error, FString(L"Error has occurred during opening a socket: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        UWUtilities::Print(EWLogType::Error, FString(L"Error has occurred during opening a socket."));
#endif
        return false;
    }
    return true;
}
void UWUDPClient::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
	close(UDPSocket);
#endif
}
void UWUDPClient::ListenServer()
{
    while (bClientStarted)
    {
        auto Buffer = new ANSICHAR[UDP_BUFFER_SIZE];

        auto RetrievedSize = static_cast<int32>(recvfrom(UDPSocket, Buffer, UDP_BUFFER_SIZE, 0, SocketAddress, &SocketAddressLength));
        if (RetrievedSize < 0 || !bClientStarted)
        {
            delete[] Buffer;
            if (!bClientStarted) return;
            continue;
        }
        if (RetrievedSize == 0) continue;

        TArray<UWAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new UWUDPTaskParameter(RetrievedSize, Buffer, SocketAddress, false));

        WFutureAsyncTask Lambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2)
            {
                auto ClientInstance = dynamic_cast<UWUDPClient*>(TaskParameters[0]);
                auto Parameter = dynamic_cast<UWUDPTaskParameter*>(TaskParameters[1]);

                if (!ClientInstance || !ClientInstance->bClientStarted || !ClientInstance->UDPListenCallback || !ClientInstance->UDPHandler) return;

                if (Parameter)
                {
                    FWCHARWrapper BufferWrapped(Parameter->Buffer, Parameter->BufferSize, false);

                    WJson::Node AnalyzedData = ClientInstance->UDPHandler->AnalyzeNetworkDataWithByteArray(BufferWrapped, Parameter->OtherParty);

                    ClientInstance->UDPListenCallback(ClientInstance->SocketAddress, ClientInstance->UDPHandler, AnalyzedData);
                    delete (Parameter);
                }
            }
        };
        UWAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 UWUDPClient::ServerListenerStopped()
{
    if (!bClientStarted) return 0;
    if (UDPClientThread) delete (UDPClientThread);
    UDPClientThread = new WThread(std::bind(&UWUDPClient::ListenServer, this), std::bind(&UWUDPClient::ServerListenerStopped, this));
    return 0;
}