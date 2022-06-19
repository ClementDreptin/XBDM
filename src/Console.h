#pragma once

#include "Definitions.h"


namespace XBDM
{
    class Console
    {
    public:
        Console();
        Console(const std::string &ipAddress);

        bool OpenConnection();
        bool CloseConnection();

        std::string GetName();
        std::vector<Drive> GetDrives();
        std::set<File> GetDirectoryContents(const std::string &directoryPath);

        void LaunchXEX(const std::string &xexPath);

        void ReceiveFile(const std::string &remotePath, const std::string &localPath);
        void SendFile(const std::string &remotePath, const std::string &localPath);
        void DeleteFile(const std::string &path, bool isDirectory);
        void CreateDirectory(const std::string &path);
        void RenameFile(const std::string &oldName, const std::string &newName);

        bool IsConnected() { return m_Connected; }
    private:
        bool m_Connected = false;
        std::string m_IpAddress;
        SOCKET m_Socket;
        static const int s_PacketSize = 2048;

        std::string Receive();
        void SendCommand(const std::string &command);

        std::vector<std::string> SplitResponse(const std::string &response, const std::string &delimiter);
        bool EndsWith(const std::string &line, const std::string &ending);

        DWORD GetIntegerProperty(const std::string &line, const std::string &propertyName, bool hex = true);
        std::string GetStringProperty(const std::string &line, const std::string &propertyName);

        void ClearSocket();
        void CleanupSocket();
        void CloseSocket();

        void SleepFor(DWORD milliseconds);
    };
}
