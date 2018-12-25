// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHTTPServer
#define Pragma_Once_BKHTTPServer

#include "BKEngine.h"
#include "../Private/BKHTTPServerHelper.h"
#include "BKThread.h"
#include "BKUtilities.h"
#include "BKJson.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>

class BKHTTPServer : public BKAsyncTaskParameter
{

public:
    bool StartSystem(uint16 Port, uint32 TimeoutMs);
    void EndSystem();

    explicit BKHTTPServer(std::function<void(BKHTTPAcceptedClient*)> Callback)
    {
        HTTPListenCallback = std::move(Callback);
    }

private:
    BKHTTPServer() = default;

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

    std::function<void(BKHTTPAcceptedClient*)> HTTPListenCallback;

    BKThread* HTTPSystemThread{};
};

#endif //Pragma_Once_BKHTTPServer