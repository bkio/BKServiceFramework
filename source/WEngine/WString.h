// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WString
#define Pragma_Once_WString

#include "WEngine.h"
#include "WArray.h"
#include <string>

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
        FromEnd,
    };
}

class FString
{

private:
    std::wstring Data;

public:
    int32 Len() const
    {
        return Data.length();
    }

    int32 AddUninitialized(int32 Count = 1)
    {
        const int32 OldNum = Data.length();
        Data.resize(Data.length() + Count);
        return OldNum;
    }
    void InsertUninitialized(int32 Index, int32 Count = 1)
    {
        const int32 OldNum = Data.length();
        Data.resize(Data.length() + Count);
    }

    FString() {}
    FString(const FString& Other)
    {
        Data = Other.Data;
    }
    FString(FString&& Other)
    {
        Data = Other.Data;
    }
    FString(const TCHAR* Other)
    {
        Data = Other;
    }
    FString(const ANSICHAR* Other)
    {
        TCHAR Tmp[1024];
        int32 Length = swprintf(Tmp, L"%s", Other);
        Data = std::wstring(Tmp, Length);
    }
    FString(const std::wstring& Other)
    {
        Data = Other;
    }
    FString(std::wstring&& Other)
    {
        Data = Other;
    }
    FString(int32 InCount, const TCHAR* InSrc)
    {
        AddUninitialized(InCount ? InCount + 1 : 0);

        if(Data.length() > 0)
        {
            for (int32 i = InCount; i < InCount + 1; i++)
            {
                Data[i] = InSrc[i - InCount];
            }
        }
    }
    FString(const TCHAR* Other, int32 Size)
    {
        Data.resize(Size);
        for (int32 i = 0; i < Size; i++)
        {
            Data[i] = Other[i];
        }
    }
    FString& operator=(const FString& Other)
    {
        Data = Other.Data;
        return *this;
    }
    FString& operator=(const TCHAR* Other)
    {
        Data = Other;
        return *this;
    }
    FString& operator=(const ANSICHAR* Other)
    {
        TCHAR Tmp[1024];
        int32 Length = swprintf(Tmp, L"%s", Other);
        Data = std::wstring(Tmp, Length);
        return *this;
    }
    FString& operator=(const std::wstring& Other)
    {
        Data = Other;
        return *this;
    }
    bool operator==(const FString& Other)
    {
        return Data == Other.Data;
    }
    bool operator!=(const FString& Other)
    {
        return Data != Other.Data;
    }

    const TCHAR* operator*() const
    {
        return Data.data();
    }
    FString& operator/= (const FString& Str)
    {
        Data += L"/";
        Data += Str.Data;
        return *this;
    }
    FString& operator/= (FString&& Str)
    {
        Data += L"/";
        Data += Str.Data;
        return *this;
    }
    FString& operator/= (const TCHAR* Str)
    {
        Data += L"/";
        Data += Str;
        return *this;
    }
    TCHAR& operator[] (int32 Index)
    {
        return Data.at(Index);
    }
    const TCHAR& operator[] (int32 Index) const
    {
        return Data.at(Index);
    }
    FString& operator+= (const FString& Str)
    {
        Data += Str.Data;
        return *this;
    }
    FString& operator+= (const TCHAR* Str)
    {
        Data += Str;
        return *this;
    }
    FString& operator+= (TCHAR Character)
    {
        Data += Character;
        return *this;
    }
    FString operator+ (FString&& Str)
    {
        FString NewString = Data;
        NewString += Str;
        return NewString;
    }
    FString operator+ (const TCHAR* Str)
    {
        FString NewString = Data;
        NewString += Str;
        return NewString;
    }
    FString operator+ (TCHAR Character)
    {
        FString NewString = Data;
        NewString += Character;
        return NewString;
    }

    FString& Append(const TCHAR* Text, int32 Count)
    {
        Data.append(Text);
        return *this;
    }
    FString& Append(const FString& Text)
    {
        Data.append(Text.Data);
        return *this;
    }
    FString& Append(FString&& Text)
    {
        Data.append(Text.Data);
        return *this;
    }
    FString& AppendChar(const TCHAR InChar)
    {
        Data += InChar;
        return *this;
    }
    void AppendChars(const TCHAR* Array, int32 Count)
    {
        if (Count <= 0) return;
        int32 OldSize = Data.size();
        Data.reserve(OldSize + Count);
        for (int32 i = 0; i < Count; i++)
        {
            Data[OldSize + i] = Array[i];
        }
    }
    void AppendInt( int32 InNum )
    {
        int64 Num						= InNum; // This avoids having to deal with negating -MAX_int32-1
        const TCHAR* NumberChar[11]		= { L"0", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"-" };
        bool bIsNumberNegative			= false;
        TCHAR TempNum[16];				// 16 is big enough
        int32 TempAt					= 16; // fill the temp string from the top down.

        // Correctly handle negative numbers and convert to positive integer.
        if( Num < 0 )
        {
            bIsNumberNegative = true;
            Num = -Num;
        }

        TempNum[--TempAt] = 0; // NULL terminator

        // Convert to string assuming base ten and a positive integer.
        do
        {
            TempNum[--TempAt] = *NumberChar[Num % 10];
            Num /= 10;
        }
        while( Num );

        // Append sign as we're going to reverse string afterwards.
        if( bIsNumberNegative )
        {
            TempNum[--TempAt] = *NumberChar[10];
        }

        Data += TempNum + TempAt;
    }
    static FString FromInt( int32 Num )
    {
        FString Ret;
        Ret.AppendInt(Num);
        return Ret;
    }
    bool Contains(const TCHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = Data;
            std::wstring LookFor = SubStr;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            return SearchDir == ESearchDir::FromStart ?
                   Tmp.find(LookFor) != std::wstring::npos :
                    Tmp.rfind(LookFor) != std::wstring::npos;
        }
        return SearchDir == ESearchDir::FromStart ?
               Data.find(SubStr) != std::wstring::npos :
               Data.rfind(SubStr) != std::wstring::npos;
    }
    bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        return Contains(SubStr.Data.data(), SubStr.Data.length(), SearchCase, SearchDir);
    }
    void Empty(int32 Slack = 0)
    {
        Data.empty();
        if (Slack > 0)
        {
            Data.resize(Slack);
        }
    }
    bool EndsWith(const TCHAR* InSuffix, int32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= Data.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = Data;
                std::wstring LookFor = InSuffix;

                std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
                std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

                for (int32 i = Len - 1; i >= 0; i--)
                {
                    if (LookFor.at(i) != Tmp.at(i))
                    {
                        return false;
                    }
                }
            }
            else
            {
                for (int32 i = Len - 1; i >= 0; i--)
                {
                    if (InSuffix[i] != Data.at(i))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool EndsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        return EndsWith(InSuffix.Data.data(), InSuffix.Data.length(), SearchCase);
    }
    bool StartsWith(const TCHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= Data.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = Data;
                std::wstring LookFor = InPrefix;

                std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
                std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

                for (int32 i = 0; i < Len; i++)
                {
                    if (LookFor.at(i) != Tmp.at(i))
                    {
                        return false;
                    }
                }
            }
            else
            {
                for (int32 i = 0; i < Len; i++)
                {
                    if (InPrefix[i] != Data.at(i))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool StartsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        return StartsWith(InSuffix.Data.data(), InSuffix.Data.length(), SearchCase);
    }
    const TCHAR* GetCharArray() const
    {
        return Data.data();
    }
    void InsertAt(int32 Index, TCHAR Character)
    {
        Data.insert(Index, 1, Character);
    }
    void InsertAt(int32 Index, const FString& Characters)
    {
        Data.insert(Index, Characters.Data);
    }
    bool IsEmpty()
    {
        return Data.length() == 0;
    }
    bool IsNumeric()
    {
        int32 DotCount = 0;
        std::wstring::const_iterator it = Data.begin();
        while (it != Data.end() && (std::isdigit(*it) || (it == Data.begin() && *it == '-') || *it == '.'))
        {
            if (*it == '.')
            {
                DotCount++;
            }
            ++it;
        }
        return !Data.empty() && it == Data.end() && DotCount <= 1;
    }
    bool IsValidIndex(int32 Index)
    {
        return Index >= 0 && Index < Data.length();
    }
    FString Left(int32 Count)
    {
        return Data.substr(0, Count);
    }
    FString Right(int32 FromIx)
    {
        return Data.substr(FromIx, Data.length() - FromIx);
    }
    FString Mid(int32 Start, int32 Count) const
    {
        return Data.substr(Start, Count);
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, const TCHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        // Make sure the delimit string is not null or empty
        OutArray.Empty();
        const TCHAR *Start = Data.data();
        const int32 Length = Data.length();
        if (Start)
        {
            int32 SubstringBeginIndex = 0;

            // Iterate through string.
            for(int32 i = 0; i < Data.length();)
            {
                int32 SubstringEndIndex = INDEX_NONE;
                int32 DelimiterLength = 0;

                // Attempt each delimiter.
                for(int32 DelimIndex = 0; DelimIndex < NumDelims; ++DelimIndex)
                {
                    DelimiterLength = wcslen(DelimArray[DelimIndex]);

                    // If we found a delimiter...
                    if (wcsncmp(Start + i, DelimArray[DelimIndex], DelimiterLength) == 0)
                    {
                        // Mark the end of the substring.
                        SubstringEndIndex = i;
                        break;
                    }
                }

                if (SubstringEndIndex != INDEX_NONE)
                {
                    const int32 SubstringLength = SubstringEndIndex - SubstringBeginIndex;
                    // If we're not culling empty strings or if we are but the string isn't empty anyways...
                    if(!InCullEmpty || SubstringLength != 0)
                    {
                        // ... add new string from substring beginning up to the beginning of this delimiter.
                        OutArray.Add(FString(SubstringEndIndex - SubstringBeginIndex, Start + SubstringBeginIndex));
                    }
                    // Next substring begins at the end of the discovered delimiter.
                    SubstringBeginIndex = SubstringEndIndex + DelimiterLength;
                    i = SubstringBeginIndex;
                }
                else
                {
                    ++i;
                }
            }

            // Add any remaining characters after the last delimiter.
            const int32 SubstringLength = Length - SubstringBeginIndex;
            // If we're not culling empty strings or if we are but the string isn't empty anyways...
            if(!InCullEmpty || SubstringLength != 0)
            {
                // ... add new string from substring beginning up to the beginning of this delimiter.
                OutArray.Add(FString(Start + SubstringBeginIndex));
            }
        }

        return OutArray.Num();
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, const TCHAR* pchDelim, bool InCullEmpty)
    {
        OutArray.Reset();
        const TCHAR* Start = Data.data();
        const int32 DelimLength = wcslen(pchDelim);
        if (Start && DelimLength)
        {
            while(const TCHAR *At = wcsstr(Start,pchDelim) )
            {
                if (!InCullEmpty || At-Start)
                {
                    OutArray.Emplace(At-Start,Start);
                }
                Start = At + DelimLength;
            }
            if (!InCullEmpty || *Start)
            {
                OutArray.Emplace(Start);
            }
        }
        return OutArray.Num();
    }
    int32 ParseIntoArrayWS(TArray<FString>& OutArray, const TCHAR* pchExtraDelim, bool InCullEmpty) const
    {
        static const TCHAR* WhiteSpace[] =
        {
            L" ",
            L"\t",
            L"\r",
            L"\n",
            L""
        };

        // start with just the standard whitespaces
        int32 NumWhiteSpaces = 4;
        // if we got one passed in, use that in addition
        if (pchExtraDelim && *pchExtraDelim)
        {
            WhiteSpace[NumWhiteSpaces++] = pchExtraDelim;
        }
        return ParseIntoArray(OutArray, WhiteSpace, NumWhiteSpaces, InCullEmpty);
    }

    int32 ParseIntoArrayLines(TArray<FString>& OutArray, bool InCullEmpty) const
    {
        // default array of LineEndings
        static const TCHAR* LineEndings[] =
        {
            L"\r\n",
            L"\r",
            L"\n"
        };

        // start with just the standard line endings
        int32 NumLineEndings = 3;
        return ParseIntoArray(OutArray, LineEndings, NumLineEndings, InCullEmpty);
    }
    static FString Printf(const TCHAR* Fmt, ...)
    {
        TCHAR Destination[1024];

        va_list args;
        va_start(args, Fmt);
        int32 Len = swprintf(Destination, Fmt, args);
        va_end(args);

        return FString(Destination, Len);
    }
    void RemoveAt(int32 Index, int32 Count)
    {
        Data.erase(Index, Count);
    }
    bool RemoveFromEnd(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = Data;
            std::wstring LookFor = InSuffix.Data;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            int32 Pos = Tmp.rfind(LookFor);
            if (Pos != std::wstring::npos)
            {
                Data.replace(Pos, LookFor.length(), L"");
                return true;
            }
        }
        else
        {
            int32 Pos = Data.rfind(InSuffix.Data);
            if (Pos != std::wstring::npos)
            {
                Data.replace(Pos, InSuffix.Data.length(), L"");
                return true;
            }
        }
        return false;
    }
    bool RemoveFromStart(const FString& InPrefix, ESearchCase::Type SearchCase)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = Data;
            std::wstring LookFor = InPrefix.Data;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            int32 Pos = Tmp.find(LookFor);
            if (Pos != std::wstring::npos)
            {
                Data.replace(Pos, LookFor.length(), L"");
                return true;
            }
        }
        else
        {
            auto Pos = Data.find(InPrefix.Data);
            if (Pos != std::wstring::npos)
            {
                Data.replace(Pos, InPrefix.Data.length(), L"");
                return true;
            }
        }
        return false;
    }
    FString Replace(const TCHAR* From, const TCHAR* To, ESearchCase::Type SearchCase)
    {
        std::wstring NewData = Data;

        std::wstring FromString = From;
        std::wstring ToString = To;

        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp;
            std::transform(NewData.begin(), NewData.end(), Tmp.begin(), ::tolower);
            std::transform(FromString.begin(), FromString.end(), FromString.begin(), ::tolower);

            auto Pos = Tmp.find(From);
            while (Pos != std::wstring::npos)
            {
                NewData.replace(Pos, FromString.size(), ToString);
                Pos = Tmp.find(FromString);
            }
        }
        else
        {
            size_t Pos = NewData.find(FromString);
            if(Pos != std::string::npos)
            {
                NewData.replace(Pos, FromString.length(), ToString);
            }
        }
        return NewData;
    }
    void Reserve(uint32 CharacterCount)
    {
        Data.reserve(CharacterCount);
    }
    void Reset()
    {
        Data.empty();
    }
    void ReverseString()
    {
        std::reverse(Data.begin(), Data.end());
    }
    FString ToLower()
    {
        std::wstring NewData = Data;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return NewData;
    }
    FString ToUpper()
    {
        std::wstring NewData = Data;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return NewData;
    }
    FString Trim()
    {
        auto first = Data.find_first_not_of(' ');
        if (std::wstring::npos == first)
        {
            return Data;
        }

        auto last = Data.find_last_not_of(' ');
        return Data.substr(first);
    }
    FString TrimTrailing()
    {
        auto last = Data.find_last_not_of(' ');
        return Data.substr(0, last + 1);
    }
    static FString SanitizeFloat(double InFloat)
    {
        // Avoids negative zero
        if( InFloat == 0 )
        {
            InFloat = 0;
        }

        // First create the string
        FString TempString = FString::Printf(L"%f", InFloat);

        const TCHAR Zero = '0';
        const TCHAR Period = '.';

        int32 TrimIndex = 0;

        // Find the first non-zero char in the array
        for (int32 Index = TempString.Len() - 2; Index >= 2; --Index )
        {
            const TCHAR EachChar = TempString[Index];
            const TCHAR NextChar = TempString[Index-1];
            if ((EachChar != Zero) || (NextChar == Period))
            {
                TrimIndex = Index;
                break;
            }
        }
        // If we changed something trim the string
        if( TrimIndex != 0 )
        {
            TempString = TempString.Left( TrimIndex + 1 );
        }
        return TempString;
    }
    bool FindLastChar(TCHAR InChar, int32& Index) const
    {
        Index = Data.find_last_of(InChar);
        return Index != std::wstring::npos;
    }

    typedef TCHAR* iterator;
    typedef const TCHAR* const_iterator;
    iterator begin() { return &Data[0]; }
    const_iterator begin() const { return &Data[0]; }
    iterator end() { return &Data[Data.length()]; }
    const_iterator end() const { return &Data[Data.size()]; }
};

#endif