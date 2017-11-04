// Copyright Pagansoft.com, All rights reserved.

#include "WMemory.h"
#include "WHTTPClient.h"

FWHTTPClient::FWHTTPClient(std::string _ServerAddress, uint16 _ServerPort, std::wstring _Payload, std::string _Verb, std::string _Path, std::map<std::string, std::string> _Headers, uint32 _TimeoutMs)
{
    ServerAddress = std::move(_ServerAddress);
    ServerPort = _ServerPort;
    Headers = std::move(_Headers);
    Payload = std::move(_Payload);
    RequestLine = _Verb + " " + _Path + " HTTP/1.1";
    TimeoutMs = _TimeoutMs;
}
FWHTTPClient::~FWHTTPClient()
{
    CloseSocket();
}

bool FWHTTPClient::ProcessRequest()
{
    WScopeGuard RequestGuard(&RequestMutex);
    if (bRequestInitialized) return false;
    bRequestInitialized = true;

    bool bResult = false;

    if (InitializeSocket())
    {
        SendData();
        bResult = ReceiveData();
        CloseSocket();
    }
    return bResult;
}

void FWHTTPClient::CancelRequest()
{
    WScopeGuard RequestGuard(&RequestMutex);
    if (!bRequestInitialized) return;
    bRequestInitialized = false;
    CloseSocket();
}

bool FWHTTPClient::InitializeSocket()
{
#if PLATFORM_WINDOWS
    HTTPSocket = static_cast<SOCKET>(-1);
#else
    HTTPSocket = -1;
#endif

    int32 RetVal;
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    DWORD AddrInfo;

    RetVal = WSAStartup(0x202, &WSAData);
    if (RetVal != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"WSAStartup() failed with error: ") + FString::FromInt(WSAGetLastError()));
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
        UWUtilities::Print(EWLogType::Error, FString(L"Cannot resolve address: ") + FString::FromInt((int32)AddrInfo));
#endif
        return false;
    }

    // Each of the returned IP address is tried.
    for (; Result != nullptr; Result = Result->ai_next)
    {
        HTTPSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
        if (HTTPSocket < 0)
        {
#if PLATFORM_WINDOWS
            UWUtilities::Print(EWLogType::Error, FString(L"Create socket failed with error: ") + FString::FromInt(WSAGetLastError()));
            WSACleanup();
#else
            UWUtilities::Print(EWLogType::Error, FString(L"Create socket failed."));
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
        UWUtilities::Print(EWLogType::Error, FString(L"Error has occurred during connecting to server: ") + FString::FromInt(WSAGetLastError()));
        WSACleanup();
#else
        UWUtilities::Print(EWLogType::Error, FString(L"Error has occurred during connecting to server."));
#endif
        return false;
    }
    return true;
}
void FWHTTPClient::CloseSocket()
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

void FWHTTPClient::SendData()
{
    std::stringstream HeaderBuilder;
    HeaderBuilder << RequestLine.c_str() << "\r\n";
    for (auto& Header : Headers)
    {
        HeaderBuilder << Header.first.c_str() << ':' << Header.second.c_str() << "\r\n";
    }

    std::string Response = HeaderBuilder.str() + FString::WStringToString(Payload);

#if PLATFORM_WINDOWS
    send(HTTPSocket, Response.c_str(), strlen(Response.c_str()), 0);
#else
    send(HTTPSocket, Response.c_str(), strlen(Response.c_str()), MSG_NOSIGNAL);
#endif
}

bool FWHTTPClient::ReceiveData()
{
    if (!bRequestInitialized) return false;

    bool HeadersReady = false;
    bool BodyReady = false;

    while (!HeadersReady)
    {
        BytesReceived = static_cast<int32>(recv(HTTPSocket, RecvBuffer, RecvBufferLen, 0));

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
        BytesReceived = recv(HTTPSocket, RecvBuffer, RecvBufferLen, 0);

        if (!bRequestInitialized) return false;

        if (BytesReceived > 0)
        {
            Parser.ProcessChunkForBody(RecvBuffer, BytesReceived);
            if (Parser.ErrorOccuredInBodyParsing()) return false;
            if (Parser.AllBodyAvailable())
            {
                BodyReady = true;
            }
        }
        else return false;
    }
    return true;
}