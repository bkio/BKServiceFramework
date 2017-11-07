// Copyright Pagansoft.com, All rights reserved.

#include "WEngine.h"
#include "WSystemManager.h"
#include "WAsyncTaskManager.h"
#include "WScheduledTaskManager.h"
#include "WUDPManager.h"
#include "WHTTPServer.h"
#include "WHTTPClient.h"

void Start()
{
    UWAsyncTaskManager::StartSystem(5);
    UWScheduledAsyncTaskManager::StartSystem(20);
    UWUDPManager::StartSystem(45000, [](FWUDPTaskParameter* Parameter)
    {
        if (Parameter && Parameter->Buffer && Parameter->Client && Parameter->BufferSize > 0)
        {
            FWCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize);

            UWUDPManager::AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->Client);
        }
    });
    UWHTTPServer::StartSystem(8080, 2500, [](FWHTTPAcceptedClient* Parameter)
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
    UWSystemManager::StartSystem();
}
void Stop()
{
    UWSystemManager::EndSystem();
    UWHTTPServer::EndSystem();
    UWUDPManager::EndSystem();
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
    WFutureAsyncTask RequestLambda = [](TArray<FWAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0)
        {
            if (auto Request = dynamic_cast<FWHTTPClient*>(TaskParameters[0]))
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
    WFutureAsyncTask TimeoutLambda = [](TArray<FWAsyncTaskParameter*> TaskParameters)
    {
        if (TaskParameters.Num() > 0)
        {
            if (auto Request = dynamic_cast<FWHTTPClient*>(TaskParameters[0]))
            {
                Request->CancelRequest();
                if (Request->DestroyApproval()) delete (Request);
            }
        }
    };
    FWHTTPClient::NewHTTPRequest("google.com", 80, L"Ping...", "GET", "", DEFAULT_HTTP_REQUEST_HEADERS, DEFAULT_TIMEOUT_MS, RequestLambda, TimeoutLambda);
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