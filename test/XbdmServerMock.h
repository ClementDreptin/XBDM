#pragma once

#include <mutex>
#include <condition_variable>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
#else
    #include <cstring>
    #include <netdb.h>
    #include <unistd.h>
#endif

#ifndef _WIN32
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

    static void WaitForServerToListen();
    static void SendRequestToShutdownServer();

private:
    static SOCKET s_Socket;
    static bool s_Listening;
    static std::mutex s_Mutex;
    static std::condition_variable s_Cond;

    static bool Open();
    static void SignalListening();
    static void WaitForClientToRequestShutdown();
    static void Close();
    static void CloseSocket(SOCKET socket);
    static void CleanupSocket();
};
