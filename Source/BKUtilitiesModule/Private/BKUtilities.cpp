// Copyright Burak Kara, All rights reserved.

#include "BKUtilities.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include "BKMD5.h"
#include "BKBase64.h"
#include "BKVector2D.h"

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

BKMutex BKUtilities::PrintMutex;
void BKUtilities::Print(EBKLogType LogType, const FString& Format)
{
    FStringStream Message;
    Message << (LogType == EBKLogType::Log ? L"Log: " : (LogType == EBKLogType::Warning ? L"Warning: " : L"Error: "));
    Message << Format.GetWideCharArray();
    Message << L"\n";

    BKScopeGuard Guard(&PrintMutex);
    std::wcout << Message.Str().GetWideCharString();
}

uint64 BKUtilities::GetTimeStampInMS()
{
    return static_cast<uint64>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
}
uint32 BKUtilities::GetSafeTimeStampInMS()
{
    uint64 CurrentTime = GetTimeStampInMS();
    const uint64 MaxInt = 2147483647;
    return (uint32)(CurrentTime > MaxInt ? (CurrentTime % MaxInt) : CurrentTime);
}
double BKUtilities::GetTimeStampInMSDetailed()
{
    double NS = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return NS / 1000000.0f;
}

FString BKUtilities::WGetSafeErrorMessage()
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
    return FString(L"Last Error Code: ") + FString::FromInt(static_cast<int32>(GetLastError()));
#else
    return FString(L"Error state has not been set.");
#endif
}
FString BKUtilities::WGenerateMD5HashFromString(const FString& RawData)
{
    return FMD5::HashAnsiString(RawData);
}
FString BKUtilities::WGenerateMD5Hash(const TArray<uint8>& RawData)
{
    uint8 Digest[16];
    FMD5 Md5Gen;
    Md5Gen.Update(RawData.GetData(), RawData.Num());
    Md5Gen.Final(Digest);

    return FString::Hexify(Digest, 16);
}
FBKCHARWrapper BKUtilities::WBasicRawHash(FBKCHARWrapper& Source, int32 FromSourceIndex, int32 Size)
{
    if (Source.GetSize() == 0 || FromSourceIndex < 0 || Size > Source.GetSize()) return FBKCHARWrapper();

    int32 Sum = 0;
    for (int32 i = 0; i < Size; i++)
    {
        Sum += (uint8)Source.GetArrayElement(i + FromSourceIndex);
    }

    FBKCHARWrapper Result(new ANSICHAR[4], 4, false);
    ConvertIntegerToByteArray(Sum, Result, 4);

    return Result;
}

FString BKUtilities::Base64Encode(const FString& Source)
{
    return FBase64::Encode(Source);
}
bool BKUtilities::Base64Decode(const FString& Source, FString& Destination)
{
    return FBase64::Decode(Source, Destination);
}
FString BKUtilities::Base64EncodeExceptExtension(const FString& FileName)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        return FBase64::Encode(FileName.Mid(0, static_cast<uint32>(LastDotIndex))) + FString(L".") + FileName.Mid(
                static_cast<uint32>(LastDotIndex + 1), static_cast<uint32>(FileName.Len() - LastDotIndex - 1));
    }
    return FBase64::Encode(FileName);
}
bool BKUtilities::Base64DecodeExceptExtension(const FString& FileName, FString& Destination)
{
    int32 LastDotIndex = INDEX_NONE;
    if (FileName.FindLastChar('.', LastDotIndex) && LastDotIndex > 0 && LastDotIndex != FileName.Len() - 1)
    {
        if (FBase64::Decode(FileName.Mid(0, static_cast<uint32>(LastDotIndex)), Destination))
        {
            Destination.Append(FString(L".") + FileName.Mid(static_cast<uint32>(LastDotIndex + 1),
                                                           static_cast<uint32>(FileName.Len() - LastDotIndex - 1)));
            return true;
        }
        return false;
    }
    return FBase64::Decode(FileName, Destination);
}

int32 BKUtilities::GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen)
{
    if (SourceLen <= 0) return 0;
    return SourceLen % 8 == 0 ? (int32)(SourceLen / 8) : ((int32)(SourceLen / 8)) + 1;
}
bool BKUtilities::CompressBooleanAsBit(FBKCHARWrapper& DestArr, const TArray<bool>& SourceArr)
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
bool BKUtilities::DecompressBitAsBoolArray(TArray<bool>& DestArr, FBKCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex)
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
uint8 BKUtilities::CompressZeroOneFloatToByte(float Param)
{
    return (uint8)FMath::RoundToInt(FMath::Clamp(Param, 0.0f, 1.0f) * 255);
}
float BKUtilities::DecompressByteToZeroOneFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 1.0f), (float)Param);
}
uint8 BKUtilities::CompressAngleFloatToByte(float Param)
{
    while (Param < 0.0f) Param += 360.0f;
    while (Param >= 360.0f) Param -= 360.0f;
    return (uint8)FMath::RoundToInt((Param / 360.0f) * 255);
}
float BKUtilities::DecompressByteToAngleFloat(uint8 Param)
{
    return FVector2D::GetMappedRangeValueUnclamped(FVector2D(0, 255), FVector2D(0.0f, 360.0f), (float)Param);
}
void BKUtilities::ConvertIntegerToByteArray(int32 Param, FBKCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
void BKUtilities::ConvertFloatToByteArray(float Param, FBKCHARWrapper& Result, uint8 UnitSize)
{
    if (Result.GetSize() == 0) return;
    FMemory::Memcpy(Result.GetValue(), &Param, UnitSize);
}
int32 BKUtilities::ConvertByteArrayToInteger(FBKCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0;
    int32 Result = 0;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}
float BKUtilities::ConvertByteArrayToFloat(FBKCHARWrapper& Source, int32 StartIndex, uint8 UnitSize)
{
    if (StartIndex < 0 || StartIndex >= Source.GetSize() || Source.GetSize() == 0) return 0.0f;
    float Result = 0.0f;
    FMemory::Memcpy(&Result, Source.GetValue() + StartIndex, UnitSize);
    return Result;
}

FString BKUtilities::ConvertIntegerToHex(int32 inputValue)
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