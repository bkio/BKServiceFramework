// Copyright Pagansoft.com, All rights reserved.

#include "WUDPManager.h"
#include "WMemory.h"

bool UWUDPManager::InitializeSocket(int32 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData{};
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"WSAStartup() failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#endif

    UDPSocket = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
#if PLATFORM_WINDOWS
    if (UDPSocket == INVALID_SOCKET)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        WSACleanup();
        return false;
    }
#else
    if (UDPSocket == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket initialization failed with error: ") + UWUtilities::WGetSafeErrorMessage());
        return false;
    }
#endif

    FMemory::Memzero((ANSICHAR*)&UDPServer, sizeof(UDPServer));
    UDPServer.sin_family = AF_INET;
    UDPServer.sin_addr.s_addr = INADDR_ANY;
    UDPServer.sin_port = htons(static_cast<u_short>(Port));

    int32 ret = bind(UDPSocket ,(struct sockaddr*)&UDPServer , sizeof(UDPServer));
    if (ret == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket binding failed with error: ") + UWUtilities::WGetSafeErrorMessage());
#if PLATFORM_WINDOWS
        WSACleanup();
#endif
        return false;
    }

    return true;
}
void UWUDPManager::CloseSocket()
{
#if PLATFORM_WINDOWS
    closesocket(UDPSocket);
    WSACleanup();
#else
    close(UDPSocket);
#endif
}

void UWUDPManager::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Buffer = new ANSICHAR[1024];
        auto Client = new sockaddr;
        int32 ClientLen = sizeof(*Client);

        int32 RetrievedSize = recvfrom(UDPSocket, Buffer, 1024, 0, Client, &ClientLen);
        if (RetrievedSize < 0 || !bSystemStarted)
        {
            if (!bSystemStarted) return;
            UWUtilities::Print(EWLogType::Error, FString(L"Socket receive failed with error: ") + UWUtilities::WGetSafeErrorMessage());

            delete[] Buffer;
            delete (Client);
            EndSystem();
            return;
        }
        if (RetrievedSize == 0) continue;

        FWUDPTaskParameter* TaskParameter = new FWUDPTaskParameter(RetrievedSize, Buffer, Client);
        TArray<FWAsyncTaskParameter*> TaskParameterAsArray(TaskParameter);

        WFutureAsyncTask Lambda = [](TArray<FWAsyncTaskParameter*>& TaskParameters)
        {
            if (!bSystemStarted || !ManagerInstance) return;

            if (TaskParameters.Num() > 0)
            {
                if (FWUDPTaskParameter* Parameter = dynamic_cast<FWUDPTaskParameter*>(TaskParameters[0]))
                {
                    if (Parameter->Buffer && Parameter->Client && Parameter->BufferSize > 0)
                    {
                        ManagerInstance->Send(Parameter->Client, FWCHARWrapper(Parameter->Buffer, Parameter->BufferSize));
                    }
                }
            }
        };
        UWAsyncTaskManager::NewAsyncTask(Lambda, TaskParameterAsArray);
    }
}

void UWUDPManager::Send(sockaddr* Client, FWCHARWrapper&& SendBuffer)
{
    if (!bSystemStarted) return;

    if (Client == nullptr) return;
    if (SendBuffer.GetSize() == 0) return;

    int32 ClientLength = sizeof(*Client);
    int32 SentLength = sendto(UDPSocket, SendBuffer.GetValue(), SendBuffer.GetSize(), 0, Client, ClientLength);
#if PLATFORM_WINDOWS
    if (SentLength == SOCKET_ERROR)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#else
    if (SentLength == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#endif
}


UWUDPManager* UWUDPManager::ManagerInstance = nullptr;

bool UWUDPManager::bSystemStarted = false;
bool UWUDPManager::StartSystem(int32 Port)
{
    if (bSystemStarted) return true;
    bSystemStarted = true;

    ManagerInstance = new UWUDPManager();

    if (!ManagerInstance->StartSystem_Internal(Port))
    {
        EndSystem();
        return false;
    }

    return true;
}
bool UWUDPManager::StartSystem_Internal(int32 Port)
{
    if (InitializeSocket(Port))
    {
        UDPSystemThread = new WThread(std::bind(&UWUDPManager::ListenSocket, this));
        return true;
    }
    return false;
}

void UWUDPManager::EndSystem()
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
void UWUDPManager::EndSystem_Internal()
{
    CloseSocket();
    if (UDPSystemThread != nullptr)
    {
        if (UDPSystemThread->IsJoinable())
        {
            UDPSystemThread->Join();
        }
        delete (UDPSystemThread);
    }
}