// Copyright Pagansoft.com, All rights reserved.

#include "WUtilities.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <iterator>
#include <bitset>

FScopeSafeCharArray::FScopeSafeCharArray(const FString& Parameter, bool UseWChars)
{
    if (UseWChars)
    {
        if (Parameter.Len() == 0)
        {
            WCharArray = new wchar_t[1];
            WCharArray[0] = '\0';
            return;
        }
        WCharArray = new wchar_t[Parameter.Len() + 1];
        for (int32 i = 0; i < Parameter.Len(); i++)
        {
            WCharArray[i] = (wchar_t)(Parameter.GetCharArray()[i]);
        }
        WCharArray[Parameter.Len()] = '\0';
    }
    else
    {
        if (Parameter.Len() == 0)
        {
            CharArray = new char[1];
            CharArray[0] = '\0';
            return;
        }
        CharArray = new char[Parameter.Len() + 1];
        for (int32 i = 0; i < Parameter.Len(); i++)
        {
            CharArray[i] = (char)(Parameter.GetCharArray()[i]);
        }
        CharArray[Parameter.Len()] = '\0';
    }
}
FScopeSafeCharArray::~FScopeSafeCharArray()
{
    if (CharArray != nullptr)
    {
        delete[] CharArray;
    }
    else if (WCharArray != nullptr)
    {
        delete[] WCharArray;
    }
}

void UWUtilities::Print(EWLogType LogType, const FString& Format)
{
    std::wcout << (LogType == EWLogType::Log ? L"Log: " : (LogType == EWLogType::Warning ? L"Warning: " : L"Error")) << std::wstring(*Format) << std::endl;
}
void UWUtilities::Print(EWLogType LogType, const TCHAR* Format)
{
    std::wcout << (LogType == EWLogType::Log ? L"Log: " : (LogType == EWLogType::Warning ? L"Warning: " : L"Error")) << std::wstring(Format) << std::endl;
}

int64 UWUtilities::GetTimeStampInMS()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
int32 UWUtilities::GetSafeTimeStampInMS()
{
    int64 currentTime = GetTimeStampInMS();
    const int32 MaxInt = 2147483647;
    return (int32)(currentTime > MaxInt ? (currentTime % MaxInt) : currentTime);
}

int32 UWUtilities::WGetTotalMemory()
{
}
int32 UWUtilities::WGetAvailableMemory()
{
}
FString UWUtilities::WGetSafeErrorMessage()
{
}
FString UWUtilities::WGenerateMD5HashFromString(const FString& RawData)
{
}
FString UWUtilities::WGenerateMD5Hash(const TArray<uint8>& RawData)
{
}
FWCHARWrapper UWUtilities::WBasicRawHash(FWCHARWrapper& Source, int32 FromSourceIndex, int32 Size)
{
}

FString UWUtilities::Base64Encode(const FString& Source)
{
}
bool UWUtilities::Base64Decode(const FString& Source, FString& Destination)
{
}
FString UWUtilities::Base64EncodeFromWCHARArray(FWCHARWrapper& Source)
{
}
bool UWUtilities::Base64DecodeToWCHARArray(const FString& Source, FWCHARWrapper& Destination)
{
}
FString UWUtilities::Base64EncodeExceptExtension(const FString& FileName)
{
}
bool UWUtilities::Base64DecodeExceptExtension(const FString& FileName, FString& Destination)
{
}

int32 UWUtilities::GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen)
{
}
bool UWUtilities::CompressBooleanAsBit(FWCHARWrapper& DestArr, const TArray<bool>& SourceArr)
{
}
bool UWUtilities::DecompressBitAsBoolArray(TArray<bool>& DestArr, FWCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex)
{
}
uint8 UWUtilities::CompressZeroOneFloatToByte(float Param)
{
}
float UWUtilities::DecompressByteToZeroOneFloat(uint8 Param)
{
}
uint8 UWUtilities::CompressAngleFloatToByte(float Param)
{
}
float UWUtilities::DecompressByteToAngleFloat(uint8 Param)
{
}
void UWUtilities::ConvertIntegerToByteArray(int32 Param, FWCHARWrapper& Result, uint8 UnitSize)
{
}
void UWUtilities::ConvertFloatToByteArray(float Param, FWCHARWrapper& Result, uint8 UnitSize)
{
}
int32 UWUtilities::ConvertByteArrayToInteger(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
}
float UWUtilities::ConvertByteArrayToFloat(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
}

FString UWUtilities::StringToBinary(const FString& InputData)
{
}
FString UWUtilities::BinaryToString(FString InputData)
{
}

TArray<uint8> UWUtilities::StringToByteArray(const FString& InputData)
{
}
FString UWUtilities::ByteArrayToString(const TArray<uint8>& ByteArray)
{
}

FString UWUtilities::ConvertIntegerToHex(int32 inputValue)
{
}
int32 UWUtilities::ConvertHexToInteger(const FString& InputData)
{
}