#pragma once

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
#else
    #include <cstring>
    #include <netdb.h>
    #include <unistd.h>
#endif

#ifndef _WIND32
// clang-format off
    typedef int SOCKET;
// clang-format on

    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class XbdmServerMock
{
public:
    XbdmServerMock() = delete;

    static void ConnectRespondAndShutdown();
private:
    static SOCKET s_Socket;
    static bool s_Running;

    static bool Open();
    static void Close();
    static void CloseSocket(SOCKET socket);
    static void CleanupSocket();
};
