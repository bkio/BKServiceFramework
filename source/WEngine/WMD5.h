// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMD5
#define Pragma_Once_WMD5

#include "WEngine.h"
#include "WString.h"

class FMD5
{

public:
    FMD5();
    ~FMD5();

    /**
     * MD5 block update operation.  Continues an MD5 message-digest operation,
     * processing another message block, and updating the context.
     *
     * @param input		input data
     * @param inputLen	length of the input data in bytes
     **/
    void Update(const uint8* input, int32 inputLen);

    /**
     * MD5 finalization. Ends an MD5 message-digest operation, writing the
     * the message digest and zeroizing the context.
     * Digest is 16 BYTEs.
     *
     * @param digest	pointer to a buffer where the digest should be stored ( must have at least 16 bytes )
     **/
    void Final(uint8* digest);

    /**
     * Helper to perform the very common case of hashing an ASCII string into a hex representation.
     *
     * @param String	the string the hash
     **/
    static FString HashAnsiString(const TCHAR* String)
    {
        int32 Len = wcslen(String);
        if (Len == 0) return "";

        uint8 Digest[16];

        FMD5 Md5Gen;

        ANSICHAR* AsAnsi = new ANSICHAR[Len];
        std::wcstombs(AsAnsi, String, Len);

        Md5Gen.Update((unsigned char*)AsAnsi, Len);
        Md5Gen.Final( Digest );

        FString MD5;
        for (int32 i = 0; i < 16; i++)
        {
            MD5 += FString::Printf(L"%02x", Digest[i]);
        }

        delete[] AsAnsi;

        return MD5;
    }
private:
    struct FContext
    {
        uint32 state[4];
        uint32 count[2];
        uint8 buffer[64];
    };

    void Transform( uint32* state, const uint8* block );
    void Encode( uint8* output, const uint32* input, int32 len );
    void Decode( uint32* output, const uint8* input, int32 len );

    FContext Context;
};

struct FMD5Hash;

namespace Lex
{
    FString ToString(const FMD5Hash& Hash);
    void FromString(FMD5Hash& Hash, const TCHAR* Buffer);
}

/** Simple helper struct to ease the caching of MD5 hashes */
struct FMD5Hash
{
    /** Default constructor */
    FMD5Hash() : bIsValid(false) {}

    /** Check whether this has hash is valid or not */
    bool IsValid() const { return bIsValid; }

    /** Set up the MD5 hash from a container */
    void Set(FMD5& MD5)
    {
        MD5.Final(Bytes);
        bIsValid = true;
    }

    /** Compare one hash with another */
    friend bool operator==(const FMD5Hash& LHS, const FMD5Hash& RHS)
    {
        return LHS.bIsValid == RHS.bIsValid && (!LHS.bIsValid || FMemory::Memcmp(LHS.Bytes, RHS.Bytes, 16) == 0);
    }

    /** Compare one hash with another */
    friend bool operator!=(const FMD5Hash& LHS, const FMD5Hash& RHS)
    {
        return LHS.bIsValid != RHS.bIsValid || (LHS.bIsValid && FMemory::Memcmp(LHS.Bytes, RHS.Bytes, 16) != 0);
    }

    const uint8* GetBytes() const { return Bytes; }
    const int32 GetSize() const { return sizeof(Bytes); }

private:
    /** Whether this hash is valid or not */
    bool bIsValid;

    /** The bytes this hash comprises */
    uint8 Bytes[16];

    friend FString Lex::ToString(const FMD5Hash&);
    friend void Lex::FromString(FMD5Hash& Hash, const TCHAR*);
};

#endif //Pragma_Once_WMD5
