#pragma once

#include "Definitions.h"

namespace XBDM
{

class Console
{
public:
    Console();
    Console(const std::string &ipAddress);
    ~Console();

    bool OpenConnection();
    void CloseConnection();

    const std::string &GetName();
    std::vector<Drive> GetDrives();
    std::set<File> GetDirectoryContents(const std::string &directoryPath);

    void LaunchXex(const std::string &xexPath);
    std::string GetActiveTitle();

    void ReceiveFile(const std::string &remotePath, const std::string &localPath);
    void SendFile(const std::string &remotePath, const std::string &localPath);
    void DeleteFile(const std::string &path, bool isDirectory);
    void CreateDirectory(const std::string &path);
    void RenameFile(const std::string &oldName, const std::string &newName);

    inline bool IsConnected() { return m_Connected; }

    inline const std::string &GetIpAddress() const { return m_IpAddress; }

private:
    bool m_Connected = false;
    std::string m_IpAddress;
    std::string m_Name;
    SOCKET m_Socket;
    static const int s_PacketSize = 1024;

    std::string Receive();
    void SendCommand(const std::string &command);

    uint32_t GetIntegerProperty(const std::string &line, const std::string &propertyName, bool hex = true);
    std::string GetStringProperty(const std::string &line, const std::string &propertyName);

    void ClearSocket();
};

}
