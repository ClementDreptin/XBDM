#include "pch.h"
#include "XboxPath.h"

namespace XBDM
{

XboxPath::XboxPath(const std::string &path)
{
    SplitPath(path);
}

void XboxPath::SplitPath(const std::string &path)
{
    // Replace any potential forward slash, this can happen if path was built using the
    // std::filesystem::path API
    std::string pathCopy = path;
    std::replace(pathCopy.begin(), pathCopy.end(), '/', '\\');

    // Get the drive
    size_t colonPos = pathCopy.find_first_of(':');
    size_t firstBackslashPos = pathCopy.find_first_of('\\');

    if (colonPos == std::string::npos)
        throw std::runtime_error("Couldn't determine the drive name");

    if (firstBackslashPos != colonPos + 1)
        throw std::runtime_error("Backslash not found after the drive name");

    m_Drive = pathCopy.substr(0, colonPos + 1);

    // Get the directory
    size_t lastBackslashPos = pathCopy.find_last_of('\\');
    m_DirName = pathCopy.substr(colonPos + 1, lastBackslashPos - colonPos);

    // Get the file name
    size_t lastDotPos = pathCopy.find_last_of('.');
    m_FileName = pathCopy.substr(lastBackslashPos + 1, lastDotPos - lastBackslashPos - 1);

    // Get the extension
    if (lastDotPos != std::string::npos)
        m_Extension = pathCopy.substr(lastDotPos);
}

}
