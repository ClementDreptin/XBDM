#include "XbdmServerMock.h"

#define XBDM_PORT 730
#define CONNECT_RESPONSE "201- connected\r\n"

bool XbdmServerMock::s_Listening = false;
SOCKET XbdmServerMock::s_Socket = INVALID_SOCKET;
std::mutex XbdmServerMock::s_Mutex;
std::condition_variable XbdmServerMock::s_Cond;

void XbdmServerMock::ConnectRespondAndShutdown()
{
    Open();

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Close();
        CleanupSocket();
        return;
    }

    SignalListening();

#ifdef _WIN32
    typedef int socklen_t;
#endif

    SOCKET clientSocket = accept(s_Socket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));

    if (clientSocket != INVALID_SOCKET)
        send(clientSocket, CONNECT_RESPONSE, static_cast<int>(strlen(CONNECT_RESPONSE)), 0);

    WaitForClientToRequestShutdown();

    CloseSocket(clientSocket);
    Close();
    CleanupSocket();
}

void XbdmServerMock::NoAccept()
{
    Open();

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Close();
        CleanupSocket();
        return;
    }

    SignalListening();

    WaitForClientToRequestShutdown();

    Close();
    CleanupSocket();
}

void XbdmServerMock::WaitForServerToListen()
{
    std::unique_lock<std::mutex> lock(s_Mutex);
    while (!s_Listening)
        s_Cond.wait(lock, []() { return s_Listening; });
}

void XbdmServerMock::SendRequestToShutdownServer()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Listening = false;
    s_Cond.notify_all();
}

bool XbdmServerMock::Open()
{
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(XBDM_PORT);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;
#endif

    s_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_Socket == INVALID_SOCKET)
    {
        CleanupSocket();
        return false;
    }

    int yes = 1;
    if (setsockopt(s_Socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(int)) != 0)
    {
        Close();
        CleanupSocket();
        return false;
    }

    if (bind(s_Socket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        Close();
        CleanupSocket();
        return false;
    }

    return true;
}

void XbdmServerMock::SignalListening()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Listening = true;
    s_Cond.notify_all();
}

void XbdmServerMock::WaitForClientToRequestShutdown()
{
    std::unique_lock<std::mutex> lock(s_Mutex);
    while (s_Listening)
        s_Cond.wait(lock, []() { return !s_Listening; });
}

void XbdmServerMock::Close()
{
    CloseSocket(s_Socket);

    s_Socket = INVALID_SOCKET;
}

void XbdmServerMock::CloseSocket(SOCKET socket)
{
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

void XbdmServerMock::CleanupSocket()
{
#ifdef _WIN32
    WSACleanup();
#endif
}
