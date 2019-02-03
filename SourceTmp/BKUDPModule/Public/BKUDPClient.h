// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKUDPClient
#define Pragma_Once_BKUDPClient

#include "BKEngine.h"
#include "BKTaskDefines.h"
#include "BKJson.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "wsock32.lib")
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
#endif

class BKUDPClient : public BKAsyncTaskParameter
{

private:
    FString ServerAddress;
    uint16 ServerPort = 0;

    BKThread* UDPClientThread = nullptr;
    class BKUDPHandler* UDPHandler = nullptr;

    struct sockaddr* SocketAddress = nullptr;
    socklen_t SocketAddressLength = 0;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket{};
#else
    int32 UDPSocket{};
#endif

    BKUDPClient() = default;

    bool bClientStarted = false;

    std::function<void(class BKUDPClient*, BKJson::Node)> UDPListenCallback = nullptr;

    bool InitializeClient();
    void CloseSocket();
    void ListenServer();
    uint32 ServerListenerStopped();

    bool StartUDPClient(FString& _ServerAddress, uint16 _ServerPort);

public:
    static BKUDPClient* NewUDPClient(FString _ServerAddress, uint16 _ServerPort, std::function<void(class BKUDPClient*, BKJson::Node)>& _DataReceivedCallback);

    void EndUDPClient();
    void MarkPendingKill();
    void LazySuicide();

    struct sockaddr* GetSocketAddress()
    {
        return SocketAddress;
    }

    class BKUDPHandler* GetUDPHandler();
};

typedef std::function<void(class BKUDPClient*, BKJson::Node)> BKUDPClient_DataReceived;

#endif //Pragma_Once_BKUDPClient
