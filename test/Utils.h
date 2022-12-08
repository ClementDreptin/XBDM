#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace Utils
{

std::filesystem::path GetFixtureDir();

bool CompareFiles(const std::filesystem::path &firstFile, const std::filesystem::path &secondFile);

std::vector<std::string> StringSplit(const std::string &string, const std::string &separator);

}
