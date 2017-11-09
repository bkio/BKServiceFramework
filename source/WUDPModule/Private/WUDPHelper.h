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

class WUDPHelper
{

public:
    static std::string GetAddressPortFromClient(struct sockaddr* Client, uint32 MessageID, bool bDoNotAppendMessageID = false)
    {
        if (Client == nullptr) return "";

        auto ClientAsBroad = (struct sockaddr_in*)Client;

        std::stringstream Stream;
        Stream << inet_ntoa(ClientAsBroad->sin_addr) << ':' << htons(ClientAsBroad->sin_port);
        if (!bDoNotAppendMessageID)
        {
            Stream << ':' << MessageID;
        }

        return Stream.str();
    }
};

class UWUDPTaskParameter : public UWAsyncTaskParameter
{

private:
    UWUDPTaskParameter() = default;

public:
    int32 BufferSize = 0;
    ANSICHAR* Buffer = nullptr;
    sockaddr* Client = nullptr;

    UWUDPTaskParameter(int32 BufferSizeParameter, ANSICHAR* BufferParameter, sockaddr* ClientParameter)
    {
        BufferSize = BufferSizeParameter;
        Buffer = BufferParameter;
        Client = ClientParameter;
    }
    ~UWUDPTaskParameter() override
    {
        if (Client)
        {
            delete (Client);
        }
        delete[] Buffer;
    }
};

#pragma clang diagnostic pop

#endif //Pragma_Once_WUDPHelper