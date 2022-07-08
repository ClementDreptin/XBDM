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
    readlink("/proc/self/exe", path, MAX_SIZE);
#endif

    fs::path execFilePath(path);

    return execFilePath.parent_path().parent_path().parent_path().parent_path().append("test").append("fixtures");
}

bool CompareFiles(const fs::path &pathToFirstFile, const fs::path &pathToSecondFile)
{
    std::ifstream f1(pathToFirstFile, std::ifstream::binary | std::ifstream::ate);
    std::ifstream f2(pathToSecondFile, std::ifstream::binary | std::ifstream::ate);

    if (f1.fail() || f2.fail())
        return false;

    if (f1.tellg() != f2.tellg())
        return false;

    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);

    return std::equal(
        std::istreambuf_iterator<char>(f1.rdbuf()),
        std::istreambuf_iterator<char>(),
        std::istreambuf_iterator<char>(f2.rdbuf())
    );
}

}
