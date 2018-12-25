// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKUtilities
#define Pragma_Once_BKUtilities

#include "BKEngine.h"
#include "BKString.h"
#include "BKMutex.h"

enum class EBKLogType : uint8
{
    Error,
    Warning,
    Log
};

struct FBKCHARWrapper
{

private:
    ANSICHAR* Value = nullptr;
    int32 Size = 0;

public:
    void SetValue(ANSICHAR* const Parameter, int32 SizeParameter)
    {
        if (!Parameter || SizeParameter <= 0) return;
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
        delete[] Value;
        Size = 0;
    }

    bool IsValid()
    {
        return Size > 0 && Value;
    }

    volatile bool bDeallocateValueOnDestructor = false;

    FBKCHARWrapper() = default;
    FBKCHARWrapper(ANSICHAR* const Parameter, int32 SizeParameter, bool _bDeallocateValueOnDestructor = false)
    {
        Value = Parameter;
        Size = SizeParameter;
        bDeallocateValueOnDestructor = _bDeallocateValueOnDestructor;
    }
    FBKCHARWrapper(const FBKCHARWrapper& Other)
    {
        Value = Other.Value;
        Size = Other.Size;
        bDeallocateValueOnDestructor = Other.bDeallocateValueOnDestructor;
    }
    FBKCHARWrapper(FBKCHARWrapper&& Other) noexcept
    {
        Value = Other.Value;
        Size = Other.Size;
        bDeallocateValueOnDestructor = Other.bDeallocateValueOnDestructor;
    }
    explicit FBKCHARWrapper(FBKCHARWrapper* Other)
    {
        if (Other)
        {
            Value = Other->Value;
            Size = Other->Size;
            bDeallocateValueOnDestructor = Other->bDeallocateValueOnDestructor;
        }
    }
    FBKCHARWrapper& operator=(const FBKCHARWrapper& Other)
    {
        Value = Other.Value;
        Size = Other.Size;
        bDeallocateValueOnDestructor = Other.bDeallocateValueOnDestructor;
        return *this;
    }
    ~FBKCHARWrapper()
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
    explicit FScopeSafeCharArray(const FString& Parameter, bool UseWChars = false);
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
    FScopeSafeCharArray() = default;
    FScopeSafeCharArray(const FScopeSafeCharArray& InScopeLock);
    FScopeSafeCharArray& operator=(FScopeSafeCharArray& InScopeLock)
    {
        return *this;
    }

    ANSICHAR* CharArray = nullptr;
    UTFCHAR* WCharArray = nullptr;
};

class BKUtilities
{

private:
    static BKMutex PrintMutex;

public:
    static void Print(EBKLogType LogType, const FString& format);

    static uint64 GetTimeStampInMS();
    static uint32 GetSafeTimeStampInMS();
    static double GetTimeStampInMSDetailed();

    static FString WGetSafeErrorMessage();
    static FString WGenerateMD5HashFromString(const FString& RawData);
    static FString WGenerateMD5Hash(const TArray<uint8>& RawData);

    //Do not forget to deallocate the result. This does not return hex encoded result. Generates 4 byte basic hash.
    static FBKCHARWrapper WBasicRawHash(FBKCHARWrapper& Source, int32 FromSourceIndex, int32 Size);

    static FString Base64Encode(const FString& Source);
    static bool Base64Decode(const FString& Source, FString& Destination);

    static FString Base64EncodeExceptExtension(const FString& FileName);
    static bool Base64DecodeExceptExtension(const FString& FileName, FString& Destination);

    static int32 GetDestinationLengthBeforeCompressBoolArray(int32 SourceLen);
    static bool CompressBooleanAsBit(FBKCHARWrapper& DestArr, const TArray<bool>& SourceArr);
    static bool DecompressBitAsBoolArray(TArray<bool>& DestArr, FBKCHARWrapper& SourceAsCompressed, int32 StartIndex, int32 EndIndex);

    static uint8 CompressZeroOneFloatToByte(float Param);
    static float DecompressByteToZeroOneFloat(uint8 Param);
    static uint8 CompressAngleFloatToByte(float Param);
    static float DecompressByteToAngleFloat(uint8 Param);

    //Allocate Result with AllocateWCHARArray first.
    static void ConvertIntegerToByteArray(int32 Param, FBKCHARWrapper& Result, uint8 UnitSize);
    //Allocate Result with AllocateWCHARArray first.
    static void ConvertFloatToByteArray(float Param, FBKCHARWrapper& Result, uint8 UnitSize);
    static int32 ConvertByteArrayToInteger(FBKCHARWrapper& Source, int32 StartIndex, uint8 UnitSize);
    static float ConvertByteArrayToFloat(FBKCHARWrapper& Source, int32 StartIndex, uint8 UnitSize);

    static TArray<uint8> StringToByteArray(const FString& InputData);
    static FString ByteArrayToString(const TArray<uint8>& ByteArray);
    static FString ConvertIntegerToHex(int32 inputValue);

    static int32 ConvertHexToInteger(const FString& InputData);
};

#endif