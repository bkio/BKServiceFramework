// Copyright Pagansoft.com, All rights reserved.

#include "WUDPClient.h"
#include "WUDPHandler.h"
#include "WUDPHelper.h"
#include "WAsyncTaskManager.h"

WUDPClient* WUDPClient::NewUDPClient(FString _ServerAddress, uint16 _ServerPort, std::function<void(class WUDPClient*, WJson::Node)>& _DataReceivedCallback)
{
    auto NewClient = new WUDPClient();

    NewClient->UDPListenCallback = _DataReceivedCallback;

    if (!NewClient->StartUDPClient(_ServerAddress, _ServerPort))
    {
        delete (NewClient);
        return nullptr;
    }

    return NewClient;
}

WUDPHandler* WUDPClient::GetUDPHandler()
{
    return UDPHandler;
}

bool WUDPClient::StartUDPClient(FString& _ServerAddress, uint16 _ServerPort)
{
    if (bClientStarted) return false;
    bClientStarted = true;

    ServerAddress = _ServerAddress;
    ServerPort = _ServerPort;

    if (InitializeClient())
    {
        UDPClientThread = new WThread(std::bind(&WUDPClient::ListenServer, this), std::bind(&WUDPClient::ServerListenerStopped, this));
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

void WUDPClient::MarkPendingKill()
{
    if (UDPHandler)
    {
        UDPHandler->MarkPendingKill(std::bind(&WUDPClient::LazySuicide, this));
    }
}
void WUDPClient::LazySuicide()
{
    TArray<WAsyncTaskParameter*> PassParameters;
    PassParameters.Add(this);

    WFutureAsyncTask Lambda = [](TArray<WAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() >= 1 && TaskParameters[0])
        {
            auto ClientInstance = reinterpret_cast<WUDPClient*>(TaskParameters[0]);
            if (ClientInstance)
            {
                ClientInstance->EndUDPClient();
                //Will be deleted after, by AsyncTaskManager.
            }
        }
    };
    WAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, false); //false parameter: Deallocate self after
}

void WUDPClient::EndUDPClient()
{
    if (!bClientStarted) return;
    bClientStarted = false;

    if (UDPHandler)
    {
        UDPHandler->EndSystem();
        delete (UDPHandler);
    }
    CloseSocket();
    if (SocketAddress)
    {
        FMemory::Free(SocketAddress);
    }
    UDPListenCallback = nullptr;
    if (UDPClientThread)
    {
        if (UDPClientThread->IsJoinable())
        {
            UDPClientThread->Join();
        }
        delete (UDPClientThread);
    }
}

bool WUDPClient::InitializeClient()
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    DWORD AddrInfo;

    int32 RetVal = WSAStartup(0x202, &WSAData);
    if (RetVal != 0)
    {
        WUtilities::Print(EWLogType::Error, FString("WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
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
    AddrInfo = static_cast<DWORD>(getaddrinfo(ServerAddress.GetAnsiCharArray(), PortString.GetAnsiCharArray(), &Hint, &Result));
#else
    AddrInfo = getaddrinfo(ServerAddress.c_str(), PortString.GetAnsiCharArray(), &Hint, &Result);
#endif
    if (AddrInfo != 0)
    {
#if PLATFORM_WINDOWS
        WUtilities::Print(EWLogType::Error, FString("Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
        WSACleanup();
#else
        WUtilities::Print(EWLogType::Error, FString("Cannot resolve address."));
#endif
        return false;
    }

    UDPSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
    if (UDPSocket < 0)
    {
        freeaddrinfo(Result);

#if PLATFORM_WINDOWS
        WUtilities::Print(EWLogType::Error, FString("Cannot create socket. Error: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        WUtilities::Print(EWLogType::Error, FString("Cannot create socket."));
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
        WUtilities::Print(EWLogType::Error, FString("Error has occurred during opening a socket: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        WUtilities::Print(EWLogType::Error, FString("Error has occurred during opening a socket."));
#endif
        return false;
    }
    return true;
}
void WUDPClient::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
	close(UDPSocket);
#endif
}
void WUDPClient::ListenServer()
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

        TArray<WAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new WUDPTaskParameter(RetrievedSize, Buffer, SocketAddress, false));

        WFutureAsyncTask Lambda = [](TArray<WAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ClientInstance = reinterpret_cast<WUDPClient*>(TaskParameters[0]);
                auto Parameter = reinterpret_cast<WUDPTaskParameter*>(TaskParameters[1]);

                if (!ClientInstance || !ClientInstance->bClientStarted || !ClientInstance->UDPListenCallback || !ClientInstance->UDPHandler)
                {
                    if (Parameter)
                    {
                        delete (Parameter);
                    }
                    return;
                }

                if (Parameter)
                {
                    FWCHARWrapper BufferWrapped(Parameter->Buffer, Parameter->BufferSize, false);

                    WJson::Node AnalyzedData = ClientInstance->UDPHandler->AnalyzeNetworkDataWithByteArray(BufferWrapped, Parameter->OtherParty);

                    if (AnalyzedData.GetType() != WJson::Node::Type::T_VALIDATION &&
                        AnalyzedData.GetType() != WJson::Node::Type::T_INVALID &&
                        AnalyzedData.GetType() != WJson::Node::Type::T_NULL)
                    {
                        ClientInstance->UDPListenCallback(ClientInstance, AnalyzedData);
                    }
                    delete (Parameter);
                }
            }
        };
        WAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 WUDPClient::ServerListenerStopped()
{
    if (!bClientStarted) return 0;
    if (UDPClientThread) delete (UDPClientThread);
    UDPClientThread = new WThread(std::bind(&WUDPClient::ListenServer, this), std::bind(&WUDPClient::ServerListenerStopped, this));
    return 0;
}