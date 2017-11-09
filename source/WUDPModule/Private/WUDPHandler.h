// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDProtocol
#define Pragma_Once_WUDProtocol

#include "WEngine.h"
#include "WMemory.h"
#include "WJson.h"
#include "WMutex.h"
#include "WUtilities.h"
#include "WTaskDefines.h"
#include <unordered_map>
#include <unordered_set>
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <winsock2.h>
#else
    #include <netinet/in.h>
#endif

enum class EWReliableRecordType : uint8
{
    None,
    ClientRecord,
    ReliableConnectionRecord
};

struct WUDPRecord
{

private:
    WUDPRecord() = default;

protected:
    WMutex LastInteraction_Mutex;
    uint64 LastInteraction = 0;

    class UWUDPHandler* ResponsibleHandler = nullptr;

    explicit WUDPRecord(class UWUDPHandler* _ResponsibleHandler);

    EWReliableRecordType Type = EWReliableRecordType::None;

public:
    virtual bool ResetterFunction() = 0; //If returns true, deletes the record after execution.
    virtual uint32 TimeoutValueMS() = 0;

    uint64 GetLastInteraction()
    {
        return LastInteraction;
    }
    virtual void UpdateLastInteraction()
    {
        WScopeGuard Guard(&LastInteraction_Mutex);
        LastInteraction = UWUtilities::GetTimeStampInMS();
    }

    EWReliableRecordType GetType()
    {
        return Type;
    }

    WMutex Dangerzone_Mutex;

    ~WUDPRecord();
};

struct WClientRecord : public WUDPRecord
{

private:
    WMutex LastClientsideTimestamp_Mutex{};
    uint16 LastClientsideTimestamp = 0;

    WMutex TimedOutCount_Mutex{};
    uint32 TimedOutCount = 0;

    std::string ClientKey;

    bool ResetterFunction() override
    {
        SetLastClientsideTimestamp(0);
        {
            WScopeGuard TimedOutCount_Guard(&TimedOutCount_Mutex);
            if (++TimedOutCount > 12) //For 2 minutes, 120000 / 10000
            {
                return true;
            }
        }
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
        if (Timestamp > 0)
        {
            WScopeGuard TimedOutCount_Guard(&TimedOutCount_Mutex);
            TimedOutCount = 0;
        }
    }

    std::string GetClientKey()
    {
        return ClientKey;
    }

    explicit WClientRecord(class UWUDPHandler* ResponsibleHandler, std::string& _ClientKey) : WUDPRecord(ResponsibleHandler)
    {
        Type = EWReliableRecordType::ClientRecord;
        ClientKey = _ClientKey;
    }
};

struct WReliableConnectionRecord : public WUDPRecord
{

private:
    uint32 ClientsideMessageID = 0;

    sockaddr Client{};

    FWCHARWrapper Buffer{};

    bool ResetterFunction() override;

    uint32 TimeoutValueMS() override { return 2000; }

    explicit WReliableConnectionRecord(class UWUDPHandler* ResponsibleHandler) : WUDPRecord(ResponsibleHandler)
    {
        Type = EWReliableRecordType::ReliableConnectionRecord;
    }

    std::string ClientKey;

    bool bAsSender = false;

    //1: SYN
    //2: SYN-ACK
    //3: ACK
    //4: ACK-ACK
    uint8 HandshakingStatus = 0;
    WMutex HandshakingStatus_Mutex{};

public:
    explicit WReliableConnectionRecord(class UWUDPHandler* ResponsibleHandler, uint32 MessageID, sockaddr& ClientRef, std::string& _ClientKey, FWCHARWrapper& BufferRef, bool bAsSenderParameter) : WUDPRecord(ResponsibleHandler)
    {
        Type = EWReliableRecordType::ReliableConnectionRecord;

        ClientsideMessageID = MessageID;

        Client = ClientRef;
        ClientKey = _ClientKey;

        bAsSender = bAsSenderParameter;

        if (BufferRef.GetSize() > 0)
        {
            Buffer.SetValue(new ANSICHAR[BufferRef.GetSize()], BufferRef.GetSize());
            FMemory::Memcpy(Buffer.GetValue(), BufferRef.GetValue(), static_cast<WSIZE__T>(BufferRef.GetSize()));
            Buffer.bDeallocateValueOnDestructor = true;
        }
    }

    uint8 GetHandshakingStatus()
    {
        return HandshakingStatus;
    }
    void SetHandshakingStatus(uint8 NewStatus)
    {
        WScopeGuard Guard(&HandshakingStatus_Mutex);
        HandshakingStatus = NewStatus;
    }

    uint8 FailureTrialCount = 0;

    uint32 GetClientsideMessageID()
    {
        return ClientsideMessageID;
    }
    sockaddr* GetClient()
    {
        return &Client;
    }
    std::string GetClientKey()
    {
        return ClientKey;
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

typedef std::function<void(sockaddr* Client, const FWCHARWrapper& SendBuffer)> WUDPSendCallback;

class UWUDPHandler : public UWAsyncTaskParameter
{

private:
    WMutex ReliableConnectionRecords_Mutex{};
    std::unordered_map<std::string, WReliableConnectionRecord*> ReliableConnectionRecords{};

    WMutex LastServersideGeneratedTimestamp_Mutex{};
    uint16 LastServersideGeneratedTimestamp = 0;

    WMutex LastServersideMessageID_Mutex{};
    uint32 LastServersideMessageID = 1;

    WMutex ClientsRecord_Mutex{};
    std::unordered_map<std::string, WClientRecord*> ClientRecords{};

    WMutex UDPRecordsForTimeoutCheck_Mutex{};
    std::unordered_set<WUDPRecord*> UDPRecordsForTimeoutCheck{};

    void ClearReliableConnections();
    void ClearClientRecords();
    void ClearUDPRecordsForTimeoutCheck();

    bool bSystemStarted = false;

    //
    void AsReceiverReliableSYNSuccess(sockaddr* Client, uint32 MessageID);
    void AsReceiverReliableSYNFailure(sockaddr* Client, uint32 MessageID);

    void HandleReliableSYNDeparture(sockaddr* Client, FWCHARWrapper& Buffer, uint32 MessageID);

    void HandleReliableSYNSuccess(sockaddr* Client, uint32 MessageID);
    void HandleReliableSYNFailure(sockaddr* Client, uint32 MessageID);

    void HandleReliableSYNACKSuccess(sockaddr* Client, uint32 MessageID);

    void HandleReliableACKArrival(sockaddr* Client, uint32 MessageID);
    //

    WReliableConnectionRecord* Create_AddOrGet_ReliableConnectionRecord(sockaddr* Client, uint32 MessageID, FWCHARWrapper& Buffer, bool bAsSender, uint8 EnsureHandshakingStatusEqualsTo = 0, bool bIgnoreFailure = false);
    void CloseCase(WReliableConnectionRecord* Record);

    UWUDPHandler() = default;

public:
    explicit UWUDPHandler(WUDPSendCallback _SendCallback);

    /*
	* if bIgnoreTimestamp == false && Timestamp == 0, sends one package with bReliableSYN = true, bIgnoreTimestamp = true

	[Inclusive:Inclusive]	[Description]
	[0:0 Byte]				[Boolean Protocol Flags] { bReliableSYN, bReliableSYNSuccess, bReliableSYNFailure, bReliableSYNACKSuccess, bReliableACK, bIgnoreTimestamp, bDoubleContentCount }
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

	if (bDoubleContentCount)
     [0:1 Byte]: 0-2 Bits: Variable Type (Max 7), 3-15 Bits Variable Content Count (Max 8191)
    else
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
    WJson::Node AnalyzeNetworkDataWithByteArray(FWCHARWrapper& Parameter, sockaddr* Client);

    //Do not forget to deallocate the result manually.
    FWCHARWrapper MakeByteArrayForNetworkData(
            sockaddr* Client,
            WJson::Node Parameter,
            bool bTimeOrderCriticalData = false,
            bool bReliableSYN = false,
            bool bReliableSYNSuccess = false,
            bool bReliableSYNFailure = false,
            bool bReliableSYNACKSuccess = false,
            bool bReliableACK = false,
            int32 ReliableMessageID = 0,
            bool bDoubleContentCount = false);

    void StartSystem();
    void EndSystem();

    void AddNewUDPRecord(WUDPRecord* NewRecord);

    WUDPSendCallback SendFunction = nullptr;
};

#endif //Pragma_Once_WUDProtocol