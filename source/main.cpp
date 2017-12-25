// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WUDPServer.h"
#include "WAsyncTaskManager.h"
#include "WScheduledTaskManager.h"
#include <iostream>

WUDPServer* UDPServerInstance = nullptr;

void Start(uint16 UDPServerPort)
{
    WAsyncTaskManager::StartSystem(5);
    WScheduledAsyncTaskManager::StartSystem(20);

    UDPServerInstance = new WUDPServer([](WUDPHandler* HandlerInstance, WUDPTaskParameter* Parameter)
    {
        if (HandlerInstance && Parameter && Parameter->OtherParty)
        {
            FWCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize, false);

            WJson::Node AnalyzedData = HandlerInstance->AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->OtherParty);
            if (AnalyzedData.GetType() != WJson::Node::Type::T_VALIDATION &&
                AnalyzedData.GetType() != WJson::Node::Type::T_INVALID &&
                AnalyzedData.GetType() != WJson::Node::Type::T_NULL)
            {

            }

            //Do not deallocate buffer. Buffer will be de-allocated in parameter destruction.
        }
    });
    UDPServerInstance->StartSystem(UDPServerPort);
}
void Stop()
{
    if (UDPServerInstance)
    {
        UDPServerInstance->EndSystem();
        delete (UDPServerInstance);
    }

    WScheduledAsyncTaskManager::EndSystem();
    WAsyncTaskManager::EndSystem();
}
void Quit()
{
    Stop();
}

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");

    WUtilities::Print(EWLogType::Log, FString("Application has started."));

    WUtilities::Print(EWLogType::Log, FString("Commands:"));
    WUtilities::Print(EWLogType::Log, FString("__________________"));
    WUtilities::Print(EWLogType::Log, FString("0: Exit"));
    WUtilities::Print(EWLogType::Log, FString("1: Start"));
    WUtilities::Print(EWLogType::Log, FString("2: Stop"));
    WUtilities::Print(EWLogType::Log, FString("3: Restart"));
    WUtilities::Print(EWLogType::Log, FString("__________________"));

    uint16 UDP_Port = 27378;
    if (const ANSICHAR* UDP_Port_String = std::getenv("W_UDP_SERVER_PORT"))
    {
        UDP_Port = FString::ConvertToInteger<uint16>(UDP_Port_String);
    }

    WUtilities::Print(EWLogType::Log, FString("Auto-start. UDP Port: ") + FString::FromInt(UDP_Port));
    Start(UDP_Port);

    int32 Signal;
    while (true)
    {
        std::cin >> Signal;
        if (Signal == 0)
        {
            Quit();
            return 0;
        }
        else if (Signal == 1)
        {
            Start(UDP_Port);
            WUtilities::Print(EWLogType::Log, FString("System started."));
        }
        else if (Signal == 2)
        {
            Stop();
            WUtilities::Print(EWLogType::Log, FString("System stopped."));
        }
        else if (Signal == 3)
        {
            Stop();
            Start(UDP_Port);
            WUtilities::Print(EWLogType::Log, FString("System restarted."));
        }
    }
}