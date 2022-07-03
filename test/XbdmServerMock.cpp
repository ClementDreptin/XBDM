#include "XbdmServerMock.h"

#define XBDM_PORT 730
#define CONNECT_RESPONSE "201- connected\r\n"

bool XbdmServerMock::s_Listening = false;
SOCKET XbdmServerMock::s_Socket = INVALID_SOCKET;
std::mutex XbdmServerMock::s_Mutex;
std::condition_variable XbdmServerMock::s_Cond;

void XbdmServerMock::ConnectRespondAndShutdown()
{
    if (!Start())
        return;

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    SignalListening();

    SOCKET clientSocket = accept(s_Socket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (clientSocket == INVALID_SOCKET)
    {
        Shutdown();
        return;
    }

    if (send(clientSocket, CONNECT_RESPONSE, static_cast<int>(strlen(CONNECT_RESPONSE)), 0) == SOCKET_ERROR)
    {
        CloseSocket(clientSocket);
        Shutdown();
        return;
    }

    WaitForClientToRequestShutdown();

    CloseSocket(clientSocket);
    Shutdown();
}

void XbdmServerMock::NoAccept()
{
    if (!Start())
        return;

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    SignalListening();

    WaitForClientToRequestShutdown();

    Shutdown();
}

void XbdmServerMock::NoResponse()
{
    if (!Start())
        return;

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    SignalListening();

    SOCKET clientSocket = accept(s_Socket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (clientSocket == INVALID_SOCKET)
    {
        Shutdown();
        return;
    }

    WaitForClientToRequestShutdown();

    CloseSocket(clientSocket);
    Shutdown();
}

void XbdmServerMock::PartialConnectResponse()
{
    if (!Start())
        return;

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    SignalListening();

    SOCKET clientSocket = accept(s_Socket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (clientSocket == INVALID_SOCKET)
    {
        Shutdown();
        return;
    }

    const char *partialConnectResponse = "201";
    if (send(clientSocket, partialConnectResponse, static_cast<int>(strlen(partialConnectResponse)), 0) == SOCKET_ERROR)
    {
        CloseSocket(clientSocket);
        Shutdown();
        return;
    }

    WaitForClientToRequestShutdown();

    CloseSocket(clientSocket);
    Shutdown();
}

void XbdmServerMock::ConsoleNameResponse()
{
    if (!Start())
        return;

    if (listen(s_Socket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    SignalListening();

    SOCKET clientSocket = accept(s_Socket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (clientSocket == INVALID_SOCKET)
    {
        Shutdown();
        return;
    }

    if (send(clientSocket, CONNECT_RESPONSE, static_cast<int>(strlen(CONNECT_RESPONSE)), 0) == SOCKET_ERROR)
    {
        CloseSocket(clientSocket);
        Shutdown();
        return;
    }

    char requestBuffer[1024] = { 0 };
    int received = recv(clientSocket, requestBuffer, sizeof(requestBuffer), 0);
    if (strcmp(requestBuffer, "dbgname\r\n") != 0)
    {
        CloseSocket(clientSocket);
        Shutdown();
        return;
    }

    const char *consoleNameResponse = "200- TestXDK\r\n";
    if (send(clientSocket, consoleNameResponse, strlen(consoleNameResponse), 0) == SOCKET_ERROR)
    {
        CloseSocket(clientSocket);
        Shutdown();
        return;
    }

    WaitForClientToRequestShutdown();

    CloseSocket(clientSocket);
    Shutdown();
}

void XbdmServerMock::WaitForServerToListen()
{
    std::unique_lock<std::mutex> lock(s_Mutex);
    s_Cond.wait_for(lock, std::chrono::seconds(1), []() { return s_Listening; });
}

void XbdmServerMock::SendRequestToShutdownServer()
{
    StopListening();
}

bool XbdmServerMock::Start()
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
        Cleanup();
        return false;
    }

    int yes = 1;
    if (setsockopt(s_Socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(int)) == SOCKET_ERROR)
    {
        Shutdown();
        return false;
    }

    if (bind(s_Socket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        Shutdown();
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

void XbdmServerMock::StopListening()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Listening = false;
    s_Cond.notify_all();
}

void XbdmServerMock::WaitForClientToRequestShutdown()
{
    std::unique_lock<std::mutex> lock(s_Mutex);
    s_Cond.wait_for(lock, std::chrono::seconds(1), []() { return !s_Listening; });
}

void XbdmServerMock::Shutdown()
{
    StopListening();
    CloseSocket(s_Socket);
    Cleanup();

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

void XbdmServerMock::Cleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}
