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

}
