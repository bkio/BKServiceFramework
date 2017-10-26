// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPManager
#define Pragma_Once_WHTTPManager

#include "WEngine.h"
#include "../Private/WHTTPHelper.h"
#include "WThread.h"
#include "WUtilities.h"
#include "WJson.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>

class UWHTTPManager
{

public:
    static bool StartSystem(uint16 Port, uint32 TimeoutMs);
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

    WThread* HTTPSystemThread{};

    static UWHTTPManager* ManagerInstance;
    UWHTTPManager() = default;
};

#endif //Pragma_Once_WHTTPManager