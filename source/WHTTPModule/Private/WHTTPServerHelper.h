// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPServerHelper
#define Pragma_Once_WHTTPServerHelper

#include "WEngine.h"
#include "WHTTPRequestParser.h"
#include "WScheduledTaskManager.h"
#include "WHTTPHelper.h"

struct WHTTPAcceptedSocket
{

private:
    WHTTPAcceptedSocket() = default;

    WMutex HTTPSocket_Mutex{};

    int32 DeinitializationApproval = 0; //Must be 2 to deallocate this.

    bool bSocketOperational = true;

    //Step 1
    static void CloseSocket(WHTTPAcceptedSocket* _Socket)
    {
        if (!_Socket) return;

        if (_Socket->bSocketOperational)
        {
            _Socket->bSocketOperational = false;
#if PLATFORM_WINDOWS
            closesocket(_Socket->ClientSocket);
            shutdown(_Socket->ClientSocket, SD_BOTH);
#else
            close(_Socket->ClientSocket);
            shutdown(_Socket->ClientSocket, SHUT_RDWR);
#endif
        }
    }
    //Step 2
    static void TryDeinitializing(WHTTPAcceptedSocket* _Socket)
    {
        if (!_Socket) return;

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

    void Send(std::string& Response)
    {
        WScopeGuard SendData_Guard(&HTTPSocket_Mutex);
        if (!bSocketOperational) return;
#if PLATFORM_WINDOWS
        send(ClientSocket, Response.c_str(), strlen(Response.c_str()), 0);
#else
        send(ClientSocket, Response.c_str(), strlen(Response.c_str()), MSG_NOSIGNAL);
#endif
    }

#if PLATFORM_WINDOWS
    explicit WHTTPAcceptedSocket (SOCKET _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#else
    explicit WHTTPAcceptedSocket (int32 _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#endif
    {
        ClientSocket = _Socket;
        Client = _Client;

        static TArray<UWAsyncTaskParameter*> NoParameter;
        UWScheduledAsyncTaskManager::NewScheduledAsyncTask(std::bind(&WHTTPAcceptedSocket::StopSocketOperation, this), NoParameter, _TimeoutMs, false);
    }
};

class UWHTTPAcceptedClient : public UWAsyncTaskParameter
{

private:
    WHTTPAcceptedSocket* ClientSocket = nullptr;

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
        ClientSocket->Send(Response);
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
        Cancel();
    }

    bool GetData_Internal()
    {
        if (!bInitialized) return false;

        bool HeadersReady = false;
        bool BodyReady = false;
        while (!HeadersReady)
        {
            BytesReceived = static_cast<int32>(recv(ClientSocket->ClientSocket, RecvBuffer, RecvBufferLen, 0));

            if (!bInitialized) return false;

            if (BytesReceived > 0)
            {
                Parser.ProcessChunkForHeaders(RecvBuffer, BytesReceived);

                HeadersReady = Parser.AllHeadersAvailable();
                BodyReady = Parser.AllBodyAvailable();
            }
            else
            {
                SocketError_Internal();
                return false;
            }
        }

        if (Parser.GetMethod().rfind("HEAD", 0) == 0 || Parser.GetMethod().rfind("GET", 0) == 0) return true;

        while (!BodyReady)
        {
            BytesReceived = recv(ClientSocket->ClientSocket, RecvBuffer, RecvBufferLen, 0);

            if (!bInitialized) return false;

            if (BytesReceived > 0)
            {
                Parser.ProcessChunkForBody(RecvBuffer, BytesReceived);
                if (Parser.ErrorOccuredInBodyParsing())
                {
                    Corrupted_Internal();
                    return false;
                }
                if (Parser.AllBodyAvailable())
                {
                    BodyReady = true;
                }
            }
            else if (BytesReceived < 0)
            {
                SocketError_Internal();
                return false;
            }
        }
    }

public:
#if PLATFORM_WINDOWS
    UWHTTPAcceptedClient(SOCKET _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#else
    UWHTTPAcceptedClient(int32 _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#endif
    {
        ClientSocket = new WHTTPAcceptedSocket(_ClientSocket, _Client, _TimeoutInMs);
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

        return true;
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

#endif //Pragma_Once_WHTTPServerHelper