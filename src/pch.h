#ifdef _WIN32
    #pragma once

    #include <WinSock2.h>
    #include <Windows.h>
    #include <WS2tcpip.h>
#else
    #include <cstring>
    #include <netdb.h>
    #include <unistd.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <exception>
#include <set>
