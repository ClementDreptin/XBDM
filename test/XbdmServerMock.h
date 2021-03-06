#pragma once

#include <mutex>
#include <condition_variable>
#include <string>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
#else
    #include <cstring>
    #include <netdb.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
// clang-format off
    typedef int socklen_t;
// clang-format on
#else
// clang-format off
    typedef int SOCKET;
// clang-format on

    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

class XbdmServerMock
{
public:
    XbdmServerMock() = delete;

    static void NoResponse();
    static void PartialConnectResponse();
    static void ConnectResponse();
    static void ConsoleNameResponse();
    static void DriveResponse();
    static void DirectoryContentsResponse(const std::string &directoryPath);
    static void MagicBoot(const std::string &xexPath);
    static void ReceiveFile(const std::string &pathOnServer);
    static void SendFile(const std::string &pathOnServer, const std::string &pathOnClient);
    static void DeleteFile(const std::string &path, bool isDirectory);
    static void CreateDirectory(const std::string &path);
    static void RenameFile(const std::string &oldName, const std::string &newName);

    static void WaitForServerToListen();
    static void SendRequestToShutdownServer();

private:
    static SOCKET s_ServerSocket;
    static SOCKET s_ClientSocket;
    static bool s_Listening;
    static std::mutex s_Mutex;
    static std::condition_variable s_Cond;

    static bool Start();
    static bool StartClientConnection();
    static bool Send(const std::string &response);
    static bool Send(const char *buffer, size_t length);
    static bool CheckRequest(const std::string &expectedCommand);
    static void SignalListening(bool isListening);
    static void ProcessShutdownRequest();
    static void Shutdown();
};
