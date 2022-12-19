#include "TestServer.h"

#include <array>
#include <sstream>
#include <fstream>
#include <thread>
#include <algorithm>

#include "Utils.h"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

#define BIND_FN(fn) std::bind(&TestServer::fn, this, std::placeholders::_1)

TestServer::TestServer()
    : m_ServerSocket(INVALID_SOCKET), m_ClientSocket(INVALID_SOCKET), m_Listening(false)
{
    m_CommandMap["dbgname"] = BIND_FN(ConsoleName);
    m_CommandMap["drivelist"] = BIND_FN(DriveList);
    m_CommandMap["drivefreespace"] = BIND_FN(DriveFreeSpace);
    m_CommandMap["dirlist"] = BIND_FN(DirectoryContents);
    m_CommandMap["magicboot"] = BIND_FN(MagicBoot);
    m_CommandMap["xbeinfo"] = BIND_FN(ActiveTitle);
    m_CommandMap["consoletype"] = BIND_FN(ConsoleType);
    m_CommandMap["setsystime"] = BIND_FN(SetSystemTime);
    m_CommandMap["getfile"] = BIND_FN(ReceiveFile);
    m_CommandMap["sendfile"] = BIND_FN(SendFile);
    m_CommandMap["delete"] = BIND_FN(DeleteFile);
    m_CommandMap["mkdir"] = BIND_FN(CreateDirectory);
    m_CommandMap["rename"] = BIND_FN(RenameFile);
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
    if (!args.empty())
    {
        Send("400- no argument expected\r\n");
        return;
    }

    Send("200- TestXDK\r\n");
}

void TestServer::DriveList(const std::vector<Arg> &args)
{
    if (!args.empty())
    {
        Send("400- no argument expected\r\n");
        return;
    }

    // Build the response with the drive names and send it
    std::array<std::string, 2> driveNames = { "HDD", "Z" };
    std::stringstream response;
    response << "202- multiline response follows\r\n";

    for (auto &driveName : driveNames)
        response << "drivename=\"" << driveName << "\"\r\n";

    response << ".\r\n";

    Send(response.str());
}

void TestServer::DriveFreeSpace(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    std::string response =
        "200- "
        "freetocallerhi=0x0 freetocallerlo=0xa "
        "totalbyteshi=0x0 totalbyteslo=0xb "
        "totalfreebyteshi=0x0 totalfreebyteslo=0xc\r\n";

    Send(response);
}

void TestServer::DirectoryContents(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    // The client will create paths using backslashes (\) because that's what the Xbox 360 uses.
    // For the tests we need to forward slashes (/) on POSIX systems so we patch them here.
#ifndef _WIN32
    std::string directoryPath = args[0].Value;
    std::replace(directoryPath.begin(), directoryPath.end(), '\\', '/');
#else
    const std::string &directoryPath = args[0].Value;
#endif

    if (!fs::exists(directoryPath))
    {
        Send("404- " + directoryPath + " not found\r\n");
        return;
    }

    std::stringstream response;
    response << "202- multiline response follows\r\n";

    for (const auto &entry : std::filesystem::directory_iterator(directoryPath))
    {
        std::stringstream line;
        line << "name=\"" << entry.path().filename().string() << "\"";

        // Some random but valid creation and modification dates
        line << " createhi=0x01d11fb5 createlo=0x59683c00";
        line << " changehi=0x01d11fb5 changelo=0x59683c00";

        if (!entry.is_directory())
        {
            line << " sizehi=0x" << std::hex << (entry.file_size() & 0xffff0000);
            line << " sizelo=0x" << std::hex << (entry.file_size() & 0x0000ffff);
            line << "\r\n";
        }
        else
            line << " sizehi=0x0 sizelo=0x0 directory\r\n";

        line << ".\r\n";

        response << line.str();
    }

    Send(response.str());
}

void TestServer::MagicBoot(const std::vector<Arg> &args)
{
    // Case of launching a specific XEX
    if (args.size() == 2)
    {
        if (args[0].Name != "title")
        {
            Send("400- argument 'title' not found\r\n");
            return;
        }

        if (args[1].Name != "directory")
        {
            Send("400- argument 'directory' not found\r\n");
            return;
        }

        const std::string &title = args[0].Value;
        const std::string &providedDirectory = args[1].Value;
        std::string directoryFromTitle = title.substr(0, title.find_last_of('\\') + 1);

        if (providedDirectory != directoryFromTitle)
        {
            Send("400- the value of 'directory' does not match with the title path\r\n");
            return;
        }
    }

    Send("200- OK");
}

void TestServer::ActiveTitle(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected\r\n");
        return;
    }

    if (args[0].Name != "running")
    {
        Send("400- argument 'running' not found\r\n");
        return;
    }

    std::string response =
        "202- multiline response follows\r\n"
        "timestamp=0x00000000 checksum=0x00000000\r\n"
        "name=\"\\Device\\Harddisk0\\SystemExtPartition\\20449700\\dash.xex\"\r\n"
        ".\r\n";

    Send(response);
}

void TestServer::ConsoleType(const std::vector<Arg> &args)
{
    if (!args.empty())
    {
        Send("400- no argument expected\r\n");
        return;
    }

    Send("200- reviewerkit\r\n");
}

void TestServer::SetSystemTime(const std::vector<Arg> &args)
{
    if (args.size() != 2)
    {
        Send("400- wrong number of arguments provided, 2 expected\r\n");
        return;
    }

    if (args[0].Name != "clockhi")
    {
        Send("400- argument 'clockhi' not found\r\n");
        return;
    }

    if (args[1].Name != "clocklo")
    {
        Send("400- argument 'clocklo' not found\r\n");
        return;
    }

    Send("200- OK\r\n");
}

void TestServer::ReceiveFile(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    // The client will create paths using backslashes (\) because that's what the Xbox 360 uses.
    // For the tests we need to forward slashes (/) on POSIX systems so we patch them here.
#ifndef _WIN32
    std::string filePath = args[0].Value;
    std::replace(filePath.begin(), filePath.end(), '\\', '/');
#else
    const std::string &filePath = args[0].Value;
#endif

    // Open the requested file
    std::ifstream inFile;
    inFile.open(filePath, std::ifstream::binary);

    if (inFile.fail())
    {
        Send("404- Couldn't open file: " + filePath + "\r\n");
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

    Send(response.data(), response.size());
}

void TestServer::SendFile(const std::vector<Arg> &args)
{
    if (args.size() != 2)
    {
        Send("400- wrong number of arguments provided, two expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    if (args[1].Name != "length")
    {
        Send("400- argument 'length' not found\r\n");
        return;
    }

    const std::string &pathOnServer = args[0].Value;

    // Convert the file size that came in as a hex string to a size_t
    size_t fileSize;
    std::stringstream fileSizeStream;
    fileSizeStream << std::hex << args[1].Value;
    fileSizeStream >> fileSize;

    // Tell the client to start sending the file content
    Send("204- send binary data\r\n");

    // Create the file on the server
    std::ofstream outFile;
    outFile.open(pathOnServer, std::ofstream::binary);

    if (outFile.fail())
    {
        Send("400- Couldn't create file: " + pathOnServer + "\r\n");
        return;
    }

    int bytes = 0;
    size_t totalBytes = 0;
    char contentBuffer[s_PacketSize] = { 0 };

    // Receive the content of the file from the client and write it to the file on the server
    while (totalBytes < fileSize)
    {
        // Wait for a little to line up with how long the client waits between each packet
        std::this_thread::sleep_for(20ms);

        if ((bytes = recv(m_ClientSocket, contentBuffer, sizeof(contentBuffer), 0)) == SOCKET_ERROR)
        {
            outFile.close();
            Send("400- Couldn't receive bytes\r\n");
            return;
        }

        totalBytes += static_cast<size_t>(bytes);

        outFile.write(contentBuffer, bytes);

        // Reset contentBuffer
        memset(contentBuffer, 0, sizeof(contentBuffer));
    }

    outFile.close();

    Send("200- OK\r\n");
}

void TestServer::DeleteFile(const std::vector<Arg> &args)
{
    if (args.size() < 1 || args.size() > 2)
    {
        Send("400- wrong number of arguments provided, one or two expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    // The client will create paths using backslashes (\) because that's what the Xbox 360 uses.
    // For the tests we need to forward slashes (/) on POSIX systems so we patch them here.
#ifndef _WIN32
    std::string filePath = args[0].Value;
    std::replace(filePath.begin(), filePath.end(), '\\', '/');
#else
    const std::string &filePath = args[0].Value;
#endif

    if (!fs::exists(filePath))
    {
        Send("404- " + filePath + " not found\r\n");
        return;
    }

    Send("200- OK\r\n");
}

void TestServer::CreateDirectory(const std::vector<Arg> &args)
{
    if (args.size() != 1)
    {
        Send("400- wrong number of arguments provided, one expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    fs::path directoryPath = args[0].Value;
    fs::path parentPath = directoryPath.parent_path();

    if (fs::exists(directoryPath))
    {
        Send("400- " + args[0].Value + " already exists\r\n");
        return;
    }

    if (!fs::exists(parentPath) || !fs::is_directory(parentPath))
    {
        Send("400- Invalid path: " + args[0].Value + "\r\n");
        return;
    }

    Send("200- OK\r\n");
}

void TestServer::RenameFile(const std::vector<Arg> &args)
{
    if (args.size() != 2)
    {
        Send("400- wrong number of arguments provided, two expected\r\n");
        return;
    }

    if (args[0].Name != "name")
    {
        Send("400- argument 'name' not found\r\n");
        return;
    }

    if (args[1].Name != "newname")
    {
        Send("400- argument 'newname' not found\r\n");
        return;
    }

    const std::string &oldFilePath = args[0].Value;
    const std::string &newFilePath = args[1].Value;

    if (!fs::exists(oldFilePath))
    {
        Send("404- " + oldFilePath + " not found\r\n");
        return;
    }

    if (fs::exists(newFilePath))
    {
        Send("400- " + newFilePath + " already exists\r\n");
        return;
    }

    Send("200- OK\r\n");
}

bool TestServer::InitServerSocket()
{
    sockaddr_in address;
    memset(&address, 0, sizeof(sockaddr_in));
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
    setToNonBlockingResult = ioctlsocket(m_ClientSocket, FIONBIO, &nonBlocking);
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

bool TestServer::Send(const std::string &response)
{
    return Send(response.c_str(), response.size());
}

bool TestServer::Send(const char *buffer, size_t length)
{
    int sent = 0;
    size_t totalSent = 0;

    // Send data in chunks of s_PacketSize bytes at most to simulate packets
    for (size_t i = 0; i < length; i += s_PacketSize)
    {
        size_t toSend = std::min<size_t>(s_PacketSize, length - totalSent);
        if ((sent = send(m_ClientSocket, buffer + i, static_cast<int>(toSend), 0)) == SOCKET_ERROR)
        {
            Shutdown();
            return false;
        }

        totalSent += static_cast<size_t>(sent);
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
    std::vector<std::string> tokens = Utils::StringSplit(commandStringCopy, " ");

    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (i == 0)
        {
            command.Name = tokens[i];
            continue;
        }

        size_t equalSignIndex = tokens[i].find('=');
        if (equalSignIndex == std::string::npos)
        {
            // If there is no equal sign, it means tokens[i] is an argument with no value like 'dir'
            // (e.g. delete name="Hdd:\Path\To\Dir" dir)
            command.Args.emplace_back(tokens[i], "", Arg::ArgType::String);
            continue;
        }

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
