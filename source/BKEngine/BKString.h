// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKString
#define Pragma_Once_BKString

#include "BKEngine.h"
#include "BKArray.h"
#include <string>
#include <cstdarg>
#include <regex>
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
    std::wstring DataWide;

    static std::wstring StringToWString(const std::string& t_str, int32 Length)
    {
        if (Length == -1) Length = t_str.size();

        std::wstringstream Converter;
        Converter << (Length == t_str.size() ? t_str : t_str.substr(0, Length)).c_str();
        return Converter.str();
    }
    static std::string WStringToString(const std::wstring& _str, int32 Length)
    {
        if (Length <= 0) Length = _str.size();

        return std::string(_str.begin(), _str.begin() + Length);
    }

    //Called from all different constructors
    void Initialize()
    {
    }

public:
    UTFCHAR AtWide(uint32 Ix) const
    {
        assert(Ix >= 0 && Ix < DataWide.length());
        return DataWide.at(Ix);
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
        std::string NormalString = WStringToString(Input.DataWide, Input.Len());
        return ConvertToInteger<T>(NormalString.c_str());
    }

    #define FromNumeric_Define( CastTo ) \
        std::wstringstream Stream; \
        Stream << (CastTo)Num; \
        return FString(Stream.str());

    static FString FromInt(int8 Num)
    {
        FromNumeric_Define(int8);
    }
    static FString FromInt(int16 Num)
    {
        FromNumeric_Define(int16);
    }
    static FString FromInt(int32 Num)
    {
        FromNumeric_Define(int32);
    }
    static FString FromInt(int64 Num)
    {
        FromNumeric_Define(int64);
    }
    static FString FromInt(uint8 Num)
    {
        FromNumeric_Define(int16);
    }
    static FString FromInt(uint16 Num)
    {
        FromNumeric_Define(int32);
    }
    static FString FromInt(uint32 Num)
    {
        FromNumeric_Define(int64);
    }
    static FString FromFloat(float Num)
    {
        FromNumeric_Define(float);
    }
    static FString FromFloat(double Num)
    {
        FromNumeric_Define(double);
    }

    int32 Len() const
    {
        return static_cast<int32>(DataWide.length());
    }

    int32 AddUninitialized(int32 Count = 1)
    {
        int32 OldNum = static_cast<int32>(DataWide.length());
        DataWide.resize(DataWide.length() + Count);
        return OldNum;
    }
    void InsertUninitialized(int32 Index, int32 Count = 1)
    {
        DataWide.resize(DataWide.length() + Count);
    }

    FString()
    {
        Initialize();
    }
    FString(const FString& Other)
    {
        Initialize();
        DataWide = Other.DataWide;
    }
    explicit FString(const UTFCHAR* Other)
    {
        Initialize();
        DataWide = Other;
    }
    explicit FString(const ANSICHAR* Other)
    {
        Initialize();
        DataWide = StringToWString(Other, -1);
    }
    explicit FString(const std::wstring& Other)
    {
        Initialize();
        DataWide = Other;
    }
    explicit FString(const std::string& Other)
    {
        Initialize();
        DataWide = StringToWString(Other, -1);
    }
    FString(int32 InCount, const UTFCHAR* InSrc)
    {
        Initialize();
        DataWide = std::wstring(InSrc, InCount);
    }
    FString(int32 InCount, const ANSICHAR* InSrc)
    {
        Initialize();
        std::string Narrow = std::string(InSrc, InCount);
        DataWide = StringToWString(Narrow, -1);
    }
    FString(uint32 InCount, const UTFCHAR InSrc)
    {
        Initialize();
        DataWide = std::wstring(InCount, InSrc);
    }
    FString(uint32 InCount, const ANSICHAR InSrc)
    {
        Initialize();
        DataWide = std::wstring(InCount, (UTFCHAR)InSrc);
    }
    FString(const UTFCHAR* Other, uint32 Size)
    {
        Initialize();
        DataWide = std::wstring(Other, Size);
    }
    FString(const ANSICHAR* Other, uint32 Size)
    {
        Initialize();
        std::string Narrow = std::string(Other, Size);
        DataWide = StringToWString(Narrow, -1);
    }
    FString& operator=(const FString& Other)
    {
        Initialize();
        DataWide = Other.DataWide;
        return *this;
    };

    FString& operator=(const UTFCHAR* Other)
    {
        Initialize();
        DataWide = Other;
        return *this;
    }
    FString& operator=(const ANSICHAR* Other)
    {
        Initialize();
        DataWide = StringToWString(Other, -1);
        return *this;
    }
    FString& operator=(const std::wstring& Other)
    {
        Initialize();
        DataWide = Other;
        return *this;
    }
    FString& operator=(const std::string& Other)
    {
        Initialize();
        DataWide = StringToWString(Other, -1);
        return *this;
    }

    bool operator<(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) > 0;
    }
    bool operator>(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) < 0;
    }
    bool operator<=(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) >= 0;
    }
    bool operator>=(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) <= 0;
    }
    bool operator==(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) == 0;
    }
    bool operator!=(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) != 0;
    }
    bool Equals(const FString& Other) const
    {
        return DataWide.compare(Other.DataWide) != 0;
    }

    const UTFCHAR* operator*() const
    {
        return DataWide.data();
    }
    FString& operator/= (const FString& Str)
    {
        DataWide += L"/";
        DataWide += Str.DataWide;
        return *this;
    }
    FString& operator/= (const UTFCHAR* Str)
    {
        DataWide += L"/";
        DataWide += Str;
        return *this;
    }
    FString& operator/= (const ANSICHAR* Str)
    {
        DataWide += L"/";
        DataWide += StringToWString(Str, -1);
        return *this;
    }

    FString& operator+= (const FString& Str)
    {
        DataWide += Str.DataWide;
        return *this;
    }
    FString& operator+= (const UTFCHAR* Str)
    {
        DataWide += Str;
        return *this;
    }
    FString& operator+= (UTFCHAR Character)
    {
        DataWide += Character;
        return *this;
    }
    FString& operator+= (const ANSICHAR* Str)
    {
        DataWide += StringToWString(Str, -1);
        return *this;
    }
    FString& operator+= (ANSICHAR Character)
    {
        DataWide += (UTFCHAR)Character;
        return *this;
    }

    const UTFCHAR* GetWideCharArray() const
    {
        return DataWide.c_str();
    }
    const ANSICHAR* GetAnsiCharArray() const
    {
        std::string AsAnsiString = WStringToString(DataWide, -1);
        return AsAnsiString.c_str();
    }
    std::wstring GetWideCharString() const
    {
        return DataWide;
    }
    const std::string GetAnsiCharString() const
    {
        return WStringToString(DataWide, -1);
    }

    FString& Append(const UTFCHAR* Text, int32 Count)
    {
        DataWide = DataWide.append(Text, Count);
        return *this;
    }
    FString& Append(const ANSICHAR* Text, int32 Count)
    {
        std::wstring AsUTFString = StringToWString(Text, Count);
        DataWide = DataWide.append(AsUTFString.c_str(), Count);
        return *this;
    }
    FString& Append(const FString& Text)
    {
        DataWide = DataWide.append(Text.DataWide);
        return *this;
    }
    FString& AppendChar(const UTFCHAR InChar)
    {
        DataWide += InChar;
        return *this;
    }
    FString& AppendChar(const ANSICHAR InChar)
    {
        DataWide += (UTFCHAR)InChar;
        return *this;
    }
    void AppendChars(const UTFCHAR* Array, uint32 Count)
    {
        DataWide = DataWide.append(Array, Count);
    }
    void AppendChars(const ANSICHAR* Array, uint32 Count)
    {
        std::wstring AsUTFString = StringToWString(Array, Count);
        DataWide = DataWide.append(AsUTFString.c_str(), Count);
    }

    bool Contains(const UTFCHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = DataWide;
            std::wstring LookFor = SubStr;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            return SearchDir == ESearchDir::FromStart ?
                   Tmp.find(LookFor) != std::wstring::npos :
                   Tmp.rfind(LookFor) != std::wstring::npos;
        }
        return SearchDir == ESearchDir::FromStart ?
               DataWide.find(SubStr) != std::wstring::npos :
               DataWide.rfind(SubStr) != std::wstring::npos;
    }
    bool Contains(const ANSICHAR* SubStr, int32 Size, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        std::wstring AsUTFString = StringToWString(SubStr, Size);
        return Contains(AsUTFString.c_str(), Size, SearchCase, SearchDir);
    }
    bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart)
    {
        return Contains(SubStr.DataWide.data(), static_cast<int32>(SubStr.DataWide.length()), SearchCase, SearchDir);
    }
    void Empty(uint32 Slack = 0)
    {
        DataWide.clear();
        if (Slack > 0)
        {
            DataWide.resize(Slack);
        }
    }

    bool EndsWith(const UTFCHAR* InSuffix, uint32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= DataWide.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = DataWide;
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
                    if (InSuffix[i] != DataWide.at(i))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool EndsWith(const ANSICHAR* InSuffix, uint32 Len, ESearchCase::Type SearchCase)
    {
        std::wstring AsUTFString = StringToWString(InSuffix, Len);
        return EndsWith(AsUTFString.c_str(), Len, SearchCase);
    }
    bool EndsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        return EndsWith(InSuffix.DataWide.data(), static_cast<uint32>(InSuffix.DataWide.length()), SearchCase);
    }

    bool StartsWith(const UTFCHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        if (Len >= DataWide.length())
        {
            if (SearchCase == ESearchCase::IgnoreCase)
            {
                std::wstring Tmp = DataWide;
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
                    if (InPrefix[i] != DataWide.at(i))
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool StartsWith(const ANSICHAR* InPrefix, int32 Len, ESearchCase::Type SearchCase)
    {
        std::wstring AsUTFString = StringToWString(InPrefix, Len);
        return StartsWith(AsUTFString.c_str(), Len, SearchCase);
    }
    bool StartsWith(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        return StartsWith(InSuffix.DataWide.data(), static_cast<int32>(InSuffix.DataWide.length()), SearchCase);
    }

    void InsertAt(uint32 Index, UTFCHAR Character)
    {
        DataWide = DataWide.insert(Index, 1, Character);
    }
    void InsertAt(uint32 Index, ANSICHAR Character)
    {
        DataWide = DataWide.insert(Index, 1, (UTFCHAR)Character);
    }
    void InsertAt(uint32 Index, const FString& Characters)
    {
        DataWide = DataWide.insert(Index, Characters.DataWide);
    }
    bool IsEmpty()
    {
        return DataWide.length() == 0;
    }

    bool IsNumeric()
    {
        int32 DotCount = 0;
        std::wstring::const_iterator it = DataWide.begin();
        while (it != DataWide.end() && (std::isdigit(*it) || (it == DataWide.begin() && *it == '-') || *it == '.'))
        {
            if (*it == '.')
            {
                DotCount++;
            }
            ++it;
        }
        return !DataWide.empty() && it == DataWide.end() && DotCount <= 1;
    }
    bool IsValidIndex(uint32 Index)
    {
        return Index >= 0 && Index < DataWide.length();
    }
    FString Left(uint32 Count)
    {
        return FString(DataWide.substr(0, Count));
    }
    FString Right(uint32 FromIx)
    {
        return FString(DataWide.substr(FromIx, DataWide.length() - FromIx));
    }
    FString Mid(uint32 Start, uint32 Count) const
    {
        return FString(DataWide.substr(Start, Count));
    }

    int32 ParseIntoArray(TArray<FString>& OutArray, UTFCHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        OutArray.Empty();
        const UTFCHAR *Start = DataWide.data();
        const int32 Length = DataWide.length();
        if (Start)
        {
            int32 SubstringBeginIndex = 0;

            for(int32 i = 0; i < DataWide.length();)
            {
                int32 SubstringEndIndex = INDEX_NONE;
                uint32 DelimiterLength = 0;

                for (int32 DelimIndex = 0; DelimIndex < NumDelims; ++DelimIndex)
                {
                    DelimiterLength = wcslen(DelimArray[DelimIndex]);

                    if (wcsncmp(Start + i, DelimArray[DelimIndex], DelimiterLength) == 0)
                    {
                        SubstringEndIndex = i;
                        break;
                    }
                }

                if (SubstringEndIndex != INDEX_NONE)
                {
                    const int32 SubstringLength = SubstringEndIndex - SubstringBeginIndex;
                    if(!InCullEmpty || SubstringLength != 0)
                    {
                        OutArray.Add(FString(SubstringEndIndex - SubstringBeginIndex, Start + SubstringBeginIndex));
                    }
                    SubstringBeginIndex = SubstringEndIndex + DelimiterLength;
                    i = SubstringBeginIndex;
                }
                else
                {
                    ++i;
                }
            }

            const int32 SubstringLength = Length - SubstringBeginIndex;
            if(!InCullEmpty || SubstringLength != 0)
            {
                OutArray.Add(FString(Start + SubstringBeginIndex));
            }
        }
        return OutArray.Num();
    }
    int32 ParseIntoArray(TArray<FString>& OutArray, ANSICHAR** DelimArray, int32 NumDelims, bool InCullEmpty) const
    {
        UTFCHAR** WideDelimArray = new UTFCHAR*[NumDelims];
        for (int i = 0; i < NumDelims; i++)
        {
            WideDelimArray[i] = new UTFCHAR[strlen(DelimArray[i])];
        }

        int32 Result = ParseIntoArray(OutArray, WideDelimArray, NumDelims, InCullEmpty);

        for (int i = 0; i < NumDelims; i++)
        {
            delete[] WideDelimArray[i];
        }
        delete[] WideDelimArray;

        return Result;
    }

    int32 ParseIntoArray(TArray<FString>& OutArray, const UTFCHAR* pchDelim, bool InCullEmpty)
    {
        OutArray.Reset();
        const UTFCHAR* Start = DataWide.data();
        const int32 DelimLength = wcslen(pchDelim);
        if (Start && DelimLength)
        {
            while (const UTFCHAR *At = wcsstr(Start, pchDelim))
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

    void RemoveAt(uint32 Index, uint32 Count)
    {
        DataWide = DataWide.erase(Index, Count);
    }

    bool RemoveFromEnd(const FString& InSuffix, ESearchCase::Type SearchCase)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = DataWide;
            std::wstring LookFor = InSuffix.DataWide;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            uint32 Pos = Tmp.rfind(LookFor);
            if (Pos != std::wstring::npos)
            {
                DataWide = DataWide.replace(Pos, LookFor.length(), L"");
                return true;
            }
        }
        else
        {
            uint32 Pos = DataWide.rfind(InSuffix.DataWide);
            if (Pos != std::wstring::npos)
            {
                DataWide = DataWide.replace(Pos, InSuffix.DataWide.length(), L"");
                return true;
            }
        }
        return false;
    }

    bool RemoveFromStart(const FString& InPrefix, ESearchCase::Type SearchCase)
    {
        if (SearchCase == ESearchCase::IgnoreCase)
        {
            std::wstring Tmp = DataWide;
            std::wstring LookFor = InPrefix.DataWide;

            std::transform(Tmp.begin(), Tmp.end(), Tmp.begin(), ::tolower);
            std::transform(LookFor.begin(), LookFor.end(), LookFor.begin(), ::tolower);

            uint32 Pos = Tmp.find(LookFor);
            if (Pos != std::wstring::npos)
            {
                DataWide = DataWide.replace(Pos, LookFor.length(), L"");
                return true;
            }
        }
        else
        {
            auto Pos = DataWide.find(InPrefix.DataWide);
            if (Pos != std::wstring::npos)
            {
                DataWide = DataWide.replace(Pos, InPrefix.DataWide.length(), L"");
                return true;
            }
        }
        return false;

    }

    FString Replace(const UTFCHAR* From, const UTFCHAR* To, ESearchCase::Type SearchCase)
    {
        std::wstring NewData = DataWide;

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
                NewData = NewData.replace(Pos, FromString.size(), ToString);
                Pos = Tmp.find(FromString);
            }
        }
        else
        {
            size_t Pos = NewData.find(FromString);
            if(Pos != std::string::npos)
            {
                NewData = NewData.replace(Pos, FromString.length(), ToString);
            }
        }
        return FString(NewData);
    }
    FString Replace(const ANSICHAR* From, const ANSICHAR* To, ESearchCase::Type SearchCase)
    {
        std::wstring WideFrom = StringToWString(From, -1);
        std::wstring WideTo = StringToWString(To, -1);
        return Replace(WideFrom.c_str(), WideTo.c_str(), SearchCase);
    }

    void SetElement(uint32 Index, UTFCHAR Character)
    {
        assert(Index >= 0 && Index < DataWide.size());
        DataWide[Index] = Character;
    }
    void SetElement(uint32 Index, ANSICHAR Character)
    {
        assert(Index >= 0 && Index < DataWide.size());
        DataWide[Index] = (UTFCHAR)Character;
    }

    void Reserve(uint32 CharacterCount)
    {
        DataWide.reserve(CharacterCount);
    }
    void Resize(uint32 CharacterCount)
    {
        DataWide.resize(CharacterCount);
    }
    void Reset()
    {
        DataWide.empty();
    }
    void ReverseString()
    {
        std::reverse(DataWide.begin(), DataWide.end());
    }
    FString ToLower() const
    {
        std::wstring NewData = DataWide;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return FString(NewData);
    }
    FString ToUpper() const
    {
        std::wstring NewData = DataWide;
        std::transform(NewData.begin(), NewData.end(), NewData.begin(), ::tolower);
        return FString(NewData);
    }
    FString TrimTrailing()
    {
        auto last = DataWide.find_last_not_of(' ');
        return FString(DataWide.substr(0, last + 1));
    }
    bool FindLastChar(UTFCHAR InChar, int32& Index) const
    {
        Index = static_cast<int32>(DataWide.find_last_of(InChar));
        return Index != std::wstring::npos;
    }
    bool FindLastChar(ANSICHAR InChar, int32& Index) const
    {
        Index = static_cast<int32>(DataWide.find_last_of((UTFCHAR)InChar));
        return Index != std::string::npos;
    }

    void PopBack()
    {
        DataWide.pop_back();
    }

    uint32 LeftFind(const UTFCHAR* Chars, uint32 Pos)
    {
        return static_cast<uint32>(DataWide.find(Chars, Pos));
    }
    uint32 LeftFind(const ANSICHAR* Chars, uint32 Pos)
    {
        std::wstring AsWide = StringToWString(Chars, -1);
        return static_cast<uint32>(DataWide.find(AsWide.c_str(), Pos));
    }
    uint32 RightFind(const UTFCHAR* Chars, uint32 Pos)
    {
        return static_cast<uint32>(DataWide.rfind(Chars, Pos));
    }
    uint32 RightFind(const ANSICHAR* Chars, uint32 Pos)
    {
        std::wstring AsWide = StringToWString(Chars, -1);
        return static_cast<uint32>(DataWide.rfind(AsWide.c_str(), Pos));
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
            return std::hash<std::wstring>()(c.GetWideCharString());
        }
    };
}

class FStringStream
{

private:
    std::wstringstream UTFStringStream;

public:
    FStringStream() = default;

    explicit FStringStream(const ANSICHAR* Parameter)
    {
        UTFStringStream << Parameter;
    }
    explicit FStringStream(const UTFCHAR* Parameter)
    {
        UTFStringStream << Parameter;
    }
    explicit FStringStream(const FString& Parameter)
    {
        UTFStringStream << Parameter.GetWideCharArray();
    }

    void operator<<(const FString& Other)
    {
        UTFStringStream << Other.GetWideCharArray();
    }
    void operator<<(const ANSICHAR* Other)
    {
        UTFStringStream << Other;
    }
    void operator<<(const UTFCHAR* Other)
    {
        UTFStringStream << Other;
    }
    void operator<<(uint8 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(int8 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(uint16 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(int16 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(uint32 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(int32 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(uint64 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(int64 Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(float Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(double Numeric)
    {
        UTFStringStream << Numeric;
    }
    void operator<<(ANSICHAR Character)
    {
        UTFStringStream << Character;
    }
    void operator<<(UTFCHAR Character)
    {
        UTFStringStream << Character;
    }
    void operator>> (bool& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (uint16& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (uint32& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (uint64& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (int16& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (int32& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (int64& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (float& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (double& val)
    {
        UTFStringStream >> val;
    }
    void operator>> (void*& val)
    {
        UTFStringStream >> val;
    }

    void Fill(ANSICHAR Character)
    {
        UTFStringStream.fill((UTFCHAR)Character);
    }
    void Fill(UTFCHAR Character)
    {
        UTFStringStream.fill(Character);
    }

    FString Str()
    {
        return FString(UTFStringStream.str());
    }
};

#define EMPTY_FSTRING_UTF8 FString(L"")

#endif //Pragma_Once_BKString