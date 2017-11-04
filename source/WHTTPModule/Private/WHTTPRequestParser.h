// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPRequestParser
#define Pragma_Once_WHTTPRequestParser

#include "WEngine.h"
#include <string>
#include <map>

class WHTTPRequestParser
{

private:
    bool HalfEndOfLine{};
    bool EndOfLine{};
    bool FirstLine{};
    bool Beginning{};

    std::string Method;
    std::string Path;
    std::string ProtocolVersion;

    std::string TempHeaderName;
    std::string TempHeaderValue;

    ANSICHAR PreviousChar{};

    std::map<std::string, std::string> Headers;
    std::string Payload;
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

        Method = "";
        Path = "";
        ProtocolVersion = "";

        TempHeaderName = "";
        TempHeaderValue = "";

        PreviousChar = '\0';

        Headers.clear();
        Payload = "";
        bHeadersAvailable = false;
        bBodyAvailable = false;
        bErrorOccuredInBodyParsing = false;
    }

    WHTTPRequestParser()
    {
        Reset();
    }

    /*
     * @param buf Pointer to the buffer in which data from client socket is stored
     * @param size Size of the chunk of data
     * @return If corrupted false, otherwise true
     */
    void ProcessChunkForHeaders(const ANSICHAR* Buffer, int32 Size)
    {
        if (bHeadersAvailable) return;

        int32 Field = 0;

        ANSICHAR c;
        for(int32 i = 0; i < Size; i++)
        {
            c = Buffer[i];
            if(c == '\r')
            {
                HalfEndOfLine = true;

                PreviousChar = c;
                Beginning = false;
                continue;
            }
            else if(HalfEndOfLine && c == '\n')
            {
                if(EndOfLine)
                {
                    bHeadersAvailable = true;
                    if ((i + 1) < Size)
                    {
                        ProcessChunkForBody(Buffer + i + 1, Size - i - 1);
                        return;
                    }
                }
                else
                {
                    if(!FirstLine)
                    {
                        Headers[TempHeaderName] = TempHeaderValue;
                        TempHeaderName = "";
                        TempHeaderValue = "";
                    }
                    EndOfLine = true;
                    FirstLine = false;

                    PreviousChar = c;
                    Beginning = false;
                    continue;
                }
            }
            if(FirstLine)
            {
                if(Beginning || EndOfLine)
                {
                    Field = 0;
                }

                if(c == ' ')
                {
                    Field++;
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
                if(EndOfLine)
                {
                    Field = 0;
                }

                switch(Field)
                {
                    case 0:
                        if(c == ' ' && PreviousChar == ':')
                        {
                            TempHeaderName.pop_back();
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
        }
    }
    void ProcessChunkForBody(const ANSICHAR* Buffer, int32 Size)
    {
        if (bBodyAvailable) return;

        int32 ContentLength = 0;
        auto ContentLengthIterator = Headers.find("Content-Length");
        if (ContentLengthIterator != Headers.end())
        {
            try
            {
                ContentLength = std::stoi(ContentLengthIterator->second);
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
            if ((Size + Payload.size()) >= ContentLength) bBodyAvailable = true;
            ContentLength = ContentLength < Size ? ContentLength : Size;
        }
        else
        {
            ContentLength = Size;
            bBodyAvailable = true;
        }

        std::string ChunkString;
        ChunkString.resize(static_cast<uint32>(ContentLength));
        for (int32 x = 0; x < ContentLength; x++)
        {
            ChunkString[x] = Buffer[x];
        }
        Payload.append(ChunkString);
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

    //@return Headers in form of std::map (name -> value)
    std::map<std::string, std::string> GetHeaders()
    {
        return Headers;
    };

    //@return String representing the HTTP method used in request
    std::string GetMethod()
    {
        return Method;
    }

    //@return String representing the requested path
    std::string GetPath()
    {
        return Path;
    }

    //@return String representing the protocol (and version) used to perform the request
    std::string GetProtocol()
    {
        return ProtocolVersion;
    }

    //@return String contains payload
    std::wstring GetPayload()
    {
        return FString::StringToWString(Payload);
    }

    virtual ~WHTTPRequestParser() = default;
};

#endif //Pragma_Once_WHTTPRequestParser