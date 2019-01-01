// Copyright Burak Kara, All rights reserved.

#include <BKAsyncTaskManager.h>
#include <BKMemory.h>
#include "BKHTTPServer.h"
#include "BKMath.h"

bool BKHTTPServer::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        BKUtilities::Print(EBKLogType::Error, FString(L"BKHTTPServer: WSAStartup() failed with error: ") + BKUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    HTTPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if PLATFORM_WINDOWS
    if (HTTPSocket == INVALID_SOCKET)
    {
        BKUtilities::Print(EBKLogType::Error, FString(L"BKHTTPServer: Socket initialization failed with error: ") + BKUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (HTTPSocket == -1)
    {
        BKUtilities::Print(EBKLogType::Error, FString(L"WHTTPServer: Socket initialization failed with error: ") + BKUtilities::WGetSafeErrorMessage());
        return false;
    }
#endif

    int32 On = 1;
    setsockopt(HTTPSocket, SOL_SOCKET, SO_REUSEADDR, (const ANSICHAR*)&On, sizeof(int32));
    setsockopt(HTTPSocket, IPPROTO_TCP, TCP_NODELAY, (const ANSICHAR*)&On, sizeof(int32));

    FMemory::Memzero((ANSICHAR*)&HTTPServer, sizeof(HTTPServer));
    HTTPServer.sin_family = AF_INET;
    HTTPServer.sin_addr.s_addr = INADDR_ANY;
    HTTPServer.sin_port = htons(Port);
    HTTPPort = Port;

    int32 ret = bind(HTTPSocket, (struct sockaddr*)&HTTPServer, sizeof(HTTPServer));
    if (ret == -1)
    {
        BKUtilities::Print(EBKLogType::Error, FString(L"BKHTTPServer: Socket binding failed with error: ") + BKUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void BKHTTPServer::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(HTTPSocket);
    WSACleanup();
#else
    shutdown(HTTPSocket, SHUT_RDWR);
    close(HTTPSocket);
#endif
}

void BKHTTPServer::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Client = new sockaddr;
#if PLATFORM_WINDOWS
        int32 ClientLen = sizeof(*Client);
#else
        socklen_t ClientLen = sizeof(*Client);
#endif

        auto ListenResult = listen(HTTPSocket, SOMAXCONN);
#if PLATFORM_WINDOWS
        if (ListenResult == SOCKET_ERROR)
#else
        if (ListenResult == -1)
#endif
        {
            if (!bSystemStarted) return;

            EndSystem();
            StartSystem(HTTPPort, TimeoutInMs);
            return;
        }

        auto ClientSocket = accept(HTTPSocket, Client, &ClientLen);
#if PLATFORM_WINDOWS
        if (ClientSocket == INVALID_SOCKET)
#else
        if (ClientSocket == -1)
#endif
        {
            delete (Client);
            if (!bSystemStarted) return;
            continue;
        }

        TArray<BKAsyncTaskParameter*> PassParameters;
        PassParameters.Add(this);
        PassParameters.Add(new BKHTTPAcceptedClient(ClientSocket, Client, TimeoutInMs));

        BKFutureAsyncTask TaskLambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
        {
            if (TaskParameters.Num() >= 2 && TaskParameters[0] && TaskParameters[1])
            {
                auto ServerInstance = reinterpret_cast<BKHTTPServer*>(TaskParameters[0]);
                auto Parameter = reinterpret_cast<BKHTTPAcceptedClient*>(TaskParameters[1]);
                if (!ServerInstance || !ServerInstance->bSystemStarted || !ServerInstance->HTTPListenCallback)
                {
                    if (Parameter)
                    {
                        delete (Parameter);
                    }
                    return;
                }

                if (Parameter && Parameter->Initialize() && Parameter->GetData())
                {
                    ServerInstance->HTTPListenCallback(Parameter);

                    Parameter->SetResponseHeader_Internal(FString(L"content-type"), Parameter->ResponseContentType + FString(L"; charset=UTF-8"), false);
                    Parameter->SetResponseHeader_Internal(FString(L"content-length"), FString::FromInt(Parameter->ResponseBody.Len()), false);

                    //TODO: this will be rejected due to missing Secure, Path and Domain directives. Fix.
                    FStringStream CookieStringBuilder;
                    for (int i = 0; i < Parameter->ResponseCookies.Num(); i++)
                    {
                        if (Parameter->ResponseCookies[i].IsValid())
                        {
                            CookieStringBuilder << Parameter->ResponseCookies[i]->Item1;

                            CookieStringBuilder << L"=";

                            CookieStringBuilder << Parameter->ResponseCookies[i]->Item2;

                            Parameter->AddResponseHeader_Internal(FString(L"set-cookie"), CookieStringBuilder.Str(), false);
                        }
                    }

                    FStringStream HeaderStringBuilder;
                    HeaderStringBuilder << L"HTTP/1.1 ";
                    HeaderStringBuilder << Parameter->ResponseCode;
                    HeaderStringBuilder << " ";
                    HeaderStringBuilder << Parameter->ResponseCodeDescription;
                    HeaderStringBuilder << "\r\n";

                    for (int i = 0; i < Parameter->ResponseHeaders.Num(); i++)
                    {
                        if (Parameter->ResponseHeaders[i].IsValid())
                        {
                            HeaderStringBuilder << Parameter->ResponseHeaders[i]->Item1;

                            HeaderStringBuilder << L": ";

                            HeaderStringBuilder << Parameter->ResponseHeaders[i]->Item2;

                            HeaderStringBuilder << L"\r\n";
                        }
                    }
                    HeaderStringBuilder << L"\r\n";

                    Parameter->SendData(Parameter->ResponseBody, FString(HeaderStringBuilder.Str()));

                    Parameter->Finalize();
                    delete (Parameter);
                }
            }
        };
        BKAsyncTaskManager::NewAsyncTask(TaskLambda, PassParameters, true);
    }
}
uint32 BKHTTPServer::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (HTTPSystemThread) delete (HTTPSystemThread);
    HTTPSystemThread = new BKThread(std::bind(&BKHTTPServer::ListenSocket, this), std::bind(&BKHTTPServer::ListenerStopped, this));
    return 0;
}

bool BKHTTPServer::StartSystem(uint16 Port, uint32 TimeoutMs)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    TimeoutInMs = TimeoutMs == 0 ? 2500 : TimeoutMs;
    if (InitializeSocket(Port))
    {
        HTTPSystemThread = new BKThread(std::bind(&BKHTTPServer::ListenSocket, this), std::bind(&BKHTTPServer::ListenerStopped, this));
        return true;
    }
    return false;
}

void BKHTTPServer::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    CloseSocket();
    HTTPListenCallback = nullptr;
    if (HTTPSystemThread)
    {
        if (HTTPSystemThread->IsJoinable())
        {
            HTTPSystemThread->Join();
        }
        delete (HTTPSystemThread);
    }
}