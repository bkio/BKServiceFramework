// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPClient
#define Pragma_Once_WUDPClient

#include "WEngine.h"
#include "WTaskDefines.h"
#include "WJson.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "wsock32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
#endif

class WUDPClient : public WAsyncTaskParameter
{

private:
    FString ServerAddress;
    uint16 ServerPort = 0;

    WThread* UDPClientThread = nullptr;
    class WUDPHandler* UDPHandler = nullptr;

    struct sockaddr* SocketAddress = nullptr;
    socklen_t SocketAddressLength = 0;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    WUDPClient() = default;

    bool bClientStarted = false;

    std::function<void(class WUDPClient*, WJson::Node)> UDPListenCallback = nullptr;

    bool InitializeClient();
    void CloseSocket();
    void ListenServer();
    uint32 ServerListenerStopped();

    bool StartUDPClient(FString& _ServerAddress, uint16 _ServerPort);

public:
    static WUDPClient* NewUDPClient(FString _ServerAddress, uint16 _ServerPort, std::function<void(class WUDPClient*, WJson::Node)>& _DataReceivedCallback);

    void EndUDPClient();
    void MarkPendingKill();
    void LazySuicide();

    struct sockaddr* GetSocketAddress()
    {
        return SocketAddress;
    }

    class WUDPHandler* GetUDPHandler();
};

typedef std::function<void(class WUDPClient*, WJson::Node)> WUDPClient_DataReceived;

#endif //Pragma_Once_WUDPClient
