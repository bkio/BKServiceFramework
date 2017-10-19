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
#include <WScheduledAsyncTaskManager.h>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <WMemory.h>

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

struct WUDPRecord
{

protected:
    WMutex LastInteraction_Mutex;
    int64 LastInteraction = 0;

    WUDPRecord();

public:
    virtual bool ResetterFunction() = 0; //If returns true, deletes the record after execution.
    virtual uint32 TimeoutValueMS() = 0;

    int64 GetLastInteraction()
    {
        return LastInteraction;
    }
    virtual void UpdateLastInteraction()
    {
        WScopeGuard Guard(&LastInteraction_Mutex);
        LastInteraction = UWUtilities::GetTimeStampInMS();
    }

    WMutex Dangerzone_Mutex;

    ~WUDPRecord();
};

struct WClientRecord : public WUDPRecord
{

private:
    WMutex LastClientsideTimestamp_Mutex;
    uint16 LastClientsideTimestamp = 0;

    bool ResetterFunction() override
    {
        SetLastClientsideTimestamp(0);
        return false;
    }
    uint32 TimeoutValueMS() override { return 10000; }

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

    WClientRecord() = default;
};

struct WReliableConnectionRecord : public WUDPRecord
{

private:
    uint32 ClientsideMessageID = 0;

    sockaddr Client{};

    FWCHARWrapper Buffer;

    bool ResetterFunction() override;

    uint32 TimeoutValueMS() override { return 2000; }

    WReliableConnectionRecord() = default;

public:
    WReliableConnectionRecord(uint32 MessageID, sockaddr& ClientRef, FWCHARWrapper& BufferRef)
    {
        ClientsideMessageID = MessageID;

        Client = ClientRef;

        if (BufferRef.GetSize() > 0)
        {
            Buffer.SetValue(new ANSICHAR[BufferRef.GetSize()], BufferRef.GetSize());
            FMemory::Memcpy(Buffer.GetValue(), BufferRef.GetValue(), static_cast<WSIZE__T>(BufferRef.GetSize()));
            Buffer.bDeallocateValueOnDestructor = true;
        }
    }

    //1: SYN
    //2: SYN-ACK
    //3: ACK
    //4: ACK-ACK
    uint8 HandshakingStatus = 0;
    WMutex HandshakingStatus_Mutex;

    uint8 FailureTrialCount = 0;

    uint32 GetClientsideMessageID()
    {
        return ClientsideMessageID;
    }
    sockaddr* GetClient()
    {
        return &Client;
    }
    FWCHARWrapper* GetBuffer()
    {
        return &Buffer;
    }
    void ReplaceBuffer(FWCHARWrapper& NewBuffer)
    {
        if (NewBuffer.GetSize() > 0)
        {
            Buffer.DeallocateValue();
            Buffer.SetValue(new ANSICHAR[NewBuffer.GetSize()], NewBuffer.GetSize());
            FMemory::Memcpy(Buffer.GetValue(), NewBuffer.GetValue(), static_cast<WSIZE__T>(NewBuffer.GetSize()));
        }
    }
};

class UWUDPManager
{

public:
    static bool StartSystem(uint16 Port);
    static void EndSystem();

    static void ClearClientRecords();

    /*
	* if bIgnoreTimestamp == false && Timestamp == 0, sends one package with bReliableSYN = true, bIgnoreTimestamp = true

	[Inclusive:Inclusive]	[Description]
	[0:0 Byte]				[Boolean Protocol Flags] { bReliableSYN, bReliableSYNSuccess, bReliableSYNFailure, bReliableSYNACKSuccess, bReliableACK, bIgnoreTimestamp }
	[1:4 Byte]				[Message ID] (If one of bReliable(s) = true)
	[A:B Byte]				[Checksum] ((If bReliableSYN = true & other bReliable(s) = false) | all-bReliable(s) = false)
	[C:D Byte]				[Timestamp] (If bIgnoreTimestamp = false)

	A:B:
	if [Message ID] does not exist: 1:4
	else: 5:8

	C:D:
	if [Message ID] does not exist: 5:6
	else: 9:10

	After X (Inclusive) ((If bReliableSYN = true & other bReliable(s) = false) | all-bReliable(s) = false)
	X:
	if bIgnoreTimestamp = false & [Message ID] exists : 11th
	if bIgnoreTimestamp = false & [Message ID] does not exist: : 7th
	if bIgnoreTimestamp = true  & [Message ID] exists: : 9th
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
    static FWCHARWrapper MakeByteArrayForNetworkData(
            sockaddr* Client,
            std::shared_ptr<WJson::Node> Parameter,
            bool bTimeOrderCriticalData = false,
            bool bReliableSYN = false,
            bool bReliableSYNSuccess = false,
            bool bReliableSYNFailure = false,
            bool bReliableSYNACKSuccess = false,
            bool bReliableACK = false,
            int32 ReliableMessageID = 0);

    static void AddNewUDPRecord(WUDPRecord* NewRecord);
    static void RemoveUDPRecord(WUDPRecord* OldRecord);

    static bool ReliableDataTimedOut(WReliableConnectionRecord* Record);

private:
    static bool bSystemStarted;

    bool StartSystem_Internal(uint16 Port);
    void EndSystem_Internal();

    //
    void AsReceiverReliableSYNSuccess(sockaddr* Client, uint32 MessageID);
    void AsReceiverReliableSYNFailure(sockaddr* Client, uint32 MessageID);

    void HandleReliableSYNDeparture(sockaddr* Client, FWCHARWrapper& Buffer, uint32 MessageID);

    void HandleReliableSYNSuccess(sockaddr* Client, uint32 MessageID);
    void HandleReliableSYNFailure(sockaddr* Client, uint32 MessageID);

    void HandleReliableSYNACKSuccess(sockaddr* Client, uint32 MessageID);

    void HandleReliableACKArrival(sockaddr *Client, uint32 MessageID);
    //

    WReliableConnectionRecord* Create_AddOrGet_ReliableConnectionRecord(sockaddr* Client, uint32 MessageID, FWCHARWrapper& Buffer, uint8 EnsureHandshakingStatusEqualsTo = 0, bool bIgnoreFailure = false);
    void CloseCase(WReliableConnectionRecord* Record);

    struct sockaddr_in UDPServer;

    WMutex ClientsRecord_Mutex;
    std::unordered_map<ANSICHAR*, WClientRecord*> ClientRecords;

    WMutex ReliableConnectionRecords_Mutex;
    std::unordered_map<ANSICHAR*, TArray<WReliableConnectionRecord*>> ReliableConnectionRecords;

    WMutex UDPRecordsForTimeoutCheck_Mutex;
    std::unordered_set<WUDPRecord*> UDPRecordsForTimeoutCheck;

    WMutex LastServersideGeneratedTimestamp_Mutex;
    uint16 LastServersideGeneratedTimestamp = 0;

    WMutex LastServersideMessageID_Mutex;
    uint32 LastServersideMessageID = 1;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket;
#else
    int32 UDPSocket;
#endif

    bool InitializeSocket(uint16 Port);
    void CloseSocket();
    void ListenSocket();
    void Send(sockaddr* Client, const FWCHARWrapper& SendBuffer);

    void ClearReliableConnections();
    void ClearUDPRecordsForTimeoutCheck();

    WThread* UDPSystemThread;

    static UWUDPManager* ManagerInstance;
    UWUDPManager() = default;
};

#endif