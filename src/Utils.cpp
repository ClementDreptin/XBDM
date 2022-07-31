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
    size_t pos = 0;
    std::string line;

    // If separator is not in stringCopy, just return a vector only containing stringCopy
    if (stringCopy.find(separator) == std::string::npos)
    {
        result.push_back(stringCopy);
        return result;
    }

    while ((pos = stringCopy.find(separator)) != std::string::npos)
    {
        line = stringCopy.substr(0, pos);

        if (line != ".")
            result.push_back(line);

        stringCopy.erase(0, pos + separator.size());
    }

    return result;
}

}
}
