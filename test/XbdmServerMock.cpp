#include "XbdmServerMock.h"

bool XbdmServerMock::s_Listening = false;
SOCKET XbdmServerMock::s_ServerSocket = INVALID_SOCKET;
SOCKET XbdmServerMock::s_ClientSocket = INVALID_SOCKET;
std::mutex XbdmServerMock::s_Mutex;
std::condition_variable XbdmServerMock::s_Cond;

void XbdmServerMock::NoResponse()
{
    if (!Start())
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::PartialConnectResponse()
{
    if (!Start())
        return;

    const char *partialConnectResponse = "201";
    if (send(s_ClientSocket, partialConnectResponse, static_cast<int>(strlen(partialConnectResponse)), 0) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    ProcessShutdownRequest();
}

void XbdmServerMock::ConnectRespondAndShutdown()
{
    if (!StartClientConnection())
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::ConsoleNameResponse()
{
    if (!StartClientConnection())
        return;

    char requestBuffer[1024] = { 0 };
    int received = recv(s_ClientSocket, requestBuffer, sizeof(requestBuffer), 0);
    if (strcmp(requestBuffer, "dbgname\r\n") != 0)
    {
        Shutdown();
        return;
    }

    const char *consoleNameResponse = "200- TestXDK\r\n";
    if (send(s_ClientSocket, consoleNameResponse, static_cast<int>(strlen(consoleNameResponse)), 0) == SOCKET_ERROR)
    {
        Shutdown();
        return;
    }

    ProcessShutdownRequest();
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
    address.sin_port = htons(730);

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;
#endif

    s_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_ServerSocket == INVALID_SOCKET)
    {
        Shutdown();
        return false;
    }

    int yes = 1;
    if (setsockopt(s_ServerSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(int)) == SOCKET_ERROR)
    {
        Shutdown();
        return false;
    }

    if (bind(s_ServerSocket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        Shutdown();
        return false;
    }

    if (listen(s_ServerSocket, 5) == SOCKET_ERROR)
    {
        Shutdown();
        return false;
    }

    SignalListening();

    s_ClientSocket = accept(s_ServerSocket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (s_ClientSocket == INVALID_SOCKET)
    {
        Shutdown();
        return false;
    }

    return true;
}

bool XbdmServerMock::StartClientConnection()
{
    if (!Start())
        return false;

    const char *connectResponse = "201- connected\r\n";
    if (send(s_ClientSocket, connectResponse, static_cast<int>(strlen(connectResponse)), 0) == SOCKET_ERROR)
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

void XbdmServerMock::ProcessShutdownRequest()
{
    std::unique_lock<std::mutex> lock(s_Mutex);
    s_Cond.wait_for(lock, std::chrono::seconds(1), []() { return !s_Listening; });

    Shutdown();
}

#ifdef _WIN32
    #define CloseSocket(socket) closesocket(socket)
#else
    #define CloseSocket(socket) close(socket)
#endif

void XbdmServerMock::Shutdown()
{
    if (s_Listening)
        StopListening();

    if (s_ServerSocket != INVALID_SOCKET)
    {
        CloseSocket(s_ServerSocket);
        s_ServerSocket = INVALID_SOCKET;
    }

    if (s_ClientSocket != INVALID_SOCKET)
    {
        CloseSocket(s_ClientSocket);
        s_ClientSocket = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSACleanup();
#endif
}
