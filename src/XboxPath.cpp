#include "pch.h"
#include "XboxPath.h"

namespace XBDM
{

XboxPath::XboxPath(const std::string &path)
{
    Init(path);
}

XboxPath XboxPath::Parent() const
{
    // When the path is a directory (so ends with the separator), we make the offset
    // to be one character before so that lastSeparatorPos doesn't point the very last
    // separator but the one before
    size_t offset = std::string::npos;
    if (m_FullPath.back() == s_Separator)
        offset = m_FullPath.size() - 2;

    size_t lastSeparatorPos = m_FullPath.find_last_of(s_Separator, offset);
    if (lastSeparatorPos == std::string::npos)
        throw std::runtime_error("Could not find a separator");

    return XboxPath(m_FullPath.substr(0, lastSeparatorPos));
}

XboxPath &XboxPath::Append(const std::string &path)
{
    if (m_FullPath.back() != s_Separator)
        m_FullPath += s_Separator;

    m_FullPath += path;

    Init(m_FullPath);

    return *this;
}

bool XboxPath::Compare(const XboxPath &other) const
{
    return String() == other.String();
}

void XboxPath::Init(const std::string &path)
{
    // Replace any potential forward slash, this can happen if path was built using the
    // std::filesystem::path API
    std::string pathCopy = path;
    std::replace(pathCopy.begin(), pathCopy.end(), '/', s_Separator);

    SplitPath(pathCopy);

    m_FullPath = pathCopy;
}

void XboxPath::SplitPath(const std::string &path)
{
    // Reset the members
    m_FullPath.clear();
    m_Drive.clear();
    m_DirName.clear();
    m_FileName.clear();
    m_Extension.clear();

    // Get the drive
    size_t colonPos = path.find_first_of(':');

    if (colonPos != std::string::npos)
        m_Drive = path.substr(0, colonPos + 1);

    // Get the directory
    size_t lastSeparatorPos = path.find_last_of(s_Separator);
    m_DirName = path.substr(colonPos + 1, lastSeparatorPos - colonPos);

    // Get the file name
    size_t lastDotPos = path.find_last_of('.');

    // Case of file name starting with a dot and no extension (like .gitignore)
    if (lastDotPos == lastSeparatorPos + 1)
    {
        m_FileName = path.substr(lastSeparatorPos + 1);
        return;
    }

    m_FileName = path.substr(lastSeparatorPos + 1, lastDotPos - lastSeparatorPos - 1);

    // Get the extension
    if (lastDotPos != std::string::npos)
        m_Extension = path.substr(lastDotPos);
}

}
