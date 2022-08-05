#pragma once

#include <filesystem>

namespace Utils
{

std::filesystem::path GetFixtureDir();

bool CompareFiles(const std::filesystem::path &firstFile, const std::filesystem::path &secondFile);

}
