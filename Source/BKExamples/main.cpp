// Copyright Burak Kara, All rights reserved.

#include "BKEngine.h"
#include "BKSystemManager.h"
#include "BKAsyncTaskManager.h"
#include "BKScheduledTaskManager.h"
#include "BKUDPServer.h"
#include "BKUDPClient.h"
#include "BKUDPHandler.h"
#include "BKHTTPServer.h"
#include "BKHTTPClient.h"

BKHTTPServer* HTTPServerInstance = nullptr;
BKUDPServer* UDPServerInstance = nullptr;

void Start(uint16 HTTPServerPort, uint16 UDPServerPort)
{
    BKAsyncTaskManager::StartSystem(5);
    BKScheduledAsyncTaskManager::StartSystem(20);

    UDPServerInstance = new BKUDPServer([](BKUDPHandler* HandlerInstance, WUDPTaskParameter* Parameter)
    {
        if (HandlerInstance && Parameter && Parameter->OtherParty)
        {
            FBKCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize, false);

            BKJson::Node AnalyzedData = HandlerInstance->AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->OtherParty);
            if (AnalyzedData.GetType() != BKJson::Node::Type::T_VALIDATION &&
                AnalyzedData.GetType() != BKJson::Node::Type::T_INVALID &&
                AnalyzedData.GetType() != BKJson::Node::Type::T_NULL)
            {
                BKUtilities::Print(EBKLogType::Log, AnalyzedData.ToString(FString(L"Test")));

                FBKCHARWrapper FinalBuffer = HandlerInstance->MakeByteArrayForNetworkData(Parameter->OtherParty, AnalyzedData);
                HandlerInstance->Send(Parameter->OtherParty, FinalBuffer);
                FinalBuffer.DeallocateValue();
            }

            //Do not deallocate buffer. Buffer will be de-allocated in parameter destruction.
        }
    });
    UDPServerInstance->StartSystem(UDPServerPort);

    HTTPServerInstance = new BKHTTPServer([](BKHTTPAcceptedClient* Parameter)
    {
        BKUtilities::Print(EBKLogType::Log, FString(L"Request Body: ") + Parameter->GetRequestBody());
        BKUtilities::Print(EBKLogType::Log, FString(L"Request Client IP: ") + Parameter->GetRequestClientIP());
        BKUtilities::Print(EBKLogType::Log, FString(L"Request Method: ") + Parameter->GetRequestMethod());
        BKUtilities::Print(EBKLogType::Log, FString(L"Request Path: ") + Parameter->GetRequestPath());

        auto Headers = Parameter->GetRequestHeaders();
        FString HeaderString;

        Headers.Iterate([&HeaderString](BKHashNode<FString, FString>* Header)
        {
            HeaderString.Append(Header->GetKey() + FString(L" -> ") + Header->GetValue() + FString(L", "));
        });
        BKUtilities::Print(EBKLogType::Log, FString(L"Request Headers: ") + HeaderString);

        Parameter->SetResponseCode(200);
        Parameter->SetResponseContentType(FString(L"text/html"));
        Parameter->SetResponseBody(FString(L"<html>Hello world!</html>"));
    });
    HTTPServerInstance->StartSystem(HTTPServerPort, 2500);

    WSystemInfoCallback SystemCallback = [](BKSystemInfo* CurrentSystemInfo)
    {
        BKUtilities::Print(EBKLogType::Log,
                          FString(L"Total CPU: ") +
                          FString::FromInt(CurrentSystemInfo->GetTotalCPUUtilization()) +
                          FString(L"\t Total Memory: ") +
                          FString::FromInt(CurrentSystemInfo->GetTotalMemoryUtilization()));
    };
    uint32 CallbackID;
    BKSystemManager::StartSystem(CallbackID, SystemCallback);
}
void Stop()
{
    BKSystemManager::EndSystem();

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

    BKScheduledAsyncTaskManager::EndSystem();
    BKAsyncTaskManager::EndSystem();
}
void Quit()
{
    Stop();
}

void SendPingToGoogle()
{
    BKFutureAsyncTask RequestLambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            if (auto Request = reinterpret_cast<BKHTTPClient*>(TaskParameters[0]))
            {
                if (Request->ProcessRequest())
                {
                    FString Response = FString(Request->GetResponsePayload());
                    BKUtilities::Print(EBKLogType::Log, FString(L"Response length from Google: ") + FString::FromInt(Response.Len()));
                }
                else
                {
                    BKUtilities::Print(EBKLogType::Error, FString(L"Ping send request to Google has failed."));
                }

                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    BKFutureAsyncTask TimeoutLambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            if (auto Request = reinterpret_cast<BKHTTPClient*>(TaskParameters[0]))
            {
                Request->CancelRequest();
                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    BKHTTPClient::NewHTTPRequest(FString(L"google.com"), 80, FString(L"Ping..."), FString(L"GET"), FString(L""), DEFAULT_HTTP_REQUEST_HEADERS, DEFAULT_TIMEOUT_MS, RequestLambda, TimeoutLambda);
}

void SendUDPPacketToServer()
{
    BKUDPClient_DataReceived DataReceivedLambda = [](BKUDPClient* UDPClient, BKJson::Node Parameter)
    {
        if (UDPClient && UDPClient->GetUDPHandler())
        {
            FBKCHARWrapper WrappedData = UDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(UDPClient->GetSocketAddress(), Parameter);

            UDPClient->GetUDPHandler()->Send(UDPClient->GetSocketAddress(), WrappedData);

            WrappedData.DeallocateValue();
        }
    };
    BKUDPClient* UDPClient = BKUDPClient::NewUDPClient(FString(L"127.0.0.1"), 45000, DataReceivedLambda);
    if (UDPClient && UDPClient->GetUDPHandler())
    {
        BKJson::Node DataToSend = BKJson::Node(BKJson::Node::T_OBJECT);
        DataToSend.Add(FString(L"CharArray"), BKJson::Node(L"Hello world!"));

        FBKCHARWrapper WrappedData = UDPClient->GetUDPHandler()->MakeByteArrayForNetworkData(UDPClient->GetSocketAddress(), DataToSend);

        UDPClient->GetUDPHandler()->Send(UDPClient->GetSocketAddress(), WrappedData);

        WrappedData.DeallocateValue();
    }
}

int main(int argc, char* argv[])
{
    setlocale(LC_CTYPE, "");

    BKUtilities::Print(EBKLogType::Log, FString(L"Application has started."));

    BKUtilities::Print(EBKLogType::Log, FString(L"Commands:"));
    BKUtilities::Print(EBKLogType::Log, FString(L"__________________"));
    BKUtilities::Print(EBKLogType::Log, FString(L"0: Exit"));
    BKUtilities::Print(EBKLogType::Log, FString(L"1: Start"));
    BKUtilities::Print(EBKLogType::Log, FString(L"2: Stop"));
    BKUtilities::Print(EBKLogType::Log, FString(L"3: Restart"));
    BKUtilities::Print(EBKLogType::Log, FString(L"4: Send ping to Google"));
    BKUtilities::Print(EBKLogType::Log, FString(L"5: Send reliable packet to UDP Server"));
    BKUtilities::Print(EBKLogType::Log, FString(L"__________________"));

    uint16 HTTP_Port = 8000;
    if (const ANSICHAR* HTTP_Port_String = std::getenv("BK_HTTP_SERVER_PORT"))
    {
        HTTP_Port = FString::ConvertToInteger<uint16>(HTTP_Port_String);
    }
    uint16 UDP_Port = 45000;
    if (const ANSICHAR* UDP_Port_String = std::getenv("BK_UDP_SERVER_PORT"))
    {
        UDP_Port = FString::ConvertToInteger<uint16>(UDP_Port_String);
    }

    BKUtilities::Print(EBKLogType::Log, FString(L"Auto-start. HTTP Port: ") + FString::FromInt(HTTP_Port) + FString(L", UDP Port: ") + FString::FromInt(UDP_Port));
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
            BKUtilities::Print(EBKLogType::Log, FString(L"System started."));
        }
        else if (Signal == 2)
        {
            Stop();
            BKUtilities::Print(EBKLogType::Log, FString(L"System stopped."));
        }
        else if (Signal == 3)
        {
            Stop();
            Start(HTTP_Port, UDP_Port);
            BKUtilities::Print(EBKLogType::Log, FString(L"System restarted."));
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