// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHTTPClient
#define Pragma_Once_BKHTTPClient

#include "BKEngine.h"
#include "BKTaskDefines.h"
#include "BKHashMap.h"
#include "../Private/BKHTTPRequestParser.h"
#include "../Private/BKHTTPHelper.h"

#define DEFAULT_HTTP_REQUEST_HEADERS BKHashMap<FString, FString, BKFStringKeyHash>()
#define DEFAULT_TIMEOUT_MS 2500

class BKHTTPClient : public BKAsyncTaskParameter
{

private:
    FString ServerAddress;
    uint16 ServerPort = 80;
    BKHashMap<FString, FString, BKFStringKeyHash> Headers;
    FString Payload;
    FString RequestLine;

    BKHTTPRequestParser Parser;

    bool bRequestInitialized = false;
    BKMutex RequestMutex;

    BKMutex ReceivedDestroyApproval_Mutex;
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

    BKHTTPClient() = default;

public:
    static void NewHTTPRequest(
        const FString& _ServerAddress,
        uint16 _ServerPort,
        const FString& _Payload,
        const FString& _Verb,
        const FString& _Path,
        const BKHashMap<FString, FString, BKFStringKeyHash>& _Headers,
        uint32 _TimeoutMs,
        BKFutureAsyncTask& _RequestCallback,
        BKFutureAsyncTask& _TimeoutCallback);

    bool ProcessRequest();
    void CancelRequest();

    BKHashMap<FString, FString, BKFStringKeyHash> GetResponseHeaders()
    {
        if (!bRequestInitialized) return BKHashMap<FString, FString, BKFStringKeyHash>();
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

#endif //Pragma_Once_BKHTTPClient
