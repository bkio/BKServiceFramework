// Copyright Pagansoft.com, All rights reserved.

#include "WMemory.h"
#include "WAsyncTaskManager.h"
#include "WScheduledTaskManager.h"
#include "WHTTPClient.h"

void WHTTPClient::NewHTTPRequest(
        const FString& _ServerAddress,
        uint16 _ServerPort,
        const FString& _Payload,
        const FString& _Verb,
        const FString& _Path,
        std::map<FString, FString> _Headers,
        uint32 _TimeoutMs,
        WFutureAsyncTask& _RequestCallback,
        WFutureAsyncTask& _TimeoutCallback)
{
    auto NewClient = new WHTTPClient();
    NewClient->ServerAddress = _ServerAddress;
    NewClient->ServerPort = _ServerPort;
    NewClient->Headers = std::move(_Headers);
    NewClient->Payload = _Payload;
    NewClient->RequestLine = _Verb + FString(" ") + _Path + FString(" HTTP/1.1");

    TArray<WAsyncTaskParameter*> AsArray(NewClient);
    WAsyncTaskManager::NewAsyncTask(_RequestCallback, AsArray, true);
    WScheduledAsyncTaskManager::NewScheduledAsyncTask(_TimeoutCallback, AsArray, _TimeoutMs, false, true);
}

bool WHTTPClient::ProcessRequest()
{
    bool bResult = false;
    {
        WScopeGuard RequestGuard(&RequestMutex);
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

void WHTTPClient::CancelRequest()
{
    WScopeGuard RequestGuard(&RequestMutex);
    if (!bRequestInitialized) return;
    bRequestInitialized = false;
    CloseSocket();
}

bool WHTTPClient::InitializeSocket()
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
        WUtilities::Print(EWLogType::Error, FString("WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
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
    AddrInfo = static_cast<DWORD>(getaddrinfo(ServerAddress.GetAnsiCharArray(), PortString.GetAnsiCharArray(), &Hint, &Result));
#else
    AddrInfo = getaddrinfo(ServerAddress.GetAnsiCharArray(), PortString.GetAnsiCharArray(), &Hint, &Result);
#endif
    if (AddrInfo != 0)
    {
#if PLATFORM_WINDOWS
        WUtilities::Print(EWLogType::Error, FString("Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
        WSACleanup();
#else
        WUtilities::Print(EWLogType::Error, FString("Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
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
            WUtilities::Print(EWLogType::Error, FString("Create socket failed with error: ") + FString::FromInt(WSAGetLastError()));
            WSACleanup();
#else
            WUtilities::Print(EWLogType::Error, FString("Create socket failed."));
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
        WUtilities::Print(EWLogType::Error, FString("Error has occurred during connecting to server: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        WUtilities::Print(EWLogType::Error, FString("Error has occurred during connecting to server."));
#endif
        return false;
    }
    return true;
}
void WHTTPClient::CloseSocket()
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

void WHTTPClient::SendData()
{
    if (!bRequestInitialized) return;

    FStringStream HeaderBuilder(false);
    HeaderBuilder << RequestLine.GetAnsiCharArray();
    HeaderBuilder << "\r\n";
    for (auto& Header : Headers)
    {
        HeaderBuilder << Header.first.GetAnsiCharArray();
        HeaderBuilder << ':';
        HeaderBuilder << Header.second.GetAnsiCharArray();
        HeaderBuilder << "\r\n";
    }

    FString Response = HeaderBuilder.Str() + FString::WStringToString(Payload);

#if PLATFORM_WINDOWS
    send(HTTPSocket, Response.GetAnsiCharArray(), strlen(Response.GetAnsiCharArray()), 0);
#else
    send(HTTPSocket, Response.GetAnsiCharArray(), strlen(Response.GetAnsiCharArray()), MSG_NOSIGNAL);
#endif
}

bool WHTTPClient::ReceiveData()
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
            Parser.ProcessChunkForHeaders(RecvBuffer, BytesReceived);

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
            Parser.ProcessChunkForBody(RecvBuffer, BytesReceived);

            if (Parser.ErrorOccuredInBodyParsing()) return false;

            BodyReady = Parser.AllBodyAvailable();
        }
        else return false;
    }
    return true;
}

bool WHTTPClient::DestroyApproval()
{
    WScopeGuard ReceivedDestroyApproval_Guard(&ReceivedDestroyApproval_Mutex);
    return ++ReceivedDestroyApproval >= 2;
}