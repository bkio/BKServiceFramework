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

class UWHTTPServer
{

public:
    static bool StartSystem(uint16 Port, uint32 TimeoutMs, std::function<void(FWHTTPAcceptedClient*)> Callback);
    static void EndSystem();

private:
    static bool bSystemStarted;

    bool StartSystem_Internal(uint16 Port, uint32 TimeoutMs);
    void EndSystem_Internal();

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

    std::function<void(FWHTTPAcceptedClient*)> HTTPListenCallback;

    WThread* HTTPSystemThread{};

    static UWHTTPServer* ManagerInstance;
    explicit UWHTTPServer(std::function<void(FWHTTPAcceptedClient*)> Callback)
    {
        HTTPListenCallback = std::move(Callback);
    }
};

#endif //Pragma_Once_WHTTPServer