// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WHTTPHelper
#define Pragma_Once_WHTTPHelper

#if PLATFORM_WINDOWS
    #pragma comment(lib, "ws2_32.lib")
    #include <ws2tcpip.h>
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
#endif

#define HTTP_BUFFER_SIZE 2048

#endif //Pragma_Once_WHTTPHelper