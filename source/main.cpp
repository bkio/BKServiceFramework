// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WSystemManager.h"
#include "WAsyncTaskManager.h"
#include "WScheduledTaskManager.h"
#include "WUDPServer.h"
#include "WHTTPServer.h"
#include "WHTTPClient.h"

UWHTTPServer* HTTPServerInstance = nullptr;
UWUDPServer* UDPServerInstance = nullptr;

void Start()
{
    UWAsyncTaskManager::StartSystem(5);
    UWScheduledAsyncTaskManager::StartSystem(20);

    UDPServerInstance = new UWUDPServer([](UWUDPHandler* HandlerInstance, UWUDPTaskParameter* Parameter)
    {
        if (HandlerInstance && Parameter && Parameter->Buffer && Parameter->Client && Parameter->BufferSize > 0)
        {
            FWCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize);

            HandlerInstance->AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->Client);
        }
    });
    UDPServerInstance->StartSystem(45000);

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
    HTTPServerInstance->StartSystem(8080, 2500);

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
void Restart()
{
    Stop();
    Start();
}
void Quit()
{
    Stop();
}

void SendPingToGoogle()
{
    WFutureAsyncTask RequestLambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0)
        {
            if (auto Request = dynamic_cast<UWHTTPClient*>(TaskParameters[0]))
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
        if (TaskParameters.Num() > 0)
        {
            if (auto Request = dynamic_cast<UWHTTPClient*>(TaskParameters[0]))
            {
                Request->CancelRequest();
                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    UWHTTPClient::NewHTTPRequest("google.com", 80, L"Ping...", "GET", "", DEFAULT_HTTP_REQUEST_HEADERS, DEFAULT_TIMEOUT_MS, RequestLambda, TimeoutLambda);
}

int main()
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
    UWUtilities::Print(EWLogType::Log, FString(L"__________________"));

    UWUtilities::Print(EWLogType::Log, FString(L"Auto-start..."));
    Start();

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
            Start();
            UWUtilities::Print(EWLogType::Log, FString(L"System started."));
        }
        else if (Signal == 2)
        {
            Stop();
            UWUtilities::Print(EWLogType::Log, FString(L"System stopped."));
        }
        else if (Signal == 3)
        {
            Restart();
            UWUtilities::Print(EWLogType::Log, FString(L"System restarted."));
        }
        else if (Signal == 4)
        {
            SendPingToGoogle();
        }
    }
}