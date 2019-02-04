// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKHTTPRequestParser
#define Pragma_Once_BKHTTPRequestParser

#include "BKEngine.h"
#include "BKHashMap.h"
#include <string>

class BKHTTPRequestParser
{

private:
    bool HalfEndOfLine{};
    bool EndOfLine{};
    bool FirstLine{};
    bool Beginning{};

    FString Method{};
    FString Path{};
    FString ProtocolVersion{};

    FString TempHeaderName{};
    FString TempHeaderValue{};

    UTFCHAR PreviousChar{};

    BKHashMap<FString, FString> Headers{};
    FString Payload{};
    bool bHeadersAvailable = false;
    bool bBodyAvailable = false;
    bool bErrorOccuredInBodyParsing = false;

public:

    //Prepare the object to handle another request (clear all extracted data).
    void Reset()
    {
        HalfEndOfLine = false;
        EndOfLine = false;
        FirstLine = true;
        Beginning = true;

        Method = EMPTY_FSTRING_UTF8;
        Path = EMPTY_FSTRING_UTF8;
        ProtocolVersion = EMPTY_FSTRING_UTF8;

        TempHeaderName = EMPTY_FSTRING_UTF8;
        TempHeaderValue = EMPTY_FSTRING_UTF8;

        PreviousChar = '\0';

        Headers.Clear();
        Payload = L"";
        bHeadersAvailable = false;
        bBodyAvailable = false;
        bErrorOccuredInBodyParsing = false;
    }

    BKHTTPRequestParser()
    {
        Reset();
    }

    /*
     * @param buf Pointer to the buffer in which data from client socket is stored
     * @param size Size of the chunk of data
     * @return If corrupted false, otherwise true
     */
    void ProcessChunkForHeaders(const FString& Buffer)
    {
        if (bHeadersAvailable) return;

        int32 Field = 0;

        UTFCHAR c;
        for (int32 i = 0; i < Buffer.Len(); i++)
        {
            c = Buffer.AtWide(i);
            if (c == L'\r')
            {
                HalfEndOfLine = true;

                PreviousChar = c;
                Beginning = false;
                continue;
            }
            else if (HalfEndOfLine && c == L'\n')
            {
                if (EndOfLine)
                {
                    bHeadersAvailable = true;
                    if ((i + 1) < Buffer.Len())
                    {
                        FString BodyChunk = FString(Buffer.GetWideCharArray() + i + 1, Buffer.Len() - i - 1);
                        ProcessChunkForBody(BodyChunk);
                        return;
                    }
                }
                else
                {
                    if (!FirstLine)
                    {
                        Headers.Put(TempHeaderName, TempHeaderValue);
                        TempHeaderName = L"";
                        TempHeaderValue = L"";
                    }
                    EndOfLine = true;
                    FirstLine = false;

                    PreviousChar = c;
                    Beginning = false;
                    continue;
                }
            }
            if (FirstLine)
            {
                if (Beginning || EndOfLine)
                {
                    Field = 0;
                }

                if (c == L' ')
                {
                    Field++;
                    Beginning = false;
                }
                else
                {
                    switch(Field)
                    {
                        case 0:
                            Method += c;
                            break;

                        case 1:
                            Path += c;
                            break;

                        case 2:
                            ProtocolVersion += c;
                            break;

                        default:
                            break;
                    }
                }
            }
            else
            {
                if (EndOfLine)
                {
                    Field = 0;
                }

                switch (Field)
                {
                    case 0:
                        if (c == L' ' && PreviousChar == L':')
                        {
                            TempHeaderName.PopBack();
                            Field++;
                        }
                        else
                        {
                            TempHeaderName += c;
                        }
                        break;

                    case 1:
                        TempHeaderValue += c;
                        break;

                    default:
                        break;
                }
            }
            HalfEndOfLine = false;
            EndOfLine = false;
            PreviousChar = c;
        }
    }
    void ProcessChunkForBody(const FString& Buffer)
    {
        if (bBodyAvailable) return;

        int32 ContentLength = 0;
        FString FoundValue;
        if (Headers.Get(FString(L"Content-Length"), FoundValue))
        {
            try
            {
                ContentLength = FString::ConvertToInteger<int32>(FoundValue.GetAnsiCharArray().c_str());
            }
            catch (const std::invalid_argument &ia)
            {
                bErrorOccuredInBodyParsing = true;
                return;
            }
            if (ContentLength <= 0)
            {
                bErrorOccuredInBodyParsing = true;
                return;
            }
            if ((Buffer.Len() + Payload.Len()) >= ContentLength) bBodyAvailable = true;
            ContentLength = ContentLength < Buffer.Len() ? ContentLength : Buffer.Len();
        }
        else
        {
            ContentLength = Buffer.Len();
            bBodyAvailable = true;
        }

        FString ChunkString;
        ChunkString.Resize(static_cast<uint32>(ContentLength));
        for (int32 x = 0; x < ContentLength; x++)
        {
            ChunkString.SetElement(x, Buffer.AtWide(x));
        }
        Payload.Append(ChunkString);
    }

    //@return Information if all data was already extracted from headers and can be safely accessed
    bool AllHeadersAvailable()
    {
        return bHeadersAvailable;
    }

    bool AllBodyAvailable()
    {
        return bBodyAvailable;
    }

    bool ErrorOccuredInBodyParsing()
    {
        return bErrorOccuredInBodyParsing;
    }

    //@return Headers in form of BKHashMap<FString, FString, BKFStringKeyHash> (name -> value)
    BKHashMap<FString, FString> GetHeaders()
    {
        return Headers;
    };

    //@return String representing the HTTP method used in request
    FString GetMethod()
    {
        return Method;
    }

    //@return String representing the requested path
    FString GetPath()
    {
        return Path;
    }

    //@return String representing the protocol (and version) used to perform the request
    FString GetProtocol()
    {
        return ProtocolVersion;
    }

    //@return String contains payload
    FString GetPayload()
    {
        return Payload;
    }

    virtual ~BKHTTPRequestParser() = default;
};

#endif //Pragma_Once_BKHTTPRequestParser