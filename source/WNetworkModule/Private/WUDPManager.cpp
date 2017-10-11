// Copyright Pagansoft.com, All rights reserved.

#include "WUDPManager.h"
#include "WAsyncTaskManager.h"
#include "WMemory.h"

bool UWUDPManager::InitializeSocket(int32 Port)
{
#if PLATFORM_WINDOWS
    WSADATA WSAData;
    if (WSAStartup(0x202, &WSAData) != 0)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"WSAStartup() failed with error: %d", WSAGetLastError()));
        WSACleanup();
        return false;
    }
#endif

    UDPSocket = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
#if PLATFORM_WINDOWS
    if (UDPSocket == INVALID_SOCKET)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket initialization failed with error: %d", WSAGetLastError()));
        WSACleanup();
        return false;
    }
#else
    if (UDPSocket == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket initialization failed with error: %s", *UWUtilities::WGetSafeErrorMessage()));
        return false;
    }
#endif

    FMemory::Memzero((ANSICHAR*)&UDPServer, sizeof(UDPServer));
    UDPServer.sin_family = AF_INET;
    UDPServer.sin_addr.s_addr = INADDR_ANY;
    UDPServer.sin_port = htons(Port);

    int32 ret = bind(UDPSocket ,(struct sockaddr*)&UDPServer , sizeof(UDPServer));
#if PLATFORM_WINDOWS
    if (ret == SOCKET_ERROR)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket binding failed with error: %d", WSAGetLastError()));
        WSACleanup();
        return false;
    }
#else
    if (ret == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket binding failed with error: %s", *UWUtilities::WGetSafeErrorMessage()));
        return false;
    }
#endif

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
        fflush(stdout);

        std::shared_ptr<sockaddr_in> Client(new struct sockaddr_in);
        std::shared_ptr<ANSICHAR> Buffer(new ANSICHAR[1024]);
        int32 ClientLength = sizeof(Client.get());

        int32 RetrievedSize = recvfrom(UDPSocket, Buffer.get(), 1024, 0, (struct sockaddr*)Client.get(), &ClientLength);
#if PLATFORM_WINDOWS
        if (RetrievedSize == SOCKET_ERROR)
        {
            UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket receive failed with error: %d", WSAGetLastError()));
            continue;
        }
#else
        if (RetrievedSize == -1)
        {
            UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket receive failed with error: %s", *UWUtilities::WGetSafeErrorMessage()));
            continue;
        }
#endif
        if (RetrievedSize <= 0) continue;

        if (!bSystemStarted) return;

        /*UWAsyncTaskManager::NewAsyncTask([Client, Buffer, RetrievedSize]()
        {
            if (Client && Buffer)
            {

            }
        });*/
    }
}

void UWUDPManager::Send(std::shared_ptr<sockaddr_in> Client, const FWCHARWrapper& SendBuffer)
{
    if (!bSystemStarted) return;

    if (!Client) return;
    if (SendBuffer.GetSize() == 0) return;

    int32 ClientLength = sizeof(Client.get());
    int32 SentLength = sendto(UDPSocket, SendBuffer.GetValue(), SendBuffer.GetSize(), 0, (struct sockaddr*)Client.get(), ClientLength);
#if PLATFORM_WINDOWS
    if (SentLength == SOCKET_ERROR)
    {
        UWUtilities::Print(EWLogType::Error, FString::Printf(L"Socket send failed with error: %d", WSAGetLastError()));
    }
#else
    if (SentLength == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"Socket send failed with error: %s", *UWUtilities::WGetSafeErrorMessage()));
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