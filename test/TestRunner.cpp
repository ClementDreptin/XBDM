#include "TestRunner.h"

void TestRunner::AddTest(const char *testName, TestFunction testFunction)
{
    m_TestCases.push_back(std::make_pair(testName, testFunction));
}

bool TestRunner::RunOnly(const char *testName, TestFunction testFunction)
{
    Run(std::make_pair(testName, testFunction));

    DisplayRecap();

    return m_FailingTests == 0;
}

bool TestRunner::RunTests()
{
    for (auto &testCase : m_TestCases)
        Run(testCase);

    DisplayRecap();

    return m_FailingTests == 0;
}

void TestRunner::Run(const TestCase &testCase)
{
    auto &[testName, testFunction] = testCase;

    try
    {
        testFunction();
        std::cout << "[----------] " << testName << '\n';
        m_PassingTests++;
    }
    catch (const std::exception &exception)
    {
        std::cout << "[  FAILED  ] " << testName << ". Error: " << exception.what() << '\n';
        m_FailingTests++;
    }
}

void TestRunner::DisplayRecap()
{
    std::cout << '\n';

    if (m_FailingTests > 0)
    {
        std::cout << "[  PASSED  ] " << m_PassingTests << " test" << (m_PassingTests != 1 ? "s" : "") << '\n';
        std::cout << "[  FAILED  ] " << m_FailingTests << " test" << (m_FailingTests != 1 ? "s" : "") << '\n';
        std::cout << '\n';
        std::cout << m_FailingTests << " FAILED TEST" << (m_FailingTests != 1 ? "S" : "") << '\n';
        return;
    }

    std::cout << m_PassingTests << " test" << (m_PassingTests != 1 ? "s" : "") << " passed, 0 tests failed\n";
}
