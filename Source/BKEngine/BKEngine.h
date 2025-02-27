// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKEngine
#define Pragma_Once_BKEngine

#include <cstring>
#include <string>

#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #ifdef WINVER
        #undef WINVER
    #endif
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif
    #define WINVER 0x0A00
    #define _WIN32_WINNT 0x0A00
#elif __APPLE__
    #define PLATFORM_APPLE 1
    #define PLATFORM_MAC 1
#elif __linux__ || __unix__ || defined(_POSIX_VERSION)
    #define PLATFORM_LINUX 1
#endif

#if !defined(PLATFORM_WINDOWS)
    #define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_MAC)
    #define PLATFORM_MAC 0
#endif
#if !defined(PLATFORM_APPLE)
    #define PLATFORM_APPLE 0
#endif
#if !defined(PLATFORM_LINUX)
    #define PLATFORM_LINUX 0
#endif

#define INDEX_NONE (-1)

template<typename T32BITS, typename T64BITS, int PointerSize>
struct SelectIntPointerType
{
    // nothing here are is it an error if the partial specializations fail
};

template<typename T32BITS, typename T64BITS>
struct SelectIntPointerType<T32BITS, T64BITS, 8>
{
    typedef T64BITS TIntPointer; // select the 64 bit type
};

template<typename T32BITS, typename T64BITS>
struct SelectIntPointerType<T32BITS, T64BITS, 4>
{
    typedef T32BITS TIntPointer; // select the 32 bit type
};

// Unsigned base types.
typedef unsigned char 		uint8;		// 8-bit  unsigned.
typedef unsigned short int	uint16;		// 16-bit unsigned.
typedef unsigned int		uint32;		// 32-bit unsigned.
typedef unsigned long long	uint64;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			int8;		// 8-bit  signed.
typedef signed short int	int16;		// 16-bit signed.
typedef signed int	 		int32;		// 32-bit signed.
typedef signed long long	int64;		// 64-bit signed.

// Character types.
typedef char				ANSICHAR;	// An ANSI character       -                  8-bit fixed-width representation of 7-bit characters.
typedef wchar_t				WIDECHAR;	// A wide character        - In-memory only.  ?-bit fixed-width representation of the platform's natural wide character set.  Could be different sizes on different platforms.
typedef uint8				CHAR8;		// An 8-bit character type - In-memory only.  8-bit representation.
typedef uint16				CHAR16;		// A 16-bit character type - In-memory only.  16-bit representation.
typedef uint32				CHAR32;		// A 32-bit character type - In-memory only.  32-bit representation.
typedef WIDECHAR			UTFCHAR;		// A switchable character  - In-memory only.  Either ANSICHAR or WIDECHAR, depending on a licensee's requirements.

typedef SelectIntPointerType<uint32, uint64, sizeof(void*)>::TIntPointer UPTRINT;	// unsigned int the same size as a pointer
typedef SelectIntPointerType<int32, int64, sizeof(void*)>::TIntPointer PTRINT;		// signed int the same size as a pointer
typedef UPTRINT WSIZE__T;																// unsigned int the same size as a pointer
typedef PTRINT WSSIZE__T;																// signed int the same size as a pointer

typedef int32					TYPE_OF_NULL;
typedef decltype(nullptr)		TYPE_OF_NULLPTR;

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)
#define BIG_NUMBER			(3.4e+38f)

// Aux constants.
#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)

#define THRESH_POINT_ON_PLANE			(0.10f)		/* Thickness of plane for front/back/inside test */
#define THRESH_POINTS_ARE_SAME			(0.00002f)	/* Two points are same if within this distance */
#define THRESH_NORMALS_ARE_PARALLEL		(0.999845f)	/* Two unit vectors are parallel if abs(A dot B) is greater than or equal to this. This is roughly cosine(1.0 degrees). */
#define THRESH_NORMALS_ARE_ORTHOGONAL	(0.017455f)	/* Two unit vectors are orthogonal (perpendicular) if abs(A dot B) is less than or equal this. This is roughly cosine(89.0 degrees). */

#define THRESH_VECTOR_NORMALIZED		(0.01f)		/** Allowed error for a normalized vector (against squared magnitude) */

#define MAX_int16		((int16)	0x7fff)

#endif