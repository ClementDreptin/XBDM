#include "pch.h"
#include "XboxPath.h"

namespace XBDM
{

const char XboxPath::s_Separator = '\\';

XboxPath::XboxPath(const std::string &path)
    : m_Path(path)
{
}

XboxPath::XboxPath(const char *path)
    : XboxPath(std::string(path))
{
}

XboxPath XboxPath::Drive() const
{
    size_t colonPos = m_Path.find_first_of(':');

    if (colonPos == std::string::npos)
        return XboxPath();

    return XboxPath(m_Path.substr(0, colonPos + 1));
}

XboxPath XboxPath::FileName() const
{
    size_t lastSeparatorPos = m_Path.find_last_of(s_Separator);
    size_t lastDotPos = m_Path.find_last_of('.');
    size_t colonPos = m_Path.find_first_of(':');
    size_t characterBeforeFileNamePos = lastSeparatorPos != std::string::npos ? lastSeparatorPos : colonPos;

    // Case of path only containing a drive
    if (colonPos == m_Path.size() - 1)
        return XboxPath();

    // Case of file name starting with a dot and no extension (like .gitignore)
    if (lastDotPos == characterBeforeFileNamePos + 1)
        return XboxPath(m_Path.substr(characterBeforeFileNamePos + 1));

    return XboxPath(m_Path.substr(characterBeforeFileNamePos + 1, lastDotPos - characterBeforeFileNamePos - 1));
}

XboxPath XboxPath::Extension() const
{
    size_t lastSeparatorPos = m_Path.find_last_of(s_Separator);
    size_t lastDotPos = m_Path.find_last_of('.');
    size_t colonPos = m_Path.find_first_of(':');
    size_t characterBeforeFileNamePos = lastSeparatorPos != std::string::npos ? lastSeparatorPos : colonPos;

    if (lastDotPos == std::string::npos || lastDotPos == characterBeforeFileNamePos + 1)
        return XboxPath();

    return XboxPath(m_Path.substr(lastDotPos));
}

XboxPath XboxPath::Parent() const
{
    if (m_Path.empty())
        return XboxPath();

    // When the path is a directory (so ends with the separator), we make the offset
    // to be one character before so that lastSeparatorPos doesn't point the very last
    // separator but the one before
    size_t offset = std::string::npos;
    if (m_Path.back() == s_Separator)
        offset = m_Path.size() - 2;

    size_t lastSeparatorPos = m_Path.find_last_of(s_Separator, offset);
    if (lastSeparatorPos == std::string::npos)
    {
        XboxPath drive = Drive();
        if (!drive.IsEmpty())
            return drive;

        return XboxPath();
    }

    return XboxPath(m_Path.substr(0, lastSeparatorPos));
}

XboxPath &XboxPath::Append(const XboxPath &path)
{
    if (!m_Path.empty() && m_Path.back() != s_Separator)
        m_Path += s_Separator;

    m_Path += path.String();

    return *this;
}

bool XboxPath::Compare(const XboxPath &other) const
{
    return String() == other.String();
}

bool XboxPath::IsEmpty() const
{
    return m_Path.empty();
}

bool XboxPath::IsRoot() const
{
    if (m_Path.empty())
        return true;

    // If the path is a path of a file it can't be the root of drive
    if (!FileName().IsEmpty())
        return false;

    size_t lastSeparatorPos = m_Path.find_last_of(s_Separator);
    size_t colonPos = m_Path.find_first_of(':');

    return colonPos == m_Path.size() - 1 || lastSeparatorPos == colonPos + 1;
}

}
