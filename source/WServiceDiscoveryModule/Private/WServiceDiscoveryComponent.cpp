// Copyright Pagansoft.com, All rights reserved.

#include "WServiceDiscoveryComponent.h"
#include "WUDPClient.h"
#include "WUDPHandler.h"

WServiceDiscoveryComponent* WServiceDiscoveryComponent::ComponentInstance = nullptr;
bool WServiceDiscoveryComponent::bComponentStarted = false;

bool WServiceDiscoveryComponent::StartComponent(const FString& _DiscoveryServerIP, uint16 _DiscoveryServerPort, const FString& _ServiceName, const FString& _ServiceIP, uint16 _ServicePort)
{
    if (bComponentStarted) return false;
    bComponentStarted = true;

    ComponentInstance = new WServiceDiscoveryComponent();
    ComponentInstance->DiscoveryServerIP = _DiscoveryServerIP;
    ComponentInstance->DiscoveryServerPort = _DiscoveryServerPort;
    ComponentInstance->ServiceName = _ServiceName;
    ComponentInstance->ServiceIP = _ServiceIP;
    ComponentInstance->ServicePort = _ServicePort;
    if (!ComponentInstance->RegisterService())
    {
        StopComponent();
        return false;
    }
    return true;
}
void WServiceDiscoveryComponent::StopComponent()
{
    if (!bComponentStarted) return;
    bComponentStarted = false;

    if (ComponentInstance)
    {
        ComponentInstance->UnregisterService();

        delete (ComponentInstance);
        ComponentInstance = nullptr;
    }
}

bool WServiceDiscoveryComponent::RegisterService()
{
    WUDPClient_DataReceived DataReceivedLambda = [](WUDPClient* UDPClient, WJson::Node Parameter)
    {
        if (UDPClient && UDPClient->GetUDPHandler())
        {
            
        }
    };
    ServiceDiscovery_UDPClient = WUDPClient::NewUDPClient(DiscoveryServerIP, DiscoveryServerPort, DataReceivedLambda);
    if (ServiceDiscovery_UDPClient && ServiceDiscovery_UDPClient->GetUDPHandler())
    {
        WJson::Node DataToSend = WJson::Node(WJson::Node::T_OBJECT);
        DataToSend.Add(FString("CharArray"), WJson::Node("Hello pagan world!"));

        FWCHARWrapper WrappedData = ServiceDiscovery_UDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(ServiceDiscovery_UDPClient->GetSocketAddress(), DataToSend, false, false, true);

        ServiceDiscovery_UDPClient->GetUDPHandler()->Send(ServiceDiscovery_UDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();
    }
}
void WServiceDiscoveryComponent::UnregisterService()
{

}