// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHTTPServerHelper
#define Pragma_Once_BKHTTPServerHelper

#include "BKEngine.h"
#include "BKHTTPRequestParser.h"
#include "BKScheduledTaskManager.h"
#include "BKHTTPHelper.h"
#include "BKTuple.h"
#include "BKSharedPtr.h"

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
            _Socket->HTTPSocket_Mutex.Unlock();
            delete (_Socket);
        }
        else
        {
            _Socket->HTTPSocket_Mutex.Unlock();
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
        HTTPSocket_Mutex.Lock(); //Will be unlocked in private static functions.
        CloseSocket(this);
        TryDeinitializing(this);
    }

    void Send(FString& Response)
    {
        BKScopeGuard SendData_Guard(&HTTPSocket_Mutex);
        if (!bSocketOperational) return;
        std::string ResponseString = Response.GetAnsiCharString();
#if PLATFORM_WINDOWS
        send(ClientSocket, ResponseString.c_str(), ResponseString.size(), 0);
#else
        send(ClientSocket, ResponseString.c_str(), ResponseString.size(), MSG_NOSIGNAL);
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

    void Finalize()
    {
        Cancel();
    }

    void SendData(const FString& Body, const FString& Header, bool bCancelAfter = false)
    {
        FString Response = Header + Body;
        ClientSocket->Send(Response);
        if (bCancelAfter)
        {
            Cancel();
        }
    }
    static FString CorruptedResponseBody;
    static FString CorruptedResponseHeaders;
    void Corrupted_Internal()
    {
        SendData(CorruptedResponseBody, CorruptedResponseHeaders, true);
    }
    void SocketError_Internal()
    {
        Cancel();
    }

    //@return: If succeed true, otherwise false.
    bool GetData()
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
                FString PreParseString = FString(RecvBuffer, BytesReceived);

                Parser.ProcessChunkForHeaders(PreParseString);

                HeadersReady = Parser.AllHeadersAvailable();
                BodyReady = Parser.AllBodyAvailable();
            }
            else
            {
                SocketError_Internal();
                return false;
            }
        }

        if (Parser.GetMethod().RightFind(L"HEAD", 0) == 0 || Parser.GetMethod().RightFind(L"GET", 0) == 0) return true;

        while (!BodyReady)
        {
            BytesReceived = recv(ClientSocket->ClientSocket, RecvBuffer, RecvBufferLen, 0);

            if (!bInitialized) return false;

            if (BytesReceived > 0)
            {
                FString PreParseString = FString(RecvBuffer, BytesReceived);

                Parser.ProcessChunkForBody(PreParseString);
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
        return true;
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

    //Uniquely
    bool SetResponseHeader_Internal(const FString& _HeaderKey, const FString& _HeaderValue, bool _bAssertReservedsNotUsed = true)
    {
        //Case insensitive, can have multiple headers with same key
        FString CaseLoweredKey = _HeaderKey.ToLower();

        if (_bAssertReservedsNotUsed)
        {
            assert(CaseLoweredKey != FString(L"content-type"));
            assert(CaseLoweredKey != FString(L"content-length"));
            assert(CaseLoweredKey != FString(L"set-cookie"));
        }

        bool bFound = false;
        for (int i = 0; i < ResponseHeaders.Num(); i++)
        {
            if (ResponseHeaders[i].IsValid())
            {
                if (ResponseHeaders[i]->Item1.ToLower() == CaseLoweredKey)
                {
                    bFound = true;
                    break;
                }
            }
        }

        if (!bFound)
        {
            ResponseHeaders.Add(TSharedPtr<BKTuple_Two<FString, FString>>(new BKTuple_Two<FString, FString>(CaseLoweredKey, _HeaderValue)));
            return true;
        }
        return false;
    }

    void AddResponseHeader_Internal(const FString& _HeaderKey, const FString& _HeaderValue, bool _bAssertReservedsNotUsed = true)
    {
        //Case insensitive, can have multiple headers with same key
        FString CaseLoweredKey = _HeaderKey.ToLower();

        if (_bAssertReservedsNotUsed)
        {
            assert(CaseLoweredKey != FString(L"content-type"));
            assert(CaseLoweredKey != FString(L"content-length"));
            assert(CaseLoweredKey != FString(L"set-cookie"));
        }

        ResponseHeaders.Add(TSharedPtr<BKTuple_Two<FString, FString>>(new BKTuple_Two<FString, FString>(CaseLoweredKey, _HeaderValue)));
    }

    friend class BKHTTPServer;

    //Response variables
    FString ResponseBody;
    FString ResponseContentType = FString(L"text/html");
    int ResponseCode = 200;
    FString ResponseCodeDescription = FString(L"OK");
    TArray<TSharedPtr<BKTuple_Two<FString, FString>>> ResponseHeaders;
    TArray<TSharedPtr<BKTuple_Two<FString, FString>>> ResponseCookies;

    static std::map<int32, FString> HttpCodeDescriptionMap;

public:
#if PLATFORM_WINDOWS
    BKHTTPAcceptedClient(SOCKET _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#else
    BKHTTPAcceptedClient(int32 _ClientSocket, sockaddr* _Client, uint32 _TimeoutInMs)
#endif
    {
        ClientSocket = new BKHTTPAcceptedSocket(_ClientSocket, _Client, _TimeoutInMs);
    }

    void SetResponseBody(const FString& _Body)
    {
        ResponseBody = _Body;
    }

    void SetResponseCode(int _Code)
    {
        ResponseCode = _Code;
        ResponseCodeDescription = GetResponseCodeDescription(ResponseCode);
    }

    void SetResponseContentType(const FString& _ContentType)
    {
        ResponseContentType = _ContentType;
    }

    void AddResponseHeader(const FString& _HeaderKey, const FString& _HeaderValue)
    {
        AddResponseHeader_Internal(_HeaderKey, _HeaderValue);
    }

    //Uniquely
    bool SetResponseHeader(const FString& _HeaderKey, const FString& _HeaderValue)
    {
        return SetResponseHeader_Internal(_HeaderKey, _HeaderValue);
    }

    bool SetResponseCookie(const FString& _CookieKey, const FString& _CookieValue)
    {
        //Case sensitive, cannot have multiple cookies with same key

        bool bFound = false;
        for (int i = 0; i < ResponseCookies.Num(); i++)
        {
            if (ResponseCookies[i].IsValid())
            {
                if (ResponseCookies[i]->Item1 == _CookieKey)
                {
                    bFound = true;
                    break;
                }
            }
        }

        if (!bFound)
        {
            ResponseCookies.Add(TSharedPtr<BKTuple_Two<FString, FString>>(new BKTuple_Two<FString, FString>(_CookieKey, _CookieValue)));
            return true;
        }
        return false;
    }

    FString GetRequestClientIP()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return ClientIP;
    }

    FString GetRequestMethod()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return Parser.GetMethod();
    }

    FString GetRequestPath()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return Parser.GetPath();
    }

    FString GetRequestProtocol()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return Parser.GetProtocol();
    }

    BKHashMap<FString, FString, BKFStringKeyHash> GetRequestHeaders()
    {
        if (!bInitialized) return BKHashMap<FString, FString, BKFStringKeyHash>();
        return Parser.GetHeaders();
    };

    FString GetRequestBody()
    {
        if (!bInitialized) return EMPTY_FSTRING_UTF8;
        return Parser.GetPayload();
    }

    FString GetResponseCodeDescription(int32 Code)
    {
        if (HttpCodeDescriptionMap.find(Code) != HttpCodeDescriptionMap.end())
        {
            return FString(HttpCodeDescriptionMap[Code]);
        }
        return FString();
    }
};
std::map<int32, FString> BKHTTPAcceptedClient::HttpCodeDescriptionMap =
{
    {100, FString(L"Continue")},
    {101, FString(L"Switching Protocols")},
    {200, FString(L"OK")},
    {201, FString(L"Created")},
    {202, FString(L"Accepted")},
    {203, FString(L"Non-Authoritative Information")},
    {204, FString(L"No Content")},
    {205, FString(L"Reset Content")},
    {206, FString(L"Partial Content")},
    {300, FString(L"Multiple Choices")},
    {301, FString(L"Moved Permanently")},
    {302, FString(L"Found")},
    {303, FString(L"See Other")},
    {304, FString(L"Not Modified")},
    {305, FString(L"Use Proxy")},
    {307, FString(L"Temporary Redirect")},
    {400, FString(L"Bad Request")},
    {401, FString(L"Unauthorized")},
    {402, FString(L"Payment Required")},
    {403, FString(L"Forbidden")},
    {404, FString(L"Not Found")},
    {405, FString(L"Method Not Allowed")},
    {406, FString(L"Not Acceptable")},
    {407, FString(L"Proxy Authentication Required")},
    {408, FString(L"Request Time-out")},
    {409, FString(L"Conflict")},
    {410, FString(L"Gone")},
    {411, FString(L"Length Required")},
    {412, FString(L"Precondition Failed")},
    {413, FString(L"Request Entity Too Large")},
    {414, FString(L"Request-URI Too Large")},
    {415, FString(L"Unsupported Media Type")},
    {416, FString(L"Requested range not satisfiable")},
    {417, FString(L"Expectation Failed")},
    {500, FString(L"Internal Server Error")},
    {501, FString(L"Not Implemented")},
    {502, FString(L"Bad Gateway")},
    {503, FString(L"Service Unavailable")},
    {504, FString(L"Gateway Time-out")},
    {505, FString(L"HTTP Version not supported")}
};

FString BKHTTPAcceptedClient::CorruptedResponseBody = FString(L"<html>Corrupted</html>");
FString BKHTTPAcceptedClient::CorruptedResponseHeaders(FString(L"HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: 22 \r\n\r\n"));

#endif //Pragma_Once_BKHTTPServerHelper