// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPServer
#define Pragma_Once_WUDPServer

#include "WEngine.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif
#include "../Private/WUDPHelper.h"
#include "WUDPHandler.h"

class WUDPServer : public WAsyncTaskParameter
{

public:
    bool StartSystem(uint16 Port);
    void EndSystem();

    explicit WUDPServer(std::function<void(WUDPHandler* HandlerInstance, WUDPTaskParameter*)> Callback)
    {
        UDPListenCallback = std::move(Callback);
    }

private:
    WUDPServer() = default;

    bool bSystemStarted = false;

    struct sockaddr_in UDPServer{};

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    WUDPHandler* UDPHandler = nullptr;

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    uint32 ListenerStopped();

    std::function<void(WUDPHandler* HandlerInstance, WUDPTaskParameter*)> UDPListenCallback = nullptr;

    WThread* UDPSystemThread = nullptr;
};

#endif //Pragma_Once_WUDPServer