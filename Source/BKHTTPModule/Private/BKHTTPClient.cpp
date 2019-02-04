// Copyright Burak Kara, All rights reserved.

#include "BKMemory.h"
#include "BKAsyncTaskManager.h"
#include "BKScheduledTaskManager.h"
#include "BKHTTPClient.h"

void BKHTTPClient::NewHTTPRequest(
        const FString& _ServerAddress,
        uint16 _ServerPort,
        const FString& _Payload,
        const FString& _Verb,
        const FString& _Path,
        const BKHashMap<FString, FString>& _Headers,
        uint32 _TimeoutMs,
        BKFutureAsyncTask& _RequestCallback,
        BKFutureAsyncTask& _TimeoutCallback)
{
    auto NewClient = new BKHTTPClient();
    NewClient->ServerAddress = _ServerAddress;
    NewClient->ServerPort = _ServerPort;
    NewClient->Headers = _Headers;
    NewClient->Payload = _Payload;
    NewClient->RequestLine = _Verb + FString(L" ") + _Path + FString(L" HTTP/1.1");

    TArray<BKAsyncTaskParameter*> AsArray(NewClient);
    BKAsyncTaskManager::NewAsyncTask(_RequestCallback, AsArray, true);
    BKScheduledAsyncTaskManager::NewScheduledAsyncTask(_TimeoutCallback, AsArray, _TimeoutMs, false, true);
}

bool BKHTTPClient::ProcessRequest()
{
    bool bResult = false;
    {
        BKScopeGuard RequestGuard(&RequestMutex);
        if (bRequestInitialized) return false;
        bRequestInitialized = true;
    }

    if (InitializeSocket())
    {
        SendData();
        bResult = ReceiveData();
        CloseSocket();
    }
    return bResult;
}

void BKHTTPClient::CancelRequest()
{
    BKScopeGuard RequestGuard(&RequestMutex);
    if (!bRequestInitialized) return;
    bRequestInitialized = false;
    CloseSocket();
}

bool BKHTTPClient::InitializeSocket()
{
#if PLATFORM_WINDOWS
    HTTPSocket = static_cast<SOCKET>(-1);
#else
    HTTPSocket = -1;
#endif

#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    DWORD AddrInfo;

    int32 RetVal = WSAStartup(0x202, &WSAData);
    if (RetVal != 0)
    {
        BKUtilities::Print(EBKLogType::Error, FString(L"WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
        return false;
    }
#else
    int32 AddrInfo;
#endif

    struct addrinfo Hint{};
    FMemory::Memset(&Hint, 0, sizeof(struct addrinfo));
    Hint.ai_family = AF_INET;
    Hint.ai_socktype = SOCK_STREAM;
    Hint.ai_protocol = IPPROTO_TCP;

    FString PortString = FString::FromInt(ServerPort);

    struct addrinfo* Result;

#if PLATFORM_WINDOWS
    AddrInfo = static_cast<DWORD>(getaddrinfo(ServerAddress.GetAnsiCharArray().c_str(), PortString.GetAnsiCharArray().c_str(), &Hint, &Result));
#else
    AddrInfo = getaddrinfo(ServerAddress.GetAnsiCharArray().c_str(), PortString.GetAnsiCharArray().c_str(), &Hint, &Result);
#endif
    if (AddrInfo != 0)
    {
#if PLATFORM_WINDOWS
        BKUtilities::Print(EBKLogType::Error, FString(L"Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
        WSACleanup();
#else
        BKUtilities::Print(EBKLogType::Error, FString(L"Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
#endif
        return false;
    }

    // Each of the returned IP address is tried.
    for (; Result; Result = Result->ai_next)
    {
        HTTPSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
        if (HTTPSocket < 0)
        {
#if PLATFORM_WINDOWS
            BKUtilities::Print(EBKLogType::Error, FString(L"Create socket failed with error: ") + FString::FromInt(WSAGetLastError()));
            WSACleanup();
#else
            BKUtilities::Print(EBKLogType::Error, FString(L"Create socket failed."));
#endif
            return false;
        }

        int32 On = 1;
        setsockopt(HTTPSocket, SOL_SOCKET, SO_REUSEADDR, (const ANSICHAR*)&On, sizeof(int32));
        setsockopt(HTTPSocket, IPPROTO_TCP, TCP_NODELAY, (const ANSICHAR*)&On, sizeof(int32));

#if PLATFORM_WINDOWS
        AddrInfo = static_cast<DWORD>(connect(HTTPSocket, Result->ai_addr, Result->ai_addrlen));
        if (AddrInfo == SOCKET_ERROR)
#else
        AddrInfo = connect(HTTPSocket, Result->ai_addr, Result->ai_addrlen);
        if (AddrInfo == -1)
#endif
        {
#if PLATFORM_WINDOWS
            closesocket(HTTPSocket);
            HTTPSocket = INVALID_SOCKET;
#else
            close(HTTPSocket);
            HTTPSocket = -1;
#endif
            continue;
        }
        break;
    }

    freeaddrinfo(Result);

#if PLATFORM_WINDOWS
    if (HTTPSocket == INVALID_SOCKET)
#else
    if (HTTPSocket == -1)
#endif
    {
#if PLATFORM_WINDOWS
        BKUtilities::Print(EBKLogType::Error, FString(L"Error has occurred during connecting to server: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        BKUtilities::Print(EBKLogType::Error, FString(L"Error has occurred during connecting to server."));
#endif
        return false;
    }
    return true;
}
void BKHTTPClient::CloseSocket()
{
    if (bSocketClosed) return;
    bSocketClosed = true;

#if PLATFORM_WINDOWS
    closesocket(HTTPSocket);
    shutdown(HTTPSocket, SD_BOTH);
    WSACleanup();
#else
    shutdown(HTTPSocket, SHUT_RDWR);
	close(HTTPSocket);
#endif
}

void BKHTTPClient::SendData()
{
    if (!bRequestInitialized) return;

    FStringStream HeaderBuilder;
    HeaderBuilder << RequestLine;
    HeaderBuilder << L"\r\n";
    Headers.Iterate([&HeaderBuilder](BKSharedPtr<BKHashNode<FString, FString>> Node)
    {
        HeaderBuilder << Node->GetKey();
        HeaderBuilder << L':';
        HeaderBuilder << Node->GetValue();
        HeaderBuilder << L"\r\n";
    });

    FString Response = HeaderBuilder.Str() + Payload;

    std::string ResponseString = Response.GetAnsiCharString();

#if PLATFORM_WINDOWS
    send(HTTPSocket, ResponseString.c_str(), ResponseString.size(), 0);
#else
    send(HTTPSocket, ResponseString.c_str(), ResponseString.size(), MSG_NOSIGNAL);
#endif
}

bool BKHTTPClient::ReceiveData()
{
    if (!bRequestInitialized) return false;

    bool HeadersReady = false;
    bool BodyReady = false;

    while (!HeadersReady)
    {
        BytesReceived = static_cast<int32>(recv(HTTPSocket, RecvBuffer, static_cast<size_t>(RecvBufferLen), 0));

        if (!bRequestInitialized) return false;

        if (BytesReceived > 0)
        {
            FString PreParseString = FString(RecvBuffer, static_cast<uint32>(BytesReceived));

            Parser.ProcessChunkForHeaders(PreParseString);

            HeadersReady = Parser.AllHeadersAvailable();
            BodyReady = Parser.AllBodyAvailable();
        }
        else return false;
    }

    while (!BodyReady)
    {
        BytesReceived = static_cast<int32>(recv(HTTPSocket, RecvBuffer, static_cast<size_t>(RecvBufferLen), 0));

        if (!bRequestInitialized) return false;

        if (BytesReceived > 0)
        {
            FString PreParseString = FString(RecvBuffer, static_cast<uint32>(BytesReceived));

            Parser.ProcessChunkForBody(PreParseString);

            if (Parser.ErrorOccuredInBodyParsing()) return false;

            BodyReady = Parser.AllBodyAvailable();
        }
        else return false;
    }
    return true;
}

bool BKHTTPClient::DestroyApproval()
{
    BKScopeGuard ReceivedDestroyApproval_Guard(&ReceivedDestroyApproval_Mutex);
    return ++ReceivedDestroyApproval >= 2;
}