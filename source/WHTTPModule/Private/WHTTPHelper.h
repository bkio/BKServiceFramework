// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPHelper
#define Pragma_Once_WHTTPHelper

#include "WEngine.h"
#include "WHTTPRequestParser.h"
#include "WScheduledTaskManager.h"

#if PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>
#include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
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

    sockaddr* Client = nullptr;

    std::string ClientIP;

    WHTTPRequestParser Parser{};

    int32 RecvBufferLen = HTTP_BUFFER_SIZE;
    ANSICHAR RecvBuffer[HTTP_BUFFER_SIZE]{};
    int32 BytesReceived{};

    bool bInitialized = false;
    WMutex bInitialized_Mutex;
    void Cancel()
    {
        if (!bInitialized) return;

        WScopeGuard Guard(&bInitialized_Mutex);
        bInitialized = false;
    }

    bool bSocketClosed = false;
    void CloseSocket()
    {
        if (bSocketClosed) return;
        bSocketClosed = true;

#if PLATFORM_WINDOWS
        closesocket(ClientSocket);
        shutdown(ClientSocket, SD_BOTH);
#else
        close(ClientSocket);
        shutdown(ClientSocket, SD_BOTH);
#endif
    }

    void SendData_Internal(const std::wstring& Body, const std::string& Header)
    {
        std::string Response = Header + FString::WStringToString(Body);
        send(ClientSocket, Response.c_str(), strlen(Response.c_str()), 0);
    }
    void Corrupted_Internal()
    {
        std::wstring ResponseBody = L"<html>Corrupted</html>";
        std::string ResponseHeaders = "HTTP/1.1 400 Bad Request\r\n"
                                              "Content-Type: text/html; charset=UTF-8\r\n"
                                              "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
        SendData_Internal(ResponseBody, ResponseHeaders);
    }
    void Timeout_Internal()
    {
        std::wstring ResponseBody = L"<html>Timeout</html>";
        std::string ResponseHeaders = "HTTP/1.1 408 Request Timeout\r\n"
                                              "Content-Type: text/html; charset=UTF-8\r\n"
                                              "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
        SendData_Internal(ResponseBody, ResponseHeaders);

        Cancel();
        CloseSocket();
    }

public:
#if PLATFORM_WINDOWS
    FWHTTPClient(SOCKET _ClientSocket, sockaddr* _Client, uint32 TimeoutInMs)
#else
    FWHTTPClient(int32 _ClientSocket, sockaddr* _Client, uint32 TimeoutInMs)
#endif
    {
        ClientSocket = _ClientSocket;
        Client = _Client;

        TArray<FWAsyncTaskParameter*> NoParameter;
        UWScheduledAsyncTaskManager::NewScheduledAsyncTask(std::bind(Timeout_Internal, this), NoParameter, TimeoutInMs, false, true);
    }
    ~FWHTTPClient() override
    {
        CloseSocket();
        if (Client)
        {
            delete (Client);
        }
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

    //@return: If succeed true, otherwise false.
    bool GetData()
    {
        if (!bInitialized) return false;

        bool HeadersReady = false;
        while (!HeadersReady)
        {
            BytesReceived = recv(ClientSocket, RecvBuffer, RecvBufferLen, 0);

            if (!bInitialized) return false;

            if (BytesReceived > 0)
            {
                Parser.ProcessChunkForHeaders(RecvBuffer, BytesReceived);
                if (Parser.AllHeadersAvailable())
                {
                    HeadersReady = true;
                }
            }
            else
            {
                Cancel();
                return false;
            }
        }

        if (Parser.GetMethod().rfind("HEAD", 0) == 0 || Parser.GetMethod().rfind("GET", 0) == 0) return true;

        while (true)
        {
            BytesReceived = recv(ClientSocket, RecvBuffer, RecvBufferLen, 0);

            if (!bInitialized) return false;

            if (BytesReceived > 0)
            {
                if (Parser.ProcessChunkForBody(RecvBuffer, BytesReceived))
                {
                    if (Parser.AllBodyAvailable())
                    {
                        return true;
                    }
                }
                else
                {
                    Corrupted_Internal();
                    return false;
                }
            }
            else
            {
                Cancel();
                return false;
            }
        }
    }

    void SendData(const std::wstring& Body, const std::string& Header)
    {
        if (!bInitialized) return;
        SendData_Internal(Body, Header);
    }

    void Finalize()
    {
        Cancel();
    }

    std::string GetClientIP()
    {
        if (!bInitialized) return "";
        return ClientIP;
    }

    std::string GetMethod()
    {
        if (!bInitialized) return "";
        return Parser.GetMethod();
    }

    std::string GetPath()
    {
        if (!bInitialized) return "";
        return Parser.GetPath();
    }

    std::string GetProtocol()
    {
        if (!bInitialized) return "";
        return Parser.GetProtocol();
    }

    std::map<std::string, std::string> GetHeaders()
    {
        if (!bInitialized) return std::map<std::string, std::string>();
        return Parser.GetHeaders();
    };

    std::wstring GetPayload()
    {
        if (!bInitialized) return L"";
        return Parser.GetPayload();
    }
};

#endif //Pragma_Once_WHTTPHelper
