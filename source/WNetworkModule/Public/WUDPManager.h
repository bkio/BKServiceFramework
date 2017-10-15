// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPManager
#define Pragma_Once_WUDPManager

#include "WEngine.h"
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif
#include "WThread.h"
#include "WUtilities.h"
#include <WJson.h>
#include <WAsyncTaskManager.h>
#include <iostream>
#include <memory>

struct FWUDPTaskParameter : public FWAsyncTaskParameter
{

public:
    int32 BufferSize = 0;
    ANSICHAR* Buffer = nullptr;
    sockaddr* Client = nullptr;

    FWUDPTaskParameter(int32 BufferSizeParameter, ANSICHAR* BufferParameter, sockaddr* ClientParameter)
    {
        BufferSize = BufferSizeParameter;
        Buffer = BufferParameter;
        Client = ClientParameter;
    }
    ~FWUDPTaskParameter() override
    {
        if (Client)
        {
            delete (Client);
        }
        delete[] Buffer;
    }
};

struct WClientRecord
{

private:
    WMutex LastClientsideTimestamp_Mutex;
    uint16 LastClientsideTimestamp = 0;

    WMutex LastInteraction_Mutex;
    int64 LastInteraction = 0;

public:
    uint16 GetLastClientsideTimestamp()
    {
        return LastClientsideTimestamp;
    }
    void SetLastClientsideTimestamp(uint16 Timestamp)
    {
        WScopeGuard Guard(&LastClientsideTimestamp_Mutex);
        LastClientsideTimestamp = Timestamp;
    }

    int64 GetLastInteraction()
    {
        return LastInteraction;
    }

    WClientRecord()
    {
        LastInteraction = UWUtilities::GetTimeStampInMS();
    }
    WClientRecord* UpdateLastInteraction()
    {
        {
            WScopeGuard Guard(&LastInteraction_Mutex);
            LastInteraction = UWUtilities::GetTimeStampInMS();
        }
        return this;
    }
};

class UWUDPManager
{

public:
    static bool StartSystem(uint16 Port);
    static void EndSystem();

    static void ClearClientRecords();

    /*
    * Server: if bIgnoreTimestamp == false && Timestamp == 0, sends one package with bReliable = true, bIgnoreTimestamp = true

    [Inclusive:Inclusive]	[Description]
    [0:0 Byte]				[Boolean Protocol Flags] { bReliable, bIgnoreTimestamp }
    [1:4 Byte]				[Message ID] (If bReliable = true)
    [A:B Byte]				[Checksum]
    [C:D Byte]				[Timestamp] (If bIgnoreTimestamp = false)

    A:B:
    if bReliable == false: 1:4
    else: 5:8

    C:D: (If bIgnoreTimestamp = false, else ignore)
    if bReliable == false: 5:6
    else: 9:10

    After X (Inclusive)
    X:
    if bIgnoreTimestamp == false && bReliable == true: 11th
    if bIgnoreTimestamp == false && bReliable == false: 7th
    if bIgnoreTimestamp == true  && bReliable == true: 9th
    else: 5th

    0th Byte: 0-2 Bits: Variable Type (Max 7), 3-7 Bits Variable Content Count (Max 31)

    Variable Types:
    0->Boolean Array	Variable Content Count: Number of booleans (per: 1 bit)
    1->Byte	Array		Variable Content Count: Number of bytes (per: 1 byte)
    2->Char Array		Variable Content Count: Number of chars (per: 1 byte)
    3->Short Array		Variable Content Count: Number of shorts (per: 2 bytes)
    4->Integer Array	Variable Content Count: Number of integers (per: 4 bytes)
    5->Float Array		Variable Content Count: Number of floats (per: 4 bytes)

    Result:
    {
        "BooleanArray": [false, true, false],
        "FloatArray": [1.0, 5.5],
        "CharArray": "Demonstration"
    }
    */
    static std::shared_ptr<WJson::Node> AnalyzeNetworkDataWithByteArray(FWCHARWrapper& Parameter, sockaddr* Client);

    //Do not forget to deallocate the result manually.
    static FWCHARWrapper MakeByteArrayForNetworkData(std::shared_ptr<WJson::Node> Parameter, bool bReliable = false, bool bTimeOrderCriticalData = false);

private:
    static bool bSystemStarted;

    bool StartSystem_Internal(uint16 Port);
    void EndSystem_Internal();

    void HandleReliableData(uint32 MessageID, bool bSuccess);

    struct sockaddr_in UDPServer;

    WMutex ClientsRecord_Mutex;
    std::map<ANSICHAR*, WClientRecord*> ClientRecords;

    WMutex LastServersideGeneratedTimestamp_Mutex;
    uint16 LastServersideGeneratedTimestamp = 0;

    WMutex LastServersideMessageID_Mutex;
    uint32 LastServersideMessageID = 0;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket;
#else
    int32 UDPSocket;
#endif

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    void Send(sockaddr* Client, const FWCHARWrapper& SendBuffer);

    WThread* UDPSystemThread;

    static UWUDPManager* ManagerInstance;
    UWUDPManager() = default;
};

#endif