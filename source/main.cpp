// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WSystemManager.h"
#include "WAsyncTaskManager.h"
#include "WScheduledTaskManager.h"
#include "WUDPServer.h"
#include "WUDPClient.h"
#include "WHTTPServer.h"
#include "WHTTPClient.h"

UWHTTPServer* HTTPServerInstance = nullptr;
UWUDPServer* UDPServerInstance = nullptr;

void Start(uint16 HTTPServerPort, uint16 UDPServerPort)
{
    UWAsyncTaskManager::StartSystem(5);
    UWScheduledAsyncTaskManager::StartSystem(20);

    UDPServerInstance = new UWUDPServer([](UWUDPHandler* HandlerInstance, UWUDPTaskParameter* Parameter)
    {
        if (HandlerInstance && Parameter && Parameter->OtherParty)
        {
            FWCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize, false);

            WJson::Node AnalyzedData = HandlerInstance->AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->OtherParty);
            if (AnalyzedData.GetType() != WJson::Node::Type::T_VALIDATION &&
                AnalyzedData.GetType() != WJson::Node::Type::T_INVALID &&
                AnalyzedData.GetType() != WJson::Node::Type::T_NULL)
            {
                FWCHARWrapper FinalBuffer = HandlerInstance->MakeByteArrayForNetworkData(Parameter->OtherParty, AnalyzedData, false, false, true);
                HandlerInstance->Send(Parameter->OtherParty, FinalBuffer);
                FinalBuffer.DeallocateValue();
            }

            //Do not deallocate buffer. Buffer will be de-allocated in parameter destruction.
        }
    });
    UDPServerInstance->StartSystem(UDPServerPort);

    HTTPServerInstance = new UWHTTPServer([](UWHTTPAcceptedClient* Parameter)
    {
        if (Parameter && Parameter->Initialize())
        {
            if (!Parameter->GetData()) return;

            std::wstring ResponseBody = L"<html>Hello pagan world!</html>";
            std::string ResponseHeaders = "HTTP/1.1 200 OK\r\n"
                                                "Content-Type: text/html; charset=UTF-8\r\n"
                                                "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
            Parameter->SendData(ResponseBody, ResponseHeaders);
            Parameter->Finalize();
        }
    });
    HTTPServerInstance->StartSystem(HTTPServerPort, 2500);

    UWSystemManager::StartSystem();
}
void Stop()
{
    UWSystemManager::EndSystem();

    if (HTTPServerInstance)
    {
        HTTPServerInstance->EndSystem();
        delete (HTTPServerInstance);
    }

    if (UDPServerInstance)
    {
        UDPServerInstance->EndSystem();
        delete (UDPServerInstance);
    }

    UWScheduledAsyncTaskManager::EndSystem();
    UWAsyncTaskManager::EndSystem();
}
void Quit()
{
    Stop();
}

void SendPingToGoogle()
{
    WFutureAsyncTask RequestLambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            if (auto Request = reinterpret_cast<UWHTTPClient*>(TaskParameters[0]))
            {
                if (Request->ProcessRequest())
                {
                    FString Response = FString(Request->GetResponsePayload());
                    UWUtilities::Print(EWLogType::Log, FString(L"Response length from Google: ") + FString::FromInt(Response.Len()));
                }
                else
                {
                    UWUtilities::Print(EWLogType::Error, FString(L"Ping send request to Google has failed."));
                }

                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    WFutureAsyncTask TimeoutLambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            if (auto Request = reinterpret_cast<UWHTTPClient*>(TaskParameters[0]))
            {
                Request->CancelRequest();
                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    UWHTTPClient::NewHTTPRequest("google.com", 80, L"Ping...", "GET", "", DEFAULT_HTTP_REQUEST_HEADERS, DEFAULT_TIMEOUT_MS, RequestLambda, TimeoutLambda);
}

void SendUDPPacketToServer()
{
    WUDPClient_DataReceived DataReceivedLambda = [](UWUDPClient* UDPClient, WJson::Node Parameter)
    {
        if (UDPClient && UDPClient->GetUDPHandler())
        {
            FWCHARWrapper WrappedData = UDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(UDPClient->GetSocketAddress(), Parameter, false, false, true);

            UDPClient->GetUDPHandler()->Send(UDPClient->GetSocketAddress(), WrappedData);

            WrappedData.DeallocateValue();
        }
    };
    UWUDPClient* UDPClient = UWUDPClient::NewUDPClient("127.0.0.1", 45000, DataReceivedLambda);
    if (UDPClient && UDPClient->GetUDPHandler())
    {
        WJson::Node DataToSend = WJson::Node(WJson::Node::T_OBJECT);
        DataToSend.Add("CharArray", WJson::Node("Hello pagan world!"));

        FWCHARWrapper WrappedData = UDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(UDPClient->GetSocketAddress(), DataToSend, false, false, true);

        UDPClient->GetUDPHandler()->Send(UDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");

    UWUtilities::Print(EWLogType::Log, FString(L"Application has started."));

    UWUtilities::Print(EWLogType::Log, FString(L"Commands:"));
    UWUtilities::Print(EWLogType::Log, FString(L"__________________"));
    UWUtilities::Print(EWLogType::Log, FString(L"0: Exit"));
    UWUtilities::Print(EWLogType::Log, FString(L"1: Start"));
    UWUtilities::Print(EWLogType::Log, FString(L"2: Stop"));
    UWUtilities::Print(EWLogType::Log, FString(L"3: Restart"));
    UWUtilities::Print(EWLogType::Log, FString(L"4: Send ping to Google"));
    UWUtilities::Print(EWLogType::Log, FString(L"5: Send reliable packet to UDP Server"));
    UWUtilities::Print(EWLogType::Log, FString(L"__________________"));

    uint16 HTTP_Port = 8000;
    if (const ANSICHAR* HTTP_Port_String = std::getenv("W_HTTP_SERVER_PORT"))
    {
        HTTP_Port = FString::ConvertToInteger<uint16>(HTTP_Port_String);
    }
    uint16 UDP_Port = 50000;
    if (const ANSICHAR* UDP_Port_String = std::getenv("W_UDP_SERVER_PORT"))
    {
        UDP_Port = FString::ConvertToInteger<uint16>(UDP_Port_String);
    }

    UWUtilities::Print(EWLogType::Log, FString(L"Auto-start. HTTP Port: ") + FString::FromInt(HTTP_Port) + FString(L", UDP Port: ") + FString::FromInt(UDP_Port));
    Start(HTTP_Port, UDP_Port);

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
            Start(HTTP_Port, UDP_Port);
            UWUtilities::Print(EWLogType::Log, FString(L"System started."));
        }
        else if (Signal == 2)
        {
            Stop();
            UWUtilities::Print(EWLogType::Log, FString(L"System stopped."));
        }
        else if (Signal == 3)
        {
            Stop();
            Start(HTTP_Port, UDP_Port);
            UWUtilities::Print(EWLogType::Log, FString(L"System restarted."));
        }
        else if (Signal == 4)
        {
            SendPingToGoogle();
        }
        else if (Signal == 5)
        {
            SendUDPPacketToServer();
        }
    }
}