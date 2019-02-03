// Copyright Burak Kara, All rights reserved.

#include "BKSystemManager.h"
#include "BKScheduledTaskManager.h"
#include "BKServiceDiscoveryComponent.h"
#include "BKUDPClient.h"
#include "BKUDPHandler.h"

BKServiceDiscoveryComponent* BKServiceDiscoveryComponent::ComponentInstance = nullptr;
bool BKServiceDiscoveryComponent::bComponentStarted = false;

void BKServiceDiscoveryComponent::StartComponent(std::function<void(bool)> ResultCallback, const FString& _DiscoveryServerIP, uint16 _DiscoveryServerPort, const FString& _ServiceName, const FString& _ServiceIP, uint16 _ServicePort)
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

    ComponentInstance = new BKServiceDiscoveryComponent();
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
            StopComponent(FString(L"Initialization Failure"));
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
            StopComponent(FString(L"Initialization Failure"));
            if (ComponentInstance->StartComponentResultCallback)
                ComponentInstance->StartComponentResultCallback(false);
        }
        ComponentInstance->StartComponentResultCallback = nullptr;
    });
}
void BKServiceDiscoveryComponent::StopComponent(const FString& Reason)
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

void BKServiceDiscoveryComponent::RegisterService(const std::function<void(bool)> ResultCallback)
{
    SystemCallback = [](class BKSystemInfo* CurrentInfo)
    {
        if (bComponentStarted && ComponentInstance && CurrentInfo)
        {
            BKScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);
            ComponentInstance->CurrentSystemInfo = CurrentInfo->ToJsonObject();
        }
    };
    BKSystemManager::AddCallback(SystemCallbackUniqueID, SystemCallback);

    BKUDPClient_DataReceived DataReceivedLambda = [ResultCallback](BKUDPClient* UDPClient, BKJson::Node Parameter)
    {
        if (ComponentInstance)
        {
            BKScopeGuard LocalGuard(&ComponentInstance->bInitialMessageOperated_Mutex);
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
    ComponentUDPClient = BKUDPClient::NewUDPClient(DiscoveryServerIP, DiscoveryServerPort, DataReceivedLambda);
    if (ComponentUDPClient && ComponentUDPClient->GetUDPHandler())
    {
        BKJson::Node DataToSend = BKJson::Node(BKJson::Node::T_OBJECT);
        DataToSend.Add(FString(L"CharArray"), BKJson::Node(CompileServiceInformation()));

        FBKCHARWrapper WrappedData = ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

        ComponentUDPClient->GetUDPHandler()->Send(ComponentUDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();

        static TArray<BKAsyncTaskParameter*> NoParameter;
        BKFutureAsyncTask Lambda = [ResultCallback](TArray<BKAsyncTaskParameter*> TaskParameters)
        {
            if (ComponentInstance)
            {
                BKScopeGuard LocalGuard(&ComponentInstance->bInitialMessageOperated_Mutex);
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
        BKScheduledAsyncTaskManager::NewScheduledAsyncTask(Lambda, NoParameter, 5000, false);
    }
    else
    {
        ResultCallback(false);
    }
}
void BKServiceDiscoveryComponent::UnregisterService(const FString& Reason)
{
    BKSystemManager::RemoveCallback(SystemCallbackUniqueID, SystemCallback);
    BKScheduledAsyncTaskManager::CancelScheduledAsyncTask(HeartbeatTaskUniqueIx);

    if (ComponentUDPClient && ComponentUDPClient->GetUDPHandler())
    {
        BKJson::Node DataToSend = BKJson::Node(BKJson::Node::T_OBJECT);
        DataToSend.Add(FString(L"CharArray"), BKJson::Node(CompileUnregisterServiceMessage(Reason)));

        FBKCHARWrapper WrappedData = ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

        ComponentUDPClient->GetUDPHandler()->Send(ComponentUDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();
    }
}

FString BKServiceDiscoveryComponent::CompileServiceInformation()
{
    BKJson::Node ResultObject = BKJson::Node(BKJson::Node::T_OBJECT);
    ResultObject.Add(FString(L"ServiceName"), BKJson::Node(ServiceName));
    ResultObject.Add(FString(L"ServiceIP"), BKJson::Node(ServiceIP));
    ResultObject.Add(FString(L"ServicePort"), BKJson::Node((int32)ServicePort));

    BKJson::Writer Writer;
    return Writer.WriteString(ResultObject);
}
FString BKServiceDiscoveryComponent::CompileUnregisterServiceMessage(const FString& Reason)
{
    BKJson::Node ResultObject = BKJson::Node(BKJson::Node::T_OBJECT);
    ResultObject.Add(FString(L"Status"), BKJson::Node(FString(L"Unregister")));
    ResultObject.Add(FString(L"Reason"), BKJson::Node(Reason));

    BKJson::Writer Writer;
    return Writer.WriteString(ResultObject);
}

void BKServiceDiscoveryComponent::StartHeartbeating()
{
    static TArray<BKAsyncTaskParameter*> NoParameter;
    BKFutureAsyncTask Lambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        if (bComponentStarted && ComponentInstance && ComponentInstance->ComponentUDPClient && ComponentInstance->ComponentUDPClient->GetUDPHandler())
        {
            FString CompiledHeartbeatData = ComponentInstance->CompileCurrentHeartbeatData();
            if (CompiledHeartbeatData.Len() > 0)
            {
                BKJson::Node DataToSend = BKJson::Node(BKJson::Node::T_OBJECT);
                DataToSend.Add(FString(L"CharArray"), BKJson::Node(CompiledHeartbeatData));

                FBKCHARWrapper WrappedData = ComponentInstance->ComponentUDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ComponentInstance->ComponentUDPClient->GetSocketAddress(), DataToSend, true, false, true);

                ComponentInstance->ComponentUDPClient->GetUDPHandler()->Send(ComponentInstance->ComponentUDPClient->GetSocketAddress(), WrappedData);

                WrappedData.DeallocateValue();
            }
        }
        else if (bComponentStarted && ComponentInstance)
        {
            StopComponent(FString(L"UDP Failure"));
            StartComponent([](bool bResult){}, ComponentInstance->DiscoveryServerIP, ComponentInstance->DiscoveryServerPort, ComponentInstance->ServiceName, ComponentInstance->ServiceIP, ComponentInstance->ServicePort);
        }
    };
    HeartbeatTaskUniqueIx = BKScheduledAsyncTaskManager::NewScheduledAsyncTask(Lambda, NoParameter, 1000, true);
}

void BKServiceDiscoveryComponent::SetCurrentUserNo(int32 _NewUsersNo)
{
    BKScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);
    ComponentInstance->CurrentUsersNo = _NewUsersNo;
}

FString BKServiceDiscoveryComponent::CompileCurrentHeartbeatData()
{
    if (bComponentStarted && CurrentSystemInfo.GetType() != BKJson::Node::Type::T_INVALID)
    {
        BKScopeGuard LocalGuard(&ComponentInstance->CurrentHeartbeatData_Mutex);

        BKJson::Node ResultObject = BKJson::Node(BKJson::Node::T_OBJECT);
        ResultObject.Add(FString(L"CurrentUsersNo"), BKJson::Node(CurrentUsersNo));
        ResultObject.Add(FString(L"CurrentSystemInfo"), CurrentSystemInfo);

        BKJson::Writer Writer;
        return Writer.WriteString(ResultObject);
    }
    return FString(L"");
}