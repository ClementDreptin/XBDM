#pragma once

namespace XBDM
{

class XboxPath
{
public:
    XboxPath() = default;
    XboxPath(const std::string &path);
    ~XboxPath() = default;

    const std::string &GetDrive() const { return m_Drive; }

    const std::string &GetDirName() const { return m_DirName; }

    const std::string &GetFileName() const { return m_FileName; }

    const std::string &GetExtension() const { return m_Extension; }

private:
    std::string m_Drive;
    std::string m_DirName;
    std::string m_FileName;
    std::string m_Extension;

    void SplitPath(const std::string &path);
};

}
