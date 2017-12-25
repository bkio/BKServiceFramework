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

    WMutex bInitialMessageOperated_Mutex;
    bool bInitialMessageOperated = false;

    class WUDPClient* ComponentUDPClient = nullptr;

    std::function<void(bool)> StartComponentResultCallback;

    //Number of active clients of this service etc.
    int32 CurrentUsersNo = 0;
    WJson::Node CurrentSystemInfo;
    WMutex CurrentHeartbeatData_Mutex;
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