// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WString
#define Pragma_Once_WString

#include "WEngine.h"
#include "WArray.h"
#include <string>
#include <cstdarg>
#include <regex>
#include <codecvt>

#if PLATFORM_WINDOWS
    #include "windows.h"
#endif

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

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
    std::wstring Data;

public:
    static std::wstring StringToWString(const std::string& t_str)
    {
        std::wstringstream WStringStream;

        uint32 Length = t_str.size();
        for (uint32 i = 0; i < Length; i++)
        {
            WStringStream << t_str.at(i);
        }
        return WStringStream.str();
    }
    static std::string WStringToString(const std::wstring& _str)
    {
        typedef std::codecvt_utf8<wchar_t> convert_type;
        std::wstring_convert<convert_type, wchar_t> converter;
        return converter.to_bytes(_str);
    }

    template <typename T>
    static T ConvertToInteger(const ANSICHAR *Data)
    {
        if (Data)
        {
            std::string AsString;
            std::stringstream StringBuilder;

            int32 Length = strlen(Data);
            for (int32 i = 0; i < Length; i++)
            {
                if (isdigit(Data[i]) || Data[i] == '-')
                {
                    StringBuilder << Data[i];
                }
            }
            AsString = StringBuilder.str();
            if (AsString.size() == 0)
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
        std::string NormalString = FString::WStringToString(Input.Data);
        return ConvertToInteger<T>(NormalString.c_str());
    }

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
        Data.resize(Data.length() + Count);
    }

    FString() = default;
    FString(const FString& Other)
    {
        Data = Other.Data;
    }
    explicit FString(const UTFCHAR* Other)
    {
        Data = Other;
    }
    explicit FString(const ANSICHAR* Other)
    {
        Data = StringToWString(Other);
    }
    explicit FString(const std::wstring& Other)
    {
        Data = Other;
    }
    explicit FString(const std::string& Other)
    {
        Data = StringToWString(Other);
    }
    FString(int32 InCount, const UTFCHAR* InSrc)
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
    FString(const UTFCHAR* Other, uint32 Size)
    {
        Data.resize(Size);
        for (int32 i = 0; i < Size; i++)
        {
            Data[i] = Other[i];
        }
    }
    FString(const ANSICHAR* Other, uint32 Size)
    {
        Data.resize(Size);
        for (int32 i = 0; i < Size; i++)
        {
            Data[i] = Other[i];
        }
    }
    FString& operator=(const FString& Other) = default;

    FString& operator=(const UTFCHAR* Other)
    {
        Data = Other;
        return *this;
    }
    FString& operator=(const ANSICHAR* Other)
    {
        Data = StringToWString(Other);
        return *this;
    }
    FString& operator=(const std::wstring& Other)
    {
        Data = Other;
        return *this;
    }
    FString& operator=(const std::string& Other)
    {
        Data = StringToWString(Other);
        return *this;
    }
    bool operator==(const FString& Other)
    {
        return 0 == Data.compare(Other.Data);
    }
    bool operator!=(const FString& Other)
    {
        return 0 == Data.compare(Other.Data);
    }

    const UTFCHAR* operator*() const
    {
        return Data.data();
    }
    FString& operator/= (const FString& Str)
    {
        Data += L"/";
        Data += Str.Data;
        return *this;
    }
    FString& operator/= (const UTFCHAR* Str)
    {
        Data += L"/";
        Data += Str;
        return *this;
    }
    UTFCHAR& operator[] (uint32 Index)
    {
        return Data.at(Index);
    }
    const UTFCHAR& operator[] (uint32 Index) const
    {
        return Data.at(Index);
    }
    FString& operator+= (const FString& Str)
    {
        Data += Str.Data;
        return *this;
    }
    FString& operator+= (const UTFCHAR* Str)
    {
        Data += Str;
        return *this;
    }
    FString& operator+= (UTFCHAR Character)
    {
        Data += Character;
        return *this;
    }

    FString& Append(const UTFCHAR* Text, int32 Count)
    {
        Data.append(Text);
        return *this;
    }
    FString& Append(const FString& Text)
    {
        Data.append(Text.Data);
        return *this;
    }
    FString& AppendChar(const UTFCHAR InChar)
    {
        Data += InChar;
        return *this;
    }
    void AppendChars(const UTFCHAR* Array, uint32 Count)
    {
        if (Count <= 0) return;
        int32 OldSize = Data.size();
        Data.reserve(OldSize + Count);
        for (int32 i = 0; i < Count; i++)
        {
            Data[OldSize + i] = Array[i];
        }
    }
    static FString FromInt( int8 Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    static FString FromInt( int16 Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    static FString FromInt( int32 Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    static FString FromInt( int64 Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    static FString FromInt( uint8 Num )
    {
        std::wstringstream Stream;
        Stream << (int16)Num;
        return FString(Stream.str());
    }
    static FString FromInt( uint16 Num )
    {
        std::wstringstream Stream;
        Stream << (int32)Num;
        return FString(Stream.str());
    }
    static FString FromInt( uint32 Num )
    {
        std::wstringstream Stream;
        Stream << (int64)Num;
        return FString(Stream.str());
    }
    static FString FromFloat( float Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    static FString FromFloat( double Num )
    {
        std::wstringstream Stream;
        Stream << Num;
        return FString(Stream.str());
    }
    bool Contains(const UTFCHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
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
    void Empty(uint32 Slack = 0)
    {
        Data.empty();
        if (Slack > 0)
        {
            Data.resize(Slack);
        }
    }
    bool EndsWith(const UTFCHAR* InSuffix, uint32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= Data.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = Data;
                std::wstring LookFor = InSuffix;

                std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
                std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

                for (uint32 i = Len - 1; i >= 0; i--)
                {
                    if (LookFor.at(i) != Tmp.at(i))
                    {
                        return false;
                    }
                }
            }
            else
            {
                for (uint32 i = Len - 1; i >= 0; i--)
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
    bool StartsWith(const UTFCHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= Data.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = Data;
                std::wstring LookFor = InPrefix;

                std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
                std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

                for (uint32 i = 0; i < Len; i++)
                {
                    if (LookFor.at(i) != Tmp.at(i))
                    {
                        return false;
                    }
                }
            }
            else
            {
                for (uint32 i = 0; i < Len; i++)
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
    const UTFCHAR* GetCharArray() const
    {
        return Data.data();
    }
    const std::string GetAnsiCharArray() const
    {
        return WStringToString(Data);
    }
    std::wstring GetRawData() const
    {
        return Data;
    }
    void InsertAt(uint32 Index, UTFCHAR Character)
    {
        Data.insert(Index, 1, Character);
    }
    void InsertAt(uint32 Index, const FString& Characters)
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
    bool IsValidIndex(uint32 Index)
    {
        return Index >= 0 && Index < Data.length();
    }
    FString Left(uint32 Count)
    {
        return FString(Data.substr(0, Count));
    }
    FString Right(uint32 FromIx)
    {
        return FString(Data.substr(FromIx, Data.length() - FromIx));
    }
    FString Mid(uint32 Start, uint32 Count) const
    {
        return FString(Data.substr(Start, Count));
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, const UTFCHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        // Make sure the delimit string is not null or empty
        OutArray.Empty();
        const UTFCHAR *Start = Data.data();
        const int32 Length = Data.length();
        if (Start)
        {
            int32 SubstringBeginIndex = 0;

            // Iterate through string.
            for(int32 i = 0; i < Data.length();)
            {
                int32 SubstringEndIndex = INDEX_NONE;
                uint32 DelimiterLength = 0;

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
    int32 ParseIntoArray(TArray<FString>& OutArray, const UTFCHAR* pchDelim, bool InCullEmpty)
    {
        OutArray.Reset();
        const UTFCHAR* Start = Data.data();
        const int32 DelimLength = wcslen(pchDelim);
        if (Start && DelimLength)
        {
            while(const UTFCHAR *At = wcsstr(Start,pchDelim) )
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
    int32 ParseIntoArrayWS(TArray<FString>& OutArray, const UTFCHAR* pchExtraDelim, bool InCullEmpty) const
    {
        static const UTFCHAR* WhiteSpace[] =
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
        static const UTFCHAR* LineEndings[] =
        {
            L"\r\n",
            L"\r",
            L"\n"
        };

        // start with just the standard line endings
        int32 NumLineEndings = 3;
        return ParseIntoArray(OutArray, LineEndings, NumLineEndings, InCullEmpty);
    }
    void RemoveAt(uint32 Index, uint32 Count)
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

            uint32 Pos = Tmp.rfind(LookFor);
            if (Pos != std::wstring::npos)
            {
                Data.replace(Pos, LookFor.length(), L"");
                return true;
            }
        }
        else
        {
            uint32 Pos = Data.rfind(InSuffix.Data);
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

            uint32 Pos = Tmp.find(LookFor);
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
    FString Replace(const UTFCHAR* From, const UTFCHAR* To, ESearchCase::Type SearchCase)
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
        return FString(NewData);
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
        return FString(NewData);
    }
    FString ToUpper()
    {
        std::wstring NewData = Data;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return FString(NewData);
    }
    FString TrimTrailing()
    {
        auto last = Data.find_last_not_of(' ');
        return FString(Data.substr(0, last + 1));
    }
    bool FindLastChar(UTFCHAR InChar, int32& Index) const
    {
        Index = Data.find_last_of(InChar);
        return Index != std::wstring::npos;
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

    typedef UTFCHAR* iterator;
    typedef const UTFCHAR* const_iterator;
    iterator begin() { return &Data[0]; }
    const_iterator begin() const { return &Data[0]; }
    iterator end() { return &Data[Data.length()]; }
    const_iterator end() const { return &Data[Data.size()]; }
};

inline FString operator+ (const FString& Left, const FString& Right)
{
    FString NewString = Left;
    NewString += Right;
    return NewString;
}

#define EMPTY_FSTRING FString(L"")

#endif //Pragma_Once_WString