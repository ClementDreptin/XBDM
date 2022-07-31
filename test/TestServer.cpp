#include "TestServer.h"

#include "../src/Utils.h"

TestServer::TestServer()
    : m_ServerSocket(INVALID_SOCKET), m_ClientSocket(INVALID_SOCKET), m_Listening(false)
{
    m_CommandMap["dbgname"] = std::bind(&TestServer::ConsoleName, this, std::placeholders::_1);
}

void TestServer::Start()
{
    if (!InitServerSocket())
    {
        Shutdown();
        return;
    }

    if (!InitClientSocket())
    {
        Shutdown();
        return;
    }

    if (!Send("201- connected\r\n"))
    {
        Shutdown();
        return;
    }

    if (!Run())
    {
        Shutdown();
        return;
    }
}

void TestServer::WaitForServerToListen()
{
    // Wait to get ownership of the mutex
    std::unique_lock<std::mutex> lock(m_Mutex);

    // Wait for s_Cond to be notified that the server started listening with a timeout of one second
    m_Cond.wait_for(lock, std::chrono::seconds(1), [&]() { return m_Listening; });
}

void TestServer::RequestShutdown()
{
    SignalListening(false);
}

void TestServer::ConsoleName(const std::vector<Arg> &args)
{
    Send("200- TestXDK\r\n");
}

bool TestServer::InitServerSocket()
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

    m_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_ServerSocket == INVALID_SOCKET)
        return false;

    int yes = 1;
    if (setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(int)) == SOCKET_ERROR)
        return false;

    if (bind(m_ServerSocket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == SOCKET_ERROR)
        return false;

    if (listen(m_ServerSocket, 1) == SOCKET_ERROR)
        return false;

    SignalListening(true);

    return true;
}

bool TestServer::InitClientSocket()
{
    m_ClientSocket = accept(m_ServerSocket, static_cast<sockaddr *>(nullptr), static_cast<socklen_t *>(nullptr));
    if (m_ClientSocket == INVALID_SOCKET)
        return false;

    // clang-format off

    // Mark the client socket as nonblocking
    int setToNonBlockingResult = 0;
#ifdef _WIN32
    unsigned long nonBlocking = 1;
    setToNonBlockingResult = ioctlsocket(m_ClientSocket, FIONBIO, &nonBlocking)
#else
    int flags = fcntl(m_ClientSocket, F_GETFL, 0);
    if (flags == SOCKET_ERROR)
        return false;

    flags |= O_NONBLOCK;
    setToNonBlockingResult = fcntl(m_ClientSocket, F_SETFL, flags);
#endif

    if (setToNonBlockingResult != 0)
        return false;

    // clang-format on

    return true;
}

bool TestServer::Run()
{
    char buffer[1024] = { 0 };

    while (m_Listening)
    {
        if (recv(m_ClientSocket, buffer, sizeof(buffer) - 1, 0) <= 0)
            continue;

        Command command = Parse(buffer);

        memset(buffer, 0, sizeof(buffer));

        if (m_CommandMap.find(command.Name) != m_CommandMap.end())
            m_CommandMap.at(command.Name)(command.Args);
    }

    return true;
}

bool TestServer::Send(const std::string &response)
{
    return Send(response.c_str(), response.size());
}

bool TestServer::Send(const char *buffer, size_t length)
{
    const size_t chunkSize = 16;
    int sent = 0;
    int totalSent = 0;

    // Send data in chunks of 16 bytes at most to simulate packets
    for (size_t i = 0; i < length; i += chunkSize)
    {
        size_t toSend = std::min<size_t>(chunkSize, length - totalSent);
        if ((sent = send(m_ClientSocket, buffer + i, static_cast<int>(toSend), 0)) == SOCKET_ERROR)
        {
            Shutdown();
            return false;
        }

        totalSent += sent;
    }

    return true;
}

void TestServer::SignalListening(bool isListening)
{
    // Wait to get ownership of the mutex
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Notify s_Cond that the server started or stopped listening
    m_Listening = isListening;
    m_Cond.notify_all();
}

#ifdef _WIN32
    #define CloseSocket(socket) closesocket(socket)
#else
    #define CloseSocket(socket) close(socket)
#endif

void TestServer::Shutdown()
{
    if (m_Listening)
        SignalListening(false);

    if (m_ServerSocket != INVALID_SOCKET)
    {
        CloseSocket(m_ServerSocket);
        m_ServerSocket = INVALID_SOCKET;
    }

    if (m_ClientSocket != INVALID_SOCKET)
    {
        CloseSocket(m_ClientSocket);
        m_ClientSocket = INVALID_SOCKET;
    }
}

TestServer::Command TestServer::Parse(const std::string &commandString)
{
    // Remove the new line at the end ('\r\n')
    std::string commandStringCopy = commandString.substr(0, commandString.size() - 2);

    Command command;
    std::vector<std::string> tokens = XBDM::Utils::String::Split(commandStringCopy, " ");

    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (i == 0)
        {
            command.Name = tokens[i];
            continue;
        }

        size_t equalSignIndex = tokens[i].find('=');
        if (equalSignIndex == std::string::npos)
            continue;

        std::string name = tokens[i].substr(0, equalSignIndex);
        std::string value = tokens[i].substr(equalSignIndex + 1, tokens[i].size());
        Arg::ArgType type = Arg::ArgType::Int;

        if (value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2);
            type = Arg::ArgType::String;
        }

        command.Args.emplace_back(name, value, type);
    }

    return command;
}
