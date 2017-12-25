// Copyright Pagansoft.com, All rights reserved.

#include "WSystemManager.h"
#include "WScheduledTaskManager.h"
#include "WServiceDiscoveryComponent.h"
#include "WUDPClient.h"
#include "WUDPHandler.h"

WServiceDiscoveryComponent* WServiceDiscoveryComponent::ComponentInstance = nullptr;
bool WServiceDiscoveryComponent::bComponentStarted = false;

void WServiceDiscoveryComponent::StartComponent(std::function<void(bool)> ResultCallback, const FString& _DiscoveryServerIP, uint16 _DiscoveryServerPort, const FString& _ServiceName, const FString& _ServiceIP, uint16 _ServicePort)
{
    if (bComponentStarted)
    {
        if (ResultCallback)
        {
            ResultCallback(false);
        }
        return;
    }
    bComponentStarted = true;

    ComponentInstance = new WServiceDiscoveryComponent();
    ComponentInstance->DiscoveryServerIP = _DiscoveryServerIP;
    ComponentInstance->DiscoveryServerPort = _DiscoveryServerPort;
    ComponentInstance->ServiceName = _ServiceName;
    ComponentInstance->ServiceIP = _ServiceIP;
    ComponentInstance->ServicePort = _ServicePort;

    ComponentInstance->StartComponentResultCallback = std::move(ResultCallback);

    ComponentInstance->RegisterService([](bool bSuccess)
    {
        if (!ComponentInstance)
        {
            StopComponent(FString("Initialization Failure"));
            return;
        }
        if (bSuccess)
        {
            ComponentInstance->StartHeartbeating();
            if (ComponentInstance->StartComponentResultCallback)
                ComponentInstance->StartComponentResultCallback(true);
        }
        else
        {
            StopComponent(FString("Initialization Failure"));
            if (ComponentInstance->StartComponentResultCallback)
                ComponentInstance->StartComponentResultCallback(false);
        }
        ComponentInstance->StartComponentResultCallback = nullptr;
    });
}
void WServiceDiscoveryComponent::StopComponent(const FString& Reason)
{
    if (!bComponentStarted) return;
    bComponentStarted = false;

    if (ComponentInstance)
    {
        ComponentInstance->UnregisterService(Reason);

        delete (ComponentInstance);
        ComponentInstance = nullptr;
    }
}

void WServiceDiscoveryComponent::RegisterService(const std::function<void(bool)> ResultCallback)
{
    SystemCallback = [](class WSystemInfo* CurrentInfo)
    {
        if (bComponentStarted && ComponentInstance && CurrentInfo)
        {
            WScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);
            ComponentInstance->CurrentSystemInfo = CurrentInfo->ToJsonObject();
        }
    };
    WSystemManager::AddCallback(SystemCallbackUniqueID, SystemCallback);

    WUDPClient_DataReceived DataReceivedLambda = [ResultCallback](WUDPClient* UDPClient, WJson::Node Parameter)
    {
        if (ComponentInstance)
        {
            WScopeGuard LocalGuard(&ComponentInstance->bInitialMessageOperated_Mutex);
            if (bComponentStarted && ComponentInstance && !ComponentInstance->bInitialMessageOperated)
            {
                ComponentInstance->bInitialMessageOperated = true;
                if (ResultCallback)
                {
                    ResultCallback(true);
                }
            }
        }
    };
    ComponentUDPClient = WUDPClient::NewUDPClient(DiscoveryServerIP, DiscoveryServerPort, DataReceivedLambda);
    if (ComponentUDPClient && ComponentUDPClient->GetUDPHandler())
    {
        WJson::Node DataToSend = WJson::Node(WJson::Node::T_OBJECT);
        DataToSend.Add(FString("CharArray"), WJson::Node(CompileServiceInformation()));

        FWCHARWrapper WrappedData = ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

        ComponentUDPClient->GetUDPHandler()->Send(ComponentUDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();

        static TArray<WAsyncTaskParameter*> NoParameter;
        WFutureAsyncTask Lambda = [ResultCallback](TArray<WAsyncTaskParameter*> TaskParameters)
        {
            if (ComponentInstance)
            {
                WScopeGuard LocalGuard(&ComponentInstance->bInitialMessageOperated_Mutex);
                if (bComponentStarted && ComponentInstance && !ComponentInstance->bInitialMessageOperated)
                {
                    ComponentInstance->bInitialMessageOperated = true;
                    if (ResultCallback)
                    {
                        ResultCallback(false);
                    }
                }
            }
        };
        WScheduledAsyncTaskManager::NewScheduledAsyncTask(Lambda, NoParameter, 5000, false);
    }
    else
    {
        ResultCallback(false);
    }
}
void WServiceDiscoveryComponent::UnregisterService(const FString& Reason)
{
    WSystemManager::RemoveCallback(SystemCallbackUniqueID, SystemCallback);
    WScheduledAsyncTaskManager::CancelScheduledAsyncTask(HeartbeatTaskUniqueIx);

    if (ComponentUDPClient && ComponentUDPClient->GetUDPHandler())
    {
        WJson::Node DataToSend = WJson::Node(WJson::Node::T_OBJECT);
        DataToSend.Add(FString("CharArray"), WJson::Node(CompileUnregisterServiceMessage(Reason)));

        FWCHARWrapper WrappedData = ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

        ComponentUDPClient->GetUDPHandler()->Send(ComponentUDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();
    }
}

FString WServiceDiscoveryComponent::CompileServiceInformation()
{
    WJson::Node ResultObject = WJson::Node(WJson::Node::T_OBJECT);
    ResultObject.Add(FString("ServiceName"), WJson::Node(ServiceName));
    ResultObject.Add(FString("ServiceIP"), WJson::Node(ServiceIP));
    ResultObject.Add(FString("ServicePort"), WJson::Node((int32)ServicePort));

    WJson::Writer Writer;
    return Writer.WriteString(ResultObject);
}
FString WServiceDiscoveryComponent::CompileUnregisterServiceMessage(const FString& Reason)
{
    WJson::Node ResultObject = WJson::Node(WJson::Node::T_OBJECT);
    ResultObject.Add(FString("Status"), WJson::Node(FString("Unregister")));
    ResultObject.Add(FString("Reason"), WJson::Node(Reason));

    WJson::Writer Writer;
    return Writer.WriteString(ResultObject);
}

void WServiceDiscoveryComponent::StartHeartbeating()
{
    static TArray<WAsyncTaskParameter*> NoParameter;
    WFutureAsyncTask Lambda = [](TArray<WAsyncTaskParameter*> TaskParameters)
    {
        if (bComponentStarted && ComponentInstance && ComponentInstance->ComponentUDPClient && ComponentInstance->ComponentUDPClient->GetUDPHandler())
        {
            FString CompiledHeartbeatData = ComponentInstance->CompileCurrentHeartbeatData();
            if (CompiledHeartbeatData.Len() > 0)
            {
                WJson::Node DataToSend = WJson::Node(WJson::Node::T_OBJECT);
                DataToSend.Add(FString("CharArray"), WJson::Node(CompiledHeartbeatData));

                FWCHARWrapper WrappedData = ComponentInstance->ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentInstance->ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

                ComponentInstance->ComponentUDPClient->GetUDPHandler()->Send(ComponentInstance->ComponentUDPClient->GetSocketAddress(), WrappedData);

                WrappedData.DeallocateValue();
            }
        }
        else if (bComponentStarted && ComponentInstance)
        {
            StopComponent(FString("UDP Failure"));
            StartComponent([](bool bResult){}, ComponentInstance->DiscoveryServerIP, ComponentInstance->DiscoveryServerPort, ComponentInstance->ServiceName, ComponentInstance->ServiceIP, ComponentInstance->ServicePort);
        }
    };
    HeartbeatTaskUniqueIx = WScheduledAsyncTaskManager::NewScheduledAsyncTask(Lambda, NoParameter, 1000, true);
}

void WServiceDiscoveryComponent::SetCurrentUserNo(int32 _NewUsersNo)
{
    WScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);
    ComponentInstance->CurrentUsersNo = _NewUsersNo;
}

FString WServiceDiscoveryComponent::CompileCurrentHeartbeatData()
{
    if (bComponentStarted && CurrentSystemInfo.GetType() != WJson::Node::Type::T_INVALID)
    {
        WScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);

        WJson::Node ResultObject = WJson::Node(WJson::Node::T_OBJECT);
        ResultObject.Add(FString("CurrentUsersNo"), WJson::Node(CurrentUsersNo));
        ResultObject.Add(FString("CurrentSystemInfo"), CurrentSystemInfo);

        WJson::Writer Writer;
        return Writer.WriteString(ResultObject);
    }
    return FString("");
}