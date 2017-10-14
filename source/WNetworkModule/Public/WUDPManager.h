// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPManager
#define Pragma_Once_WUDPManager

#include "WEngine.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif
#include "WThread.h"
#include "WUtilities.h"
#include <iostream>
#include <memory>
#include <WAsyncTaskManager.h>

struct FWUDPTaskParameter : public FWAsyncTaskParameter
{

public:
    int32 BufferSize = 0;
    ANSICHAR* Buffer = nullptr;
    sockaddr* Client = nullptr;

    FWUDPTaskParameter(int32 BufferSizeParameter, ANSICHAR* BufferParameter, sockaddr* ClientParameter)
    {
        BufferSize = BufferSizeParameter;
        Buffer = BufferParameter;
        Client = ClientParameter;
    }
    ~FWUDPTaskParameter()
    {
        if (Client)
        {
            delete (Client);
        }
        delete[] Buffer;
    }
};

class UWUDPManager
{

public:
    static bool StartSystem(uint16 Port);
    static void EndSystem();

private:
    static bool bSystemStarted;

    bool StartSystem_Internal(uint16 Port);
    void EndSystem_Internal();

    struct sockaddr_in UDPServer;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket;
#else
    int32 UDPSocket;
#endif

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    void Send(sockaddr* Client, FWCHARWrapper&& SendBuffer);

    WThread* UDPSystemThread;

    int32 WorkerThreadCount;

    static UWUDPManager* ManagerInstance;
    UWUDPManager() = default;
};

#endif