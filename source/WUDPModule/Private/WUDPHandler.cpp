// Copyright Pagansoft.com, All rights reserved.

#include "WUDPHandler.h"
#include "WUDPHelper.h"
#include "WMath.h"
#include "WScheduledTaskManager.h"

#if PLATFORM_WINDOWS
UWUDPHandler::UWUDPHandler(SOCKET _UDPSocket)
#else
UWUDPHandler::UWUDPHandler(int32 _UDPSocket)
#endif
{
    UDPSocket_Ref = _UDPSocket;
}

void UWUDPHandler::ClearReliableConnections()
{
    if (!bSystemStarted) return;

    WScopeGuard Guard(&ReliableConnectionRecords_Mutex);
    for (auto& It : ReliableConnectionRecords)
    {
        WReliableConnectionRecord* Record = It.second;
        if (Record)
        {
            WScopeGuard Dangerzone_Guard(&Record->Dangerzone_Mutex);
            delete (Record);
        }
    }
    ReliableConnectionRecords.clear();
}
void UWUDPHandler::ClearClientRecords()
{
    if (!bSystemStarted) return;

    WScopeGuard Guard(&ClientsRecord_Mutex);
    for (auto& It : ClientRecords)
    {
        if (It.second != nullptr)
        {
            WScopeGuard Dangerouszone_Guard(&It.second->Dangerzone_Mutex);
            delete (It.second);
        }
    }
    ClientRecords.clear();
}

WJson::Node UWUDPHandler::AnalyzeNetworkDataWithByteArray(FWCHARWrapper& Parameter, sockaddr* OtherParty)
{
    if (!bSystemStarted || OtherParty == nullptr) return WJson::Node(WJson::Node::T_INVALID);
    if (Parameter.GetSize() < 5) return WJson::Node(WJson::Node::T_INVALID);

    //Boolean flags operation starts.
    TArray<bool> ResultOfDecompress;
    if (!UWUtilities::DecompressBitAsBoolArray(ResultOfDecompress, Parameter, 0, 0)) return WJson::Node(WJson::Node::T_INVALID);
    bool bReliableSYN = ResultOfDecompress[0];
    bool bReliableSYNSuccess = ResultOfDecompress[1];
    bool bReliableSYNFailure = ResultOfDecompress[2];
    bool bReliableSYNACKSuccess = ResultOfDecompress[3];
    bool bReliableACK = ResultOfDecompress[4];
    bool bIgnoreTimestamp = ResultOfDecompress[5];
    bool bDoubleContentCount = ResultOfDecompress[6];
    //

    bool bReliable = bReliableSYN || bReliableSYNSuccess || bReliableSYNFailure || bReliableSYNACKSuccess || bReliableACK;

    //Reliable operation starts.
    uint32 MessageID = 0;
    if (bReliable)
    {
        FMemory::Memcpy(&MessageID, Parameter.GetValue() + 1, 4);
    }
    //

    if (!bReliableSYN && MessageID > 0)
    {
        if (bReliableSYNSuccess)
        {
            HandleReliableSYNSuccess(OtherParty, MessageID);
        }
        else if (bReliableSYNFailure)
        {
            HandleReliableSYNFailure(OtherParty, MessageID);
        }
        else if (bReliableSYNACKSuccess)
        {
            HandleReliableSYNACKSuccess(OtherParty, MessageID);
        }
        else if (bReliableACK)
        {
            HandleReliableACKArrival(OtherParty, MessageID);
        }
        else
        {
            return WJson::Node(WJson::Node::T_INVALID);
        }

        return WJson::Node(WJson::Node::T_VALIDATION);
    }
    else if (bReliable && !bReliableSYN && MessageID == 0)
    {
        return WJson::Node(WJson::Node::T_INVALID);
    }

    //Checksum operations start.
    const int32 AfterChecksumStartIx = bReliable ? 9 : 5;
    if (Parameter.GetSize() < (AfterChecksumStartIx + 1))
    {
        if (bReliableSYN)
        {
            AsReceiverReliableSYNFailure(OtherParty, MessageID);
        }
        return WJson::Node(WJson::Node::T_INVALID);
    }
    const int32 ChecksumStartIx = bReliable ? 5 : 1;

    FWCHARWrapper Hashed = UWUtilities::WBasicRawHash(Parameter, AfterChecksumStartIx, Parameter.GetSize() - AfterChecksumStartIx);
    Hashed.bDeallocateValueOnDestructor = true;
    for (int32 i = 0; i < 4; i++)
    {
        if (Hashed.GetArrayElement(i) != Parameter.GetArrayElement(i + ChecksumStartIx))
        {
            if (bReliableSYN)
            {
                AsReceiverReliableSYNFailure(OtherParty, MessageID);
            }
            return WJson::Node(WJson::Node::T_INVALID);
        }
    }
    //

    //Timestamp operation starts.
    {
        WClientRecord* ClientRecord = nullptr;
        {
            std::string ClientKey = WUDPHelper::GetAddressPortFromClient(OtherParty, MessageID, true);

            WScopeGuard Guard(&ClientsRecord_Mutex);
            auto It = ClientRecords.find(ClientKey);
            if (It != ClientRecords.end())
            {
                if (It->second == nullptr)
                {
                    It->second = new WClientRecord(this, ClientKey);
                }
                else
                {
                    It->second->UpdateLastInteraction();
                }
                ClientRecord = It->second;
            }
            else
            {
                ClientRecord = new WClientRecord(this, ClientKey);
                ClientRecords.insert(std::pair<std::string, WClientRecord*>(ClientKey, ClientRecord));
            }
        }

        uint16 Timestamp = 0;
        if (!bIgnoreTimestamp)
        {
            if (Parameter.GetSize() < (AfterChecksumStartIx + 3 /* + 2 + 1 */))
            {
                if (bReliableSYN)
                {
                    AsReceiverReliableSYNFailure(OtherParty, MessageID);
                }
                return WJson::Node(WJson::Node::T_INVALID);
            }

            FMemory::Memcpy(&Timestamp, Parameter.GetValue() + AfterChecksumStartIx, 2);

            const uint16 LastClientsideTimestamp = ClientRecord->GetLastClientsideTimestamp();
            if (LastClientsideTimestamp != 0 && Timestamp < LastClientsideTimestamp)
            {
                if (bReliableSYN)
                {
                    AsReceiverReliableSYNFailure(OtherParty, MessageID);
                }
                return WJson::Node(WJson::Node::T_INVALID);
            }

            ClientRecord->SetLastClientsideTimestamp(Timestamp);
        }
        else
        {
            ClientRecord->SetLastClientsideTimestamp(0);
        }
    }
    //

    //Reliable confirmation
    if (bReliableSYN)
    {
        AsReceiverReliableSYNSuccess(OtherParty, MessageID);
    }
    //

    //Generic parts decoding starts.
    const int32 GenericPartStartIx =
            (bIgnoreTimestamp && bReliable) ? 9 :
            ((!bIgnoreTimestamp && bReliable) ? 11 :
             (!bIgnoreTimestamp ? 7 : 5));

    if (Parameter.GetSize() < (GenericPartStartIx + 1)) return WJson::Node(WJson::Node::T_INVALID);

    WJson::Node ResultMap = WJson::Node(WJson::Node::T_OBJECT);

    int32 RemainedBytes = Parameter.GetSize() - GenericPartStartIx;
    while (RemainedBytes > 0)
    {
        if (RemainedBytes <= (bDoubleContentCount ? 2 : 1)) break;

        ANSICHAR CurrentChar = Parameter.GetArrayElement(Parameter.GetSize() - RemainedBytes);

        //Variable Type
        auto VariableType = static_cast<uint8>(CurrentChar & 0b00000111);

        //Variable Content Count
        uint16 VariableContentCount = 0;
        auto VariableContentCount_1 = static_cast<uint8>((CurrentChar & 0b11111000) >> 3);
        if (bDoubleContentCount)
        {
            FWCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
            Wrapper.SetArrayElement(0, VariableContentCount_1);
            auto VariableContentCount_2 = Parameter.GetArrayElement(Parameter.GetSize() - RemainedBytes + 1);
            Wrapper.SetArrayElement(1, VariableContentCount_2);
            VariableContentCount = static_cast<uint16>(UWUtilities::ConvertByteArrayToInteger(Wrapper, 0, 2));
        }
        else
        {
            VariableContentCount = VariableContentCount_1;
        }

        RemainedBytes -= (bDoubleContentCount ? 2 : 1);

        if (VariableContentCount == 0) continue;

        int32 StartIndex = Parameter.GetSize() - RemainedBytes;

        //Boolean Array
        if (VariableType == 0)
        {
            //Variable Content Count: Number of booleans (per: 1 bit)

            auto AsByteNo = (uint8)FMath::CeilToInt(((float)VariableContentCount) / 8.0f);
            if (RemainedBytes < AsByteNo) break;

            TArray<bool> BoolArray;
            if (!UWUtilities::DecompressBitAsBoolArray(BoolArray, Parameter, StartIndex, StartIndex + AsByteNo - 1)) break;

            RemainedBytes -= AsByteNo;

            BoolArray.SetNum(VariableContentCount);

            WJson::Node Exists = ResultMap.Get("BooleanArray");
            if (Exists.GetType() == WJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(BoolArray[i]));
                ResultMap.Remove("BooleanArray");
                ResultMap.Add("BooleanArray", Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Node(WJson::Node::T_ARRAY);
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(BoolArray[i]));
                ResultMap.Add("BooleanArray", NewList);
            }
        }
            //Byte Array
        else if (VariableType == 1)
        {
            //Variable Content Count: Number of bytes (per: 1 byte)

            if (RemainedBytes < VariableContentCount) break;

            TArray<uint8> ByteArray;
            for (int32 i = 0; i < VariableContentCount; i++) ByteArray.Add((uint8)Parameter.GetArrayElement(i + StartIndex));

            RemainedBytes -= VariableContentCount;

            WJson::Node Exists = ResultMap.Get("ByteArray");
            if (Exists.GetType() == WJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(ByteArray[i]));
                ResultMap.Remove("ByteArray");
                ResultMap.Add("ByteArray", Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Node(WJson::Node::T_ARRAY);
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(ByteArray[i]));
                ResultMap.Add("ByteArray", NewList);
            }
        }
            //Char Array
        else if (VariableType == 2)
        {
            //Variable Content Count: Number of chars (per: 1 byte)

            if (RemainedBytes < VariableContentCount) break;

            const ANSICHAR* StringPart = Parameter.GetValue() + StartIndex;

            FString CharArray;
            for (int32 i = 0; i < VariableContentCount; i++)
            {
                CharArray.AppendChar(StringPart[i]);
            }

            RemainedBytes -= VariableContentCount;

            WJson::Node Exists = ResultMap.Get("CharArray");
            if (Exists.GetType() == WJson::Node::Type::T_STRING)
            {
                CharArray = FString(Exists.ToString("")) + CharArray;
                ResultMap.Remove("CharArray");
            }
            std::string AsString = CharArray.GetAnsiCharArray();
            ResultMap.Add("CharArray", WJson::Node(AsString));
        }
            //Short, Integer, Float Array
        else if (VariableType == 3 || VariableType == 4 || VariableType == 5)
        {
            //Variable Content Count: Number of shorts (per: 2 bytes)
            //Variable Content Count: Number of integers (per: 4 bytes)
            //Variable Content Count: Number of floats (per: 4 bytes)

            auto UnitSize = static_cast<uint8>(VariableType == 3 ? 2 : 4);

            int32 AsArraySize = VariableContentCount * UnitSize;
            if (RemainedBytes < AsArraySize) break;

            TArray<int32> IntArray;
            TArray<float> FloatArray;
            for (int32 i = 0; i < AsArraySize; i += UnitSize)
            {
                if (VariableType == 5)	FloatArray.Add(UWUtilities::ConvertByteArrayToFloat(Parameter, StartIndex + i, UnitSize));
                else					IntArray.Add(UWUtilities::ConvertByteArrayToInteger(Parameter, StartIndex + i, UnitSize));
            }

            RemainedBytes -= AsArraySize;

            std::string Key = VariableType == 3 ? "ShortArray" : (VariableType == 4 ? "IntegerArray" : "FloatArray");
            WJson::Node Exists = ResultMap.Get(Key);
            if (Exists.GetType() == WJson::Node::Type::T_ARRAY)
            {
                if (VariableType == 5)
                {
                    for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(FloatArray[i]));
                }
                else
                {
                    for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(IntArray[i]));
                }
                ResultMap.Remove(Key);
                ResultMap.Add(Key, Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Node(WJson::Node::T_ARRAY);

                if (VariableType == 5)
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(FloatArray[i]));
                }
                else
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(IntArray[i]));
                }
                ResultMap.Add(Key, NewList);
            }
        }
    }
    //

    return ResultMap;
}

FWCHARWrapper UWUDPHandler::MakeByteArrayForNetworkData(
        sockaddr* OtherParty,
        WJson::Node Parameter,
        bool bTimeOrderCriticalData,
        bool bReliableSYN,
        bool bReliableSYNSuccess,
        bool bReliableSYNFailure,
        bool bReliableSYNACKSuccess,
        bool bReliableACK,
        int32 ReliableMessageID,
        bool bDoubleContentCount)
{
    if (!bSystemStarted) return FWCHARWrapper();
    if (OtherParty == nullptr ||
        (!Parameter.IsObject() &&
         (Parameter.IsValidation() && ReliableMessageID == 0)))
        return FWCHARWrapper();

    TArray<ANSICHAR> Result;

    if (LastServersideGeneratedTimestamp == 65535)
    {
        bReliableSYN = true;
        bTimeOrderCriticalData = false;
    }

    //Boolean flags operation starts.
    TArray<bool> Flags;
    Flags.Add(bReliableSYN);
    Flags.Add(bReliableSYNSuccess);
    Flags.Add(bReliableSYNFailure);
    Flags.Add(bReliableSYNACKSuccess);
    Flags.Add(bReliableACK);
    Flags.Add(!bTimeOrderCriticalData);
    Flags.Add(bDoubleContentCount);

    bool bReliable = bReliableSYN || bReliableSYNSuccess || bReliableSYNFailure || bReliableSYNACKSuccess || bReliableACK;

    FWCHARWrapper CompressedFlags(new ANSICHAR[1], 1, true);
    if (!UWUtilities::CompressBooleanAsBit(CompressedFlags, Flags)) return FWCHARWrapper();

    Result.Add(CompressedFlags.GetArrayElement(0));
    //

    //Reliable operations start.
    bool bReliableValidation = false;
    uint32 MessageID = 0;
    if (bReliable)
    {
        if (ReliableMessageID != 0)
        {
            MessageID = (uint32)ReliableMessageID;
            bReliableValidation = true;
        }
        else
        {
            WScopeGuard Guard(&LastServersideMessageID_Mutex);

            if (LastServersideMessageID == (uint32)4294967295) LastServersideMessageID = 1;
            else LastServersideMessageID++;

            MessageID = LastServersideMessageID;
        }

        FWCHARWrapper ConvertedMessageID(new ANSICHAR[4], 4, true);
        FMemory::Memcpy(ConvertedMessageID.GetValue(), &MessageID, 4);
        for (int32 i = 0; i < 4; i++) Result.Add(ConvertedMessageID.GetArrayElement(i));
    }
    //

    if (!bReliableValidation)
    {
        //Checksum initial operation starts.
        int32 ChecksumInsertIx = Result.Num();
        //

        //Timestamp operations start.
        if (bTimeOrderCriticalData)
        {
            uint16 Timestamp;
            {
                WScopeGuard Guard(&LastServersideGeneratedTimestamp_Mutex);

                if (LastServersideGeneratedTimestamp == (uint16)65535) LastServersideGeneratedTimestamp = 0;
                else LastServersideGeneratedTimestamp++;

                Timestamp = LastServersideGeneratedTimestamp;
            }

            FWCHARWrapper ConvertedTimestamp(new ANSICHAR[2], 2, true);
            FMemory::Memcpy(ConvertedTimestamp.GetValue(), &Timestamp, 2);
            for (int32 i = 0; i < 2; i++) Result.Add(ConvertedTimestamp.GetArrayElement(i));
        }
        //

        auto MaxValue = static_cast<uint16>(bDoubleContentCount ? 8192 : 32);

        //Generic parts encoding starts.
        for (const WJson::NamedNode& NamedNode : Parameter)
        {
            uint16 InfoByte;

            std::string KeyString = NamedNode.first;
            if (KeyString == "CharArray")
            {
                std::string ValueString = NamedNode.second.ToString("");
                int32 Length = ValueString.length();
                if (Length > 0 && Length < MaxValue)
                {
                    InfoByte = 2;
                    InfoByte |= Length << 3;

                    FWCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                    UWUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                    Result.Add(Wrapper.GetArrayElement(0));
                    if (bDoubleContentCount)
                    {
                        Result.Add(Wrapper.GetArrayElement(1));
                    }

                    for (int32 i = 0; i < Length; i++) Result.Add(ValueString[i]);
                }
            }
            else
            {
                WJson::Node ValueList = NamedNode.second;
                if (ValueList.GetType() == WJson::Node::Type::T_ARRAY)
                {
                    auto Length = (ANSICHAR)ValueList.GetSize();
                    if (Length > 0 && Length < MaxValue)
                    {
                        if (KeyString == "BooleanArray")
                        {
                            InfoByte = 0;

                            auto ByteSizeOfCompressed = static_cast<uint8>(UWUtilities::GetDestinationLengthBeforeCompressBoolArray(Length));
                            if (ByteSizeOfCompressed <= 0) continue;

                            TArray<bool> DecompressedArray;
                            for (int32 i = 0; i < Length; i++)
                            {
                                if (WJson::Node AsValue = ValueList.Get(static_cast<size_t>(i)))
                                {
                                    if (AsValue.IsBoolean())
                                    {
                                        DecompressedArray.Add(AsValue.ToBoolean(false));
                                    }
                                }
                            }
                            if (DecompressedArray.Num() == 0) continue;

                            FWCHARWrapper CompressedArray(new ANSICHAR[ByteSizeOfCompressed], ByteSizeOfCompressed, true);
                            if (!UWUtilities::CompressBooleanAsBit(CompressedArray, DecompressedArray)) continue;
                            if (CompressedArray.GetSize() == 0) continue;

                            InfoByte |= Length << 3;

                            FWCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                            UWUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                            Result.Add(Wrapper.GetArrayElement(0));
                            if (bDoubleContentCount)
                            {
                                Result.Add(Wrapper.GetArrayElement(1));
                            }

                            for (int32 i = 0; i < CompressedArray.GetSize(); i++) Result.Add(CompressedArray.GetArrayElement(i));
                        }
                        else
                        {
                            uint8 UnitSize;
                            uint8 BasicType = 0;
                            if (KeyString == "ByteArray")
                            {
                                InfoByte = 1;
                                UnitSize = 1;
                                BasicType = 0;
                            }
                            else if (KeyString == "ShortArray")
                            {
                                InfoByte = 3;
                                UnitSize = 2;
                                BasicType = 1;
                            }
                            else if (KeyString == "IntegerArray")
                            {
                                InfoByte = 4;
                                UnitSize = 4;
                                BasicType = 2;
                            }
                            else if (KeyString == "FloatArray")
                            {
                                InfoByte = 5;
                                UnitSize = 4;
                                BasicType = 3;
                            }
                            else continue;

                            InfoByte |= Length << 3;

                            FWCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                            UWUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                            Result.Add(Wrapper.GetArrayElement(0));
                            if (bDoubleContentCount)
                            {
                                Result.Add(Wrapper.GetArrayElement(1));
                            }

                            bool bFilled = false;
                            for (int32 i = 0; i < Length; i++)
                            {
                                if (WJson::Node CurrentData = ValueList.Get(static_cast<size_t>(i)))
                                {
                                    FWCHARWrapper ConvertedValue(new ANSICHAR[UnitSize], UnitSize, true);

                                    if (BasicType == 0)
                                    {
                                        auto Val = static_cast<uint8>(CurrentData.ToInteger(0));
                                        FMemory::Memcpy(ConvertedValue.GetValue(), &Val, UnitSize);
                                    }
                                    else if (BasicType == 1 || BasicType == 2)
                                    {
                                        int32 Val = CurrentData.ToInteger(0);
                                        FMemory::Memcpy(ConvertedValue.GetValue(), &Val, UnitSize);
                                    }
                                    else
                                    {
                                        float Val = CurrentData.ToFloat(0.0f);
                                        FMemory::Memcpy(ConvertedValue.GetValue(), &Val, UnitSize);
                                    }

                                    for (int32 j = 0; j < UnitSize; j++) Result.Add(ConvertedValue.GetArrayElement(j));
                                    bFilled = true;
                                }
                            }
                            if (!bFilled) Result.RemoveAt(Result.Num() - 1);
                        }
                    }
                }
            }
        }
        //

        //Checksum final operation starts.
        int32 ChecksumDestinationSize = Result.Num() - ChecksumInsertIx;
        if (ChecksumDestinationSize == 0) return FWCHARWrapper();

        FWCHARWrapper WCHARWrapper(Result.GetMutableData() + ChecksumInsertIx, ChecksumDestinationSize, false);

        FWCHARWrapper Hashed = UWUtilities::WBasicRawHash(WCHARWrapper, 0, ChecksumDestinationSize);
        Hashed.bDeallocateValueOnDestructor = true;
        Result.Insert(Hashed.GetValue(), 4, ChecksumInsertIx);
        //
    }

    auto ResultArray = new ANSICHAR[Result.Num()];
    FMemory::Memcpy(ResultArray, Result.GetData(), static_cast<WSIZE__T>(Result.Num()));

    FWCHARWrapper ResultWrapper(ResultArray, Result.Num(), false);
    if (bReliableSYN)
    {
        HandleReliableSYNDeparture(OtherParty, ResultWrapper, MessageID);
    }

    return ResultWrapper;
}

WReliableConnectionRecord* UWUDPHandler::Create_AddOrGet_ReliableConnectionRecord(sockaddr* OtherParty, uint32 MessageID, FWCHARWrapper& Buffer, bool bAsSender, uint8 EnsureHandshakingStatusEqualsTo, bool bIgnoreFailure)
{
    //If EnsureHandshakingStatusEqualsTo = 0: Function can create a new record.
    //Otherwise will only try to get from existing records and if found, will ensure HandshakingStatus = EnsureHandshakingStatusEqualsTo, otherwise returns null.
    //HandshakingStatus_Mutex may be locked after. Do not forget to try unlocking it.

    std::string ClientKey = WUDPHelper::GetAddressPortFromClient(OtherParty, MessageID);

    WReliableConnectionRecord* ReliableConnection = nullptr;
    {
        WScopeGuard Guard(&ReliableConnectionRecords_Mutex);

        auto It = ReliableConnectionRecords.find(ClientKey);
        if (It != ReliableConnectionRecords.end())
        {
            WReliableConnectionRecord* Record = It->second;
            if (Record)
            {
                WScopeGuard ClientRecord_Dangerzone_Guard(&Record->Dangerzone_Mutex);
                if (Record->GetClientsideMessageID() == MessageID &&
                    Record->GetHandshakingStatus() == EnsureHandshakingStatusEqualsTo &&
                    (bIgnoreFailure || Record->FailureTrialCount < 5))
                {
                    ReliableConnection = Record;
                    if (Buffer.IsValid())
                    {
                        ReliableConnection->ReplaceBuffer(Buffer);
                    }
                }
                Record->UpdateLastInteraction();
            }
            else
            {
                ReliableConnectionRecords.erase(It);
            }
        }
        if (EnsureHandshakingStatusEqualsTo == 0 && ReliableConnection == nullptr)
        {
            ReliableConnection = new WReliableConnectionRecord(this, MessageID, *OtherParty, ClientKey, Buffer, bAsSender);
            ReliableConnectionRecords.insert(std::pair<std::string, WReliableConnectionRecord*>(ClientKey, ReliableConnection));
        }
    }
    return ReliableConnection;
}
void UWUDPHandler::CloseCase(WReliableConnectionRecord* Record)
{
    if (Record == nullptr) return;

    WScopeGuard DangerZoneGuard(&Record->Dangerzone_Mutex);
    std::string ClientKey = Record->GetClientKey();

    WScopeGuard UDPRecordsForTimeoutCheck_Guard(&UDPRecordsForTimeoutCheck_Mutex);
    UDPRecordsForTimeoutCheck.erase(Record);

    delete (Record);

    WScopeGuard Guard(&ReliableConnectionRecords_Mutex);
    ReliableConnectionRecords.erase(ClientKey);
}
void UWUDPHandler::AsReceiverReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //Send SYN-ACK
    //UWUtilities::Print(EWLogType::Log, L"Send SYN-ACK: " + FString::FromInt(MessageID));

    FWCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, WJson::Node(WJson::Node::T_VALIDATION), false, false, true, false, false, false, MessageID);

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 0, false);
    if (Record)
    {
        Record->SetHandshakingStatus(2);
        Send(OtherParty, WrappedFinalData);
        Record->FailureTrialCount = 0;
    }
    else
    {
        Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 2, false);
        if (Record)
        {
            Send(OtherParty, WrappedFinalData);
            Record->FailureTrialCount = 0;
        }
    }

    WrappedFinalData.DeallocateValue();
}
void UWUDPHandler::AsReceiverReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //Send SYN-fail
    //UWUtilities::Print(EWLogType::Log, L"Send SYN-fail: " + FString::FromInt(MessageID));

    FWCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, WJson::Node(WJson::Node::T_VALIDATION), false, false, false, true, false, false, MessageID);

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 0, true);
    if (Record)
    {
        Send(OtherParty, WrappedFinalData);
        //We are receiver, to receive actual buffer again, we should not increase FailureCount.
        Record->FailureTrialCount = 0;
    }

    WrappedFinalData.DeallocateValue();
}
void UWUDPHandler::HandleReliableSYNDeparture(sockaddr* OtherParty, FWCHARWrapper& Buffer, uint32 MessageID) //Sender
{
    //This is called by MakeByteArrayForNetworkData
    if (!bSystemStarted) return;

    //Set the case
    //UWUtilities::Print(EWLogType::Log, L"SYNDeparture: " + FString::FromInt(MessageID));

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, Buffer, true, 0, true);
    if (Record)
    {
        Record->SetHandshakingStatus(1);
        Record->FailureTrialCount = 0; //To reset, just in case.
    }
}
void UWUDPHandler::HandleReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //Send ACK
    //UWUtilities::Print(EWLogType::Log, L"Send ACK: " + FString::FromInt(MessageID));

    FWCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, WJson::Node(WJson::Node::T_VALIDATION), false, false, false, false, true, false, MessageID);

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, true, 1, false);
    if (Record)
    {
        Record->SetHandshakingStatus(3);
        Send(OtherParty, WrappedFinalData);
        Record->FailureTrialCount = 0;
    }
    else
    {
        Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, true, 3, false);
        if (Record)
        {
            Send(OtherParty, WrappedFinalData);
            Record->FailureTrialCount = 0;
        }
    }

    WrappedFinalData.DeallocateValue();
}
void UWUDPHandler::HandleReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //Re-send SYN(Buffer)
    //UWUtilities::Print(EWLogType::Log, L"Re-send SYN(Buffer): " + FString::FromInt(MessageID));

    FWCHARWrapper NullBuffer;

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, NullBuffer, true, 1, false);
    if (Record)
    {
        Send(OtherParty, *Record->GetBuffer());
        Record->FailureTrialCount++;
    }
}
void UWUDPHandler::HandleReliableSYNACKSuccess(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //ACK received. Send ACK-ACK, then close the case.
    //UWUtilities::Print(EWLogType::Log, L"ACK received. Send ACK-ACK, then close the case: " + FString::FromInt(MessageID));

    FWCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, WJson::Node(WJson::Node::T_VALIDATION), false, false, false, false, false, true, MessageID);

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 2, false);
    if (Record)
    {
        Record->SetHandshakingStatus(4);
        Send(OtherParty, WrappedFinalData);
        Record->FailureTrialCount = 0;
    }
    else
    {
        Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 4, false);
        if (Record)
        {
            Send(OtherParty, WrappedFinalData);
            Record->FailureTrialCount = 0;
        }
    }

    WrappedFinalData.DeallocateValue();

    if (Record)
    {
        CloseCase(Record);
    }
}
void UWUDPHandler::HandleReliableACKArrival(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //ACK-ACK received. Close the case.
    //UWUtilities::Print(EWLogType::Log, L"ACK-ACK received. Close the case: " + FString::FromInt(MessageID));

    FWCHARWrapper NullBuffer;

    WReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, NullBuffer, true, 3, false);
    if (Record)
    {
        CloseCase(Record);
    }
}

void UWUDPHandler::ClearUDPRecordsForTimeoutCheck()
{
    if (!bSystemStarted) return;

    WScopeGuard Guard(&UDPRecordsForTimeoutCheck_Mutex);
    UDPRecordsForTimeoutCheck.clear();
}

void UWUDPHandler::AddNewUDPRecord(WUDPRecord* NewRecord)
{
    if (!bSystemStarted || NewRecord == nullptr) return;

    WScopeGuard Guard(&UDPRecordsForTimeoutCheck_Mutex);
    UDPRecordsForTimeoutCheck.insert(NewRecord);
}

WUDPRecord::WUDPRecord(UWUDPHandler* _ResponsibleHandler) : LastInteraction(UWUtilities::GetTimeStampInMS())
{
    if (_ResponsibleHandler)
    {
        _ResponsibleHandler->AddNewUDPRecord(this);
        ResponsibleHandler = _ResponsibleHandler;
    }
}
WUDPRecord::~WUDPRecord() = default;

void UWUDPHandler::StartSystem()
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    TArray<UWAsyncTaskParameter*> SelfAsArray(this);
    WFutureAsyncTask Lambda = [](TArray<UWAsyncTaskParameter*> TaskParameters)
    {
        UWUDPHandler* HandlerInstance = nullptr;
        if (TaskParameters.Num() > 0)
        {
            HandlerInstance = dynamic_cast<UWUDPHandler *>(TaskParameters[0]);
        }
        if (HandlerInstance == nullptr || !HandlerInstance->bSystemStarted) return;

        uint64 CurrentTimestamp = UWUtilities::GetTimeStampInMS();

        WUDPRecord* Record = nullptr;
        WScopeGuard Guard(&HandlerInstance->UDPRecordsForTimeoutCheck_Mutex);
        for (auto It = HandlerInstance->UDPRecordsForTimeoutCheck.begin(); It != HandlerInstance->UDPRecordsForTimeoutCheck.end();)
        {
            Record = *It;
            if (Record)
            {
                bool bDeleted = false;

                WScopeGuard DangerZoneGuard(&Record->Dangerzone_Mutex);
                if ((CurrentTimestamp - Record->GetLastInteraction()) >= Record->TimeoutValueMS())
                {
                    if (Record->ResetterFunction())
                    {
                        HandlerInstance->UDPRecordsForTimeoutCheck.erase(It++);

                        if (Record->GetType() == EWReliableRecordType::ClientRecord)
                        {
                            auto AsClientRecord = (WClientRecord*)Record;
                            if (AsClientRecord)
                            {
                                WScopeGuard ClientRecords_Guard(&HandlerInstance->ClientsRecord_Mutex);
                                HandlerInstance->ClientRecords.erase(AsClientRecord->GetClientKey());
                            }
                        }
                        else if (Record->GetType() == EWReliableRecordType::ReliableConnectionRecord)
                        {
                            auto AsReliableConnectionRecord = (WReliableConnectionRecord*)Record;
                            if (AsReliableConnectionRecord)
                            {
                                WScopeGuard ReliableConnectionRecords_Guard(&HandlerInstance->ReliableConnectionRecords_Mutex);
                                HandlerInstance->ReliableConnectionRecords.erase(AsReliableConnectionRecord->GetClientKey());
                            }
                        }

                        delete (Record);
                        bDeleted = true;
                    }
                }

                if (!bDeleted)
                {
                    ++It;
                }
            }
            else
            {
                HandlerInstance->UDPRecordsForTimeoutCheck.erase(It++);
            }
        }
    };
    UWScheduledAsyncTaskManager::NewScheduledAsyncTask(Lambda, SelfAsArray, 100, true, true);
}
void UWUDPHandler::EndSystem()
{
    if (!bSystemStarted) return;
    bSystemStarted = false;

    ClearUDPRecordsForTimeoutCheck();
    ClearReliableConnections();
    LastServersideMessageID = 1;
    LastServersideGeneratedTimestamp = 0;
    ClearClientRecords();
}

#if PLATFORM_WINDOWS
void UWUDPHandler::Send(sockaddr* OtherParty, const FWCHARWrapper& SendBuffer)
#else
void UWUDPHandler::Send(sockaddr* OtherParty, const FWCHARWrapper& SendBuffer)
#endif
{
    if (!bSystemStarted) return;

    if (OtherParty == nullptr) return;
    if (SendBuffer.GetSize() == 0) return;

#if PLATFORM_WINDOWS
    int32 OtherPartyLen = sizeof(*OtherParty);
#else
    socklen_t OtherPartyLen = sizeof(*OtherParty);
#endif
    int32 SentLength;
    WScopeGuard SendGuard(&SendMutex);
    {
#if PLATFORM_WINDOWS
        SentLength = static_cast<int32>(sendto(UDPSocket_Ref, SendBuffer.GetValue(), static_cast<size_t>(SendBuffer.GetSize()), 0, OtherParty, OtherPartyLen));
#else
        SentLength = static_cast<int32>(sendto(UDPSocket_Ref, SendBuffer.GetValue(), static_cast<size_t>(SendBuffer.GetSize()), MSG_NOSIGNAL, OtherParty, OtherPartyLen));
#endif
    }

#if PLATFORM_WINDOWS
    if (SentLength == SOCKET_ERROR)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPHandler: Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#else
    if (SentLength == -1)
    {
        UWUtilities::Print(EWLogType::Error, FString(L"UWUDPHandler: Socket send failed with error: ") + UWUtilities::WGetSafeErrorMessage());
    }
#endif
}