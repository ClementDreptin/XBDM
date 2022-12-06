#include "pch.h"
#include "Console.h"

#include "Utils.h"

#define FILETIME_TO_TIMET(time) ((time) / 10000000LL - 11644473600LL)
#define TIMET_TO_FILETIME(time) ((time) * 10000000LL + 116444736000000000LL)

namespace XBDM
{

using namespace std::chrono_literals;

Console::Console()
    : m_Socket(INVALID_SOCKET)
{
}

Console::Console(const std::string &ipAddress)
    : m_IpAddress(ipAddress), m_Socket(INVALID_SOCKET)
{
}

Console::~Console()
{
    CloseConnection();
}

bool Console::OpenConnection()
{
    m_Connected = false;
    addrinfo hints;
    addrinfo *addrInfo;
    memset(&hints, 0, sizeof(hints));

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;
#endif

    if (getaddrinfo(m_IpAddress.c_str(), "730", &hints, &addrInfo) != 0)
    {
        CloseConnection();
        return false;
    }

    m_Socket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
    if (m_Socket < 0)
    {
        CloseConnection();
        return false;
    }

    // The only way I found to reduce the response time on Linux is to set the timeout
    // to 10 000 microseconds but it makes the response time extremely long on Windows.
    // It's far from being an optimal solution but it works.
#ifdef _WIN32
    timeval tv = { 5, 0 };
#else
    timeval tv = { 0, 10000 };
#endif
    setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(timeval));

    if (connect(m_Socket, addrInfo->ai_addr, static_cast<int>(addrInfo->ai_addrlen)) == SOCKET_ERROR)
    {
        CloseConnection();
        return false;
    }

    if (m_Socket == INVALID_SOCKET)
    {
        CloseConnection();
        return false;
    }

    std::string response = Receive();
    if (response != "201- connected\r\n")
        return false;

    m_Connected = true;

    return true;
}

#ifdef _WIN32
    #define CloseSocket(socket) closesocket(socket)
#else
    #define CloseSocket(socket) close(socket)
#endif

void Console::CloseConnection()
{
    if (m_Socket != INVALID_SOCKET)
    {
        CloseSocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    m_Connected = false;
}

const std::string &Console::GetName()
{
    // If the console name has already been requested, just sent what was cached last time
    if (!m_Name.empty())
        return m_Name;

    SendCommand("dbgname");
    std::string response = Receive();

    if (response.size() <= 5)
        throw std::runtime_error("Response length too short");

    if (response[0] != '2')
        throw std::runtime_error("Couldn't get the console name");

    // The response is "200- <name>\r\n"
    // We don't want the first 5 characters ("200- ") nor the last 2 ("\r\n") so the console name
    // starts at index 5 and is of size response.size() - the first 5 characters - the last 2 characters
    m_Name = response.substr(5, response.size() - 7);

    return m_Name;
}

std::vector<Drive> Console::GetDrives()
{
    std::vector<Drive> drives;

    SendCommand("drivelist");
    std::string listResponse = Receive();

    if (listResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (listResponse[0] != '2')
        throw std::runtime_error("Couldn't get the drive list");

    std::vector<std::string> lines = Utils::String::Split(listResponse, "\r\n");

    // Delete the first line because it doesn't contain any info about the drives
    lines.erase(lines.begin());

    for (auto &line : lines)
    {
        if (line.empty() || line == ".")
            continue;

        std::string driveName;

        try
        {
            driveName = GetStringProperty(line, "drivename");
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Unable to get drive name");
        }

        Drive drive;
        drive.Name = driveName += ':';

        // Get the friendly name for volume, these are from neighborhood
        if (drive.Name == "DEVKIT:" || drive.Name == "E:")
            drive.FriendlyName = "Game Development Volume";
        else if (drive.Name == "HDD:")
            drive.FriendlyName = "Retail Hard Drive Emulation";
        else if (drive.Name == "Y:")
            drive.FriendlyName = "Xbox360 Dashboard Volume";
        else if (drive.Name == "Z:")
            drive.FriendlyName = "Devkit Drive";
        else if (drive.Name == "D:" || drive.Name == "GAME:")
            drive.FriendlyName = "Active Title Media";
        else
            drive.FriendlyName = "Volume";

        // Get the free space for each drive
        SendCommand("drivefreespace name=\"" + drive.Name + "\\\"");
        std::string spaceResponse = Receive();

        // Create 64-bit unsigned integers and making the value of 'XXXhi' properties
        // their upper 32 bits and the value of 'XXXlo' properties their lower 32 bits.
        try
        {
            drive.FreeBytesAvailable = static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "freetocallerhi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "freetocallerlo"));
            drive.TotalBytes = static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "totalbyteshi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "totalbyteslo"));
            drive.TotalFreeBytes = static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "totalfreebyteshi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(spaceResponse, "totalfreebyteslo"));
            drive.TotalUsedBytes = drive.TotalBytes - drive.FreeBytesAvailable;

            drives.push_back(drive);
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Unable to fetch some data about the drives");
        }
    }

    return drives;
}

std::set<File> Console::GetDirectoryContents(const std::string &directoryPath)
{
    std::set<File> files;

    SendCommand("dirlist name=\"" + directoryPath + "\"");
    std::string contentResponse = Receive();

    if (contentResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (contentResponse[0] != '2')
        throw std::invalid_argument("Invalid directory path: " + directoryPath);

    std::vector<std::string> lines = Utils::String::Split(contentResponse, "\r\n");

    // Delete the first line because it doesn't contain any info about the files
    lines.erase(lines.begin());

    for (auto &line : lines)
    {
        if (line.empty() || line == ".")
            continue;

        std::string fileName;

        try
        {
            fileName = GetStringProperty(line, "name");
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Unable get file name");
        }

        File file;

        // Create an 8-byte integer and making the value of sizehi
        // its upper 4 bytes and the value of sizelo its lower 4 bytes.
        try
        {
            file.Name = fileName;
            file.Size = static_cast<uint64_t>(GetIntegerProperty(line, "sizehi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(line, "sizelo"));
            file.IsDirectory = Utils::String::EndsWith(line, " directory");

            std::filesystem::path filePath(file.Name);
            file.IsXex = filePath.extension() == ".xex";

            file.CreationDate = FILETIME_TO_TIMET(static_cast<uint64_t>(GetIntegerProperty(line, "createhi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(line, "createlo")));
            file.ModificationDate = FILETIME_TO_TIMET(static_cast<uint64_t>(GetIntegerProperty(line, "changehi")) << 32 | static_cast<uint64_t>(GetIntegerProperty(line, "changelo")));

            files.emplace(file);
        }
        catch (const std::exception &)
        {
            throw std::runtime_error("Unable to fetch some data about the files");
        }
    }

    return files;
}

void Console::LaunchXex(const std::string &xexPath)
{
    std::string directory = xexPath.substr(0, xexPath.find_last_of('\\') + 1);

    SendCommand("magicboot title=\"" + xexPath + "\" directory=\"" + directory + "\"");

    // Nothing should be received but just in case
    ClearSocket();
}

std::string Console::GetActiveTitle()
{
    SendCommand("xbeinfo running");
    std::string activeTitleResponse = Receive();

    if (activeTitleResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (activeTitleResponse[0] != '2')
        throw std::runtime_error("Couldn't get the active title");

    return GetStringProperty(activeTitleResponse, "name");
}

std::string Console::GetType()
{
    SendCommand("consoletype");
    std::string consoleTypeResponse = Receive();

    if (consoleTypeResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (consoleTypeResponse[0] != '2')
        throw std::runtime_error("Couldn't get the console type");

    // The response is "200- <type>\r\n"
    // We don't want the first 5 characters ("200- ") nor the last 2 ("\r\n") so the console type
    // starts at index 5 and is of size consoleTypeResponse.size() - the first 5 characters - the last 2 characters
    return consoleTypeResponse.substr(5, consoleTypeResponse.size() - 7);
}

void Console::SynchronizeTime()
{
    time_t now = std::time(nullptr);

    SetTime(now);
}

void Console::SetTime(time_t time)
{
    uint64_t filetime = TIMET_TO_FILETIME(time);
    uint32_t clockHigh = filetime >> 32;
    uint32_t clockLow = filetime & 0xFFFFFFFF;

    // The command could include a tz argument for the timezone but finding a cross-platform way of
    // getting the current system timezone is not easy. The time sent to the console will just be
    // interpreted as a time on the timezone it's currently on
    std::stringstream command;
    command << "setsystime ";
    command << "clockhi=0x" << std::hex << clockHigh << ' ';
    command << "clocklo=0x" << std::hex << clockLow;

    SendCommand(command.str());
    std::string setTimeResponse = Receive();

    if (setTimeResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (setTimeResponse[0] != '2')
        throw std::runtime_error("Couldn't set the system time");
}

void Console::Reboot()
{
    SendCommand("magicboot COLD");

    std::string rebootResponse = Receive();

    if (rebootResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (rebootResponse[0] != '2')
        throw std::runtime_error("Couldn't cold reboot the console");
}

void Console::GoToDashboard()
{
    SendCommand("magicboot");

    // I don't know why but going to the dashboard seems to take longer than other operations
    // and the console takes longer to respond
    std::this_thread::sleep_for(20ms);

    std::string goToDashboardResponse = Receive();

    if (goToDashboardResponse.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (goToDashboardResponse[0] != '2')
        throw std::runtime_error("Couldn't go to the dashboard");
}

void Console::RestartActiveTitle()
{
    std::string activeTitlePath = GetActiveTitle();

    LaunchXex(activeTitlePath);
}

void Console::ReceiveFile(const std::string &remotePath, const std::string &localPath)
{
    char headerBuffer[40] = { 0 };
    std::string header = "203- binary response follows\r\n";

    SendCommand("getfile name=\"" + remotePath + "\"");

    // Receive the header
    if (recv(m_Socket, headerBuffer, static_cast<int>(header.size()), 0) == SOCKET_ERROR)
        throw std::runtime_error("Couldn't receive the response header");

    if (strlen(headerBuffer) <= 4)
    {
        ClearSocket();
        throw std::runtime_error("Response length too short");
    }

    if (headerBuffer[0] != '2')
    {
        ClearSocket();
        throw std::invalid_argument("Invalid remote path: " + remotePath);
    }

    if (header != headerBuffer)
    {
        ClearSocket();
        throw std::runtime_error("Couldn't receive the file");
    }

    // Receive the file size (4-byte integer sent right after the header)
    int fileSize = 0;
    if (recv(m_Socket, reinterpret_cast<char *>(&fileSize), sizeof(int), 0) == SOCKET_ERROR)
    {
        ClearSocket();
        throw std::runtime_error("Couldn't receive the file size");
    }

    int bytes = 0;
    int totalBytes = 0;
    char contentBuffer[s_PacketSize] = { 0 };

    std::ofstream outFile;
    outFile.open(localPath, std::ofstream::binary);

    if (outFile.fail())
    {
        ClearSocket();
        throw std::runtime_error("Invalid local path: " + localPath);
    }

    // Receive the content of the file from the server and write it to the file on the client
    while (totalBytes < fileSize)
    {
        if ((bytes = recv(m_Socket, contentBuffer, sizeof(contentBuffer), 0)) == SOCKET_ERROR)
        {
            outFile.close();
            throw std::runtime_error("Couldn't receive the file");
        }

        totalBytes += bytes;

        outFile.write(contentBuffer, bytes);

        // Reset contentBuffer
        memset(contentBuffer, 0, sizeof(contentBuffer));
    }

    // Give write permission to the group (only effective on POSIX systems)
    std::filesystem::permissions(localPath, std::filesystem::perms::group_write, std::filesystem::perm_options::add);

    outFile.close();

    // Clean the socket just in case there is more to receive
    ClearSocket();
}

void Console::SendFile(const std::string &remotePath, const std::string &localPath)
{
    std::ifstream file;
    file.open(localPath, std::ifstream::binary);

    if (file.fail())
        throw std::runtime_error("Invalid local path: " + localPath);

    // Get the file size
    file.seekg(0, file.end);
    size_t fileSize = file.tellg();
    file.seekg(0, file.beg);

    // Create the command
    std::stringstream command;
    command << "sendfile name=\"" << remotePath << "\" ";
    command << "length=0x" << std::hex << fileSize << "\r\n";

    SendCommand(command.str());

    char headerBuffer[40] = { 0 };
    std::string header = "204- send binary data\r\n";

    // Receive the header
    if (recv(m_Socket, headerBuffer, static_cast<int>(header.size()), 0) == SOCKET_ERROR)
        throw std::runtime_error("Couldn't receive the response header");

    if (strlen(headerBuffer) <= 4)
    {
        ClearSocket();
        file.close();
        throw std::runtime_error("Response length too short");
    }

    if (headerBuffer[0] != '2')
    {
        ClearSocket();
        file.close();
        throw std::invalid_argument("Invalid remote path: " + remotePath);
    }

    if (header != headerBuffer)
    {
        ClearSocket();
        file.close();
        throw std::runtime_error("Couldn't send the file");
    }

    char contentBuffer[s_PacketSize] = { 0 };

    while (!file.eof())
    {
        file.read(contentBuffer, sizeof(contentBuffer));

        if (send(m_Socket, contentBuffer, static_cast<int>(file.gcount()), 0) == SOCKET_ERROR)
        {
            CloseConnection();
            file.close();
            throw std::runtime_error("Couldn't send the file");
        }

        // Reset contentBuffer
        memset(contentBuffer, 0, sizeof(contentBuffer));

        // Give the Xbox 360 some time to process what was sent...
        std::this_thread::sleep_for(20ms);
    }

    file.close();

    // Receive the "200- OK\r\n" message the Xbox sends when the entire file is received
    ClearSocket();
}

void Console::DeleteFile(const std::string &path, bool isDirectory)
{
    if (isDirectory)
    {
        std::set<File> files = GetDirectoryContents(path);

        for (auto &file : files)
            DeleteFile(path + '\\' + file.Name, file.IsDirectory);
    }

    SendCommand("delete name=\"" + path + '\"' + (isDirectory ? " dir" : ""));
    std::string response = Receive();

    if (response.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (response[0] != '2')
        throw std::runtime_error("Couldn't delete " + path);
}

void Console::CreateDirectory(const std::string &path)
{
    SendCommand("mkdir name=\"" + path + "\"");
    std::string response = Receive();

    if (response.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (response.substr(0, 3) == "410")
        throw std::invalid_argument("A file or directory with the name \"" + path + "\" already exists");

    if (response[0] != '2')
        throw std::runtime_error("Couldn't create directory " + path);
}

void Console::RenameFile(const std::string &oldName, const std::string &newName)
{
    SendCommand("rename name=\"" + oldName + "\" newname=\"" + newName + "\"");
    std::string response = Receive();

    if (response.size() <= 4)
        throw std::runtime_error("Response length too short");

    if (response[0] != '2')
        throw std::runtime_error("Couldn't rename " + oldName);
}

std::string Console::Receive()
{
    // We allocate s_PacketSize + 1 but will only receive s_PacketSize at a time to guarantee
    // that the last byte is always 0 so that strings stay null terminated
    char buffer[s_PacketSize + 1] = { 0 };
    std::stringstream stream;

    while (recv(m_Socket, buffer, s_PacketSize, 0) > 0)
    {
        // Give the Xbox 360 some time to notice we received something...
        std::this_thread::sleep_for(10ms);
        stream << buffer;

        // Reset buffer
        memset(buffer, 0, s_PacketSize);
    }

    std::string result = stream.str();

    // Sometimes the response ends but we still receive some stuff.
    // If that's the case, we want to remove everything after ".\r\n".
    size_t endPos = result.find(".\r\n");
    if (endPos != std::string::npos && result.substr(result.size() - 3, 3) != ".\r\n")
        result = result.substr(0, endPos);

    return result;
}

void Console::SendCommand(const std::string &command)
{
    std::string fullCommand = command + "\r\n";
    if (send(m_Socket, fullCommand.c_str(), static_cast<int>(fullCommand.size()), 0) == SOCKET_ERROR)
        CloseConnection();

    // Give the Xbox 360 some time to process the command and create a response...
    std::this_thread::sleep_for(10ms);
}

uint32_t Console::GetIntegerProperty(const std::string &line, const std::string &propertyName, bool hex)
{
    if (line.find(propertyName) == std::string::npos)
        throw std::runtime_error("Property '" + propertyName + "' not found");

    // All integer properties are like this: NAME=VALUE
    size_t startIndex = line.find(propertyName) + propertyName.size() + 1;
    size_t spaceIndex = line.find(' ', startIndex);
    size_t crIndex = line.find('\r', startIndex);
    size_t endIndex = (spaceIndex != std::string::npos) ? spaceIndex : crIndex;

    uint32_t toReturn;
    if (hex)
        std::istringstream(line.substr(startIndex, endIndex - startIndex)) >> std::hex >> toReturn;
    else
        std::istringstream(line.substr(startIndex, endIndex - startIndex)) >> toReturn;

    return toReturn;
}

std::string Console::GetStringProperty(const std::string &line, const std::string &propertyName)
{
    if (line.find(propertyName) == std::string::npos)
        throw std::runtime_error("Property '" + propertyName + "' not found");

    // All string properties are like this: NAME="VALUE"
    size_t startIndex = line.find(propertyName) + propertyName.size() + 2;
    size_t endIndex = line.find('"', startIndex);

    std::string toReturn = line.substr(startIndex, endIndex - startIndex);

    return toReturn;
}

void Console::ClearSocket()
{
    char buffer[s_PacketSize] = { 0 };
    while (recv(m_Socket, buffer, s_PacketSize, 0) > 0)
    {
    }
}

}
