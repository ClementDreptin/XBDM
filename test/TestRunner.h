#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <utility>

// clang-format off
#define TEST_EQ(expression, value) if ((expression) != (value)) throw std::runtime_error(static_cast<const std::stringstream &>(std::stringstream() << "Expected " << (expression) << " to be " << (value) << " at " << __FILE__ << ":" << __LINE__).str())

// clang-format on

class TestRunner
{
    using TestFunction = void (*)();

public:
    TestRunner() = default;

    void AddTest(const char *testName, TestFunction testFunction);
    bool RunTests();

private:
    size_t m_PassingTests = 0;
    size_t m_FailingTests = 0;
    std::vector<std::pair<const char *, TestFunction>> m_TestCases;
};
