#include "pch.h"
#include "Console.h"


namespace XBDM
{
    Console::Console()
        : m_Socket(INVALID_SOCKET) {}

    Console::Console(const std::string &ipAddress)
        : m_IpAddress(ipAddress), m_Socket(INVALID_SOCKET) {}

    bool Console::OpenConnection()
    {
        m_Connected = false;
        addrinfo hints;
        addrinfo *addrInfo;
        ZeroMemory(&hints, sizeof(hints));

#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;
#endif

        if (getaddrinfo(m_IpAddress.c_str(), "730", &hints, &addrInfo) != 0)
        {
            CleanupSocket();
            return false;
        }

        m_Socket = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
        if (m_Socket < 0)
        {
            CleanupSocket();
            return false;
        }

        /**
         * The only way I found to reduce the response time on Linux is to set the timeout
         * to 10 000 microseconds but it makes the response time extremely long on Windows.
         * It's far from being an optimal solution but it works.
         */
#ifdef _WIN32
        timeval tv = { 5, 0 };
#else
        timeval tv = { 0, 10000 };
#endif
        setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(timeval));

        if (connect(m_Socket, addrInfo->ai_addr, static_cast<int>(addrInfo->ai_addrlen)) == SOCKET_ERROR)
        {
            CloseSocket();
            return false;
        }

        if (m_Socket == INVALID_SOCKET)
        {
            CleanupSocket();
            return false;
        }

        if (Receive() != "201- connected\r\n")
            return false;

        m_Connected = true;
        return true;
    }

    bool Console::CloseConnection()
    {
        CloseSocket();
        CleanupSocket();

        m_Connected = false;
        return m_Socket == INVALID_SOCKET;
    }

    std::string Console::GetName()
    {
        SendCommand("dbgname");
        std::string response = Receive();

        if (response.length() <= 5)
            throw std::runtime_error("Response length too short");

        if (response[0] != '2')
            throw std::runtime_error("Couldn't get the console name");

        std::string result = response.substr(5, response.length() - 5);
        return result;
    }

    std::vector<Drive> Console::GetDrives()
    {
        std::vector<Drive> drives;

        SendCommand("drivelist");
        std::string listResponse = Receive();

        if (listResponse.length() <= 4)
            throw std::runtime_error("Response length too short");

        if (listResponse[0] != '2')
            throw std::runtime_error("Couldn't get the drive list");

        std::vector<std::string> lines = SplitResponse(listResponse, "\r\n");

        // We delete the first line because it doesn't contain any info about the drives
        lines.erase(lines.begin());

        for (auto &line : lines)
        {
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
            drive.Name = driveName;

            // Gets the friendly name for volume, these are from neighborhood
            if (drive.Name == "DEVKIT" || drive.Name == "E")
                drive.FriendlyName = "Game Development Volume (" + drive.Name + ":)";
            else if (drive.Name == "HDD")
                drive.FriendlyName = "Retail Hard Drive Emulation (" + drive.Name + ":)";
            else if (drive.Name == "Y")
                drive.FriendlyName = "Xbox360 Dashboard Volume (" + drive.Name + ":)";
            else if (drive.Name == "Z")
                drive.FriendlyName = "Devkit Drive (" + drive.Name + ":)";
            else if (drive.Name == "D" || drive.Name == "GAME")
                drive.FriendlyName = "Active Title Media (" + drive.Name + ":)";
            else
                drive.FriendlyName = "Volume (" + drive.Name + ":)";

            // Gets the free space for each drive
            SendCommand("drivefreespace name=\"" + drive.Name + ":\\\"");
            std::string spaceResponse = Receive();

            /**
             * Creating 64-bit unsigned integers and making the value of 'XXXhi' properties
             * their upper 32 bits and the value of 'XXXlo' properties their lower 32 bits.
             */
            try
            {
                drive.FreeBytesAvailable = static_cast<UINT64>(GetIntegerProperty(spaceResponse, "freetocallerhi")) << 32 | static_cast<UINT64>(GetIntegerProperty(spaceResponse, "freetocallerlo"));
                drive.TotalBytes = static_cast<UINT64>(GetIntegerProperty(spaceResponse, "totalbyteshi")) << 32 | static_cast<UINT64>(GetIntegerProperty(spaceResponse, "totalbyteslo"));
                drive.TotalFreeBytes = static_cast<UINT64>(GetIntegerProperty(spaceResponse, "totalfreebyteshi")) << 32 | static_cast<UINT64>(GetIntegerProperty(spaceResponse, "totalfreebyteslo"));
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

        if (contentResponse.length() <= 4)
            throw std::runtime_error("Response length too short");

        if (contentResponse[0] != '2')
            throw std::invalid_argument("Invalid directory path: " + directoryPath);

        std::vector<std::string> lines = SplitResponse(contentResponse, "\r\n");

        // We delete the first line because it doesn't contain any info about the files
        lines.erase(lines.begin());

        for (auto &line : lines)
        {
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

            /**
             * Creating an 8-byte integer and making the value of sizehi
             * its upper 4 bytes and the value of sizelo its lower 4 bytes.
             */
            try
            {
                file.Name = fileName;
                file.Size = static_cast<UINT64>(GetIntegerProperty(line, "sizehi")) << 32 | static_cast<UINT64>(GetIntegerProperty(line, "sizelo"));
                file.IsDirectory = EndsWith(line, " directory");

                std::filesystem::path filePath(file.Name);
                file.IsXEX = filePath.extension() == ".xex";

                files.emplace(file);
            }
            catch (const std::exception &)
            {
                throw std::runtime_error("Unable to fetch some data about the files");
            }
        }

        return files;
    }

    void Console::LaunchXEX(const std::string &xexPath)
    {
        std::string directory = xexPath.substr(0, xexPath.find_last_of('\\') + 1);

        SendCommand("magicboot title=\"" + xexPath + "\" directory=\"" + directory + "\"");
        std::string response = Receive();
    }

    void Console::ReceiveFile(const std::string &remotePath, const std::string &localPath)
    {
        char headerBuffer[40] = { 0 };
        std::string header = "203- binary response follows\r\n";

        SendCommand("getfile name=\"" + remotePath + "\"");

        // Receiving the header
        if (recv(m_Socket, headerBuffer, static_cast<int>(header.length()), 0) == SOCKET_ERROR)
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

        // Receiving the file size (4-byte integer sent right after the header)
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

        while (totalBytes < fileSize)
        {
            if ((bytes = recv(m_Socket, contentBuffer, sizeof(contentBuffer), 0)) == SOCKET_ERROR)
                throw std::runtime_error("Couldn't receive the file");

            totalBytes += bytes;

            outFile.write(contentBuffer, bytes);

            // Resetting contentBuffer
            ZeroMemory(contentBuffer, sizeof(contentBuffer));
        }

        outFile.close();

        // Cleaning the socket just in case there is more to receive
        ClearSocket();
    }

    void Console::SendFile(const std::string &remotePath, const std::string &localPath)
    {
        std::ifstream file;
        file.open(localPath, std::ifstream::binary);

        if (file.fail())
            throw std::runtime_error("Invalid local path: " + localPath);

        // Getting the file size
        file.seekg(0, file.end);
        size_t fileSize = file.tellg();
        file.seekg(0, file.beg);

        // Creating the command
        std::stringstream command;
        command << "sendfile name=\"" << remotePath << "\" ";
        command << "length=0x" << std::hex << fileSize << "\r\n";

        char headerBuffer[40] = { 0 };
        std::string header = "204- send binary data\r\n";

        SendCommand(command.str());

        // Receiving the header
        if (recv(m_Socket, headerBuffer, static_cast<int>(header.length()), 0) == SOCKET_ERROR)
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
                CloseSocket();
                CleanupSocket();
                file.close();
                throw std::runtime_error("Couldn't send the file");
            }

            // Resetting contentBuffer
            ZeroMemory(contentBuffer, sizeof(contentBuffer));

            // Giving the Xbox 360 some time to process what was sent...
            SleepFor(10);
        }

        file.close();

        // Receiving the "200- OK\r\n" message the Xbox sends when the entire file is received
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

        if (response.length() <= 4)
            throw std::runtime_error("Response length too short");

        if (response[0] != '2')
            throw std::runtime_error("Couldn't delete " + path);
    }

    void Console::CreateDirectory(const std::string &path)
    {
        SendCommand("mkdir name=\"" + path + "\"");
        std::string response = Receive();

        if (response.length() <= 4)
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

        if (response.length() <= 4)
            throw std::runtime_error("Response length too short");

        if (response[0] != '2')
            throw std::runtime_error("Couldn't rename " + oldName);
    }

    std::string Console::Receive()
    {
        std::string result;
        char buffer[s_PacketSize] = { 0 };

        /**
         * We only receive s_PacketSize - 1 bytes to make sure the last byte
         * is always 0 so that we concat a null terminated string.
         */
        while (recv(m_Socket, buffer, s_PacketSize - 1, 0) != SOCKET_ERROR)
        {
            // Giving the Xbox 360 some time to notice we received something...
            SleepFor(10);
            result += buffer;
        }

        /**
         * Sometimes the response ends but we still receive some stuff.
         * If that's the case, we want to remove everything after ".\r\n".
         */
        size_t endPos = result.find(".\r\n");
        if (endPos != std::string::npos && result.substr(result.length() - 3, 3) != ".\r\n")
            result = result.substr(0, endPos);

        return result;
    }

    void Console::SendCommand(const std::string &command)
    {
        std::string fullCommand = command + "\r\n";
        if (send(m_Socket, fullCommand.c_str(), static_cast<int>(fullCommand.length()), 0) == SOCKET_ERROR)
        {
            CloseSocket();
            CleanupSocket();
        }

        // Giving the Xbox 360 some time to process the command and create a response...
        SleepFor(10);
    }

    std::vector<std::string> Console::SplitResponse(const std::string &response, const std::string &delimiter)
    {
        std::vector<std::string> result;
        std::string responseCopy = response;
        size_t pos = 0;
        std::string line;

        while ((pos = responseCopy.find(delimiter)) != std::string::npos)
        {
            line = responseCopy.substr(0, pos);

            if (line != ".")
                result.push_back(line);

            responseCopy.erase(0, pos + delimiter.length());
        }

        return result;
    }

    bool Console::EndsWith(const std::string &line, const std::string &ending)
    {
        if (ending.size() > line.size())
            return false;

        return std::equal(ending.rbegin(), ending.rend(), line.rbegin());
    }

    DWORD Console::GetIntegerProperty(const std::string &line, const std::string &propertyName, bool hex)
    {
        if (line.find(propertyName) == std::string::npos)
            throw std::runtime_error(std::string("Property '" + propertyName + "' not found").c_str());

        // all integers properties are like this: NAME=VALUE
        size_t startIndex = line.find(propertyName) + propertyName.size() + 1;
        size_t spaceIndex = line.find(' ', startIndex);
        size_t crIndex = line.find('\r', startIndex);
        size_t endIndex = (spaceIndex != std::string::npos) ? spaceIndex : crIndex;

        DWORD toReturn;
        if (hex)
            std::istringstream(line.substr(startIndex, endIndex - startIndex)) >> std::hex >> toReturn;
        else
            std::istringstream(line.substr(startIndex, endIndex - startIndex)) >> toReturn;

        return toReturn;
    }

    std::string Console::GetStringProperty(const std::string &line, const std::string &propertyName)
    {
        if (line.find(propertyName) == std::string::npos)
            throw std::runtime_error(std::string("Property '" + propertyName + "' not found").c_str());

        // all string properties are like this: NAME="VALUE"
        size_t startIndex = line.find(propertyName) + propertyName.size() + 2;
        size_t endIndex = line.find('"', startIndex);

        std::string toReturn = line.substr(startIndex, endIndex - startIndex);

        return toReturn;
    }

    void Console::ClearSocket()
    {
        char buffer[s_PacketSize] = { 0 };
        while (recv(m_Socket, buffer, sizeof(buffer), 0) != SOCKET_ERROR);
    }

    void Console::CleanupSocket()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Console::CloseSocket()
    {
#ifdef _WIN32
        closesocket(m_Socket);
#else
        close(m_Socket);
#endif
        m_Socket = INVALID_SOCKET;
    }

    void Console::SleepFor(DWORD milliseconds)
    {
#ifdef _WIN32
        Sleep(milliseconds);
#else
        usleep(milliseconds * 1000);
#endif
    }
}
