// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WUDPHelper
#define Pragma_Once_WUDPHelper

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

#include "WEngine.h"
#include "WTaskDefines.h"

#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

#define UDP_BUFFER_SIZE 1024

class WUDPHelper
{

public:
    static FString GetAddressPortFromOtherParty(struct sockaddr *OtherParty, uint32 MessageID, bool bDoNotAppendMessageID = false)
    {
        if (!OtherParty) return EMPTY_FSTRING_ANSI;

        auto OtherPartyAsBroad = (struct sockaddr_in*)OtherParty;

        FStringStream Stream(false);
        Stream << inet_ntoa(OtherPartyAsBroad->sin_addr);
        Stream << ':';
        Stream << htons(OtherPartyAsBroad->sin_port);
        if (!bDoNotAppendMessageID)
        {
            Stream << ':';
            Stream << MessageID;
        }

        return Stream.Str();
    }
};

class WUDPTaskParameter : public WAsyncTaskParameter
{

private:
    WUDPTaskParameter() = default;

    bool AsServer = false;

public:
    int32 BufferSize = 0;
    ANSICHAR* Buffer = nullptr;
    sockaddr* OtherParty = nullptr;

    WUDPTaskParameter(int32 BufferSizeParameter, ANSICHAR* BufferParameter, sockaddr* OtherPartyParameter, bool AsServerParameter)
    {
        BufferSize = BufferSizeParameter;
        Buffer = BufferParameter;
        OtherParty = OtherPartyParameter;
        AsServer = AsServerParameter;
    }
    ~WUDPTaskParameter() override
    {
        if (AsServer && OtherParty)
        {
            delete (OtherParty);
        }
        delete[] Buffer;
    }
};

#pragma clang diagnostic pop

#endif //Pragma_Once_WUDPHelper