#pragma once

namespace XBDM
{

class XboxPath
{
public:
    XboxPath() = default;
    XboxPath(const std::string &path);
    XboxPath(const char *path);
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

    inline XboxPath &operator/=(const XboxPath &path)
    {
        return Append(path);
    }

    friend XboxPath operator/(const XboxPath &path, const XboxPath &right)
    {
        XboxPath tmp = path;
        tmp /= right;
        return tmp;
    }

    inline friend bool operator==(const XboxPath &left, const XboxPath &right)
    {
        return left.Compare(right);
    }

    inline friend bool operator!=(const XboxPath &left, const XboxPath &right)
    {
        return !(left == right);
    }

    inline const std::string &String() const { return m_Path; }

    XboxPath Drive() const;

    XboxPath FileName() const;

    XboxPath Extension() const;

    XboxPath Parent() const;

    XboxPath &Append(const XboxPath &path);

    bool Compare(const XboxPath &other) const;

    bool IsEmpty() const;

    bool IsRoot() const;

private:
    std::string m_Path;

    static const char s_Separator;
};

}
