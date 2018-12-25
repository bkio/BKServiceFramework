// Copyright Burak Kara, All rights reserved.

#include "BKUDPClient.h"
#include "BKUDPHandler.h"
#include "BKUDPHelper.h"
#include "BKAsyncTaskManager.h"

BKUDPClient* BKUDPClient::NewUDPClient(FString _ServerAddress, uint16 _ServerPort, std::function<void(class BKUDPClient*, BKJson::Node)>& _DataReceivedCallback)
{
    auto NewClient = new BKUDPClient();

    NewClient->UDPListenCallback = _DataReceivedCallback;

    if (!NewClient->StartUDPClient(_ServerAddress, _ServerPort))
    {
        delete (NewClient);
        return nullptr;
    }

    return NewClient;
}

BKUDPHandler* BKUDPClient::GetUDPHandler()
{
    return UDPHandler;
}

bool BKUDPClient::StartUDPClient(FString& _ServerAddress, uint16 _ServerPort)
{
    if (bClientStarted) return false;
    bClientStarted = true;

    ServerAddress = _ServerAddress;
    ServerPort = _ServerPort;

    if (InitializeClient())
    {
        UDPClientThread = new BKThread(std::bind(&BKUDPClient::ListenServer, this), std::bind(&BKUDPClient::ServerListenerStopped, this));
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

void BKUDPClient::MarkPendingKill()
{
    if (UDPHandler)
    {
        UDPHandler->MarkPendingKill(std::bind(&BKUDPClient::LazySuicide, this));
    }
}
void BKUDPClient::LazySuicide()
{
    TArray<BKAsyncTaskParameter*> PassParameters;
    PassParameters.Add(this);

    BKFutureAsyncTask Lambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() >= 1 && TaskParameters[0])
        {
            auto ClientInstance = reinterpret_cast<BKUDPClient*>(TaskParameters[0]);
            if (ClientInstance)
            {
                ClientInstance->EndUDPClient();
                //Will be deleted after, by AsyncTaskManager.
            }
        }
    };
    BKAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, false); //false parameter: Deallocate self after
}

void BKUDPClient::EndUDPClient()
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

bool BKUDPClient::InitializeClient()
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    DWORD AddrInfo;

    int32 RetVal = WSAStartup(0x202, &WSAData);
    if (RetVal != 0)
    {
        BKUtilities::Print(EBKLogType::Error, FString("WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
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
    AddrInfo = getaddrinfo(ServerAddress.GetAnsiCharArray(), PortString.GetAnsiCharArray(), &Hint, &Result);
#endif
    if (AddrInfo != 0)
    {
#if PLATFORM_WINDOWS
        BKUtilities::Print(EBKLogType::Error, FString("Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
        WSACleanup();
#else
        WUtilities::Print(EBKLogType::Error, FString("Cannot resolve address."));
#endif
        return false;
    }

    UDPSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
    if (UDPSocket < 0)
    {
        freeaddrinfo(Result);

#if PLATFORM_WINDOWS
        BKUtilities::Print(EBKLogType::Error, FString("Cannot create socket. Error: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        WUtilities::Print(EBKLogType::Error, FString("Cannot create socket."));
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
        BKUtilities::Print(EBKLogType::Error, FString("Error has occurred during opening a socket: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        WUtilities::Print(EBKLogType::Error, FString("Error has occurred during opening a socket."));
#endif
        return false;
    }
    return true;
}
void BKUDPClient::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    shutdown(UDPSocket, SHUT_RDWR);
	close(UDPSocket);
#endif
}
void BKUDPClient::ListenServer()
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

        TArray<BKAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new WUDPTaskParameter(RetrievedSize, Buffer, SocketAddress, false));

        BKFutureAsyncTask Lambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ClientInstance = reinterpret_cast<BKUDPClient*>(TaskParameters[0]);
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
                    FBKCHARWrapper BufferWrapped(Parameter->Buffer, Parameter->BufferSize, false);

                    BKJson::Node AnalyzedData = ClientInstance->UDPHandler->AnalyzeNetworkDataWithByteArray(BufferWrapped, Parameter->OtherParty);

                    if (AnalyzedData.GetType() != BKJson::Node::Type::T_VALIDATION &&
                        AnalyzedData.GetType() != BKJson::Node::Type::T_INVALID &&
                        AnalyzedData.GetType() != BKJson::Node::Type::T_NULL)
                    {
                        ClientInstance->UDPListenCallback(ClientInstance, AnalyzedData);
                    }
                    delete (Parameter);
                }
            }
        };
        BKAsyncTaskManager::NewAsyncTask(Lambda, PassParameters, true);
    }
}
uint32 BKUDPClient::ServerListenerStopped()
{
    if (!bClientStarted) return 0;
    if (UDPClientThread) delete (UDPClientThread);
    UDPClientThread = new BKThread(std::bind(&BKUDPClient::ListenServer, this), std::bind(&BKUDPClient::ServerListenerStopped, this));
    return 0;
}