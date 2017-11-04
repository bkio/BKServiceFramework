// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMemory
#define Pragma_Once_WMemory

#include "WEngine.h"

class FMemory
{

public:
    static void* Memcpy(void* Dest, const void* Src, WSIZE__T Count)
    {
        return memcpy(Dest, Src, Count);
    }
    template<class T>
    static void Memcpy(T& Dest, const T& Src )
    {
        Memcpy(&Dest, &Src, sizeof(T));
    }

    static void* Memmove(void* Dest, const void* Src, WSIZE__T Count)
    {
        return memmove(Dest, Src, Count);
    }

    static int32 Memcmp(const void* Buf1, const void* Buf2, WSIZE__T Count)
    {
        return memcmp(Buf1, Buf2, Count);
    }

    static void* Memset(void* Dest, uint8 Char, WSIZE__T Count)
    {
        return memset(Dest, Char, Count);
    }

    template<class T>
    static void Memset( T& Src, uint8 ValueToSet )
    {
        Memset(&Src, ValueToSet, sizeof(T));
    }

    static void* Memzero(void* Dest, WSIZE__T Count)
    {
        return memset(Dest, 0, Count);
    }

    template<class T>
    static void Memzero( T& Src )
    {
        Memzero(&Src, sizeof(T));
    }

    static void* Malloc(size_t Count)
    {
        return malloc(Count);
    }
    static void Free(void* Original)
    {
        free(Original);
    }
};

#endif //Pragma_Once_WMemory