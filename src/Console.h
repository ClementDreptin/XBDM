#pragma once

#include "Definitions.h"
#include "XboxPath.h"

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
    std::set<File> GetDirectoryContents(const XboxPath &directoryPath);
    File GetFileAttributes(const XboxPath &path);

    void LaunchXex(const XboxPath &xexPath);

    XboxPath GetActiveTitle();
    std::string GetType();

    void SynchronizeTime();
    void SetTime(time_t time);

    void Reboot();
    void GoToDashboard();
    void RestartActiveTitle();

    void ReceiveFile(const XboxPath &remotePath, const std::filesystem::path &localPath);
    void ReceiveDirectory(const XboxPath &remotePath, const std::filesystem::path &localPath);
    void SendFile(const XboxPath &remotePath, const std::filesystem::path &localPath);
    void DeleteFile(const XboxPath &path, bool isDirectory);
    void CreateDirectory(const XboxPath &path);
    void RenameFile(const XboxPath &oldName, const XboxPath &newName);

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
