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

    std::array<std::string, 2> driveNames = { "HDD", "Z" };
    std::stringstream driveResponse;
    driveResponse << "202- multiline response follows\r\n";

    for (auto &driveName : driveNames)
        driveResponse << "drivename=\"" << driveName << "\"\r\n";

    driveResponse << ".\r\n";

    if (!Send(driveResponse.str()))
        return;

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

    const char *response =
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

    std::string directory = xexPath.substr(0, xexPath.find_last_of('\\') + 1);

    if (!CheckRequest("magicboot title=\"" + xexPath + "\" directory=\"" + directory + "\"\r\n"))
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

    std::ifstream inFile;
    inFile.open(pathOnServer, std::ifstream::binary);

    if (inFile.fail())
        return;

    std::string responseHeader = "203- binary response follows\r\n";

    inFile.seekg(0, inFile.end);
    int fileSize = inFile.tellg();
    inFile.seekg(0, inFile.beg);

    std::vector<char> response;
    response.reserve(responseHeader.size() + sizeof(fileSize) + fileSize);

    response.insert(response.end(), responseHeader.begin(), responseHeader.end());
    response.insert(response.end(), reinterpret_cast<const char *>(&fileSize), reinterpret_cast<const char *>(&fileSize) + sizeof(fileSize));

    char buffer[256] = { 0 };

    while (!inFile.eof())
    {
        inFile.read(buffer, sizeof(buffer));

        response.insert(response.end(), buffer, buffer + inFile.gcount());

        memset(buffer, 0, sizeof(buffer));
    }

    if (!Send(response.data(), response.size()))
        return;

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

    for (size_t i = 0; i < length; i += chunkSize)
    {
        int toSend = std::min(chunkSize, length - totalSent);
        if ((sent = send(s_ClientSocket, buffer + i, toSend, 0)) == SOCKET_ERROR)
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
