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
    void CloseConnection();

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

    uint32_t GetIntegerProperty(const std::string &line, const std::string &propertyName, bool hex = true);
    std::string GetStringProperty(const std::string &line, const std::string &propertyName);

    void ClearSocket();
};

}
