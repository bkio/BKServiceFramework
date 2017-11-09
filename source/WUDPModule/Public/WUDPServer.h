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
#endif
#include "../Private/WUDPHelper.h"
#include "../Private/WUDPHandler.h"

class UWUDPServer : public UWAsyncTaskParameter
{

public:
    bool StartSystem(uint16 Port);
    void EndSystem();

    explicit UWUDPServer(std::function<void(UWUDPHandler* HandlerInstance, UWUDPTaskParameter*)> Callback)
    {
        UDPListenCallback = std::move(Callback);
    }

private:
    UWUDPServer() = default;

    bool bSystemStarted = false;

    struct sockaddr_in UDPServer{};

    WMutex SendMutex;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    UWUDPHandler* UDPHandler = nullptr;

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    uint32 ListenerStopped();
    void Send(sockaddr* Client, const FWCHARWrapper& SendBuffer);

    std::function<void(UWUDPHandler* HandlerInstance, UWUDPTaskParameter*)> UDPListenCallback = nullptr;

    WThread* UDPSystemThread = nullptr;
};

#endif //Pragma_Once_WUDPManager