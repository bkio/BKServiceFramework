// Copyright Pagansoft.com, All rights reserved.

#include <WAsyncTaskManager.h>
#include <WMemory.h>
#include "WHTTPManager.h"
#include "WMath.h"

bool UWHTTPManager::InitializeSocket(uint16 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPManager: WSAStartup() failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    HTTPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if PLATFORM_WINDOWS
    if (HTTPSocket == INVALID_SOCKET)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPManager: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (HTTPSocket == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPManager: Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
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
        UWUtilities::Print(EWLogType::Error, FString(L"UWHTTPManager: Socket binding failed with error: ") + UWUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void UWHTTPManager::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(HTTPSocket);
    WSACleanup();
#else
    shutdown(HTTPSocket, SHUT_RDWR);
    close(HTTPSocket);
#endif
}

void UWHTTPManager::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Buffer = new ANSICHAR[HTTP_BUFFER_SIZE];
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
        if (ClientSocket == -1)
#endif
        {
            delete[] Buffer;
            delete (Client);

            if (!bSystemStarted) return;

            EndSystem();
            StartSystem(HTTPPort);
            return;
        }

        auto ClientSocket = accept(HTTPSocket, Client, &ClientLen);
#if PLATFORM_WINDOWS
        if (ClientSocket == INVALID_SOCKET)
#else
        if (ClientSocket == -1)
#endif
        {
            delete[] Buffer;
            delete (Client);
            if (!bSystemStarted) return;
            continue;
        }
        auto TaskParameter = new FWHTTPClient(ClientSocket);
        TArray<FWAsyncTaskParameter*> TaskParameterAsArray(TaskParameter);

        WFutureAsyncTask Lambda = [](TArray<FWAsyncTaskParameter*>& TaskParameters)
        {
            if (!bSystemStarted || !ManagerInstance) return;

            if (TaskParameters.Num() > 0)
            {
                if (auto Parameter = dynamic_cast<FWHTTPClient*>(TaskParameters[0]))
                {
                    if (Parameter->Initialize())
                    {
                        if (Parameter->GetData())
                        {
                            std::string ResponseBody = "<html>Hello pagan world!</html>";
                            std::string ResponseHeaders = "HTTP/1.1 200 OK\r\n"
                                                           "Content-Type: text/html; charset=UTF-8\r\n"
                                                           "Content-Length: " + std::to_string(ResponseBody.length()) + "\r\n\r\n";
                            Parameter->SendData(ResponseBody, ResponseHeaders);
                        }
                    }
                }
            }
        };
        UWAsyncTaskManager::NewAsyncTask(Lambda, TaskParameterAsArray);
    }
}
uint32 UWHTTPManager::ListenerStopped()
{
    if (!bSystemStarted) return 0;
    if (HTTPSystemThread) delete (HTTPSystemThread);
    HTTPSystemThread = new WThread(std::bind(&UWHTTPManager::ListenSocket, this), std::bind(&UWHTTPManager::ListenerStopped, this));
    return 0;
}

UWHTTPManager* UWHTTPManager::ManagerInstance = nullptr;

bool UWHTTPManager::bSystemStarted = false;
bool UWHTTPManager::StartSystem(uint16 Port)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new UWHTTPManager();

    if (!ManagerInstance->StartSystem_Internal(Port))
    {
        EndSystem();
        return false;
    }

    return true;
}
bool UWHTTPManager::StartSystem_Internal(uint16 Port)
{
    if (InitializeSocket(Port))
    {
        HTTPSystemThread = new WThread(std::bind(&UWHTTPManager::ListenSocket, this), std::bind(&UWHTTPManager::ListenerStopped, this));
        return true;
    }
    return false;
}

void UWHTTPManager::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    if (ManagerInstance != nullptr)
    {
        ManagerInstance->EndSystem_Internal();
        delete (ManagerInstance);
        ManagerInstance = nullptr;
    }
}
void UWHTTPManager::EndSystem_Internal()
{
    CloseSocket();
    if (HTTPSystemThread != nullptr)
    {
        if (HTTPSystemThread->IsJoinable())
        {
            HTTPSystemThread->Join();
        }
        delete (HTTPSystemThread);
    }
}