#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

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
    static void CleanupSocket();
};
