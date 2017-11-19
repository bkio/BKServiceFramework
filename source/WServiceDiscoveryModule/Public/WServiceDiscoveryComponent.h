// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WServiceDiscoveryComponent
#define Pragma_Once_WServiceDiscoveryComponent

#include "WEngine.h"
#include "WString.h"
#include "WJson.h"

class WServiceDiscoveryComponent
{

private:
    static WServiceDiscoveryComponent* ComponentInstance;
    static bool bComponentStarted;

    WServiceDiscoveryComponent() = default;
    ~WServiceDiscoveryComponent() = default;

    bool RegisterService();
    void UnregisterService();

    FString DiscoveryServerIP;
    uint16 DiscoveryServerPort = 0;

    FString ServiceName;
    FString ServiceIP;
    uint16 ServicePort = 0;

    class WUDPClient* ServiceDiscovery_UDPClient = nullptr;

public:
    static bool StartComponent(const FString& _DiscoveryServerIP, uint16 _DiscoveryServerPort, const FString& _ServiceName, const FString& _ServiceIP, uint16 _ServicePort);
    static void StopComponent();
};

#endif