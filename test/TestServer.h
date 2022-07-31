#pragma once

#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h>
#else
    #include <cstring>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
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

class TestServer
{
public:
    TestServer();

    void Start();

    void WaitForServerToListen();
    void RequestShutdown();

private:
    SOCKET m_ServerSocket;
    SOCKET m_ClientSocket;
    bool m_Listening;
    std::mutex m_Mutex;
    std::condition_variable m_Cond;

    struct Arg;

    void ConsoleName(const std::vector<Arg> &args);

    bool InitServerSocket();
    bool InitClientSocket();
    bool Run();
    bool Send(const std::string &response);
    bool Send(const char *buffer, size_t length);
    void SignalListening(bool isListening);
    void Shutdown();

private:
    struct Arg
    {
        enum class ArgType
        {
            String,
            Int,
        };

        Arg(const std::string &name, const std::string &value, ArgType type)
            : Name(name), Value(value), Type(type) {}

        std::string Name;
        std::string Value;
        ArgType Type;
    };

    struct Command
    {
        std::string Name;
        std::vector<Arg> Args;
    };

    std::unordered_map<std::string, std::function<void(const std::vector<Arg> &)>> m_CommandMap;

    Command Parse(const std::string &commandString);
};
