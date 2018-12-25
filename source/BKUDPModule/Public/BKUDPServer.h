// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKUDPServer
#define Pragma_Once_BKUDPServer

#include "BKEngine.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif
#include "../Private/BKUDPHelper.h"
#include "BKUDPHandler.h"

class BKUDPServer : public BKAsyncTaskParameter
{

public:
    bool StartSystem(uint16 Port);
    void EndSystem();

    explicit BKUDPServer(std::function<void(BKUDPHandler* HandlerInstance, WUDPTaskParameter*)> Callback)
    {
        UDPListenCallback = std::move(Callback);
    }

private:
    BKUDPServer() = default;

    bool bSystemStarted = false;

    struct sockaddr_in UDPServer{};

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    BKUDPHandler* UDPHandler = nullptr;

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    uint32 ListenerStopped();

    std::function<void(BKUDPHandler* HandlerInstance, WUDPTaskParameter*)> UDPListenCallback = nullptr;

    BKThread* UDPSystemThread = nullptr;
};

#endif //Pragma_Once_BKUDPServer