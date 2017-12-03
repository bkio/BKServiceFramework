// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WString
#define Pragma_Once_WString

#include "WEngine.h"
#include "WArray.h"
#include <string>
#include <cstdarg>
#include <regex>
#include <codecvt>
#include <cassert>
#include <iomanip>

#if PLATFORM_WINDOWS
    #include "windows.h"
#endif

namespace ESearchCase
{
    enum Type
    {
        /** Case sensitive. Upper/lower casing must match for strings to be considered equal. */
       CaseSensitive,

        /** Ignore case. Upper/lower casing does not matter when making a comparison. */
       IgnoreCase,
    };
};
namespace ESearchDir
{
    enum Type
    {
        /** Search from the start, moving forward through the string. */
        FromStart,

        /** Search from the end, moving backward through the string. */
        FromEnd
    };
}

class FString
{

private:
    bool bIsWide = false;

    std::wstring DataWide;
    std::string DataAnsi;

public:
    bool IsWide()const
    {
        return bIsWide;
    }

    const UTFCHAR& AtWide(uint32 Ix) const
    {
        assert(bIsWide);
        assert(Ix >= 0 && Ix < DataWide.length());
        return DataWide.at(Ix);
    }
    UTFCHAR& AtWide(uint32 Ix)
    {
        assert(bIsWide);
        assert(Ix >= 0 && Ix < DataWide.length());
        return DataWide.at(Ix);
    }

    const ANSICHAR& AtAnsi(uint32 Ix) const
    {
        assert(!bIsWide);
        assert(Ix >= 0 && Ix < DataAnsi.length());
        return DataAnsi.at(Ix);
    }
    ANSICHAR& AtAnsi(uint32 Ix)
    {
        assert(!bIsWide);
        assert(Ix >= 0 && Ix < DataAnsi.length());
        return DataAnsi.at(Ix);
    }

    static std::wstring StringToWString(const std::string& t_str)
    {
        std::wstringstream WStringStream;

        auto Length = static_cast<uint32>(t_str.size());
        for (uint32 i = 0; i < Length; i++)
        {
            WStringStream << t_str.at(i);
        }
        return WStringStream.str();
    }
    static FString StringToWString(const FString& t_str)
    {
        if (t_str.bIsWide) return t_str;
        return FString(StringToWString(t_str.GetAnsiCharString()));
    }
    static std::string WStringToString(const std::wstring& _str)
    {
        typedef std::codecvt_utf8<wchar_t> convert_type;
        std::wstring_convert<convert_type, wchar_t> converter;
        return converter.to_bytes(_str);
    }
    static FString WStringToString(const FString& _str)
    {
        if (!_str.bIsWide) return _str;
        return FString(WStringToString(_str.GetWideCharString()));
    }

    template <typename T>
    static T ConvertToInteger(const ANSICHAR *Data)
    {
        if (Data)
        {
            std::string AsString;
            std::stringstream StringBuilder;

            auto Length = static_cast<int32>(strlen(Data));
            for (int32 i = 0; i < Length; i++)
            {
                if (isdigit(Data[i]) || Data[i] == '-')
                {
                    StringBuilder << Data[i];
                }
            }
            AsString = StringBuilder.str();
            if (AsString.empty())
            {
                return T();
            }

            T Val;
            std::istringstream ISS(AsString);
            ISS >> std::dec >> Val;

            if (ISS.fail())
            {
                return T();
            }
            return Val;
        }
        return T();
    }
    template <typename T>
    static T ConvertToInteger(const FString &Input)
    {
        std::string NormalString;
        if (Input.bIsWide)
        {
            NormalString = WStringToString(Input.DataWide);
        }
        else
        {
            NormalString = Input.DataAnsi;
        }
        return ConvertToInteger<T>(NormalString.c_str());
    }

    #define FromNumeric_Define( CastTo ) \
        if (bMakeWideOutput){ \
            std::wstringstream Stream; \
            Stream << (CastTo)Num; \
            return FString(Stream.str()); \
        } \
        std::stringstream Stream; \
        Stream << (CastTo)Num; \
        return FString(Stream.str());

    static FString FromInt( int8 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int8);
    }
    static FString FromInt( int16 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int16);
    }
    static FString FromInt( int32 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int32);
    }
    static FString FromInt( int64 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int64);
    }
    static FString FromInt( uint8 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int16);
    }
    static FString FromInt( uint16 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int32);
    }
    static FString FromInt( uint32 Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(int64);
    }
    static FString FromFloat( float Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(float);
    }
    static FString FromFloat( double Num, bool bMakeWideOutput = false )
    {
        FromNumeric_Define(double);
    }

    int32 Len() const
    {
        if (bIsWide) return static_cast<int32>(DataWide.length());
        return static_cast<int32>(DataAnsi.length());
    }

    int32 AddUninitialized(int32 Count = 1)
    {
        int32 OldNum;
        if (bIsWide)
        {
            OldNum = static_cast<int32>(DataWide.length());
            DataWide.resize(DataWide.length() + Count);
        }
        else
        {
            OldNum = static_cast<int32>(DataAnsi.length());
            DataAnsi.resize(DataAnsi.length() + Count);
        }
        return OldNum;
    }
    void InsertUninitialized(int32 Index, int32 Count = 1)
    {
        if (bIsWide)
        {
            DataWide.resize(DataWide.length() + Count);
        }
        else
        {
            DataAnsi.resize(DataAnsi.length() + Count);
        }
    }

    FString() = default;
    FString(const FString& Other)
    {
        if (Other.bIsWide)
        {
            bIsWide = true;
            DataWide = Other.DataWide;
        }
        else
        {
            bIsWide = false;
            DataAnsi = Other.DataAnsi;
        }
    }
    explicit FString(const UTFCHAR* Other)
    {
        bIsWide = true;
        DataWide = Other;
    }
    explicit FString(const ANSICHAR* Other)
    {
        bIsWide = false;
        DataAnsi = Other;
    }
    explicit FString(const std::wstring& Other)
    {
        bIsWide = true;
        DataWide = Other;
    }
    explicit FString(const std::string& Other)
    {
        bIsWide = false;
        DataAnsi = Other;
    }
    FString(int32 InCount, const UTFCHAR* InSrc)
    {
        bIsWide = true;

        AddUninitialized(InCount ? InCount + 1 : 0);

        if (DataWide.length() > 0)
        {
            for (int32 i = InCount; i < InCount + 1; i++)
            {
                DataWide[i] = InSrc[i - InCount];
            }
        }
    }
    FString(int32 InCount, const ANSICHAR* InSrc)
    {
        bIsWide = false;

        AddUninitialized(InCount ? InCount + 1 : 0);

        if (DataAnsi.length() > 0)
        {
            for (int32 i = InCount; i < InCount + 1; i++)
            {
                DataAnsi[i] = InSrc[i - InCount];
            }
        }
    }
    FString(uint32 InCount, const UTFCHAR InSrc)
    {
        bIsWide = true;

        DataWide = std::wstring(InCount, InSrc);
    }
    FString(uint32 InCount, const ANSICHAR InSrc)
    {
        bIsWide = false;

        DataAnsi = std::string(InCount, InSrc);
    }
    FString(const UTFCHAR* Other, uint32 Size)
    {
        DataWide.resize(Size);
        for (int32 i = 0; i < Size; i++)
        {
            DataWide[i] = Other[i];
        }
    }
    FString(const ANSICHAR* Other, uint32 Size)
    {
        DataAnsi.resize(Size);
        for (int32 i = 0; i < Size; i++)
        {
            DataAnsi[i] = Other[i];
        }
    }
    FString& operator=(const FString& Other) = default;

    FString& operator=(const UTFCHAR* Other)
    {
        bIsWide = true;
        DataAnsi.clear();
        DataWide = Other;
        return *this;
    }
    FString& operator=(const ANSICHAR* Other)
    {
        bIsWide = false;
        DataWide.clear();
        DataAnsi = Other;
        return *this;
    }
    FString& operator=(const std::wstring& Other)
    {
        bIsWide = true;
        DataAnsi.clear();
        DataWide = Other;
        return *this;
    }
    FString& operator=(const std::string& Other)
    {
        bIsWide = false;
        DataWide.clear();
        DataAnsi = Other;
        return *this;
    }

    bool operator<(const FString& Other) const
    {
        assert(bIsWide == Other.bIsWide);
        if (bIsWide) return DataWide.compare(Other.DataWide) > 0;
        return DataAnsi.compare(Other.DataAnsi) > 0;
    }
    bool operator>(const FString& Other) const
    {
        assert(bIsWide == Other.bIsWide);
        if (bIsWide) return DataWide.compare(Other.DataWide) < 0;
        return DataAnsi.compare(Other.DataAnsi) < 0;
    }
    bool operator<=(const FString& Other) const
    {
        assert(bIsWide == Other.bIsWide);
        if (bIsWide) return DataWide.compare(Other.DataWide) >= 0;
        return DataAnsi.compare(Other.DataAnsi) >= 0;
    }
    bool operator>=(const FString& Other) const
    {
        assert(bIsWide == Other.bIsWide);
        if (bIsWide) return DataWide.compare(Other.DataWide) <= 0;
        return DataAnsi.compare(Other.DataAnsi) <= 0;
    }
    bool operator==(const FString& Other) const
    {
        assert(bIsWide == Other.bIsWide);
        if (bIsWide) return DataWide.compare(Other.DataWide) == 0;
        return DataAnsi.compare(Other.DataAnsi) == 0;
    }
    bool operator!=(const FString& Other) const
    {
        if (bIsWide) return DataWide.compare(Other.DataWide) != 0;
        return DataAnsi.compare(Other.DataAnsi) != 0;
    }
    bool Equals(const FString& Other) const
    {
        if (bIsWide) return DataWide.compare(Other.DataWide) != 0;
        return DataAnsi.compare(Other.DataAnsi) != 0;
    }

    const UTFCHAR* operator*() const
    {
        assert(bIsWide);
        return DataWide.data();
    }
    FString& operator/= (const FString& Str)
    {
        assert(bIsWide == Str.bIsWide);
        if (bIsWide)
        {
            DataWide += L"/";
            DataWide += Str.DataWide;
        }
        else
        {
            DataAnsi += "/";
            DataAnsi += Str.DataAnsi;
        }
        return *this;
    }
    FString& operator/= (const UTFCHAR* Str)
    {
        assert(bIsWide);
        DataWide += L"/";
        DataWide += Str;
        return *this;
    }
    FString& operator/= (const ANSICHAR* Str)
    {
        assert(!bIsWide);
        DataAnsi += "/";
        DataAnsi += Str;
        return *this;
    }

    FString& operator+= (const FString& Str)
    {
        assert(bIsWide == Str.bIsWide);
        if (bIsWide)
        {
            DataWide += Str.DataWide;
        }
        else
        {
            DataAnsi += Str.DataAnsi;
        }
        return *this;
    }
    FString& operator+= (const UTFCHAR* Str)
    {
        assert(bIsWide);
        DataWide += Str;
        return *this;
    }
    FString& operator+= (UTFCHAR Character)
    {
        assert(bIsWide);
        DataWide += Character;
        return *this;
    }
    FString& operator+= (const ANSICHAR* Str)
    {
        assert(!bIsWide);
        DataAnsi += Str;
        return *this;
    }
    FString& operator+= (ANSICHAR Character)
    {
        assert(!bIsWide);
        DataAnsi += Character;
        return *this;
    }

    const UTFCHAR* GetWideCharArray() const
    {
        assert(bIsWide);
        return DataWide.data();
    }
    const ANSICHAR* GetAnsiCharArray() const
    {
        assert(!bIsWide);
        return DataAnsi.data();
    }
    std::wstring GetWideCharString() const
    {
        assert(bIsWide);
        return DataWide;
    }
    const std::string GetAnsiCharString() const
    {
        assert(!bIsWide);
        return DataAnsi;
    }

    FString& Append(const UTFCHAR* Text, int32 Count)
    {
        assert(bIsWide);
        DataWide.append(Text);
        return *this;
    }
    FString& Append(const ANSICHAR* Text, int32 Count)
    {
        assert(!bIsWide);
        DataAnsi.append(Text);
        return *this;
    }
    FString& Append(const FString& Text)
    {
        assert(bIsWide == Text.bIsWide);
        if (bIsWide)
        {
            DataWide.append(Text.DataWide);
        }
        else
        {
            DataAnsi.append(Text.DataAnsi);
        }
        return *this;
    }
    FString& AppendChar(const UTFCHAR InChar)
    {
        assert(bIsWide);
        DataWide += InChar;
        return *this;
    }
    FString& AppendChar(const ANSICHAR InChar)
    {
        assert(!bIsWide);
        DataAnsi += InChar;
        return *this;
    }
    void AppendChars(const UTFCHAR* Array, uint32 Count)
    {
        assert(bIsWide);

        if (Count <= 0) return;
        auto OldSize = static_cast<int32>(DataWide.size());
        DataWide.reserve(OldSize + Count);
        for (int32 i = 0; i < Count; i++)
        {
            DataWide[OldSize + i] = Array[i];
        }
    }
    void AppendChars(const ANSICHAR* Array, uint32 Count)
    {
        assert(!bIsWide);

        if (Count <= 0) return;
        auto OldSize = static_cast<int32>(DataAnsi.size());
        DataAnsi.reserve(OldSize + Count);
        for (int32 i = 0; i < Count; i++)
        {
            DataAnsi[OldSize + i] = Array[i];
        }
    }

    #define Contains_Define( DataVariable, std_str ) \
    if (SearchCase == ESearchCase::IgnoreCase) \
    { \
        std::std_str Tmp = DataVariable; \
        std::std_str LookFor = SubStr; \
        \
        std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower); \
        std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower); \
        \
        return SearchDir == ESearchDir::FromStart ? \
               Tmp.find(LookFor) != std::std_str::npos : \
               Tmp.rfind(LookFor) != std::std_str::npos; \
    } \
    return SearchDir == ESearchDir::FromStart ? \
    DataVariable.find(SubStr) != std::std_str::npos : \
            DataVariable.rfind(SubStr) != std::std_str::npos;

    bool Contains(const UTFCHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        assert(bIsWide);
        Contains_Define(DataWide, wstring);
    }
    bool Contains(const ANSICHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        assert(!bIsWide);
        Contains_Define(DataAnsi, string);
    }
    bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        assert(bIsWide == SubStr.bIsWide);
        if (bIsWide)
        {
            return Contains(SubStr.DataWide.data(), static_cast<int32>(SubStr.DataWide.length()), SearchCase, SearchDir);
        }
        return Contains(SubStr.DataAnsi.data(), static_cast<int32>(SubStr.DataAnsi.length()), SearchCase, SearchDir);
    }
    void Empty(uint32 Slack = 0)
    {
        if (bIsWide)
        {
            DataWide.clear();
            if (Slack > 0)
            {
                DataWide.resize(Slack);
            }
        }
        else
        {
            DataAnsi.clear();
            if (Slack > 0)
            {
                DataAnsi.resize(Slack);
            }
        }
    }

    #define EndsWith_Define( DataVariable, std_str ) \
    if (Len >= DataVariable.length()) \
    { \
        if (SearchCase == ESearchCase::IgnoreCase) \
        { \
            std::std_str Tmp = DataVariable; \
            std::std_str LookFor = InSuffix; \
            \
            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower); \
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower); \
            \
            for (uint32 i = Len - 1; i >= 0; i--) \
            { \
                if (LookFor.at(i) != Tmp.at(i)) \
                { \
                    return false; \
                } \
            } \
        } \
        else \
        { \
            for (uint32 i = Len - 1; i >= 0; i--) \
            { \
                if (InSuffix[i] != DataVariable.at(i)) \
                { \
                    return false; \
                } \
            } \
        } \
        return true; \
    } \
    return false;

    bool EndsWith(const UTFCHAR* InSuffix, uint32 Len, ESearchCase::Type SearchCase)
    {
        assert(bIsWide);
        EndsWith_Define(DataWide, wstring);
    }
    bool EndsWith(const ANSICHAR* InSuffix, uint32 Len, ESearchCase::Type SearchCase)
    {
        assert(!bIsWide);
        EndsWith_Define(DataAnsi, string);
    }
    bool EndsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        assert(bIsWide == InSuffix.bIsWide);
        if (bIsWide)
        {
            return EndsWith(InSuffix.DataWide.data(), static_cast<uint32>(InSuffix.DataWide.length()), SearchCase);
        }
        return EndsWith(InSuffix.DataAnsi.data(), static_cast<uint32>(InSuffix.DataAnsi.length()), SearchCase);
    }

    #define StartsWith_Define( DataVariable, std_str ) \
    if (Len >= DataVariable.length()) \
    { \
        if (SearchCase == ESearchCase::IgnoreCase) \
        { \
            std::std_str Tmp = DataVariable; \
            std::std_str LookFor = InPrefix; \
             \
            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower); \
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower); \
             \
            for (uint32 i = 0; i < Len; i++) \
            { \
                if (LookFor.at(i) != Tmp.at(i)) \
                { \
                    return false; \
                } \
            } \
        } \
        else \
        { \
            for (uint32 i = 0; i < Len; i++) \
            { \
                if (InPrefix[i] != DataVariable.at(i)) \
                { \
                    return false; \
                } \
            } \
        } \
        return true; \
    } \
    return false;

    bool StartsWith(const UTFCHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        assert(bIsWide);
        StartsWith_Define(DataWide, wstring);
    }
    bool StartsWith(const ANSICHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        assert(!bIsWide);
        StartsWith_Define(DataAnsi, string);
    }
    bool StartsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        assert(bIsWide == InSuffix.bIsWide);
        if (bIsWide)
        {
            return StartsWith(InSuffix.DataWide.data(), static_cast<int32>(InSuffix.DataWide.length()), SearchCase);
        }
        return StartsWith(InSuffix.DataAnsi.data(), static_cast<int32>(InSuffix.DataAnsi.length()), SearchCase);
    }

    void InsertAt(uint32 Index, UTFCHAR Character)
    {
        assert(bIsWide);
        DataWide.insert(Index, 1, Character);
    }
    void InsertAt(uint32 Index, ANSICHAR Character)
    {
        assert(!bIsWide);
        DataAnsi.insert(Index, 1, Character);
    }
    void InsertAt(uint32 Index, const FString& Characters)
    {
        if (bIsWide)
        {
            DataWide.insert(Index, Characters.DataWide);
        }
        else
        {
            DataAnsi.insert(Index, Characters.DataAnsi);
        }
    }
    bool IsEmpty()
    {
        if (bIsWide)
        {
            return DataWide.length() == 0;
        }
        return DataAnsi.length() == 0;
    }

    #define IsNumeric_Define( DataVariable, std_str ) \
    int32 DotCount = 0; \
    std::std_str::const_iterator it = DataVariable.begin(); \
    while (it != DataVariable.end() && (std::isdigit(*it) || (it == DataVariable.begin() && *it == '-') || *it == '.')) \
    { \
        if (*it == '.') \
        { \
            DotCount++; \
        } \
        ++it; \
    } \
    return !DataVariable.empty() && it == DataVariable.end() && DotCount <= 1;

    bool IsNumeric()
    {
        if (bIsWide)
        {
            IsNumeric_Define(DataWide, wstring);
        }
        IsNumeric_Define(DataAnsi, string);
    }
    bool IsValidIndex(uint32 Index)
    {
        if (bIsWide)
        {
            return Index >= 0 && Index < DataWide.length();
        }
        return Index >= 0 && Index < DataAnsi.length();
    }
    FString Left(uint32 Count)
    {
        if (bIsWide)
        {
            return FString(DataWide.substr(0, Count));
        }
        return FString(DataAnsi.substr(0, Count));
    }
    FString Right(uint32 FromIx)
    {
        if (bIsWide)
        {
            return FString(DataWide.substr(FromIx, DataWide.length() - FromIx));
        }
        return FString(DataAnsi.substr(FromIx, DataAnsi.length() - FromIx));
    }
    FString Mid(uint32 Start, uint32 Count) const
    {
        if (bIsWide)
        {
            return FString(DataWide.substr(Start, Count));
        }
        return FString(DataAnsi.substr(Start, Count));
    }

    #define ParseIntoArray_Define_1( DataVariable, char_type, len_func, comp_func ) \
    OutArray.Empty(); \
    const char_type *Start = DataVariable.data(); \
    const int32 Length = DataWide.length(); \
    if (Start) \
    { \
        int32 SubstringBeginIndex = 0; \
         \
        for(int32 i = 0; i < DataVariable.length();) \
        { \
            int32 SubstringEndIndex = INDEX_NONE; \
            uint32 DelimiterLength = 0; \
             \
            for (int32 DelimIndex = 0; DelimIndex < NumDelims; ++DelimIndex) \
            { \
                DelimiterLength = len_func(DelimArray[DelimIndex]); \
                 \
                if (comp_func(Start + i, DelimArray[DelimIndex], DelimiterLength) == 0) \
                { \
                    SubstringEndIndex = i; \
                    break; \
                } \
            } \
             \
            if (SubstringEndIndex != INDEX_NONE) \
            { \
                const int32 SubstringLength = SubstringEndIndex - SubstringBeginIndex; \
                if(!InCullEmpty || SubstringLength != 0) \
                { \
                    OutArray.Add(FString(SubstringEndIndex - SubstringBeginIndex, Start + SubstringBeginIndex)); \
                } \
                SubstringBeginIndex = SubstringEndIndex + DelimiterLength; \
                i = SubstringBeginIndex; \
            } \
            else \
            { \
                ++i; \
            } \
        } \
         \
        const int32 SubstringLength = Length - SubstringBeginIndex; \
        if(!InCullEmpty || SubstringLength != 0) \
        { \
            OutArray.Add(FString(Start + SubstringBeginIndex)); \
        } \
    } \
    return OutArray.Num();

    int32 ParseIntoArray(TArray<FString>& OutArray, const UTFCHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        assert(bIsWide);
        ParseIntoArray_Define_1(DataWide, UTFCHAR, wcslen, wcsncmp);
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, const ANSICHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        assert(!bIsWide);
        ParseIntoArray_Define_1(DataAnsi, ANSICHAR, strlen, strncmp);
    }

    #define ParseIntoArray_Define_2( DataVariable, char_type, len_func, strstr_func ) \
    OutArray.Reset(); \
    const char_type* Start = DataVariable.data(); \
    const int32 DelimLength = len_func(pchDelim); \
    if (Start && DelimLength) \
    { \
        while (const char_type *At = strstr_func(Start,pchDelim) ) \
        { \
            if (!InCullEmpty || At-Start) \
            { \
                OutArray.Emplace(At-Start,Start); \
            } \
            Start = At + DelimLength; \
        } \
        if (!InCullEmpty || *Start) \
        { \
            OutArray.Emplace(Start); \
        } \
    } \
    return OutArray.Num();

    int32 ParseIntoArray(TArray<FString>& OutArray, const UTFCHAR* pchDelim, bool InCullEmpty)
    {
        assert(bIsWide);
        ParseIntoArray_Define_2(DataWide, UTFCHAR, wcslen, wcsstr);
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, const ANSICHAR* pchDelim, bool InCullEmpty)
    {
        assert(!bIsWide);
        ParseIntoArray_Define_2(DataAnsi, ANSICHAR, strlen, strstr);
    }

    int32 ParseIntoArrayWS(TArray<FString>& OutArray, const UTFCHAR* pchExtraDelim, bool InCullEmpty) const
    {
        assert(bIsWide);

        static const UTFCHAR* WhiteSpace[] = { L" ", L"\t", L"\r", L"\n", L"" };
        int32 NumWhiteSpaces = 4;
        if (pchExtraDelim && *pchExtraDelim)
        {
            WhiteSpace[NumWhiteSpaces++] = pchExtraDelim;
        }
        return ParseIntoArray(OutArray, WhiteSpace, NumWhiteSpaces, InCullEmpty);
    }
    int32 ParseIntoArrayWS(TArray<FString>& OutArray, const ANSICHAR* pchExtraDelim, bool InCullEmpty) const
    {
        assert(!bIsWide);

        static const ANSICHAR* WhiteSpace[] = { " ", "\t", "\r", "\n", "" };
        int32 NumWhiteSpaces = 4;
        if (pchExtraDelim && *pchExtraDelim)
        {
            WhiteSpace[NumWhiteSpaces++] = pchExtraDelim;
        }
        return ParseIntoArray(OutArray, WhiteSpace, NumWhiteSpaces, InCullEmpty);
    }

    int32 ParseIntoArrayLines(TArray<FString>& OutArray, bool InCullEmpty) const
    {
        static const UTFCHAR* Wide_LineEndings[] = { L"\r\n", L"\r", L"\n" };
        static const ANSICHAR* Ansi_LineEndings[] = { "\r\n", "\r", "\n" };
        static const int32 NumLineEndings = 3;

        if (bIsWide)
        {
            return ParseIntoArray(OutArray, Wide_LineEndings, NumLineEndings, InCullEmpty);
        }
        return ParseIntoArray(OutArray, Ansi_LineEndings, NumLineEndings, InCullEmpty);
    }
    void RemoveAt(uint32 Index, uint32 Count)
    {
        if (bIsWide)
        {
            DataWide.erase(Index, Count);
        }
        else
        {
            DataAnsi.erase(Index, Count);
        }
    }

    #define RemoveFromEnd_Define( DataVariable, std_str, empty_char ) \
    if (SearchCase == ESearchCase::IgnoreCase) \
    { \
        std::std_str Tmp = DataVariable; \
        std::std_str LookFor = InSuffix.DataVariable; \
         \
        std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower); \
        std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower); \
         \
        uint32 Pos = Tmp.rfind(LookFor); \
        if (Pos != std::std_str::npos) \
        { \
            DataVariable.replace(Pos, LookFor.length(), empty_char); \
            return true; \
        } \
    } \
    else \
    { \
        uint32 Pos = DataVariable.rfind(InSuffix.DataVariable); \
        if (Pos != std::wstring::npos) \
        { \
            (DataVariable).replace(Pos, InSuffix.DataVariable.length(), empty_char); \
            return true; \
        } \
    } \
    return false;

    bool RemoveFromEnd(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        if (bIsWide)
        {
            RemoveFromEnd_Define(DataWide, wstring, L"");
        }
        RemoveFromEnd_Define(DataAnsi, string, "");
    }

    #define RemoveFromStart_Define( DataVariable, std_str, empty_char ) \
    if (SearchCase == ESearchCase::IgnoreCase) \
    { \
        std::std_str Tmp = DataVariable; \
        std::std_str LookFor = InPrefix.DataVariable; \
         \
        std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower); \
        std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower); \
         \
        uint32 Pos = Tmp.find(LookFor); \
        if (Pos != std::std_str::npos) \
        { \
            DataVariable.replace(Pos, LookFor.length(), empty_char); \
            return true; \
        } \
    } \
    else \
    { \
        auto Pos = DataVariable.find(InPrefix.DataVariable); \
        if (Pos != std::std_str::npos) \
        { \
            DataVariable.replace(Pos, InPrefix.DataVariable.length(), empty_char); \
            return true; \
        } \
    } \
    return false;

    bool RemoveFromStart(const FString& InPrefix, ESearchCase::Type SearchCase)
    {
        if (bIsWide)
        {
            RemoveFromStart_Define(DataWide, wstring, L"");
        }
        RemoveFromStart_Define(DataAnsi, string, "");
    }

    #define Replace_Define( DataVariable, std_str ) \
    std::std_str NewData = DataVariable; \
     \
    std::std_str FromString = From; \
    std::std_str ToString = To; \
     \
    if (SearchCase == ESearchCase::IgnoreCase) \
    { \
        std::std_str Tmp; \
        std::transform(NewData.begin(), NewData.end(), Tmp.begin(), ::tolower); \
        std::transform(FromString.begin(), FromString.end(), FromString.begin(), ::tolower); \
         \
        auto Pos = Tmp.find(From); \
        while (Pos != std::std_str::npos) \
        { \
            NewData.replace(Pos, FromString.size(), ToString); \
            Pos = Tmp.find(FromString); \
        } \
    } \
    else \
    { \
        size_t Pos = NewData.find(FromString); \
        if(Pos != std::string::npos) \
        { \
            NewData.replace(Pos, FromString.length(), ToString); \
        } \
    } \
    return FString(NewData);

    FString Replace(const UTFCHAR* From, const UTFCHAR* To, ESearchCase::Type SearchCase)
    {
        assert(bIsWide);
        Replace_Define(DataWide, wstring);
    }
    FString Replace(const ANSICHAR* From, const ANSICHAR* To, ESearchCase::Type SearchCase)
    {
        assert(!bIsWide);
        Replace_Define(DataAnsi, string);
    }

    void SetElement(uint32 Index, UTFCHAR Character)
    {
        assert(bIsWide);
        assert(Index >= 0 && Index < DataWide.size());
        DataWide[Index] = Character;
    }
    void SetElement(uint32 Index, ANSICHAR Character)
    {
        assert(!bIsWide);
        assert(Index >= 0 && Index < DataAnsi.size());
        DataAnsi[Index] = Character;
    }

    void Reserve(uint32 CharacterCount)
    {
        if (bIsWide)
        {
            DataWide.reserve(CharacterCount);
        }
        else
        {
            DataAnsi.reserve(CharacterCount);
        }
    }
    void Resize(uint32 CharacterCount)
    {
        if (bIsWide)
        {
            DataWide.resize(CharacterCount);
        }
        else
        {
            DataAnsi.resize(CharacterCount);
        }
    }
    void Reset()
    {
        if (bIsWide)
        {
            DataWide.empty();
        }
        else
        {
            DataAnsi.empty();
        }
    }
    void ReverseString()
    {
        if (bIsWide)
        {
            std::reverse(DataWide.begin(), DataWide.end());
        }
        else
        {
            std::reverse(DataAnsi.begin(), DataAnsi.end());
        }
    }
    FString ToLower() const
    {
        if (bIsWide)
        {
            std::wstring NewData = DataWide;
            std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
            return FString(NewData);
        }
        std::string NewData = DataAnsi;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return FString(NewData);
    }
    FString ToUpper() const
    {
        if (bIsWide)
        {
            std::wstring NewData = DataWide;
            std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
            return FString(NewData);
        }
        std::string NewData = DataAnsi;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return FString(NewData);
    }
    FString TrimTrailing()
    {
        if (bIsWide)
        {
            auto last = DataWide.find_last_not_of(' ');
            return FString(DataWide.substr(0, last + 1));
        }
        auto last = DataAnsi.find_last_not_of(' ');
        return FString(DataAnsi.substr(0, last + 1));
    }
    bool FindLastChar(UTFCHAR InChar, int32& Index) const
    {
        assert(bIsWide);
        Index = static_cast<int32>(DataWide.find_last_of(InChar));
        return Index != std::wstring::npos;
    }
    bool FindLastChar(ANSICHAR InChar, int32& Index) const
    {
        assert(!bIsWide);
        Index = static_cast<int32>(DataAnsi.find_last_of(InChar));
        return Index != std::string::npos;
    }

    void PopBack()
    {
        if (bIsWide)
        {
            DataWide.pop_back();
        }
        else
        {
            DataAnsi.pop_back();
        }
    }

    uint32 LeftFind(const UTFCHAR* Chars, uint32 Pos)
    {
        assert(bIsWide);
        return static_cast<uint32>(DataWide.find(Chars, Pos));
    }
    uint32 LeftFind(const ANSICHAR*  Chars, uint32 Pos)
    {
        assert(!bIsWide);
        return static_cast<uint32>(DataAnsi.find(Chars, Pos));
    }
    uint32 RightFind(const UTFCHAR* Chars, uint32 Pos)
    {
        assert(bIsWide);
        return static_cast<uint32>(DataWide.rfind(Chars, Pos));
    }
    uint32 RightFind(const ANSICHAR* Chars, uint32 Pos)
    {
        assert(!bIsWide);
        return static_cast<uint32>(DataAnsi.rfind(Chars, Pos));
    }

    static FString Hexify(const uint8* Input, uint32 InputSize)
    {
        std::stringstream Buf;
        Buf.fill('0');
        for (int32 i = 0; i < InputSize; i++)
            Buf << std::hex << std::setfill('0') << std::setw(2) << (uint16)Input[i];
        return FString(Buf.str());
    }

#if PLATFORM_WINDOWS
    static FString GetLastErrorAsString()
    {
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if(errorMessageID == 0)
            return FString(L"No error.");

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

        std::string message(messageBuffer, size);
        FString NewString = FString(message);

        //Free the buffer.
        LocalFree(messageBuffer);

        return NewString;
    }
#endif
};

inline FString operator+ (const FString& Left, const FString& Right)
{
    assert(Left.IsWide() == Right.IsWide());

    FString NewString = Left;
    NewString += Right;
    return NewString;
}

namespace std
{
    template <> struct hash<FString>
    {
        size_t operator()(const FString & c) const
        {
            if (c.IsWide())
            {
                return std::hash<std::wstring>()(c.GetWideCharString());
            }
            return std::hash<std::string>()(c.GetAnsiCharString());
        }
    };
}

class FStringStream
{

private:
    bool bIsWide = false;

    std::stringstream AnsiStringStream;
    std::wstringstream UTFStringStream;

    FStringStream() = default;

public:
    explicit FStringStream(bool _bIsWide)
    {
        bIsWide = _bIsWide;
    }
    explicit FStringStream(const ANSICHAR* Parameter)
    {
        bIsWide = false;
        AnsiStringStream = std::stringstream(Parameter);
    }
    explicit FStringStream(const UTFCHAR* Parameter)
    {
        bIsWide = true;
        UTFStringStream = std::wstringstream(Parameter);
    }
    explicit FStringStream(const FString& Parameter)
    {
        bIsWide = Parameter.IsWide();
        if (bIsWide)
        {
            UTFStringStream << Parameter.GetWideCharArray();
        }
        else
        {
            AnsiStringStream << Parameter.GetAnsiCharArray();
        }
    }

    bool IsWide()
    {
        return bIsWide;
    }

    void operator<<(const FString& Other)
    {
        assert(bIsWide == Other.IsWide());

        if (bIsWide)
        {
            UTFStringStream << Other.GetWideCharArray();
        }
        else
        {
            AnsiStringStream << Other.GetAnsiCharArray();
        }
    }
    void operator<<(const ANSICHAR* Other)
    {
        assert(!bIsWide);
        AnsiStringStream << Other;
    }
    void operator<<(const UTFCHAR* Other)
    {
        assert(bIsWide);
        UTFStringStream << Other;
    }
    void operator<<(uint8 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(int8 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(uint16 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(int16 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(uint32 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(int32 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(uint64 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(int64 Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(float Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(double Numeric)
    {
        if (bIsWide) UTFStringStream << Numeric;
        else AnsiStringStream << Numeric;
    }
    void operator<<(ANSICHAR Character)
    {
        assert(!bIsWide);
        AnsiStringStream << Character;
    }
    void operator<<(UTFCHAR Character)
    {
        assert(bIsWide);
        UTFStringStream << Character;
    }
    void operator>> (bool& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (uint16& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (uint32& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (uint64& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (int16& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (int32& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (int64& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (float& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (double& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }
    void operator>> (void*& val)
    {
        if (bIsWide) UTFStringStream >> val;
        else AnsiStringStream >> val;
    }

    void Fill(ANSICHAR Character)
    {
        assert(!bIsWide);
        AnsiStringStream.fill(Character);
    }
    void Fill(UTFCHAR Character)
    {
        assert(bIsWide);
        UTFStringStream.fill(Character);
    }

    FString Str()
    {
        if (bIsWide)
        {
            return FString(UTFStringStream.str());
        }
        return FString(AnsiStringStream.str());
    }
};

#define EMPTY_FSTRING_UTF8 FString(L"")
#define EMPTY_FSTRING_ANSI FString("")

#endif //Pragma_Once_WString