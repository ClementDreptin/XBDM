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

}
}
