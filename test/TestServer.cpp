#include "TestServer.h"

#include <sstream>
#include <fstream>

#include "../src/Utils.h"

TestServer::TestServer()
    : m_ServerSocket(INVALID_SOCKET), m_ClientSocket(INVALID_SOCKET), m_Listening(false)
{
    m_CommandMap["dbgname"] = std::bind(&TestServer::ConsoleName, this, std::placeholders::_1);
    m_CommandMap["drivelist"] = std::bind(&TestServer::DriveList, this, std::placeholders::_1);
    m_CommandMap["drivefreespace"] = std::bind(&TestServer::DriveFreeSpace, this, std::placeholders::_1);
    m_CommandMap["dirlist"] = std::bind(&TestServer::DirectoryContents, this, std::placeholders::_1);
    m_CommandMap["magicboot"] = std::bind(&TestServer::MagicBoot, this, std::placeholders::_1);
    m_CommandMap["getfile"] = std::bind(&TestServer::ReceiveFile, this, std::placeholders::_1);
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

    if (!Send("201- connected"))
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
    if (!args.empty())
    {
        Send("400- no argument expected");
        return;
    }

    Send("200- TestXDK");
}

void TestServer::DriveList(const std::vector<Arg> &args)
{
    if (!args.empty())
    {
        Send("400- no argument expected");
        return;
    }

    // Build the response with the drive names and send it
    std::array<std::string, 2> driveNames = { "HDD", "Z" };
    std::stringstream response;
    response << "202- multiline response follows\r\n";

    for (auto &driveName : driveNames)
        response << "drivename=\"" << driveName << "\"\r\n";

    response << '.';

    Send(response.str());
}

void TestServer::DriveFreeSpace(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found");
        return;
    }

    std::string response =
        "200- "
        "freetocallerhi=0x0 freetocallerlo=0xa "
        "totalbyteshi=0x0 totalbyteslo=0xb "
        "totalfreebyteshi=0x0 totalfreebyteslo=0xc";

    Send(response);
}

void TestServer::DirectoryContents(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found");
        return;
    }

    std::string response =
        "202- multiline response follows\r\n"
        "name=\"dir1\" sizehi=0x0 sizelo=0x0 directory\r\n"
        "name=\"file1.txt\" sizehi=0x0 sizelo=0xa\r\n"
        "name=\"dir2\" sizehi=0x0 sizelo=0x0 directory\r\n"
        "name=\"file2.xex\" sizehi=0x0 sizelo=0xb\r\n.";

    Send(response);
}

void TestServer::MagicBoot(const std::vector<Arg> &args)
{
    if (args.size() != 2)
    {
        Send("400- wrong number of arguments provided, two expected");
        return;
    }

    if (args[0].Name != "title")
    {
        Send("400- argument 'title' not found");
        return;
    }

    if (args[1].Name != "directory")
    {
        Send("400- argument 'directory' not found");
        return;
    }

    Send("200- OK");
}

void TestServer::ReceiveFile(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found");
        return;
    }

    const std::string &filePath = args[0].Value;

    // Open the requested file
    std::ifstream inFile;
    inFile.open(filePath, std::ifstream::binary);

    if (inFile.fail())
    {
        Send("404- Couldn't open file: " + filePath);
        return;
    }

    // Get the file size
    inFile.seekg(0, inFile.end);
    int fileSize = static_cast<int>(inFile.tellg());
    inFile.seekg(0, inFile.beg);

    // Start building the response
    std::vector<char> response;
    std::string header = "203- binary response follows\r\n";

    // The final response will contain the header, a 32-bit integer for the file size and the file content
    // so we preallocate enough memory for that
    response.reserve(header.size() + sizeof(fileSize) + fileSize);

    // Append the header and the file size to the response
    response.insert(response.end(), header.begin(), header.end());
    response.insert(response.end(), reinterpret_cast<const char *>(&fileSize), reinterpret_cast<const char *>(&fileSize) + sizeof(fileSize));

    // Buffer to hold file chunks while reading it
    char buffer[s_PacketSize] = { 0 };

    while (!inFile.eof())
    {
        inFile.read(buffer, sizeof(buffer));

        // Append the current chunk to the response
        response.insert(response.end(), buffer, buffer + inFile.gcount());

        // Reset the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    inFile.close();

    Send(response.data(), response.size(), false);
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
    char buffer[s_PacketSize + 1] = { 0 };

    while (m_Listening)
    {
        if (recv(m_ClientSocket, buffer, s_PacketSize, 0) <= 0)
            continue;

        Command command = Parse(buffer);

        memset(buffer, 0, sizeof(buffer));

        if (m_CommandMap.find(command.Name) != m_CommandMap.end())
            m_CommandMap.at(command.Name)(command.Args);
    }

    return true;
}

bool TestServer::Send(const std::string &response, bool sendFinalNewLine)
{
    return Send(response.c_str(), response.size(), sendFinalNewLine);
}

bool TestServer::Send(const char *buffer, size_t length, bool sendFinalNewLine)
{
    int sent = 0;
    int totalSent = 0;

    // Send data in chunks of s_PacketSize bytes at most to simulate packets
    for (size_t i = 0; i < length; i += s_PacketSize)
    {
        size_t toSend = std::min<size_t>(s_PacketSize, length - totalSent);
        if ((sent = send(m_ClientSocket, buffer + i, static_cast<int>(toSend), 0)) == SOCKET_ERROR)
        {
            Shutdown();
            return false;
        }

        totalSent += sent;
    }

    // If the final new line characters are not necessary, stop here
    if (!sendFinalNewLine)
        return true;

    // Send the new line character to mark the end of the response
    const char *responseEnd = "\r\n";
    if (send(m_ClientSocket, responseEnd, static_cast<int>(strlen(responseEnd)), 0) == SOCKET_ERROR)
    {
        Shutdown();
        return false;
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
