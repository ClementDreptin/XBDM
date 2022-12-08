#include "Utils.h"

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

#include <fstream>

namespace fs = std::filesystem;

namespace Utils
{

fs::path GetFixtureDir()
{
    const int MAX_SIZE = 200;
    char path[MAX_SIZE] = { 0 };

#ifdef _WIN32
    GetModuleFileNameA(NULL, path, MAX_SIZE);
#else
    size_t result = readlink("/proc/self/exe", path, MAX_SIZE);
    (void)result;
#endif

    fs::path execFilePath(path);

    return execFilePath
        .parent_path() // <debug|release>
        .parent_path() // bin
        .parent_path() // build
        .parent_path() // XBDM
        .append("test")
        .append("fixtures");
}

bool CompareFiles(const fs::path &pathToFirstFile, const fs::path &pathToSecondFile)
{
    std::ifstream firstFile(pathToFirstFile, std::ifstream::binary | std::ifstream::ate);
    std::ifstream secondFile(pathToSecondFile, std::ifstream::binary | std::ifstream::ate);

    // Make sure that files can be opened
    if (firstFile.fail() || secondFile.fail())
        return false;

    // Compare the file sizes
    if (firstFile.tellg() != secondFile.tellg())
        return false;

    // Go back to the beginning of both files
    firstFile.seekg(0, std::ifstream::beg);
    secondFile.seekg(0, std::ifstream::beg);

    // Compare the file contents
    return std::equal(
        std::istreambuf_iterator<char>(firstFile.rdbuf()),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(secondFile.rdbuf())
    );
}

std::vector<std::string> StringSplit(const std::string &string, const std::string &separator)
{
    std::vector<std::string> result;
    std::string stringCopy = string;

    if (separator.empty())
        return result;

    for (;;)
    {
        size_t pos = stringCopy.find(separator);

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
}

}
