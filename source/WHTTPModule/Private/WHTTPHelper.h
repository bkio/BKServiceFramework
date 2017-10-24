// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPHelper
#define Pragma_Once_WHTTPHelper

#include "WEngine.h"
#include "WHTTPRequestParser.h"

#if PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#define HTTP_BUFFER_SIZE 2048

struct FWHTTPClient : public FWAsyncTaskParameter
{

private:
#if PLATFORM_WINDOWS
    SOCKET ClientSocket {};
#else
    int32 ClientSocket = 0;
#endif

    bool bInitialized = false;

    std::string ClientIP;

    WHTTPRequestParser Parser{};

    int32 RecvBufferLen = HTTP_BUFFER_SIZE;
    ANSICHAR RecvBuffer[HTTP_BUFFER_SIZE]{};
    int32 BytesReceived{};

public:
    int32 BufferSize = 0;

#if PLATFORM_WINDOWS

    explicit FWHTTPClient(SOCKET _ClientSocket)
#else
    explicit FWHTTPClient(int32 _ClientSocket)
#endif
    {
        ClientSocket = _ClientSocket;
    }
    ~FWHTTPClient() override
    {
        closesocket(ClientSocket);
    }

    bool Initialize()
    {
        if (bInitialized) return true;
        bInitialized = true;

        sockaddr_in ClientInfo{};
#if PLATFORM_WINDOWS
        int32 ClientInfoLen = sizeof(sockaddr_in);
#else
        socklen_t ClientInfoLen = sizeof(sockaddr_in);
#endif

        auto PeerNameResult = getpeername(ClientSocket, (sockaddr*)(&ClientInfo), &ClientInfoLen);
#if PLATFORM_WINDOWS
        if (PeerNameResult == SOCKET_ERROR) return false;
#else
        if (PeerNameResult == -1) return false;
#endif

        ClientIP = inet_ntoa(ClientInfo.sin_addr);
    }

    bool GetData()
    {
        Parser.Reset();

        bool HeadersReady = false;
        while(!HeadersReady)
        {
            BytesReceived = recv(ClientSocket, RecvBuffer, RecvBufferLen, 0);
            if(BytesReceived > 0)
            {
                Parser.ProcessChunk(RecvBuffer, BytesReceived);
                if(Parser.AllHeadersAvailable())
                {
                    HeadersReady = true;
                }
            }
            else return false;
        }
        return true;
    }

    void SendData(const std::string& Body, const std::string& Header)
    {
        std::string Response = Header + Body;
        send(ClientSocket, Response.c_str(), strlen(Response.c_str()), 0);
    }

    std::string GetClientIP()
    {
        return ClientIP;
    }

    std::string GetMethod()
    {
        return Parser.GetMethod();
    }

    std::string GetPath()
    {
        return Parser.GetPath();
    }

    std::string GetProtocol()
    {
        return Parser.GetProtocol();
    }

    std::map<std::string, std::string> GetHeaders()
    {
        return Parser.GetHeaders();
    };
};

#endif //Pragma_Once_WHTTPHelper
