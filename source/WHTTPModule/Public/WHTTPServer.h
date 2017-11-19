// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPServer
#define Pragma_Once_WHTTPServer

#include "WEngine.h"
#include "../Private/WHTTPServerHelper.h"
#include "WThread.h"
#include "WUtilities.h"
#include "WJson.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>

class WHTTPServer : public WAsyncTaskParameter
{

public:
    bool StartSystem(uint16 Port, uint32 TimeoutMs);
    void EndSystem();

    explicit WHTTPServer(std::function<void(WHTTPAcceptedClient*)> Callback)
    {
        HTTPListenCallback = std::move(Callback);
    }

private:
    WHTTPServer() = default;

    bool bSystemStarted = false;

    struct sockaddr_in HTTPServer{};

    uint16 HTTPPort = 80;
    uint32 TimeoutInMs = 2500;

#if PLATFORM_WINDOWS
    SOCKET HTTPSocket{};
#else
    int32 HTTPSocket;
#endif

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    uint32 ListenerStopped();

    std::function<void(WHTTPAcceptedClient*)> HTTPListenCallback;

    WThread* HTTPSystemThread{};
};

#endif //Pragma_Once_WHTTPServer