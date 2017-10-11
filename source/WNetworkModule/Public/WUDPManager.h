// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPManager
#define Pragma_Once_WUDPManager

#include "WEngine.h"
#if PLATFORM_WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
#endif
#include "WThread.h"
#include "WUtilities.h"
#include <iostream>
#include <memory>

class UWUDPManager
{

public:
    static bool StartSystem(int32 Port);
    static void EndSystem();

private:
    static bool bSystemStarted;

    bool StartSystem_Internal(int32 Port);
    void EndSystem_Internal();

    struct sockaddr_in UDPServer;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket;
#else
    int32 UDPSocket;
#endif

    bool InitializeSocket(int32 Port);
    void CloseSocket();
    void ListenSocket();
    void Send(std::shared_ptr<sockaddr_in> Client, const FWCHARWrapper& SendBuffer);

    WThread* UDPSystemThread;

    int32 WorkerThreadCount;

    static UWUDPManager* ManagerInstance;
    UWUDPManager() {}
};

#endif