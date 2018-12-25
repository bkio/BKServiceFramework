// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKMD5
#define Pragma_Once_BKMD5

#include "BKEngine.h"
#include "BKString.h"
#include "BKMemory.h"

class FMD5
{

public:
    FMD5();
    ~FMD5() = default;

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
    static FString HashAnsiString(const FString& String)
    {
        assert(!String.IsWide());

        if (String.Len() == 0) return EMPTY_FSTRING_ANSI;

        uint8 Digest[16];

        FMD5 Md5Gen;
        Md5Gen.Update((uint8*)String.GetAnsiCharArray(), String.Len());
        Md5Gen.Final(Digest);

        return FString::Hexify(Digest, 16);
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

    FContext Context{};

    enum {S11=7};
    enum {S12=12};
    enum {S13=17};
    enum {S14=22};
    enum {S21=5};
    enum {S22=9};
    enum {S23=14};
    enum {S24=20};
    enum {S31=4};
    enum {S32=11};
    enum {S33=16};
    enum {S34=23};
    enum {S41=6};
    enum {S42=10};
    enum {S43=15};
    enum {S44=21};
};

struct FMD5Hash;

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
    bool bIsValid = false;

    /** The bytes this hash comprises */
    uint8 Bytes[16]{};
};

static uint8 PADDING[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0
};

//
// Basic MD5 transformations.
//
#define MD5_F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~(z))))

//
// Rotates X left N bits.
//
#define ROTLEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

//
// Rounds 1, 2, 3, and 4 MD5 transformations.
// Rotation is separate from addition to prevent recomputation.
//
#define MD5_FF(a, b, c, d, x, s, ac) { \
	(a) += MD5_F ((b), (c), (d)) + (x) + (uint32)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define MD5_GG(a, b, c, d, x, s, ac) { \
	(a) += MD5_G ((b), (c), (d)) + (x) + (uint32)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define MD5_HH(a, b, c, d, x, s, ac) { \
	(a) += MD5_H ((b), (c), (d)) + (x) + (uint32)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define MD5_II(a, b, c, d, x, s, ac) { \
	(a) += MD5_I ((b), (c), (d)) + (x) + (uint32)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

FMD5::FMD5()
{
    Context.count[0] = Context.count[1] = 0;
    // Load magic initialization constants.
    Context.state[0] = 0x67452301;
    Context.state[1] = 0xefcdab89;
    Context.state[2] = 0x98badcfe;
    Context.state[3] = 0x10325476;
}

void FMD5::Update( const uint8* input, int32 inputLen )
{
    int32 i, index, partLen;

    // Compute number of bytes mod 64.
    index = (int32)((Context.count[0] >> 3) & 0x3F);

    // Update number of bits.
    if ((Context.count[0] += ((uint32)inputLen << 3)) < ((uint32)inputLen << 3))
    {
        Context.count[1]++;
    }
    Context.count[1] += ((uint32)inputLen >> 29);

    partLen = 64 - index;

    // Transform as many times as possible.
    if (inputLen >= partLen)
    {
        FMemory::Memcpy(&Context.buffer[index], input, static_cast<WSIZE__T>(partLen));
        Transform( Context.state, Context.buffer );
        for (i = partLen; i + 63 < inputLen; i += 64)
        {
            Transform( Context.state, &input[i] );
        }
        index = 0;
    }
    else
    {
        i = 0;
    }

    // Buffer remaining input.
    FMemory::Memcpy(&Context.buffer[index], &input[i], static_cast<WSIZE__T>(inputLen - i));
}

void FMD5::Final( uint8* digest )
{
    uint8 bits[8];
    int32 index, padLen;

    // Save number of bits.
    Encode( bits, Context.count, 8 );

    // Pad out to 56 mod 64.
    index = (int32)((Context.count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    Update( PADDING, padLen );

    // Append length (before padding).
    Update( bits, 8 );

    // Store state in digest
    Encode( digest, Context.state, 16 );

    // Zeroize sensitive information.
    FMemory::Memset( &Context, 0, sizeof(Context) );
}

void FMD5::Transform( uint32* state, const uint8* block )
{
    uint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    Decode( x, block, 64 );

    // Round 1
    MD5_FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    MD5_FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    MD5_FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    MD5_FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    MD5_FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    MD5_FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    MD5_FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    MD5_FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    MD5_FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    MD5_FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    MD5_FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    MD5_FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    MD5_FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    MD5_FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    MD5_FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    MD5_FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    // Round 2
    MD5_GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    MD5_GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    MD5_GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    MD5_GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    MD5_GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    MD5_GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    MD5_GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    MD5_GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    MD5_GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    MD5_GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    MD5_GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    MD5_GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    MD5_GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    MD5_GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    MD5_GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    MD5_GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    // Round 3
    MD5_HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    MD5_HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    MD5_HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    MD5_HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    MD5_HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    MD5_HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    MD5_HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    MD5_HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    MD5_HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    MD5_HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    MD5_HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    MD5_HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    MD5_HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    MD5_HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    MD5_HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    MD5_HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    // Round 4
    MD5_II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    MD5_II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    MD5_II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    MD5_II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    MD5_II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    MD5_II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    MD5_II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    MD5_II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    MD5_II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    MD5_II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    MD5_II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    MD5_II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    MD5_II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    MD5_II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    MD5_II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    MD5_II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    // Zeroize sensitive information.
    FMemory::Memset( x, 0, sizeof(x) );
}

void FMD5::Encode( uint8* output, const uint32* input, int32 len )
{
    int32 i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
    {
        output[j] = (uint8)(input[i] & 0xff);
        output[j+1] = (uint8)((input[i] >> 8) & 0xff);
        output[j+2] = (uint8)((input[i] >> 16) & 0xff);
        output[j+3] = (uint8)((input[i] >> 24) & 0xff);
    }
}

void FMD5::Decode( uint32* output, const uint8* input, int32 len )
{
    int32 i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
    {
        output[i] = ((uint32)input[j]) | (((uint32)input[j+1]) << 8) |
                    (((uint32)input[j+2]) << 16) | (((uint32)input[j+3]) << 24);
    }
}

#endif //Pragma_Once_BKMD5
