// Copyright Burak Kara, All rights reserved.

#include "BKUDPHandler.h"
#include "BKUDPHelper.h"
#include "BKMath.h"
#include "BKScheduledTaskManager.h"

#if PLATFORM_WINDOWS
BKUDPHandler::BKUDPHandler(SOCKET _UDPSocket)
#else
WUDPHandler::WUDPHandler(int32 _UDPSocket)
#endif
{
    UDPSocket_Ref = _UDPSocket;
}

void BKUDPHandler::ClearReliableConnections()
{
    if (!bSystemStarted) return;

    BKScopeGuard Guard(&ReliableConnectionRecords_Mutex);
    for (auto& It : ReliableConnectionRecords)
    {
        BKReliableConnectionRecord* Record = It.second;
        if (Record && !Record->bBeingDeleted)
        {
            BKReferenceCounter SafetyCounter(Record);
            AddRecordToPendingDeletePool(Record);
        }
    }
    ReliableConnectionRecords.clear();
}
void BKUDPHandler::ClearOtherPartiesRecords()
{
    if (!bSystemStarted) return;

    BKScopeGuard Guard(&OtherPartiesRecords_Mutex);
    for (auto& It : OtherPartiesRecords)
    {
        if (It.second && !It.second->bBeingDeleted)
        {
            BKReferenceCounter SafetyCounter(It.second);
            AddRecordToPendingDeletePool(It.second);
        }
    }
    OtherPartiesRecords.clear();
}

BKJson::Node BKUDPHandler::AnalyzeNetworkDataWithByteArray(FBKCHARWrapper& Parameter, sockaddr* OtherParty)
{
    if (!bSystemStarted || !OtherParty) return BKJson::Node(BKJson::Node::T_INVALID);
    if (Parameter.GetSize() < 5) return BKJson::Node(BKJson::Node::T_INVALID);

    //Boolean flags operation starts.
    TArray<bool> ResultOfDecompress;
    if (!BKUtilities::DecompressBitAsBoolArray(ResultOfDecompress, Parameter, 0, 0)) return BKJson::Node(BKJson::Node::T_INVALID);
    bool bReliableSYN = ResultOfDecompress[0];
    bool bReliableSYNSuccess = ResultOfDecompress[1];
    bool bReliableSYNFailure = ResultOfDecompress[2];
    bool bReliableSYNACKSuccess = ResultOfDecompress[3];
    bool bReliableACK = ResultOfDecompress[4];
    bool bIgnoreTimestamp = ResultOfDecompress[5];
    bool bDoubleContentCount = ResultOfDecompress[6];
    //

    bool bReliable = bReliableSYN || bReliableSYNSuccess || bReliableSYNFailure || bReliableSYNACKSuccess || bReliableACK;

    if (bPendingKill && (bReliableSYN || !bReliable)) return BKJson::Node(BKJson::Node::T_INVALID);

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
            return BKJson::Node(BKJson::Node::T_INVALID);
        }

        return BKJson::Node(BKJson::Node::T_VALIDATION);
    }
    else if (bReliable && !bReliableSYN && MessageID == 0)
    {
        return BKJson::Node(BKJson::Node::T_INVALID);
    }

    //Checksum operations start.
    const int32 AfterChecksumStartIx = bReliable ? 9 : 5;
    if (Parameter.GetSize() < (AfterChecksumStartIx + 1))
    {
        if (bReliableSYN)
        {
            AsReceiverReliableSYNFailure(OtherParty, MessageID);
        }
        return BKJson::Node(BKJson::Node::T_INVALID);
    }
    const int32 ChecksumStartIx = bReliable ? 5 : 1;

    FBKCHARWrapper Hashed = BKUtilities::WBasicRawHash(Parameter, AfterChecksumStartIx, Parameter.GetSize() - AfterChecksumStartIx);
    for (int32 i = 0; i < 4; i++)
    {
        if (Hashed.GetArrayElement(i) != Parameter.GetArrayElement(i + ChecksumStartIx))
        {
            if (bReliableSYN)
            {
                AsReceiverReliableSYNFailure(OtherParty, MessageID);
            }
            Hashed.DeallocateValue();
            return BKJson::Node(BKJson::Node::T_INVALID);
        }
    }
    if (Hashed.IsValid())
    {
        Hashed.DeallocateValue();
    }
    //

    //Timestamp operation starts.
    {
        BKOtherPartyRecord* OtherPartyRecord = nullptr;
        {
            FString OtherPartyKey = BKUDPHelper::GetAddressPortFromOtherParty(OtherParty, MessageID, true);

            BKScopeGuard Guard(&OtherPartiesRecords_Mutex);
            auto It = OtherPartiesRecords.find(OtherPartyKey);
            if (It != OtherPartiesRecords.end())
            {
                if (!It->second || (It->second && It->second->bBeingDeleted))
                {
                    It->second = new BKOtherPartyRecord(this, OtherPartyKey);
                }
                else
                {
                    It->second->UpdateLastInteraction();
                }
                OtherPartyRecord = It->second;
            }
            else
            {
                OtherPartyRecord = new BKOtherPartyRecord(this, OtherPartyKey);
                OtherPartiesRecords.insert(std::pair<FString, BKOtherPartyRecord*>(OtherPartyKey, OtherPartyRecord));
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
                return BKJson::Node(BKJson::Node::T_INVALID);
            }

            FMemory::Memcpy(&Timestamp, Parameter.GetValue() + AfterChecksumStartIx, 2);

            const uint16 LastSendersideTimestamp = OtherPartyRecord->GetLastSendersideTimestamp();
            if (LastSendersideTimestamp != 0 && Timestamp < LastSendersideTimestamp)
            {
                if (bReliableSYN)
                {
                    AsReceiverReliableSYNFailure(OtherParty, MessageID);
                }
                return BKJson::Node(BKJson::Node::T_INVALID);
            }

            OtherPartyRecord->SetLastSendersideTimestamp(Timestamp);
        }
        else
        {
            OtherPartyRecord->SetLastSendersideTimestamp(0);
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

    if (Parameter.GetSize() < (GenericPartStartIx + 1)) return BKJson::Node(BKJson::Node::T_INVALID);

    BKJson::Node ResultMap = BKJson::Node(BKJson::Node::T_OBJECT);

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
            FBKCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
            Wrapper.SetArrayElement(0, VariableContentCount_1);
            auto VariableContentCount_2 = Parameter.GetArrayElement(Parameter.GetSize() - RemainedBytes + 1);
            Wrapper.SetArrayElement(1, VariableContentCount_2);
            VariableContentCount = static_cast<uint16>(BKUtilities::ConvertByteArrayToInteger(Wrapper, 0, 2));
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
            if (!BKUtilities::DecompressBitAsBoolArray(BoolArray, Parameter, StartIndex, StartIndex + AsByteNo - 1)) break;

            RemainedBytes -= AsByteNo;

            BoolArray.SetNum(VariableContentCount);

            static const FString BooleanArrayString("BooleanArray");

            BKJson::Node Exists = ResultMap.Get(BooleanArrayString);
            if (Exists.GetType() == BKJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(BKJson::Node(BoolArray[i]));
                ResultMap.Remove(BooleanArrayString);
                ResultMap.Add(BooleanArrayString, Exists);
            }
            else
            {
                BKJson::Node NewList = BKJson::Node(BKJson::Node::T_ARRAY);
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(BKJson::Node(BoolArray[i]));
                ResultMap.Add(BooleanArrayString, NewList);
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

            static const FString ByteArrayString("ByteArray");

            BKJson::Node Exists = ResultMap.Get(ByteArrayString);
            if (Exists.GetType() == BKJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(BKJson::Node(ByteArray[i]));
                ResultMap.Remove(ByteArrayString);
                ResultMap.Add(ByteArrayString, Exists);
            }
            else
            {
                BKJson::Node NewList = BKJson::Node(BKJson::Node::T_ARRAY);
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(BKJson::Node(ByteArray[i]));
                ResultMap.Add(ByteArrayString, NewList);
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

            static const FString CharArrayString("CharArray");

            BKJson::Node Exists = ResultMap.Get(CharArrayString);
            if (Exists.GetType() == BKJson::Node::Type::T_STRING)
            {
                CharArray = FString(Exists.ToString(EMPTY_FSTRING_ANSI)) + CharArray;
                ResultMap.Remove(CharArrayString);
            }
            ResultMap.Add(CharArrayString, BKJson::Node(CharArray));
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
                if (VariableType == 5)	FloatArray.Add(BKUtilities::ConvertByteArrayToFloat(Parameter, StartIndex + i, UnitSize));
                else					IntArray.Add(BKUtilities::ConvertByteArrayToInteger(Parameter, StartIndex + i, UnitSize));
            }

            RemainedBytes -= AsArraySize;

            static const FString ShortArrayString("ShortArray");
            static const FString IntegerArrayString("IntegerArray");
            static const FString FloatArrayString("FloatArray");

            FString Key = VariableType == 3 ? ShortArrayString : (VariableType == 4 ? IntegerArrayString : FloatArrayString);
            BKJson::Node Exists = ResultMap.Get(Key);
            if (Exists.GetType() == BKJson::Node::Type::T_ARRAY)
            {
                if (VariableType == 5)
                {
                    for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(BKJson::Node(FloatArray[i]));
                }
                else
                {
                    for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(BKJson::Node(IntArray[i]));
                }
                ResultMap.Remove(Key);
                ResultMap.Add(Key, Exists);
            }
            else
            {
                BKJson::Node NewList = BKJson::Node(BKJson::Node::T_ARRAY);

                if (VariableType == 5)
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(BKJson::Node(FloatArray[i]));
                }
                else
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(BKJson::Node(IntArray[i]));
                }
                ResultMap.Add(Key, NewList);
            }
        }
    }
    //

    return ResultMap;
}

FBKCHARWrapper BKUDPHandler::MakeByteArrayForNetworkData(
        sockaddr* OtherParty,
        BKJson::Node Parameter,
        bool bDoubleContentCount,
        bool bTimeOrderCriticalData,
        bool bReliableSYN,
        bool bReliableSYNSuccess,
        bool bReliableSYNFailure,
        bool bReliableSYNACKSuccess,
        bool bReliableACK,
        int32 ReliableMessageID)
{
    if (!bSystemStarted) return FBKCHARWrapper();
    if (!OtherParty ||
        (!Parameter.IsObject() &&
         (Parameter.IsValidation() && ReliableMessageID == 0)))
        return FBKCHARWrapper();

    TArray<ANSICHAR> Result;

    if (LastThissideGeneratedTimestamp == 65535)
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

    if (bPendingKill && (bReliableSYN || !bReliable)) return FBKCHARWrapper();

    FBKCHARWrapper CompressedFlags(new ANSICHAR[1], 1, true);
    if (!BKUtilities::CompressBooleanAsBit(CompressedFlags, Flags)) return FBKCHARWrapper();

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
            BKScopeGuard Guard(&LastThissideMessageID_Mutex);

            if (LastThissideMessageID == (uint32)4294967295) LastThissideMessageID = 1;
            else LastThissideMessageID++;

            MessageID = LastThissideMessageID;
        }

        FBKCHARWrapper ConvertedMessageID(new ANSICHAR[4], 4, true);
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
                BKScopeGuard Guard(&LastThissideGeneratedTimestamp_Mutex);

                if (LastThissideGeneratedTimestamp == (uint16)65535) LastThissideGeneratedTimestamp = 0;
                else LastThissideGeneratedTimestamp++;

                Timestamp = LastThissideGeneratedTimestamp;
            }

            FBKCHARWrapper ConvertedTimestamp(new ANSICHAR[2], 2, true);
            FMemory::Memcpy(ConvertedTimestamp.GetValue(), &Timestamp, 2);
            for (int32 i = 0; i < 2; i++) Result.Add(ConvertedTimestamp.GetArrayElement(i));
        }
        //

        auto MaxValue = static_cast<uint16>(bDoubleContentCount ? 8192 : 32);

        //Generic parts encoding starts.
        for (const BKJson::NamedNode& NamedNode : Parameter)
        {
            uint16 InfoByte;

            static const FString CharArrayString("CharArray");

            FString KeyString = NamedNode.first;
            if (KeyString == CharArrayString)
            {
                FString ValueString = NamedNode.second.ToString(EMPTY_FSTRING_ANSI);
                int32 Length = ValueString.Len();
                if (Length > 0 && Length < MaxValue)
                {
                    InfoByte = 2;
                    InfoByte |= Length << 3;

                    FBKCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                    BKUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                    Result.Add(Wrapper.GetArrayElement(0));
                    if (bDoubleContentCount)
                    {
                        Result.Add(Wrapper.GetArrayElement(1));
                    }

                    for (uint32 i = 0; i < Length; i++) Result.Add(ValueString.AtAnsi(i));
                }
            }
            else
            {
                BKJson::Node ValueList = NamedNode.second;
                if (ValueList.GetType() == BKJson::Node::Type::T_ARRAY)
                {
                    auto Length = (ANSICHAR)ValueList.GetSize();
                    if (Length > 0 && Length < MaxValue)
                    {
                        static const FString BooleanArrayString("BooleanArray");

                        if (KeyString == BooleanArrayString)
                        {
                            InfoByte = 0;

                            auto ByteSizeOfCompressed = static_cast<uint8>(BKUtilities::GetDestinationLengthBeforeCompressBoolArray(Length));
                            if (ByteSizeOfCompressed <= 0) continue;

                            TArray<bool> DecompressedArray;
                            for (int32 i = 0; i < Length; i++)
                            {
                                if (BKJson::Node AsValue = ValueList.Get(static_cast<size_t>(i)))
                                {
                                    if (AsValue.IsBoolean())
                                    {
                                        DecompressedArray.Add(AsValue.ToBoolean(false));
                                    }
                                }
                            }
                            if (DecompressedArray.Num() == 0) continue;

                            FBKCHARWrapper CompressedArray(new ANSICHAR[ByteSizeOfCompressed], ByteSizeOfCompressed, true);
                            if (!BKUtilities::CompressBooleanAsBit(CompressedArray, DecompressedArray)) continue;
                            if (CompressedArray.GetSize() == 0) continue;

                            InfoByte |= Length << 3;

                            FBKCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                            BKUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                            Result.Add(Wrapper.GetArrayElement(0));
                            if (bDoubleContentCount)
                            {
                                Result.Add(Wrapper.GetArrayElement(1));
                            }

                            for (int32 i = 0; i < CompressedArray.GetSize(); i++) Result.Add(CompressedArray.GetArrayElement(i));
                        }
                        else
                        {
                            static const FString ByteArrayString("ByteArray");
                            static const FString ShortArrayString("ShortArray");
                            static const FString IntegerArrayString("IntegerArray");
                            static const FString FloatArrayString("FloatArray");

                            uint8 UnitSize;
                            uint8 BasicType = 0;
                            if (KeyString == ByteArrayString)
                            {
                                InfoByte = 1;
                                UnitSize = 1;
                                BasicType = 0;
                            }
                            else if (KeyString == ShortArrayString)
                            {
                                InfoByte = 3;
                                UnitSize = 2;
                                BasicType = 1;
                            }
                            else if (KeyString == IntegerArrayString)
                            {
                                InfoByte = 4;
                                UnitSize = 4;
                                BasicType = 2;
                            }
                            else if (KeyString == FloatArrayString)
                            {
                                InfoByte = 5;
                                UnitSize = 4;
                                BasicType = 3;
                            }
                            else continue;

                            InfoByte |= Length << 3;

                            FBKCHARWrapper Wrapper(new ANSICHAR[2], 2, true);
                            BKUtilities::ConvertIntegerToByteArray(InfoByte, Wrapper, 2);
                            Result.Add(Wrapper.GetArrayElement(0));
                            if (bDoubleContentCount)
                            {
                                Result.Add(Wrapper.GetArrayElement(1));
                            }

                            bool bFilled = false;
                            for (int32 i = 0; i < Length; i++)
                            {
                                if (BKJson::Node CurrentData = ValueList.Get(static_cast<size_t>(i)))
                                {
                                    FBKCHARWrapper ConvertedValue(new ANSICHAR[UnitSize], UnitSize, true);

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
        if (ChecksumDestinationSize == 0) return FBKCHARWrapper();

        FBKCHARWrapper WCHARWrapper(Result.GetMutableData() + ChecksumInsertIx, ChecksumDestinationSize, false);

        FBKCHARWrapper Hashed = BKUtilities::WBasicRawHash(WCHARWrapper, 0, ChecksumDestinationSize);
        Result.Insert(Hashed.GetValue(), 4, ChecksumInsertIx);
        Hashed.DeallocateValue();
        //
    }

    auto ResultArray = new ANSICHAR[Result.Num()];
    FMemory::Memcpy(ResultArray, Result.GetData(), static_cast<WSIZE__T>(Result.Num()));

    FBKCHARWrapper ResultWrapper(ResultArray, Result.Num(), false);
    if (bReliableSYN)
    {
        HandleReliableSYNDeparture(OtherParty, ResultWrapper, MessageID);
    }

    return ResultWrapper;
}

BKReliableConnectionRecord* BKUDPHandler::Create_AddOrGet_ReliableConnectionRecord(sockaddr* OtherParty, uint32 MessageID, FBKCHARWrapper& Buffer, bool bAsSender, uint8 EnsureHandshakingStatusEqualsTo, bool bIgnoreFailure)
{
    //If EnsureHandshakingStatusEqualsTo = 0: Function can create a new record.
    //Otherwise will only try to get from existing records and if found, will ensure HandshakingStatus = EnsureHandshakingStatusEqualsTo, otherwise returns null.
    //HandshakingStatus_Mutex may be locked after. Do not forget to try unlocking it.

    FString OtherPartyKey = BKUDPHelper::GetAddressPortFromOtherParty(OtherParty, MessageID);

    uint8 ExistingHandshakeStatus = RELIABLE_CONNECTION_NOT_FOUND;
    BKReliableConnectionRecord* ReliableConnection = nullptr;
    {
        BKScopeGuard Guard(&ReliableConnectionRecords_Mutex);

        auto It = ReliableConnectionRecords.find(OtherPartyKey);
        if (It != ReliableConnectionRecords.end())
        {
            BKReliableConnectionRecord* Record = It->second;
            if (Record)
            {
                if (Record->bBeingDeleted)
                {
                    return nullptr;
                }

                BKReferenceCounter SafetyCounter(Record);

                ExistingHandshakeStatus = Record->GetHandshakingStatus();

                if (Record->GetSendersideMessageID() == MessageID &&
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
                RemoveFromReliableConnections(It);
            }
        }
        if (EnsureHandshakingStatusEqualsTo == 0 && ExistingHandshakeStatus == RELIABLE_CONNECTION_NOT_FOUND && !ReliableConnection)
        {
            ReliableConnection = new BKReliableConnectionRecord(this, MessageID, *OtherParty, OtherPartyKey, Buffer, bAsSender);
            ReliableConnectionRecords.insert(std::pair<FString, BKReliableConnectionRecord*>(OtherPartyKey, ReliableConnection));
        }
    }
    return ReliableConnection;
}
void BKUDPHandler::CloseCase(BKReliableConnectionRecord* Record)
{
    if (!Record || Record->bBeingDeleted) return;
    BKReferenceCounter SafetyCounter(Record);

    BKScopeGuard Guard(&ReliableConnectionRecords_Mutex);
    RemoveFromReliableConnections(Record->GetOtherPartyKey());

    AddRecordToPendingDeletePool(Record);
}
void BKUDPHandler::AsReceiverReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //Send SYN-ACK
    //BKUtilities::Print(EBKLogType::Log, "Send SYN-ACK: " + FString::FromInt(MessageID));

    FBKCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, BKJson::Node(BKJson::Node::T_VALIDATION), false, false, false, true, false, false, false, MessageID);

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 0, false);
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
void BKUDPHandler::AsReceiverReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //Send SYN-fail
    //BKUtilities::Print(EBKLogType::Log, "Send SYN-fail: " + FString::FromInt(MessageID));

    FBKCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, BKJson::Node(BKJson::Node::T_VALIDATION), false, false, false, false, true, false, false, MessageID);

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 0, true);
    if (Record)
    {
        Send(OtherParty, WrappedFinalData);
        //We are receiver, to receive actual buffer again, we should not increase FailureCount.
        Record->FailureTrialCount = 0;
    }

    WrappedFinalData.DeallocateValue();
}
void BKUDPHandler::HandleReliableSYNDeparture(sockaddr* OtherParty, FBKCHARWrapper& Buffer, uint32 MessageID) //Sender
{
    //This is called by MakeByteArrayForNetworkData
    if (!bSystemStarted) return;

    //Set the case
    //BKUtilities::Print(EBKLogType::Log, "SYNDeparture: " + FString::FromInt(MessageID));

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, Buffer, true, 0, true);
    if (Record)
    {
        Record->SetHandshakingStatus(1);
        Record->FailureTrialCount = 0; //To reset, just in case.
    }
}
void BKUDPHandler::HandleReliableSYNSuccess(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //Send ACK
    //BKUtilities::Print(EBKLogType::Log, "Send ACK: " + FString::FromInt(MessageID));

    FBKCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, BKJson::Node(BKJson::Node::T_VALIDATION), false, false, false, false, false, true, false, MessageID);

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, true, 1, false);
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
void BKUDPHandler::HandleReliableSYNFailure(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //Re-send SYN(Buffer)
    //BKUtilities::Print(EBKLogType::Log, "Re-send SYN(Buffer): " + FString::FromInt(MessageID));

    FBKCHARWrapper NullBuffer;

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, NullBuffer, true, 1, false);
    if (Record)
    {
        Send(OtherParty, *Record->GetBuffer());
        Record->FailureTrialCount++;
    }
}
void BKUDPHandler::HandleReliableSYNACKSuccess(sockaddr* OtherParty, uint32 MessageID) //Receiver
{
    if (!bSystemStarted) return;

    //ACK received. Send ACK-ACK, then close the case.
    //BKUtilities::Print(EBKLogType::Log, "ACK received. Send ACK-ACK, then close the case: " + FString::FromInt(MessageID));

    FBKCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(OtherParty, BKJson::Node(BKJson::Node::T_VALIDATION), false, false, false, false, false, false, true, MessageID);

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, WrappedFinalData, false, 2, false);
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
void BKUDPHandler::HandleReliableACKArrival(sockaddr* OtherParty, uint32 MessageID) //Sender
{
    if (!bSystemStarted) return;

    //ACK-ACK received. Close the case.
    //BKUtilities::Print(EBKLogType::Log, "ACK-ACK received. Close the case: " + FString::FromInt(MessageID));

    FBKCHARWrapper NullBuffer;

    BKReliableConnectionRecord* Record = Create_AddOrGet_ReliableConnectionRecord(OtherParty, MessageID, NullBuffer, true, 3, false);
    if (Record)
    {
        CloseCase(Record);
    }
}

void BKUDPHandler::ClearUDPRecordsForTimeoutCheck()
{
    if (!bSystemStarted) return;
    UDPRecordsForTimeoutCheck.Clear();
}

void BKUDPHandler::ClearPendingDeletePool()
{
    if (!bSystemStarted) return;

    BKScopeGuard Guard(&UDPRecords_PendingDeletePool_Mutex);
    for (auto& It : UDPRecords_PendingDeletePool)
    {
        if (It.first)
        {
            delete (It.first);
        }
    }
    UDPRecords_PendingDeletePool.clear();
}

void BKUDPHandler::AddNewUDPRecord(BKUDPRecord* NewRecord)
{
    if (!bSystemStarted || !NewRecord) return;
    UDPRecordsForTimeoutCheck.Push(NewRecord);
}

BKUDPRecord::BKUDPRecord(BKUDPHandler* _ResponsibleHandler) : LastInteraction(BKUtilities::GetTimeStampInMS())
{
    if (_ResponsibleHandler)
    {
        _ResponsibleHandler->AddNewUDPRecord(this);
        ResponsibleHandler = _ResponsibleHandler;
    }
}

void BKUDPHandler::StartSystem()
{
    if (bSystemStarted) return;
    bSystemStarted = true;

    TArray<BKAsyncTaskParameter*> SelfAsArray(this);
    BKFutureAsyncTask TimeoutLambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        BKUDPHandler* HandlerInstance = nullptr;
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            HandlerInstance = reinterpret_cast<BKUDPHandler*>(TaskParameters[0]);
        }
        if (!HandlerInstance || !HandlerInstance->bSystemStarted) return;

        uint64 CurrentTimestamp = BKUtilities::GetTimeStampInMS();

        BKQueue<BKUDPRecord*> Tmp_RecordsForTimeoutCheck;

        BKQueue<BKUDPRecord*> CurrentSnapshotOf_RecordsForTimeoutCheck;
        HandlerInstance->UDPRecordsForTimeoutCheck.CopyTo(CurrentSnapshotOf_RecordsForTimeoutCheck, true);

        BKUDPRecord* Record = nullptr;
        while (CurrentSnapshotOf_RecordsForTimeoutCheck.Pop(Record))
        {
            if (Record)
            {
                BKReferenceCounter SafetyCounter(Record);

                bool bDeleted = false;

                if (Record->bBeingDeleted || (CurrentTimestamp - Record->GetLastInteraction()) >= Record->TimeoutValueMS())
                {
                    if (Record->bBeingDeleted)
                    {
                        bDeleted = true;
                    }
                    else if (Record->ResetterFunction())
                    {
                        bDeleted = true;
                        if (Record->GetType() == EBKReliableRecordType::OtherPartyRecord)
                        {
                            auto AsOtherPartyRecord = reinterpret_cast<BKOtherPartyRecord*>(Record);
                            if (AsOtherPartyRecord)
                            {
                                BKScopeGuard OtherPartiesRecords_Guard(&HandlerInstance->OtherPartiesRecords_Mutex);
                                HandlerInstance->OtherPartiesRecords.erase(AsOtherPartyRecord->GetOtherPartyKey());
                            }
                        }
                        else if (Record->GetType() == EBKReliableRecordType::ReliableConnectionRecord)
                        {
                            auto AsReliableConnectionRecord = reinterpret_cast<BKReliableConnectionRecord*>(Record);
                            if (AsReliableConnectionRecord)
                            {
                                BKScopeGuard ReliableConnectionRecords_Guard(&HandlerInstance->ReliableConnectionRecords_Mutex);
                                HandlerInstance->RemoveFromReliableConnections(AsReliableConnectionRecord->GetOtherPartyKey());
                            }
                        }

                        HandlerInstance->AddRecordToPendingDeletePool(Record);
                    }
                }

                if (!bDeleted)
                {
                    Tmp_RecordsForTimeoutCheck.Push(Record);
                }
            }
        }

        HandlerInstance->UDPRecordsForTimeoutCheck.AddAll_NotTSTemporaryQueue(Tmp_RecordsForTimeoutCheck);
    };
    BKScheduledAsyncTaskManager::NewScheduledAsyncTask(TimeoutLambda, SelfAsArray, TIMEOUT_CHECK_TIME_INTERVAL, true, true);

    BKFutureAsyncTask DeallocatorLambda = [](TArray<BKAsyncTaskParameter*> TaskParameters)
    {
        BKUDPHandler* HandlerInstance = nullptr;
        if (TaskParameters.Num() > 0 && TaskParameters[0])
        {
            HandlerInstance = reinterpret_cast<BKUDPHandler*>(TaskParameters[0]);
        }
        if (!HandlerInstance || !HandlerInstance->bSystemStarted) return;

        uint64 CurrentTimestamp = BKUtilities::GetTimeStampInMS();

        BKUDPRecord* DeleteRecord = nullptr;
        uint64 PooledTimestamp;

        BKScopeGuard Guard(&HandlerInstance->UDPRecords_PendingDeletePool_Mutex);
        for (auto It = HandlerInstance->UDPRecords_PendingDeletePool.begin(); It != HandlerInstance->UDPRecords_PendingDeletePool.end();)
        {
            bool bRemoved = false;

            if (It->first)
            {
                DeleteRecord = It->first;
                PooledTimestamp = It->second;

                if (!DeleteRecord->IsReferenced() && (CurrentTimestamp - PooledTimestamp) > PENDING_DELETE_CHECK_TIME_INTERVAL)
                {
                    HandlerInstance->UDPRecords_PendingDeletePool.erase(It++);
                    delete (DeleteRecord);
                    bRemoved = true;
                }

                if (!bRemoved)
                {
                    ++It;
                }
            }
            else
            {
                HandlerInstance->UDPRecords_PendingDeletePool.erase(It++);
            }
        }
    };
    BKScheduledAsyncTaskManager::NewScheduledAsyncTask(DeallocatorLambda, SelfAsArray, PENDING_DELETE_CHECK_TIME_INTERVAL, true, true);
}
void BKUDPHandler::EndSystem()
{
    if (!bSystemStarted) return;

    ClearUDPRecordsForTimeoutCheck();
    ClearReliableConnections();
    ClearOtherPartiesRecords();
    ClearPendingDeletePool();

    bSystemStarted = false;

    LastThissideMessageID = 1;
    LastThissideGeneratedTimestamp = 0;
}

#if PLATFORM_WINDOWS
void BKUDPHandler::Send(sockaddr* OtherParty, const FBKCHARWrapper& SendBuffer)
#else
void WUDPHandler::Send(sockaddr* OtherParty, const FBKCHARWrapper& SendBuffer)
#endif
{
    if (!bSystemStarted) return;

    if (!OtherParty) return;
    if (SendBuffer.GetSize() == 0) return;

#if PLATFORM_WINDOWS
    int32 OtherPartyLen = sizeof(*OtherParty);
#else
    socklen_t OtherPartyLen = sizeof(*OtherParty);
#endif
    int32 SentLength;
    BKScopeGuard SendGuard(&SendMutex);
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
        BKUtilities::Print(EBKLogType::Error, FString("BKUDPHandler: Socket send failed with error: ") + BKUtilities::WGetSafeErrorMessage());
    }
#else
    if (SentLength == -1)
    {
        BKUtilities::Print(EBKLogType::Error, FString("WUDPHandler: Socket send failed with error: ") + BKUtilities::WGetSafeErrorMessage());
    }
#endif
}

void BKUDPHandler::RemoveFromReliableConnections(std::__detail::_Node_iterator<std::pair<const FString, BKReliableConnectionRecord *>, false, true> Iterator)
{
    ReliableConnectionRecords.erase(Iterator);
    if (bPendingKill && ReliableConnectionRecords.empty() && ReadyToDieCallback)
    {
        ReadyToDieCallback();
    }
}
void BKUDPHandler::RemoveFromReliableConnections(const FString& Key)
{
    ReliableConnectionRecords.erase(Key);
    if (bPendingKill && ReliableConnectionRecords.empty() && ReadyToDieCallback)
    {
        ReadyToDieCallback();
    }
}

void BKUDPHandler::MarkPendingKill(std::function<void()> _ReadyToDieCallback)
{
    if (bPendingKill) return;
    bPendingKill = true;

    ReadyToDieCallback = std::move(_ReadyToDieCallback);

    BKScopeGuard Guard(&ReliableConnectionRecords_Mutex);
    if (ReliableConnectionRecords.empty())
    {
        ReadyToDieCallback();
    }
}

void BKUDPHandler::AddRecordToPendingDeletePool(BKUDPRecord* PendingDeleteRecord)
{
    if (!PendingDeleteRecord || PendingDeleteRecord->bBeingDeleted) return;
    PendingDeleteRecord->bBeingDeleted = true;

    BKScopeGuard Guard(&UDPRecords_PendingDeletePool_Mutex);
    auto It = UDPRecords_PendingDeletePool.find(PendingDeleteRecord);
    if (It == UDPRecords_PendingDeletePool.end())
    {
        UDPRecords_PendingDeletePool.insert(std::pair<BKUDPRecord*, uint64>(PendingDeleteRecord, BKUtilities::GetTimeStampInMS()));
    }
}

bool BKReliableConnectionRecord::ResetterFunction()
{
    if (!ResponsibleHandler) return true;
    if (!GetBuffer()->IsValid()) return true;

    if (GetHandshakingStatus() == 3) return true;
    if (bAsSender && !bAsSender_PrevFrameSkipped)
    {
        bAsSender_PrevFrameSkipped = true;
        return false;
    }
    if (++FailureTrialCount >= 2) return true;

    UpdateLastInteraction();
    ResponsibleHandler->Send(GetOtherParty(), *GetBuffer());

    return false;
}