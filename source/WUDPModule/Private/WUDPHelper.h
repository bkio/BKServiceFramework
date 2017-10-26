// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WNetworkHelper
#define Pragma_Once_WNetworkHelper

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

#include "WEngine.h"
#include "WString.h"
#include "WMemory.h"
#if PLATFORM_WINDOWS
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <WMemory.h>
#include <WTaskDefines.h>

#else
#include <netinet/in.h>
#endif

class WUDPHelper
{

public:
    static std::string GetAddressPortFromClient(struct sockaddr* Client, uint32 MessageID, bool bDoNotAppendMessageID = false)
    {
        if (Client == nullptr) return "";

        auto ClientAsBroad = (struct sockaddr_in*)Client;

        std::stringstream Stream;
        Stream << inet_ntoa(ClientAsBroad->sin_addr) << ':' << htons(ClientAsBroad->sin_port);
        if (!bDoNotAppendMessageID)
        {
            Stream << ':' << MessageID;
        }

        return Stream.str();
    }
};

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

enum class EWReliableRecordType : uint8
{
    ClientRecord,
    ReliableConnectionRecord
};

struct WUDPRecord
{

protected:
    WMutex LastInteraction_Mutex;
    uint64 LastInteraction = 0;

    EWReliableRecordType Type;

    WUDPRecord();

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

    explicit WClientRecord(std::string& _ClientKey)
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

    WReliableConnectionRecord()
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
    WReliableConnectionRecord(uint32 MessageID, sockaddr& ClientRef, std::string& _ClientKey, FWCHARWrapper& BufferRef, bool bAsSenderParameter)
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

#endif //WUDPHelper
#pragma clang diagnostic pop