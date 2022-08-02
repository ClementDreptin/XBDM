#include "pch.h"
#include "Utils.h"

namespace XBDM
{
namespace Utils
{

bool String::EndsWith(const std::string &line, const std::string &ending)
{
    if (ending.size() > line.size())
        return false;

    return std::equal(ending.rbegin(), ending.rend(), line.rbegin());
}

std::vector<std::string> String::Split(const std::string &string, const std::string &separator)
{
    std::vector<std::string> result;
    std::string stringCopy = string;

    if (separator.empty())
        return result;

    for (;;)
    {
        size_t pos = pos = stringCopy.find(separator);

        // If separator is not in stringCopy, just push what is left of stringCopy
        // into the vector and return it
        if (pos == std::string::npos)
        {
            result.push_back(stringCopy);
            return result;
        }

        std::string token = stringCopy.substr(0, pos);

        result.push_back(token);

        stringCopy.erase(0, pos + separator.size());
    }

    return result;
}

}
}
