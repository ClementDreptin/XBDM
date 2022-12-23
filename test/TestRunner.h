#pragma once

#include <sstream>
#include <vector>
#include <functional>

// clang-format off
#define TEST_EQ(expression, value) if ((expression) != (value)) throw std::runtime_error(static_cast<const std::stringstream &>(std::stringstream() << std::boolalpha << "Expected " << (expression) << " to be " << (value) << " at " << __FILE__ << ":" << __LINE__).str())

// clang-format on

class TestRunner
{
    using TestFunction = std::function<void()>;

    struct TestCase
    {
        const char *Name = nullptr;
        TestFunction Function = TestFunction();
        bool Prioritized = false;
    };

public:
    TestRunner() = default;

    void AddTest(const char *testName, TestFunction testFunction, bool prioritized = false);
    bool RunTests();

private:
    size_t m_PassingTests = 0;
    size_t m_FailingTests = 0;
    std::vector<TestCase> m_TestCases;

    void Run(const TestCase &testCase);
    void DisplayRecap();
};
