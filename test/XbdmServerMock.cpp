#include "XbdmServerMock.h"

#include <sstream>
#include <array>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

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

    if (!Send("201"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::ConnectResponse()
{
    if (!StartClientConnection())
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::ConsoleNameResponse()
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("dbgname\r\n"))
        return;

    if (!Send("200- TestXDK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::DriveResponse()
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("drivelist\r\n"))
        return;

    // Build the response with the drive names and send it
    std::array<std::string, 2> driveNames = { "HDD", "Z" };
    std::stringstream response;
    response << "202- multiline response follows\r\n";

    for (auto &driveName : driveNames)
        response << "drivename=\"" << driveName << "\"\r\n";

    response << ".\r\n";

    if (!Send(response.str()))
        return;

    // For each drive, expect a request to get the space information and send some dummy values
    for (auto &driveName : driveNames)
    {
        if (!CheckRequest("drivefreespace name=\"" + driveName + ":\\\"\r\n"))
            return;

        if (!Send("200- freetocallerhi=0x0 freetocallerlo=0xa totalbyteshi=0x0 totalbyteslo=0xb totalfreebyteshi=0x0 totalfreebyteslo=0xc\r\n"))
            return;
    }

    ProcessShutdownRequest();
}

void XbdmServerMock::DirectoryContentsResponse(const std::string &directoryPath)
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("dirlist name=\"" + directoryPath + "\"\r\n"))
        return;

    std::string response =
        "202- multiline response follows\r\n"
        "name=\"dir1\" sizehi=0x0 sizelo=0x0 directory\r\n"
        "name=\"file1.txt\" sizehi=0x0 sizelo=0xa\r\n"
        "name=\"dir2\" sizehi=0x0 sizelo=0x0 directory\r\n"
        "name=\"file2.xex\" sizehi=0x0 sizelo=0xb\r\n";

    if (!Send(response))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::MagicBoot(const std::string &xexPath)
{
    if (!StartClientConnection())
        return;

    std::string directoryPath = xexPath.substr(0, xexPath.find_last_of('\\') + 1);

    if (!CheckRequest("magicboot title=\"" + xexPath + "\" directory=\"" + directoryPath + "\"\r\n"))
        return;

    if (!Send("200- OK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::ReceiveFile(const std::string &pathOnServer)
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("getfile name=\"" + pathOnServer + "\"\r\n"))
        return;

    // Open the requested file
    std::ifstream inFile;
    inFile.open(pathOnServer, std::ifstream::binary);

    if (inFile.fail())
        return;

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
    char buffer[256] = { 0 };

    while (!inFile.eof())
    {
        inFile.read(buffer, sizeof(buffer));

        // Append the current chunk to the response
        response.insert(response.end(), buffer, buffer + inFile.gcount());

        // Reset the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    if (!Send(response.data(), response.size()))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::SendFile(const std::string &pathOnServer, const std::string &pathOnClient)
{
    if (!StartClientConnection())
        return;

    std::ifstream file;
    file.open(pathOnClient, std::ifstream::binary);

    // Get the size of the file the client is supposed to send
    file.seekg(0, file.end);
    size_t fileSize = file.tellg();
    file.seekg(0, file.beg);

    // Create the command the client is supposed to send
    std::stringstream command;
    command << "sendfile name=\"" << pathOnServer << "\" ";
    command << "length=0x" << std::hex << fileSize << "\r\n\r\n";

    if (!CheckRequest(command.str()))
        return;

    // Tell the client to start sending the file content
    if (!Send("204- send binary data\r\n"))
        return;

    // Create the file on the server
    std::ofstream outFile;
    outFile.open(pathOnServer, std::ofstream::binary);

    if (outFile.fail())
        return;

    int bytes = 0;
    int totalBytes = 0;
    char contentBuffer[256] = { 0 };

    // Receive the content of the file from the client and write it to the file on the server
    while (totalBytes < fileSize)
    {
        if ((bytes = recv(s_ClientSocket, contentBuffer, sizeof(contentBuffer), 0)) == SOCKET_ERROR)
        {
            outFile.close();
            return;
        }

        totalBytes += bytes;

        outFile.write(contentBuffer, bytes);

        // Reset contentBuffer
        memset(contentBuffer, 0, sizeof(contentBuffer));
    }

    outFile.close();

    if (!Send("200- OK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::DeleteFile(const std::string &path, bool isDirectory)
{
    if (!StartClientConnection())
        return;

    if (isDirectory)
    {
        if (!CheckRequest("dirlist name=\"" + path + "\"\r\n"))
            return;

        std::stringstream response;
        std::array<std::string, 2> files = { "file1.txt", "file2.txt" };
        response << "202- multiline response follows\r\n";

        for (auto &file : files)
            response << "name=\"" << file << "\" sizehi=0x0 sizelo=0xa\r\n";

        if (!Send(response.str()))
            return;

        for (auto &file : files)
        {
            if (!CheckRequest("delete name=\"" + path + '\\' + file + "\"\r\n"))
                return;

            if (!Send("200- OK\r\n"))
                return;
        }
    }

    if (!CheckRequest("delete name=\"" + path + '\"' + (isDirectory ? " dir" : "") + "\r\n"))
        return;

    if (!Send("200- OK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::CreateDirectory(const std::string &path)
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("mkdir name=\"" + path + "\"\r\n"))
        return;

    if (!Send("200- OK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::RenameFile(const std::string &oldName, const std::string &newName)
{
    if (!StartClientConnection())
        return;

    if (!CheckRequest("rename name=\"" + oldName + "\" newname=\"" + newName + "\"\r\n"))
        return;

    if (!Send("200- OK\r\n"))
        return;

    ProcessShutdownRequest();
}

void XbdmServerMock::WaitForServerToListen()
{
    // Wait to get ownership of the mutex
    std::unique_lock<std::mutex> lock(s_Mutex);

    // Wait for s_Cond to be notified that the server started listening with a timeout of one second
    s_Cond.wait_for(lock, std::chrono::seconds(1), []() { return s_Listening; });
}

void XbdmServerMock::SendRequestToShutdownServer()
{
    SignalListening(false);
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

    SignalListening(true);

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

    return Send("201- connected\r\n");
}

bool XbdmServerMock::Send(const std::string &response)
{
    return Send(response.c_str(), response.size());
}

bool XbdmServerMock::Send(const char *buffer, size_t length)
{
    const size_t chunkSize = 16;
    int sent = 0;
    int totalSent = 0;

    // Send data in chunks of 16 bytes at most to simulate packets
    for (size_t i = 0; i < length; i += chunkSize)
    {
        size_t toSend = std::min<size_t>(chunkSize, length - totalSent);
        if ((sent = send(s_ClientSocket, buffer + i, static_cast<int>(toSend), 0)) == SOCKET_ERROR)
        {
            Shutdown();
            return false;
        }

        totalSent += sent;
    }

    return true;
}

bool XbdmServerMock::CheckRequest(const std::string &expectedCommand)
{
    std::string result;
    char buffer[1024] = { 0 };

    if (recv(s_ClientSocket, buffer, sizeof(buffer) - 1, 0) > 0)
        result += buffer;

    if (result != expectedCommand)
    {
        Shutdown();
        return false;
    }

    return true;
}

void XbdmServerMock::SignalListening(bool isListening)
{
    // Wait to get ownership of the mutex
    std::lock_guard<std::mutex> lock(s_Mutex);

    // Notify s_Cond that the server started or stopped listening
    s_Listening = isListening;
    s_Cond.notify_all();
}

void XbdmServerMock::ProcessShutdownRequest()
{
    // Wait to get ownership of the mutex
    std::unique_lock<std::mutex> lock(s_Mutex);

    // Wait for s_Cond to be notified that the server stopped listening with a timeout of one second
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
        SignalListening(false);

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
