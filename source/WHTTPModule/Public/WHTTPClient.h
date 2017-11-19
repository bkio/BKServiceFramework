// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPClient
#define Pragma_Once_WHTTPClient

#include "WEngine.h"
#include "WTaskDefines.h"
#include "../Private/WHTTPRequestParser.h"
#include "../Private/WHTTPHelper.h"
#include <map>

#define DEFAULT_HTTP_REQUEST_HEADERS std::map<std::string, std::string>()
#define DEFAULT_TIMEOUT_MS 2500

class WHTTPClient : public WAsyncTaskParameter
{

private:
    std::string ServerAddress;
    uint16 ServerPort = 80;
    std::map<std::string, std::string> Headers{};
    std::wstring Payload;
    std::string RequestLine;

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
        std::string _ServerAddress,
        uint16 _ServerPort,
        std::wstring _Payload,
        std::string _Verb,
        std::string _Path,
        std::map<std::string, std::string> _Headers,
        uint32 _TimeoutMs,
        WFutureAsyncTask& _RequestCallback,
        WFutureAsyncTask& _TimeoutCallback);

    bool ProcessRequest();
    void CancelRequest();

    std::map<std::string, std::string> GetResponseHeaders()
    {
        if (!bRequestInitialized) return std::map<std::string, std::string>();
        return Parser.GetHeaders();
    };

    std::wstring GetResponsePayload()
    {
        if (!bRequestInitialized) return L"";
        return Parser.GetPayload();
    }

    bool DestroyApproval();

    void SendData();
    bool ReceiveData();
};

#endif //Pragma_Once_WHTTPClient
