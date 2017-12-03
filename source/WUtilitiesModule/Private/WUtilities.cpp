// Copyright Pagansoft.com, All rights reserved.

#include "WUtilities.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
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
            WCharArray[i] = (UTFCHAR)(Parameter.GetWideCharArray()[i]);
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
            CharArray[i] = (ANSICHAR)(Parameter.GetWideCharArray()[i]);
        }
        CharArray[Parameter.Len()] = '\0';
    }
}
FScopeSafeCharArray::~FScopeSafeCharArray()
{
    delete[] CharArray;
    delete[] WCharArray;
}

WMutex WUtilities::PrintMutex;
void WUtilities::Print(EWLogType LogType, const FString& Format)
{
    if (Format.IsWide())
    {
        FStringStream Message(true);
        Message << (LogType == EWLogType::Log ? L"Log: " : (LogType == EWLogType::Warning ? L"Warning: " : L"Error: "));
        Message << Format.GetWideCharArray();
        Message << L"\n";

        WScopeGuard Guard(&PrintMutex);
        std::wcout << Message.Str().GetWideCharString();
    }
    else
    {
        FStringStream Message(false);
        Message << (LogType == EWLogType::Log ? "Log: " : (LogType == EWLogType::Warning ? "Warning: " : "Error: "));
        Message << Format.GetAnsiCharArray();
        Message << "\n";

        WScopeGuard Guard(&PrintMutex);
        std::cout << Message.Str().GetAnsiCharString();
    }
}

uint64 WUtilities::GetTimeStampInMS()
{
    return static_cast<uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}
uint32 WUtilities::GetSafeTimeStampInMS()
{
    uint64 CurrentTime = GetTimeStampInMS();
    const uint64 MaxInt = 2147483647;
    return (uint32)(CurrentTime > MaxInt ? (CurrentTime % MaxInt) : CurrentTime);
}
double WUtilities::GetTimeStampInMSDetailed()
{
    double NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return NS / 1000000.0f;
}

FString WUtilities::WGetSafeErrorMessage()
{
    if (errno)
    {
#if PLATFORM_WINDOWS
        ANSICHAR ErrorBuffer[512];

        int32 Length = strerror_s(ErrorBuffer, 512, errno);
        return FString(ErrorBuffer, Length);
#else
        return FString(strerror(errno));
#endif
    }
#if PLATFORM_WINDOWS
    return FString("Last Error Code: ") + FString::FromInt(static_cast<int32>(GetLastError()));
#else
    return FString("Error state has not been set.");
#endif
}
FString WUtilities::WGenerateMD5HashFromString(const FString& RawData)
{
    return FMD5::HashAnsiString(RawData);
}
FString WUtilities::WGenerateMD5Hash(const TArray<uint8>& RawData)
{
    uint8 Digest[16];
    FMD5 Md5Gen;
    Md5Gen.Update(RawData.GetData(), RawData.Num());
    Md5Gen.Final(Digest);

    return FString::Hexify(Digest, 16);
}
FWCHARWrapper WUtilities::WBasicRawHash(FWCHARWrapper& Source, int32 FromSourceIndex, int32 Size)
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

FString WUtilities::Base64Encode(const FString& Source)
{
    return FBase64::Encode(Source);
}
bool WUtilities::Base64Decode(const FString& Source, FString& Destination)
{
    return FBase64::Decode(Source, Destination);
}
FString WUtilities::Base64EncodeExceptExtension(const FString& FileName)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        return FBase64::Encode(FileName.Mid(0, static_cast<uint32>(LastDotIndex))) + FString(".") + FileName.Mid(
                static_cast<uint32>(LastDotIndex + 1), static_cast<uint32>(FileName.Len() - LastDotIndex - 1));
    }
    return FBase64::Encode(FileName);
}
bool WUtilities::Base64DecodeExceptExtension(const FString& FileName, FString& Destination)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        if (FBase64::Decode(FileName.Mid(0, static_cast<uint32>(LastDotIndex)), Destination))
        {
            Destination.Append(FString(".") + FileName.Mid(static_cast<uint32>(LastDotIndex + 1),
                                                           static_cast<uint32>(FileName.Len() - LastDotIndex - 1)));
            return true;
        }
        return false;
    }
    return FBase64::Decode(FileName, Destination);
}

int32 WUtilities::GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen)
{
    if (SourceLen <= 0) return 0;
    return SourceLen % 8 == 0 ? (int32)(SourceLen / 8) : ((int32)(SourceLen / 8)) + 1;
}
bool WUtilities::CompressBooleanAsBit(FWCHARWrapper& DestArr, const TArray<bool>& SourceArr)
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
bool WUtilities::DecompressBitAsBoolArray(TArray<bool>& DestArr, FWCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex)
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
uint8 WUtilities::CompressZeroOneFloatToByte(float Param)
{
    return (uint8)FMath::RoundToInt(FMath::Clamp(Param, 0.0f, 1.0f) * 255);
}
float WUtilities::DecompressByteToZeroOneFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 1.0f), (float)Param);
}
uint8 WUtilities::CompressAngleFloatToByte(float Param)
{
    while (Param < 0.0f) Param += 360.0f;
    while (Param >= 360.0f) Param -= 360.0f;
    return (uint8)FMath::RoundToInt((Param / 360.0f) * 255);
}
float WUtilities::DecompressByteToAngleFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 360.0f), (float)Param);
}
void WUtilities::ConvertIntegerToByteArray(int32 Param, FWCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
void WUtilities::ConvertFloatToByteArray(float Param, FWCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
int32 WUtilities::ConvertByteArrayToInteger(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0;
    int32 Result = 0;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}
float WUtilities::ConvertByteArrayToFloat(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0.0f;
    float Result = 0.0f;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}

TArray<uint8> WUtilities::StringToByteArray(const FString& InputData)
{
    TArray<uint8> returnArray;
    for (uint32 i = 0; i < InputData.Len(); i++)
    {
        if (InputData.IsWide())
        {
            returnArray.Add((uint8)InputData.AtWide(i));
        }
        else
        {
            returnArray.Add((uint8)InputData.AtAnsi(i));
        }
    }
    return returnArray;
}
FString WUtilities::ByteArrayToString(const TArray<uint8>& ByteArray)
{
    FString returnData = EMPTY_FSTRING_UTF8;
    for (int32 i = 0; i < ByteArray.Num(); i++)
    {
        returnData += (ANSICHAR)ByteArray[i];
    }
    return returnData;
}

FString WUtilities::ConvertIntegerToHex(int32 inputValue)
{
    FString returnString = EMPTY_FSTRING_UTF8;
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
int32 WUtilities::ConvertHexToInteger(const FString& InputData)
{
    assert(!InputData.IsWide());

    ANSICHAR newData[64];
    auto iterations = static_cast<uint8>(InputData.Len());
    if (iterations > 64)
    {
        iterations = 64;
    }

    for (uint32 i = 0; i < iterations; i++)
    {
        newData[i] = InputData.AtAnsi(i);
    }
    return static_cast<int32>(std::strtoul(newData, nullptr, 16));
}