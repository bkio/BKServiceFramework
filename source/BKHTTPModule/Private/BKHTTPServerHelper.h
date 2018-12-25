// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHTTPServerHelper
#define Pragma_Once_BKHTTPServerHelper

#include "BKEngine.h"
#include "BKHTTPRequestParser.h"
#include "BKScheduledTaskManager.h"
#include "BKHTTPHelper.h"

struct BKHTTPAcceptedSocket
{

private:
    BKHTTPAcceptedSocket() = default;

    BKMutex HTTPSocket_Mutex{};

    int32 DeinitializationApproval = 0; //Must be 2 to deallocate this.

    bool bSocketOperational = true;

    //Step 1
    static void CloseSocket(BKHTTPAcceptedSocket* _Socket)
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
    static void TryDeinitializing(BKHTTPAcceptedSocket* _Socket)
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

    void Send(FString& Response)
    {
        BKScopeGuard SendData_Guard(&HTTPSocket_Mutex);
        if (!bSocketOperational) return;
#if PLATFORM_WINDOWS
        send(ClientSocket, Response.GetAnsiCharArray(), strlen(Response.GetAnsiCharArray()), 0);
#else
        send(ClientSocket, Response.GetAnsiCharArray(), strlen(Response.GetAnsiCharArray()), MSG_NOSIGNAL);
#endif
    }

#if PLATFORM_WINDOWS
    explicit BKHTTPAcceptedSocket (SOCKET _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#else
    explicit BKHTTPAcceptedSocket (int32 _Socket, sockaddr* _Client, uint32 _TimeoutMs)
#endif
    {
        ClientSocket = _Socket;
        Client = _Client;

        static TArray<BKAsyncTaskParameter*> NoParameter;
        BKScheduledAsyncTaskManager::NewScheduledAsyncTask(std::bind(&BKHTTPAcceptedSocket::StopSocketOperation, this), NoParameter, _TimeoutMs, false);
    }
};

class BKHTTPAcceptedClient : public BKAsyncTaskParameter
{

private:
    BKHTTPAcceptedSocket* ClientSocket = nullptr;

    sockaddr* Client = nullptr;

    FString ClientIP;

    BKHTTPRequestParser Parser{};

    int32 RecvBufferLen = HTTP_BUFFER_SIZE;
    ANSICHAR RecvBuffer[HTTP_BUFFER_SIZE]{};
    int32 BytesReceived{};

    BKMutex bInitialized_Mutex;
    bool bInitialized = false;
    bool GetAndDeinitialize()
    {
        BKScopeGuard bInitialized_Guard(&bInitialized_Mutex);
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

    void SendData_Internal(const FString& Body, const FString& Header, bool bCancelAfter)
    {
        FString Response = Header + FString::WStringToString(Body);
        ClientSocket->Send(Response);
        if (bCancelAfter)
        {
            Cancel();
        }
    }
    void Corrupted_Internal()
    {
        static FString ResponseBody(L"<html>Corrupted</html>");
        static FString ResponseHeaders("HTTP/1.1 400 Bad Request\r\n"
                                              "Content-Type: text/html; charset=UTF-8\r\n"
                                              "Content-Length: " + std::to_string(ResponseBody.Len()) + "\r\n\r\n");
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

        if (Parser.GetMethod().RightFind("HEAD", 0) == 0 || Parser.GetMethod().RightFind("GET", 0) == 0) return true;

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
    BKHTTPAcceptedClient(SOCKET _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#else
    BKHTTPAcceptedClient(int32 _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#endif
    {
        ClientSocket = new BKHTTPAcceptedSocket(_ClientSocket, _Client, _TimeoutInMs);
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

    void SendData(const FString& Body, const FString& Header)
    {
        if (!bInitialized) return;
        SendData_Internal(Body, Header, false);
    }

    void Finalize()
    {
        Cancel();
    }

    FString GetClientIP()
    {
        if (!bInitialized) return EMPTY_FSTRING_ANSI;
        return ClientIP;
    }

    FString GetMethod()
    {
        if (!bInitialized) return EMPTY_FSTRING_ANSI;
        return Parser.GetMethod();
    }

    FString GetPath()
    {
        if (!bInitialized) return EMPTY_FSTRING_ANSI;
        return Parser.GetPath();
    }

    FString GetProtocol()
    {
        if (!bInitialized) return EMPTY_FSTRING_ANSI;
        return Parser.GetProtocol();
    }

    std::map<FString, FString> GetHeaders()
    {
        if (!bInitialized) return std::map<FString, FString>();
        return Parser.GetHeaders();
    };

    FString GetPayload()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return Parser.GetPayload();
    }
};

#endif //Pragma_Once_BKHTTPServerHelper