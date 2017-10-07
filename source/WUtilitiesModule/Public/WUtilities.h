// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUtilities
#define Pragma_Once_WUtilities

#include "WEngine.h"
#include "WString.h"

enum class EWLogType : uint8
{
    Error,
    Warning,
    Log
};

struct FWCHARWrapper
{

private:
    char* Value = nullptr;
    int32 Size = 0;

public:
    void SetValue(char* const Parameter, int32 SizeParameter)
    {
        if (Parameter == nullptr || SizeParameter <= 0) return;
        Value = Parameter;
        Size = SizeParameter;
    }
    char* const GetValue()
    {
        return Value;
    }
    int32 GetSize()
    {
        return Size;
    }
    void SetArrayElement(int32 Index, char Character)
    {
        if (Index >= 0 && Index < Size)
        {
            Value[Index] = Character;
        }
    }
    char GetArrayElement(int32 Index)
    {
        if (Index >= 0 && Index < Size)
        {
            return Value[Index];
        }
        return (char)0;
    }
    void OrSetElement(int32 Index, char WithCharacter)
    {
        if (Index >= 0 && Index < Size)
        {
            Value[Index] |= WithCharacter;
        }
    }
    void AndSetElement(int32 Index, char WithCharacter)
    {
        if (Index >= 0 && Index < Size)
        {
            Value[Index] &= WithCharacter;
        }
    }
    void DeallocateValue()
    {
        if (Value != nullptr)
        {
            delete[] Value;
        }
        Size = 0;
    }

    bool bDeallocateValueOnDestructor = false;

    FWCHARWrapper() {}
    FWCHARWrapper(char* const Parameter, int32 SizeParameter, bool _bDeallocateValueOnDestructor = false)
    {
        Value = Parameter;
        Size = SizeParameter;
        bDeallocateValueOnDestructor = _bDeallocateValueOnDestructor;
    }
    FWCHARWrapper(const FWCHARWrapper &Other)
    {
        Value = Other.Value;
        Size = Other.Size;
        bDeallocateValueOnDestructor = Other.bDeallocateValueOnDestructor;
    }
    FWCHARWrapper& operator=(const FWCHARWrapper &Other)
    {
        Value = Other.Value;
        Size = Other.Size;
        bDeallocateValueOnDestructor = Other.bDeallocateValueOnDestructor;
        return *this;
    }
    ~FWCHARWrapper()
    {
        if (bDeallocateValueOnDestructor)
        {
            DeallocateValue();
        }
    }
};

class FScopeSafeCharArray
{
public:
    FScopeSafeCharArray(const FString& Parameter, bool UseWChars = false);
    ~FScopeSafeCharArray();

    const char* GetC()
    {
        return CharArray;
    }
    const wchar_t* GetW()
    {
        return WCharArray;
    }
private:
    FScopeSafeCharArray();
    FScopeSafeCharArray(const FScopeSafeCharArray& InScopeLock);
    FScopeSafeCharArray& operator=(FScopeSafeCharArray& InScopeLock)
    {
        return *this;
    }

    char* CharArray = nullptr;
    wchar_t* WCharArray = nullptr;
};

class UWUtilities
{
public:
    static void Print(EWLogType LogType, const FString& format);

    static int64 GetTimeStampInMS();
    static int32 GetSafeTimeStampInMS();

    static float GetFreeDiskSpace();
    static int32 WGetTotalMemory();
    static int32 WGetAvailableMemory();

    static bool WFileExists(const FString& FullPath);
    static bool WCopyFile(const FString& DestinationFullPath, const FString& SourceFullPath);
    static bool WDeleteFile(const FString& FullPath);
    static bool WCreateDirectory(const FString& FullPath);
    static bool WDirectoryExist(const FString& FullPath);
    static bool WDeleteDirectory(const FString& FullPath);

    static bool WLoadFileToString(FString& Result, const FString& FullPath);
    static bool WSaveStringToFile(const FString& Parameter, const FString& FullPath);
    static int32 WGetFileSizeInBytes(const FString& FullPath);
    static FString WGetSafeErrorMessage();
    static FString WGenerateMD5HashFromString(const FString& RawData);
    static FString WGenerateMD5Hash(const TArray<uint8>& RawData);

    //Do not forget to deallocate the result. This does not return hex encoded result. Generates 4 byte basic hash.
    static FWCHARWrapper WBasicRawHash(FWCHARWrapper& Source, int32 FromSourceIndex, int32 Size);

    static FString Base64Encode(const FString& Source);
    static bool Base64Decode(const FString& Source, FString& Destination);

    static FString Base64EncodeFromWCHARArray(FWCHARWrapper& Source);
    static bool Base64DecodeToWCHARArray(const FString& Source, FWCHARWrapper& Destination);

    static FString Base64EncodeExceptExtension(const FString& FileName);
    static bool Base64DecodeExceptExtension(const FString& FileName, FString& Destination);

    static int32 GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen);
    static bool CompressBooleanAsBit(FWCHARWrapper& DestArr, const TArray<bool>& SourceArr);
    static bool DecompressBitAsBoolArray(TArray<bool>& DestArr, FWCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex);

    static uint8 CompressZeroOneFloatToByte(float Param);
    static float DecompressByteToZeroOneFloat(uint8 Param);
    static uint8 CompressAngleFloatToByte(float Param);
    static float DecompressByteToAngleFloat(uint8 Param);

    //Allocate Result with AllocateWCHARArray first.
    static void ConvertIntegerToByteArray(int32 Param, FWCHARWrapper& Result, uint8 UnitSize);
    //Allocate Result with AllocateWCHARArray first.
    static void ConvertFloatToByteArray(float Param, FWCHARWrapper& Result, uint8 UnitSize);
    static int32 ConvertByteArrayToInteger(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize);
    static float ConvertByteArrayToFloat(FWCHARWrapper& Source, int32 StartIndex, uint8 UnitSize);

    static FString StringToBinary(const FString& InputData);
    static FString BinaryToString(FString InputData);

    static TArray<uint8> StringToByteArray(const FString& InputData);
    static FString ByteArrayToString(const TArray<uint8>& ByteArray);
    static FString ConvertIntegerToHex(int32 inputValue);

    static int32 ConvertHexToInteger(const FString& InputData);
private:
    static void PrintInternal(EWLogType LogType, const FString& format, bool IsNext, bool PrintAnyway = false);
};

#endif