// Copyright Pagansoft.com, All rights reserved.

#include "WUDPManager.h"
#include "WMemory.h"
#include "WMath.h"

bool UWUDPManager::InitializeSocket(uint16 Port)
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

    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
    UDPServer.sin_port = htons(Port);

    int32 ret = bind(UDPSocket, (struct sockaddr*)&UDPServer, sizeof(UDPServer));
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
    shutdown(UDPSocket, SHUT_RDWR);
    close(UDPSocket);
#endif
}

void UWUDPManager::ListenSocket()
{
    while (bSystemStarted)
    {
        auto Buffer = new ANSICHAR[1024];
        auto Client = new sockaddr;
#if PLATFORM_WINDOWS
        int32 ClientLen = sizeof(*Client);
#else
        socklen_t ClientLen = sizeof(*Client);
#endif

        auto RetrievedSize = static_cast<int32>(recvfrom(UDPSocket, Buffer, 1024, 0, Client, &ClientLen));
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

        auto TaskParameter = new FWUDPTaskParameter(RetrievedSize, Buffer, Client);
        TArray<FWAsyncTaskParameter*> TaskParameterAsArray(TaskParameter);

        WFutureAsyncTask Lambda = [](TArray<FWAsyncTaskParameter*>& TaskParameters)
        {
            if (!bSystemStarted || !ManagerInstance) return;

            if (TaskParameters.Num() > 0)
            {
                if (auto Parameter = dynamic_cast<FWUDPTaskParameter*>(TaskParameters[0]))
                {
                    if (Parameter->Buffer && Parameter->Client && Parameter->BufferSize > 0)
                    {
                        FWCHARWrapper WrappedBuffer(Parameter->Buffer, Parameter->BufferSize);
                        //ManagerInstance->Send(Parameter->Client, WrappedBuffer);

                        std::shared_ptr<WJson::Node> Result = AnalyzeNetworkDataWithByteArray(WrappedBuffer, Parameter->Client);
                        if (Result != nullptr)
                        {
                            FWCHARWrapper WrappedFinalData = MakeByteArrayForNetworkData(Result, false, true);
                            ManagerInstance->Send(Parameter->Client, WrappedFinalData);
                            WrappedFinalData.DeallocateValue();
                        }
                    }
                }
            }
        };
        UWAsyncTaskManager::NewAsyncTask(Lambda, TaskParameterAsArray);
    }
}

void UWUDPManager::Send(sockaddr* Client, const FWCHARWrapper& SendBuffer)
{
    if (!bSystemStarted) return;

    if (Client == nullptr) return;
    if (SendBuffer.GetSize() == 0) return;

#if PLATFORM_WINDOWS
    int32 ClientLen = sizeof(*Client);
#else
    socklen_t ClientLen = sizeof(*Client);
#endif
    auto SentLength = static_cast<int32>(sendto(UDPSocket, SendBuffer.GetValue(), static_cast<size_t>(SendBuffer.GetSize()), 0, Client, ClientLen));

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
bool UWUDPManager::StartSystem(uint16 Port)
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
bool UWUDPManager::StartSystem_Internal(uint16 Port)
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
        ClearClientRecords();
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
    LastServersideMessageID = 0;
    LastServersideGeneratedTimestamp = 0;
}

void UWUDPManager::ClearClientRecords()
{
    if (!bSystemStarted || ManagerInstance == nullptr) return;

    WScopeGuard Guard(&ManagerInstance->ClientsRecord_Mutex);
    for (auto& It : ManagerInstance->ClientRecords)
    {
        if (It.second != nullptr)
        {
            delete (It.second);
        }
    }
    ManagerInstance->ClientRecords.clear();
}

std::shared_ptr<WJson::Node> UWUDPManager::AnalyzeNetworkDataWithByteArray(FWCHARWrapper& Parameter, sockaddr* Client)
{
    if (!bSystemStarted || ManagerInstance == nullptr || Client == nullptr) return NULL_WJSON_NODE;
    if (Parameter.GetSize() < 5) return NULL_WJSON_NODE;

    //Boolean flags operation starts.
    TArray<bool> ResultOfDecompress;
    if (!UWUtilities::DecompressBitAsBoolArray(ResultOfDecompress, Parameter, 0, 0)) return NULL_WJSON_NODE;
    bool bReliable = ResultOfDecompress[0];
    bool bIgnoreTimestamp = ResultOfDecompress[1];
    //

    //Reliable operation starts.
    uint32 MessageID = 0;
    if (bReliable)
    {
        FMemory::Memcpy(&MessageID, Parameter.GetValue() + 1, 4);
    }
    //

    //Checksum operations start.
    const int32 AfterChecksumStartIx = bReliable ? 9 : 5;
    if (Parameter.GetSize() < (AfterChecksumStartIx + 1))
    {
        if (bReliable)
        {
            ManagerInstance->HandleReliableData(MessageID, false);
        }
        return NULL_WJSON_NODE;
    }
    const int32 ChecksumStartIx = bReliable ? 5 : 1;

    FWCHARWrapper Hashed = UWUtilities::WBasicRawHash(Parameter, AfterChecksumStartIx, Parameter.GetSize() - AfterChecksumStartIx);
    Hashed.bDeallocateValueOnDestructor = true;
    for (int32 i = 0; i < 4; i++)
    {
        if (Hashed.GetArrayElement(i) != Parameter.GetArrayElement(i + ChecksumStartIx))
        {
            if (bReliable)
            {
                ManagerInstance->HandleReliableData(MessageID, false);
            }
            return NULL_WJSON_NODE;
        }
    }
    //

    //Timestamp operation starts.
    WClientRecord* ClientRecord = nullptr;
    {
        WScopeGuard Guard(&ManagerInstance->ClientsRecord_Mutex);
        auto It = ManagerInstance->ClientRecords.find(Client->sa_data);
        if (It != ManagerInstance->ClientRecords.end())
        {
            if (It->second == nullptr)
            {
                It->second = new WClientRecord();
            }
            else
            {
                It->second->UpdateLastInteraction();
            }
            ClientRecord = It->second;
        }
        else
        {
            ClientRecord = new WClientRecord();
            ManagerInstance->ClientRecords.insert(std::pair<ANSICHAR*, WClientRecord*>(Client->sa_data, ClientRecord));
        }
    }
    uint16 Timestamp = 0;
    if (!bIgnoreTimestamp)
    {
        if (Parameter.GetSize() < (AfterChecksumStartIx + 3 /* + 2 + 1 */))
        {
            if (bReliable)
            {
                ManagerInstance->HandleReliableData(MessageID, false);
            }
            return NULL_WJSON_NODE;
        }

        FMemory::Memcpy(&Timestamp, Parameter.GetValue() + AfterChecksumStartIx, 2);

        const uint16 LastClientsideTimestamp = ClientRecord->GetLastClientsideTimestamp();
        if (LastClientsideTimestamp != 0 && Timestamp < LastClientsideTimestamp)
        {
            if (bReliable)
            {
                ManagerInstance->HandleReliableData(MessageID, false);
            }
            return NULL_WJSON_NODE;
        }

        ClientRecord->SetLastClientsideTimestamp(Timestamp);
    }
    else
    {
        ClientRecord->SetLastClientsideTimestamp(0);
    }
    //

    //Reliable confirmation
    if (bReliable)
    {
        ManagerInstance->HandleReliableData(MessageID, true);
    }
    //

    //Generic parts decoding starts.
    const int32 GenericPartStartIx =
            (bIgnoreTimestamp && bReliable) ? 9 :
            ((!bIgnoreTimestamp && bReliable) ? 11 :
             (!bIgnoreTimestamp ? 7 : 5));

    if (Parameter.GetSize() < (GenericPartStartIx + 1)) return NULL_WJSON_NODE;

    std::shared_ptr<WJson::Node> ResultMap = WJson::ObjectPtr();

    int32 RemainedBytes = Parameter.GetSize() - GenericPartStartIx;
    while (RemainedBytes > 0)
    {
        if (RemainedBytes <= 1) break;

        ANSICHAR CurrentChar = Parameter.GetArrayElement(Parameter.GetSize() - RemainedBytes);

        //Variable Type
        auto VariableType = static_cast<uint8>(CurrentChar & 0b00000111);

        //Variable Content Count
        auto VariableContentCount = static_cast<uint8>((CurrentChar & 0b11111000) >> 3);

        RemainedBytes--;

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

            WJson::Node Exists = ResultMap->Get("BooleanArray");
            if (Exists.GetType() == WJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(BoolArray[i]));
                ResultMap->Remove("BooleanArray");
                ResultMap->Add("BooleanArray", Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Array();
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(BoolArray[i]));
                ResultMap->Add("BooleanArray", NewList);
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

            WJson::Node Exists = ResultMap->Get("ByteArray");
            if (Exists.GetType() == WJson::Node::Type::T_ARRAY)
            {
                for (int32 i = 0; i < VariableContentCount; i++) Exists.Add(WJson::Node(ByteArray[i]));
                ResultMap->Remove("ByteArray");
                ResultMap->Add("ByteArray", Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Array();
                for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(ByteArray[i]));
                ResultMap->Add("ByteArray", NewList);
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

            WJson::Node Exists = ResultMap->Get("CharArray");
            if (Exists.GetType() == WJson::Node::Type::T_STRING)
            {
                CharArray = Exists.ToString("") + CharArray;
                ResultMap->Remove("CharArray");
            }
            std::string AsString = CharArray.GetAnsiCharArray();
            ResultMap->Add("CharArray", WJson::Node(AsString));
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
            WJson::Node Exists = ResultMap->Get(Key);
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
                ResultMap->Remove(Key);
                ResultMap->Add(Key, Exists);
            }
            else
            {
                WJson::Node NewList = WJson::Array();

                if (VariableType == 5)
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(FloatArray[i]));
                }
                else
                {
                    for (int32 i = 0; i < VariableContentCount; i++) NewList.Add(WJson::Node(IntArray[i]));
                }
                ResultMap->Add(Key, NewList);
            }
        }
    }
    //

    return ResultMap;
}

FWCHARWrapper UWUDPManager::MakeByteArrayForNetworkData(std::shared_ptr<WJson::Node> Parameter, bool bReliable, bool bTimeOrderCriticalData)
{
    if (!bSystemStarted || ManagerInstance == nullptr) return FWCHARWrapper();
    if (Parameter == nullptr || Parameter->GetType() != WJson::Node::Type::T_OBJECT) return FWCHARWrapper();

    TArray<ANSICHAR> Result;

    if (ManagerInstance->LastServersideGeneratedTimestamp == 65535)
    {
        bReliable = true;
        bTimeOrderCriticalData = false;
    }

    //Boolean flags operation starts.
    TArray<bool> Flags;
    Flags.Add(bReliable);
    Flags.Add(!bTimeOrderCriticalData);

    FWCHARWrapper CompressedFlags(new ANSICHAR[1], 1, true);
    if (!UWUtilities::CompressBooleanAsBit(CompressedFlags, Flags)) return FWCHARWrapper();

    Result.Add(CompressedFlags.GetArrayElement(0));
    //

    //Reliable operations start.
    if (bReliable)
    {
        uint32 MessageID;
        {
            WScopeGuard Guard(&ManagerInstance->LastServersideGeneratedTimestamp_Mutex);

            if (ManagerInstance->LastServersideMessageID == (uint32)4294967295) ManagerInstance->LastServersideMessageID = 0;
            else ManagerInstance->LastServersideMessageID++;

            MessageID = ManagerInstance->LastServersideMessageID;
        }

        FWCHARWrapper ConvertedMessageID(new ANSICHAR[4], 4, true);
        FMemory::Memcpy(ConvertedMessageID.GetValue(), &MessageID, 4);
        for (int32 i = 0; i < 4; i++) Result.Add(ConvertedMessageID.GetArrayElement(i));
    }
    //

    //Checksum initial operation starts.
    int32 ChecksumInsertIx = Result.Num();
    //

    //Timestamp operations start.
    if (bTimeOrderCriticalData)
    {
        uint16 Timestamp;
        {
            WScopeGuard Guard(&ManagerInstance->LastServersideGeneratedTimestamp_Mutex);

            if (ManagerInstance->LastServersideGeneratedTimestamp == (uint16)65535) ManagerInstance->LastServersideGeneratedTimestamp = 0;
            else ManagerInstance->LastServersideGeneratedTimestamp++;

            Timestamp = ManagerInstance->LastServersideGeneratedTimestamp;
        }

        FWCHARWrapper ConvertedTimestamp(new ANSICHAR[2], 2, true);
        FMemory::Memcpy(ConvertedTimestamp.GetValue(), &Timestamp, 2);
        for (int32 i = 0; i < 2; i++) Result.Add(ConvertedTimestamp.GetArrayElement(i));
    }
    //

    //Generic parts encoding starts.
    for (const WJson::NamedNode& NamedNode : *Parameter.get())
    {
        ANSICHAR InfoByte;

        std::string KeyString = NamedNode.first;
        if (KeyString == "CharArray")
        {
            std::string ValueString = NamedNode.second.ToString("");
            auto Length = (ANSICHAR)ValueString.length();
            if (Length > 0 && Length < 32)
            {
                InfoByte = 2;
                InfoByte |= Length << 3;
                Result.Add(InfoByte);

                for (int32 i = 0; i < Length; i++) Result.Add(ValueString[i]);
            }
        }
        else
        {
            WJson::Node ValueList = NamedNode.second;
            if (ValueList.GetType() == WJson::Node::Type::T_ARRAY)
            {
                auto Length = (ANSICHAR)ValueList.GetSize();
                if (Length > 0 && Length < 32)
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
                        Result.Add(InfoByte);

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
                        Result.Add(InfoByte);

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

    Result.Insert(Hashed.GetValue(), 4, ChecksumInsertIx);
    //

    auto ResultArray = new ANSICHAR[Result.Num()];
    FMemory::Memcpy(ResultArray, Result.GetData(), static_cast<WSIZE__T>(Result.Num()));
    return FWCHARWrapper(ResultArray, Result.Num(), false);
}

void UWUDPManager::HandleReliableData(uint32 MessageID, bool bSuccess)
{

}