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

typedef std::function<void(sockaddr*, class UWUDPHandler*, WJson::Node)> WUDPClient_DataReceived;

class UWUDPClient : public UWAsyncTaskParameter
{

private:
    std::string ServerAddress;
    uint16 ServerPort = 0;

    WThread* UDPClientThread = nullptr;
    class UWUDPHandler* UDPHandler = nullptr;

    struct sockaddr* SocketAddress = nullptr;
    socklen_t SocketAddressLength = 0;

    WMutex SendMutex;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    UWUDPClient() = default;

    bool bClientStarted = false;

    WUDPClient_DataReceived UDPListenCallback = nullptr;

    bool InitializeClient();
    void CloseSocket();
    void ListenServer();
    uint32 ServerListenerStopped();

    bool StartUDPClient(std::string& _ServerAddress, uint16 _ServerPort);

public:
    static UWUDPClient* NewUDPClient(std::string _ServerAddress, uint16 _ServerPort, WUDPClient_DataReceived& _DataReceivedCallback);

    void EndUDPClient();

    class UWUDPHandler* GetUDPHandler();
};

#endif //Pragma_Once_WUDPClient
