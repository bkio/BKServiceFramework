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
    ANSICHAR* Value = nullptr;
    int32 Size = 0;

public:
    void SetValue(ANSICHAR* const Parameter, int32 SizeParameter)
    {
        if (Parameter == nullptr || SizeParameter <= 0) return;
        Value = Parameter;
        Size = SizeParameter;
    }
    ANSICHAR* const GetValue() const
    {
        return Value;
    }
    int32 GetSize() const
    {
        return Size;
    }
    void SetArrayElement(int32 Index, ANSICHAR Character)
    {
        if (Index >= 0 && Index < Size)
        {
            Value[Index] = Character;
        }
    }
    ANSICHAR GetArrayElement(int32 Index)
    {
        if (Index >= 0 && Index < Size)
        {
            return Value[Index];
        }
        return (ANSICHAR)0;
    }
    void OrSetElement(int32 Index, ANSICHAR WithCharacter)
    {
        if (Index >= 0 && Index < Size)
        {
            Value[Index] |= WithCharacter;
        }
    }
    void AndSetElement(int32 Index, ANSICHAR WithCharacter)
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
    FWCHARWrapper(ANSICHAR* const Parameter, int32 SizeParameter, bool _bDeallocateValueOnDestructor = false)
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

    const ANSICHAR* GetC()
    {
        return CharArray;
    }
    const UTFCHAR* GetW()
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

    ANSICHAR* CharArray = nullptr;
    UTFCHAR* WCharArray = nullptr;
};

class UWUtilities
{
public:
    static void Print(EWLogType LogType, const FString& format);
    static void Print(EWLogType LogType, const UTFCHAR* format);

    static int64 GetTimeStampInMS();
    static int32 GetSafeTimeStampInMS();

    static FString WGetSafeErrorMessage();
    static FString WGenerateMD5HashFromString(const FString& RawData);
    static FString WGenerateMD5Hash(const TArray<uint8>& RawData);

    //Do not forget to deallocate the result. This does not return hex encoded result. Generates 4 byte basic hash.
    static FWCHARWrapper WBasicRawHash(FWCHARWrapper& Source, int32 FromSourceIndex, int32 Size);

    static FString Base64Encode(const FString& Source);
    static bool Base64Decode(const FString& Source, FString& Destination);

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

    static TArray<uint8> StringToByteArray(const FString& InputData);
    static FString ByteArrayToString(const TArray<uint8>& ByteArray);
    static FString ConvertIntegerToHex(int32 inputValue);

    static int32 ConvertHexToInteger(const FString& InputData);
};

#endif