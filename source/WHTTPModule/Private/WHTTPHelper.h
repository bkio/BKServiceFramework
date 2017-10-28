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

struct WHTTPSocket
{

private:
    WHTTPSocket() = default;

    WMutex HTTPSocket_Mutex;

    int32 DeinitializationApproval = 0; //Must be 2 to deallocate this.

    bool bSocketOperational = true;

    //Step 1
    static void CloseSocket(WHTTPSocket* _Socket)
    {
        if (_Socket == nullptr) return;

        if (_Socket->bSocketOperational)
        {
            _Socket->bSocketOperational = false;
#if PLATFORM_WINDOWS
            closesocket(_Socket->ClientSocket);
            shutdown(_Socket->ClientSocket, SD_BOTH);
#else
            close(_Socket->ClientSocket);
            shutdown(_Socket->ClientSocket, SD_BOTH);
#endif
        }
    }
    //Step 2
    static void TryDeinitializing(WHTTPSocket* _Socket)
    {
        if (_Socket == nullptr) return;

        if (++_Socket->DeinitializationApproval == 2)
        {
            if (_Socket->Client)
            {
                delete (_Socket->Client);
            }
            _Socket->HTTPSocket_Mutex.unlock();
            delete (_Socket);
        }
        else
        {
            _Socket->HTTPSocket_Mutex.unlock();
        }
    }

public:

#if PLATFORM_WINDOWS
    SOCKET ClientSocket {};
#else
    int32 ClientSocket = 0;
#endif
    sockaddr* Client = nullptr;

    void StopSocketOperation()
    {
        HTTPSocket_Mutex.lock(); //Will be unlocked in private static functions.
        CloseSocket(this);
        TryDeinitializing(this);
    }

#if PLATFORM_WINDOWS
    explicit WHTTPSocket (SOCKET _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#else
    explicit WHTTPSocket (int32 _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#endif
    {
        ClientSocket = _Socket;
        Client = _Client;

        static TArray<FWAsyncTaskParameter*> NoParameter;
        UWScheduledAsyncTaskManager::NewScheduledAsyncTask(std::bind(&WHTTPSocket::StopSocketOperation, this), NoParameter, _TimeoutMs, false);
    }
};

struct FWHTTPClient : public FWAsyncTaskParameter
{

private:
    WHTTPSocket* ClientSocket = nullptr;

    sockaddr* Client = nullptr;

    std::string ClientIP;

    WHTTPRequestParser Parser{};

    int32 RecvBufferLen = HTTP_BUFFER_SIZE;
    ANSICHAR RecvBuffer[HTTP_BUFFER_SIZE]{};
    int32 BytesReceived{};

    WMutex bInitialized_Mutex;
    bool bInitialized = false;
    bool GetAndDeinitialize()
    {
        WScopeGuard bInitialized_Guard(&bInitialized_Mutex);
        bool bOldValue = bInitialized;
        bInitialized = false;
        return bOldValue;
    }

    void Cancel()
    {
        if (GetAndDeinitialize())
        {
            if (ClientSocket)
            {
                ClientSocket->StopSocketOperation();
            }
        }
    }

    void SendData_Internal(const std::wstring& Body, const std::string& Header, bool bCancelAfter)
    {
        std::string Response = Header + FString::WStringToString(Body);
        send(ClientSocket->ClientSocket, Response.c_str(), strlen(Response.c_str()), 0);
        if (bCancelAfter)
        {
            Cancel();
        }
    }
    void Corrupted_Internal()
    {
        static std::wstring ResponseBody = L"<html>Corrupted</html>";
        static std::string ResponseHeaders = "HTTP/1.1 400 Bad Request\r\n"
                                              "Content-Type: text/html; charset=UTF-8\r\n"
                                              "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
        SendData_Internal(ResponseBody, ResponseHeaders, true);
    }
    void SocketError_Internal()
    {
        static std::wstring ResponseBody = L"<html>Socket Error</html>";
        static std::string ResponseHeaders = "HTTP/1.1 500 Internal Server Error\r\n"
                                                     "Content-Type: text/html; charset=UTF-8\r\n"
                                                     "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
        SendData_Internal(ResponseBody, ResponseHeaders, true);
    }

    bool GetData_Internal()
    {
        if (!bInitialized) return false;

        bool HeadersReady = false;
        while (!HeadersReady)
        {
            BytesReceived = recv(ClientSocket->ClientSocket, RecvBuffer, RecvBufferLen, 0);

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
                SocketError_Internal();
                return false;
            }
        }

        if (Parser.GetMethod().rfind("HEAD", 0) == 0 || Parser.GetMethod().rfind("GET", 0) == 0) return true;

        while (true)
        {
            BytesReceived = recv(ClientSocket->ClientSocket, RecvBuffer, RecvBufferLen, 0);

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
                SocketError_Internal();
                return false;
            }
        }
    }

public:
#if PLATFORM_WINDOWS
    FWHTTPClient(SOCKET _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#else
    FWHTTPClient(int32 _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#endif
    {
        ClientSocket = new WHTTPSocket(_ClientSocket, _Client, _TimeoutInMs);
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

        auto PeerNameResult = getpeername(ClientSocket->ClientSocket, (sockaddr*)(&ClientInfo), &ClientInfoLen);
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
        return GetData_Internal();
    }

    void SendData(const std::wstring& Body, const std::string& Header)
    {
        if (!bInitialized) return;
        SendData_Internal(Body, Header, false);
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