// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKServiceDiscoveryComponent
#define Pragma_Once_BKServiceDiscoveryComponent

#include "BKEngine.h"
#include "BKString.h"
#include "BKJson.h"

class BKServiceDiscoveryComponent
{

private:
    static BKServiceDiscoveryComponent* ComponentInstance;
    static bool bComponentStarted;

    BKServiceDiscoveryComponent() = default;
    ~BKServiceDiscoveryComponent() = default;

    void RegisterService(std::function<void(bool)> ResultCallback);
    void UnregisterService(const FString& Reason);

    FString CompileServiceInformation();
    FString CompileUnregisterServiceMessage(const FString& Reason);

    void StartHeartbeating();

    FString DiscoveryServerIP;
    uint16 DiscoveryServerPort = 0;

    FString ServiceName;
    FString ServiceIP;
    uint16 ServicePort = 0;

    BKMutex bInitialMessageOperated_Mutex;
    bool bInitialMessageOperated = false;

    class BKUDPClient* ComponentUDPClient = nullptr;

    std::function<void(bool)> StartComponentResultCallback;

    //Number of active clients of this service etc.
    int32 CurrentUsersNo = 0;
    BKJson::Node CurrentSystemInfo;
    BKMutex CurrentHeartbeatData_Mutex;
    FString CompileCurrentHeartbeatData();

    uint32 HeartbeatTaskUniqueIx = 0;

    uint32 SystemCallbackUniqueID;
    WSystemInfoCallback SystemCallback;

public:
    static void StartComponent(std::function<void(bool)> ResultCallback, const FString& _DiscoveryServerIP, uint16 _DiscoveryServerPort, const FString& _ServiceName, const FString& _ServiceIP, uint16 _ServicePort);
    static void StopComponent(const FString& Reason);

    static void SetCurrentUserNo(int32 _NewUsersNo);
};

#endif