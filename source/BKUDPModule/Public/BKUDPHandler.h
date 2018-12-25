// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKUDProtocol
#define Pragma_Once_BKUDProtocol

#include "BKEngine.h"
#include "BKMemory.h"
#include "BKJson.h"
#include "BKMutex.h"
#include "BKReferenceCounter.h"
#include "BKUtilities.h"
#include "BKTaskDefines.h"
#include "BKSafeQueue.h"
#include <unordered_map>
#include <unordered_set>
#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <winsock2.h>
#else
    #include <netinet/in.h>
#endif

enum class EBKReliableRecordType : uint8
{
    None,
    OtherPartyRecord,
    ReliableConnectionRecord
};

class BKUDPRecord : public BKReferenceCountable
{

private:
    BKUDPRecord() = default;

protected:
    BKMutex LastInteraction_Mutex;
    uint64 LastInteraction = 0;

    class BKUDPHandler* ResponsibleHandler = nullptr;

    explicit BKUDPRecord(class BKUDPHandler* _ResponsibleHandler);

    EBKReliableRecordType Type = EBKReliableRecordType::None;

public:
    virtual bool ResetterFunction() = 0; //If returns true, deletes the record after execution.
    virtual uint32 TimeoutValueMS() = 0;

    uint64 GetLastInteraction()
    {
        return LastInteraction;
    }
    virtual void UpdateLastInteraction()
    {
        BKScopeGuard Guard(&LastInteraction_Mutex);
        LastInteraction = BKUtilities::GetTimeStampInMS();
    }

    EBKReliableRecordType GetType()
    {
        return Type;
    }

    bool bBeingDeleted = false;

    virtual ~BKUDPRecord() = default;
};

class BKOtherPartyRecord : public BKUDPRecord
{

private:
    BKMutex LastSendersideTimestamp_Mutex{};
    uint16 LastSendersideTimestamp = 0;

    BKMutex TimedOutCount_Mutex{};
    uint32 TimedOutCount = 0;

    FString OtherPartyKey;

    bool ResetterFunction() override
    {
        SetLastSendersideTimestamp(0);
        {
            BKScopeGuard TimedOutCount_Guard(&TimedOutCount_Mutex);
            if (++TimedOutCount > 12) //For 2 minutes, 120000 / 10000
            {
                return true;
            }
        }
        return false;
    }
    uint32 TimeoutValueMS() override { return 10000; }

public:
    uint16 GetLastSendersideTimestamp()
    {
        return LastSendersideTimestamp;
    }
    void SetLastSendersideTimestamp(uint16 Timestamp)
    {
        BKScopeGuard Guard(&LastSendersideTimestamp_Mutex);
        LastSendersideTimestamp = Timestamp;
        if (Timestamp > 0)
        {
            BKScopeGuard TimedOutCount_Guard(&TimedOutCount_Mutex);
            TimedOutCount = 0;
        }
    }

    FString GetOtherPartyKey()
    {
        return OtherPartyKey;
    }

    explicit BKOtherPartyRecord(class BKUDPHandler* ResponsibleHandler, FString& _OtherPartyKey) : BKUDPRecord(ResponsibleHandler)
    {
        Type = EBKReliableRecordType::OtherPartyRecord;
        OtherPartyKey = _OtherPartyKey;
    }
};

class BKReliableConnectionRecord : public BKUDPRecord
{

private:
    uint32 SendersideMessageID = 0;

    sockaddr OtherParty{};

    FBKCHARWrapper Buffer{};

    bool ResetterFunction() override;

    uint32 TimeoutValueMS() override { return 2000; }

    explicit BKReliableConnectionRecord(class BKUDPHandler* ResponsibleHandler) : BKUDPRecord(ResponsibleHandler)
    {
        Type = EBKReliableRecordType::ReliableConnectionRecord;
    }

    FString OtherPartyKey;

    bool bAsSender = false;
    bool bAsSender_PrevFrameSkipped = false;

    //1: SYN
    //2: SYN-ACK
    //3: ACK
    //4: ACK-ACK
    uint8 HandshakingStatus = 0;
    BKMutex HandshakingStatus_Mutex{};

public:
    explicit BKReliableConnectionRecord(class BKUDPHandler* ResponsibleHandler, uint32 MessageID, sockaddr& OtherPartyRef, FString& _OtherPartyKey, FBKCHARWrapper& BufferRef, bool bAsSenderParameter) : BKUDPRecord(ResponsibleHandler)
    {
        Type = EBKReliableRecordType::ReliableConnectionRecord;

        SendersideMessageID = MessageID;

        OtherParty = OtherPartyRef;
        OtherPartyKey = _OtherPartyKey;

        bAsSender = bAsSenderParameter;

        if (BufferRef.GetSize() > 0)
        {
            Buffer.SetValue(new ANSICHAR[BufferRef.GetSize()], BufferRef.GetSize());
            FMemory::Memcpy(Buffer.GetValue(), BufferRef.GetValue(), static_cast<WSIZE__T>(BufferRef.GetSize()));
        }
    }
    ~BKReliableConnectionRecord() override
    {
        if (Buffer.IsValid())
        {
            Buffer.DeallocateValue();
        }
    }

    uint8 GetHandshakingStatus()
    {
        return HandshakingStatus;
    }
    void SetHandshakingStatus(uint8 NewStatus)
    {
        BKScopeGuard Guard(&HandshakingStatus_Mutex);
        HandshakingStatus = NewStatus;
    }

    uint8 FailureTrialCount = 0;

    uint32 GetSendersideMessageID()
    {
        return SendersideMessageID;
    }
    sockaddr* GetOtherParty()
    {
        return &OtherParty;
    }
    FString GetOtherPartyKey()
    {
        return OtherPartyKey;
    }
    FBKCHARWrapper* GetBuffer()
    {
        return &Buffer;
    }
    void ReplaceBuffer(FBKCHARWrapper& NewBuffer)
    {
        if (NewBuffer.GetSize() > 0)
        {
            Buffer.DeallocateValue();
            Buffer.SetValue(new ANSICHAR[NewBuffer.GetSize()], NewBuffer.GetSize());
            FMemory::Memcpy(Buffer.GetValue(), NewBuffer.GetValue(), static_cast<WSIZE__T>(NewBuffer.GetSize()));
        }
    }
};

#define TIMEOUT_CHECK_TIME_INTERVAL 100
#define PENDING_DELETE_CHECK_TIME_INTERVAL 100
#define RELIABLE_CONNECTION_NOT_FOUND 255

class BKUDPHandler : public BKAsyncTaskParameter
{

private:
    BKMutex ReliableConnectionRecords_Mutex{};
    std::unordered_map<FString, BKReliableConnectionRecord*> ReliableConnectionRecords;
    void RemoveFromReliableConnections(std::__detail::_Node_iterator<std::pair<const FString, BKReliableConnectionRecord *>, false, true> Iterator);
    void RemoveFromReliableConnections(const FString& Key);

    BKMutex LastThissideGeneratedTimestamp_Mutex{};
    uint16 LastThissideGeneratedTimestamp = 0;

    BKMutex LastThissideMessageID_Mutex{};
    uint32 LastThissideMessageID = 1;

    BKMutex OtherPartiesRecords_Mutex{};
    std::unordered_map<FString, BKOtherPartyRecord*> OtherPartiesRecords{};

    BKSafeQueue<BKUDPRecord*> UDPRecordsForTimeoutCheck;

    BKMutex UDPRecords_PendingDeletePool_Mutex;
    std::unordered_map<BKUDPRecord*, uint64> UDPRecords_PendingDeletePool;
    void AddRecordToPendingDeletePool(BKUDPRecord* PendingDeleteRecord);

    void ClearReliableConnections();
    void ClearOtherPartiesRecords();
    void ClearUDPRecordsForTimeoutCheck();
    void ClearPendingDeletePool();

    BKMutex SendMutex;

#if PLATFORM_WINDOWS
    SOCKET UDPSocket_Ref{};
#else
    int32 UDPSocket_Ref{};
#endif

    bool bSystemStarted = false;

    bool bPendingKill = false;
    std::function<void()> ReadyToDieCallback = nullptr;

    //
    void AsReceiverReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID);
    void AsReceiverReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID);

    void HandleReliableSYNDeparture(sockaddr* OtherParty, FBKCHARWrapper& Buffer, uint32 MessageID);

    void HandleReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID);
    void HandleReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID);

    void HandleReliableSYNACKSuccess(sockaddr* OtherParty, uint32 MessageID);

    void HandleReliableACKArrival(sockaddr* OtherParty, uint32 MessageID);
    //

    BKReliableConnectionRecord* Create_AddOrGet_ReliableConnectionRecord(sockaddr* OtherParty, uint32 MessageID, FBKCHARWrapper& Buffer, bool bAsSender, uint8 EnsureHandshakingStatusEqualsTo = 0, bool bIgnoreFailure = false);
    void CloseCase(BKReliableConnectionRecord* Record);

    BKUDPHandler() = default;

public:
#if PLATFORM_WINDOWS
    explicit BKUDPHandler(SOCKET _UDPSocket);
#else
    explicit WUDPHandler(int32 _UDPSocket);
#endif

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
    BKJson::Node AnalyzeNetworkDataWithByteArray(FBKCHARWrapper& Parameter, sockaddr* OtherParty);

    //Do not forget to deallocate the result manually.
    FBKCHARWrapper MakeByteArrayForNetworkData(
            sockaddr* OtherParty,
            BKJson::Node Parameter,
            bool bDoubleContentCount = false,
            bool bTimeOrderCriticalData = false,
            bool bReliableSYN = false,
            bool bReliableSYNSuccess = false,
            bool bReliableSYNFailure = false,
            bool bReliableSYNACKSuccess = false,
            bool bReliableACK = false,
            int32 ReliableMessageID = 0);

    void StartSystem();
    void EndSystem();

    void AddNewUDPRecord(BKUDPRecord* NewRecord);

    void MarkPendingKill(std::function<void()> _ReadyToDieCallback);

    void Send(sockaddr* OtherParty, const FBKCHARWrapper& SendBuffer);
};

#endif //Pragma_Once_BKUDProtocol