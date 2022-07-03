#include "TestRunner.h"

void TestRunner::AddTest(const char *testName, TestFunction testFunction)
{
    m_TestCases.push_back(std::make_pair(testName, testFunction));
}

bool TestRunner::RunTests()
{
    for (auto &testCase : m_TestCases)
    {
        try
        {
            (testCase.second)();
            std::cout << "[----------] " << testCase.first << '\n';
            m_PassingTests++;
        }
        catch (const std::exception &exception)
        {
            std::cout << "[  FAILED  ] " << testCase.first << ". Error: " << exception.what() << '\n';
            m_FailingTests++;
        }
    }

    std::cout << '\n';

    if (m_FailingTests > 0)
    {
        std::cout << "[  PASSED  ] " << m_PassingTests << " test" << (m_PassingTests != 1 ? "s" : "") << '\n';
        std::cout << "[  FAILED  ] " << m_FailingTests << " test" << (m_FailingTests != 1 ? "s" : "") << '\n';
        std::cout << '\n';
        std::cout << m_FailingTests << " FAILED TEST" << (m_FailingTests != 1 ? "S" : "") << '\n';
        return false;
    }
    else
    {
        std::cout << m_PassingTests << " test" << (m_PassingTests != 1 ? "s" : "") << " passed, 0 tests failed\n";
        return true;
    }
}
