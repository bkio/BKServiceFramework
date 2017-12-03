// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPClient
#define Pragma_Once_WHTTPClient

#include "WEngine.h"
#include "WTaskDefines.h"
#include "../Private/WHTTPRequestParser.h"
#include "../Private/WHTTPHelper.h"
#include <map>

#define DEFAULT_HTTP_REQUEST_HEADERS std::map<FString, FString>()
#define DEFAULT_TIMEOUT_MS 2500

class WHTTPClient : public WAsyncTaskParameter
{

private:
    FString ServerAddress;
    uint16 ServerPort = 80;
    std::map<FString, FString> Headers{};
    FString Payload;
    FString RequestLine;

    WHTTPRequestParser Parser;

    bool bRequestInitialized = false;
    WMutex RequestMutex;

    WMutex ReceivedDestroyApproval_Mutex;
    int32 ReceivedDestroyApproval = 0;

    bool InitializeSocket();
    void CloseSocket();
#if PLATFORM_WINDOWS
    SOCKET HTTPSocket{};
#else
    int32 HTTPSocket;
#endif
    bool bSocketClosed = false;

    int32 RecvBufferLen = HTTP_BUFFER_SIZE;
    ANSICHAR RecvBuffer[HTTP_BUFFER_SIZE]{};
    int32 BytesReceived{};

    WHTTPClient() = default;

public:
    static void NewHTTPRequest(
        const FString& _ServerAddress,
        uint16 _ServerPort,
        const FString& _Payload,
        const FString& _Verb,
        const FString& _Path,
        std::map<FString, FString> _Headers,
        uint32 _TimeoutMs,
        WFutureAsyncTask& _RequestCallback,
        WFutureAsyncTask& _TimeoutCallback);

    bool ProcessRequest();
    void CancelRequest();

    std::map<FString, FString> GetResponseHeaders()
    {
        if (!bRequestInitialized) return std::map<FString, FString>();
        return Parser.GetHeaders();
    };

    FString GetResponsePayload()
    {
        if (!bRequestInitialized) return FString(L"");
        return Parser.GetPayload();
    }

    bool DestroyApproval();

    void SendData();
    bool ReceiveData();
};

#endif //Pragma_Once_WHTTPClient
