// Copyright Pagansoft.com, All rights reserved.

#include "WUtilities.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include "WMD5.h"
#include "WBase64.h"
#include "WVector2D.h"

FScopeSafeCharArray::FScopeSafeCharArray(const FString& Parameter, bool UseWChars)
{
    if (UseWChars)
    {
        if (Parameter.Len() == 0)
        {
            WCharArray = new UTFCHAR[1];
            WCharArray[0] = '\0';
            return;
        }
        WCharArray = new UTFCHAR[Parameter.Len() + 1];
        for (int32 i = 0; i < Parameter.Len(); i++)
        {
            WCharArray[i] = (UTFCHAR)(Parameter.GetCharArray()[i]);
        }
        WCharArray[Parameter.Len()] = '\0';
    }
    else
    {
        if (Parameter.Len() == 0)
        {
            CharArray = new ANSICHAR[1];
            CharArray[0] = '\0';
            return;
        }
        CharArray = new ANSICHAR[Parameter.Len() + 1];
        for (int32 i = 0; i < Parameter.Len(); i++)
        {
            CharArray[i] = (ANSICHAR)(Parameter.GetCharArray()[i]);
        }
        CharArray[Parameter.Len()] = '\0';
    }
}
FScopeSafeCharArray::~FScopeSafeCharArray()
{
    if (CharArray)
    {
        delete[] CharArray;
    }
    else if (WCharArray)
    {
        delete[] WCharArray;
    }
}

WMutex UWUtilities::PrintMutex;
void UWUtilities::Print(EWLogType LogType, const FString& Format)
{
    std::wstringstream Message;
    Message << (LogType == EWLogType::Log ? L"Log: " : (LogType == EWLogType::Warning ? L"Warning: " : L"Error: ")) << std::wstring(*Format) << "\n";

    WScopeGuard Guard(&PrintMutex);
    std::wcout << Message.str();
}
void UWUtilities::Print(EWLogType LogType, const UTFCHAR* Format)
{
    std::wstringstream Message;
    Message << (LogType == EWLogType::Log ? L"Log: " : (LogType == EWLogType::Warning ? L"Warning: " : L"Error: ")) << std::wstring(Format) << "\n";

    WScopeGuard Guard(&PrintMutex);
    std::wcout << Message.str();
}
void UWUtilities::Print(EWLogType LogType, const ANSICHAR* Format)
{
    std::stringstream Message;
    Message << (LogType == EWLogType::Log ? "Log: " : (LogType == EWLogType::Warning ? "Warning: " : "Error: ")) << std::string(Format) << "\n";

    WScopeGuard Guard(&PrintMutex);
    std::cout << Message.str();
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

FString UWUtilities::WGetSafeErrorMessage()
{
    if (errno)
    {
        ANSICHAR ErrorBuffer[512];

#if PLATFORM_WINDOWS
        int32 Length = strerror_s(ErrorBuffer, 512, errno);
#else
        int32 Length = strerror_r(errno, ErrorBuffer, 512);
#endif

        return FString(ErrorBuffer, Length);
    }
#if PLATFORM_WINDOWS
    return FString(L"Last Error Code: ") + FString::FromInt(static_cast<int32>(GetLastError()));
#else
    return L"Error state has not been set.";
#endif
}
FString UWUtilities::WGenerateMD5HashFromString(const FString& RawData)
{
    return FMD5::HashAnsiString(*RawData);
}
FString UWUtilities::WGenerateMD5Hash(const TArray<uint8>& RawData)
{
    uint8 Digest[16];
    FMD5 Md5Gen;
    Md5Gen.Update(RawData.GetData(), RawData.Num());
    Md5Gen.Final(Digest);
    FString CurrentChecksum;
    for (int32 i = 0; i < 16; i++) CurrentChecksum += FString::Printf(L"%02x", Digest[i]);
    return CurrentChecksum;
}
FWCHARWrapper UWUtilities::WBasicRawHash(FWCHARWrapper& Source, int32 FromSourceIndex, int32 Size)
{
    if (Source.GetSize() == 0 || FromSourceIndex < 0 || Size > Source.GetSize()) return FWCHARWrapper();

    int32 Sum = 0;
    for (int32 i = 0; i < Size; i++)
    {
        Sum += (uint8)Source.GetArrayElement(i + FromSourceIndex);
    }

    FWCHARWrapper Result(new ANSICHAR[4], 4, false);
    ConvertIntegerToByteArray(Sum, Result, 4);

    return Result;
}

FString UWUtilities::Base64Encode(const FString& Source)
{
    return FBase64::Encode(Source);
}
bool UWUtilities::Base64Decode(const FString& Source, FString& Destination)
{
    return FBase64::Decode(Source, Destination);
}
FString UWUtilities::Base64EncodeExceptExtension(const FString& FileName)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        return FBase64::Encode(FileName.Mid(0, LastDotIndex)) + FString(L".") + FileName.Mid(LastDotIndex + 1, FileName.Len() - LastDotIndex - 1);
    }
    return FBase64::Encode(FileName);
}
bool UWUtilities::Base64DecodeExceptExtension(const FString& FileName, FString& Destination)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        if (FBase64::Decode(FileName.Mid(0, LastDotIndex), Destination))
        {
            Destination.Append(FString(L".") + FileName.Mid(LastDotIndex + 1, FileName.Len() - LastDotIndex - 1));
            return true;
        }
        return false;
    }
    return FBase64::Decode(FileName, Destination);
}

int32 UWUtilities::GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen)
{
    if (SourceLen <= 0) return 0;
    return SourceLen % 8 == 0 ? (int32)(SourceLen / 8) : ((int32)(SourceLen / 8)) + 1;
}
bool UWUtilities::CompressBooleanAsBit(FWCHARWrapper& DestArr, const TArray<bool>& SourceArr)
{
    int32 SourceLen = SourceArr.Num();
    if (SourceLen == 0) return false;

    int32 CurIx;
    for (int32 i = 0; i < SourceLen; i += 8)
    {
        CurIx = (int32)(i / 8);
        DestArr.SetArrayElement(CurIx, (ANSICHAR)0);
        for (int32 j = 0; j < 8; j++)
        {
            if ((i + j) >= SourceLen) return true;

            if (SourceArr[i + j])
            {
                DestArr.OrSetElement(CurIx, (ANSICHAR)std::pow(2, j));
            }
        }
    }
    return true;
}
bool UWUtilities::DecompressBitAsBoolArray(TArray<bool>& DestArr, FWCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex)
{
    if (SourceAsCompressed.GetSize() == 0) return false;
    if (StartIndex < 0 || EndIndex >= SourceAsCompressed.GetSize()) return false;

    DestArr.Reset();

    for (int32 i = StartIndex; i <= EndIndex; i++)
    {
        ANSICHAR CurrentChar = SourceAsCompressed.GetArrayElement(i);
        for (int32 j = 0; j < 8; j++)
        {
            ANSICHAR Tmp = CurrentChar;
            ((Tmp <<= (7 - j)) >>= 7) &= 1;
            DestArr.Add(Tmp == 1);
        }
    }
    return true;
}
uint8 UWUtilities::CompressZeroOneFloatToByte(float Param)
{
    return (uint8)FMath::RoundToInt(FMath::Clamp(Param, 0.0f, 1.0f) * 255);
}
float UWUtilities::DecompressByteToZeroOneFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 1.0f), (float)Param);
}
uint8 UWUtilities::CompressAngleFloatToByte(float Param)
{
    while (Param < 0.0f) Param += 360.0f;
    while (Param >= 360.0f) Param -= 360.0f;
    return (uint8)FMath::RoundToInt((Param / 360.0f) * 255);
}
float UWUtilities::DecompressByteToAngleFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 360.0f), (float)Param);
}
void UWUtilities::ConvertIntegerToByteArray(int32 Param, FWCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
void UWUtilities::ConvertFloatToByteArray(float Param, FWCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
int32 UWUtilities::ConvertByteArrayToInteger(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0;
    int32 Result = 0;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}
float UWUtilities::ConvertByteArrayToFloat(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0.0f;
    float Result = 0.0f;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}

TArray<uint8> UWUtilities::StringToByteArray(const FString& InputData)
{
    TArray<uint8> returnArray;
    for (int32 i = 0; i < InputData.Len(); i++)
    {
        returnArray.Add((uint8)InputData[i]);
    }
    return returnArray;
}
FString UWUtilities::ByteArrayToString(const TArray<uint8>& ByteArray)
{
    FString returnData = "";
    for (int32 i = 0; i < ByteArray.Num(); i++)
    {
        returnData += (ANSICHAR)ByteArray[i];
    }
    return returnData;
}

FString UWUtilities::ConvertIntegerToHex(int32 inputValue)
{
    FString returnString = "";
    ANSICHAR charData;

    for (int32 i = 7; i >= 0; i--)
    {
        charData = static_cast<ANSICHAR>((inputValue >> (i * 4)) & 0xf);
        if (charData >= 0 && charData <= 9)
        {
            charData = charData + '0';
        }
        else if (charData >= 0xa && charData <= 0xf)
        {
            charData = static_cast<ANSICHAR>(charData + 'a' - 10);
        }
        else
        {
            charData = '0';
        }
        returnString += charData;
    }
    return returnString;
}
int32 UWUtilities::ConvertHexToInteger(const FString& InputData)
{
    ANSICHAR newData[64];
    auto iterations = static_cast<uint8>(InputData.Len());
    if (iterations > 64)
    {
        iterations = 64;
    }

    for (int32 i = 0; i < iterations; i++)
    {
        newData[i] = (ANSICHAR) InputData[i];
    }
    return static_cast<int32>(std::strtoul(newData, nullptr, 16));
}