#pragma once

#include "pch.h"

#ifndef _WIN32
// clang-format off
    typedef int SOCKET;
// clang-format on

    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace XBDM
{

struct Drive
{
    std::string Name;
    uint64_t FreeBytesAvailable;
    uint64_t TotalBytes;
    uint64_t TotalFreeBytes;
    uint64_t TotalUsedBytes;
    std::string FriendlyName;
};

struct File
{
    std::string Name;
    uint64_t Size;
    bool IsXex;
    bool IsDirectory;

    bool operator<(const File &other) const
    {
        // Compare the file names and store the comparaisons as integers (which will be either 0 or 1)
        int thisNameGreaterThanOtherName = static_cast<int>(Name > other.Name);
        int otherNameGreaterThanThisName = static_cast<int>(other.Name > Name);

        // If the file is a directory, decrease the score by 2. The score is decreased because the lower the score the closer
        // the element will be to the start of the set and we want directories to always be before files in sets.
        int thisScore = thisNameGreaterThanOtherName - static_cast<int>(IsDirectory * 2);
        int otherScore = otherNameGreaterThanThisName - static_cast<int>(other.IsDirectory * 2);

        return thisScore < otherScore;
    }
};

}
