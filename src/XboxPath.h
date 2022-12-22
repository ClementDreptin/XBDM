#pragma once

namespace XBDM
{

class XboxPath
{
public:
    XboxPath() = default;
    XboxPath(const std::string &path);
    ~XboxPath() = default;

    template<typename T>
    inline friend std::string operator+(const T &left, const XboxPath &path)
    {
        return left + path.String();
    }

    template<typename T>
    inline friend std::string operator+(const XboxPath &path, const T &right)
    {
        return path.String() + right;
    }

    inline friend std::ostream &operator<<(std::ostream &stream, const XboxPath &path)
    {
        return stream << path.String();
    }

    inline XboxPath &operator/=(const std::string &path)
    {
        return Append(path);
    }

    friend XboxPath operator/(const XboxPath &path, const std::string &right)
    {
        XboxPath tmp = path;
        tmp /= right;
        return tmp;
    }

    inline friend bool operator==(const XboxPath &left, const XboxPath &right)
    {
        return left.Compare(right);
    }

    inline const std::string &String() const { return m_FullPath; }

    inline const std::string &Drive() const { return m_Drive; }

    inline const std::string &DirName() const { return m_DirName; }

    inline const std::string &FileName() const { return m_FileName; }

    inline const std::string &Extension() const { return m_Extension; }

    XboxPath Parent() const;

    XboxPath &Append(const std::string &path);

    bool Compare(const XboxPath &other) const;

private:
    std::string m_FullPath;
    std::string m_Drive;
    std::string m_DirName;
    std::string m_FileName;
    std::string m_Extension;

    static const char s_Separator = '\\';

    void Init(const std::string &path);
    void SplitPath(const std::string &path);
};

}
