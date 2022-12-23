#include "TestRunner.h"

#include <iostream>
#include <algorithm>

void TestRunner::AddTest(const char *testName, TestFunction testFunction, bool prioritized)
{
    m_TestCases.push_back({ testName, testFunction, prioritized });
}

bool TestRunner::RunTests()
{
    auto prioritizedTest = std::find_if(m_TestCases.begin(), m_TestCases.end(), [](const TestCase &testCase) { return testCase.Prioritized == true; });

    if (prioritizedTest != m_TestCases.end())
        Run(*prioritizedTest);
    else
        for (auto &testCase : m_TestCases)
            Run(testCase);

    DisplayRecap();

    return m_FailingTests == 0;
}

void TestRunner::Run(const TestCase &testCase)
{
    try
    {
        testCase.Function();
        std::cout << "[----------] " << testCase.Name << '\n';
        m_PassingTests++;
    }
    catch (const std::exception &exception)
    {
        std::cout << "[  FAILED  ] " << testCase.Name << ". Error: " << exception.what() << '\n';
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
