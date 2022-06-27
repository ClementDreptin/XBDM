#pragma once

#include <iostream>

#define TEST_INIT() \
    size_t passingTests = 0; \
    size_t failingTests = 0;

#define TEST_F(FunctionName, Description) \
    if (FunctionName()) \
    { \
        std::cout << "[----------] " << Description "\n"; \
        passingTests++; \
    } \
    else \
    { \
        std::cout << "[  FAILED  ] " << Description "\n"; \
        failingTests++; \
    }

#define TEST_SHUTDOWN() \
    std::cout << "\n"; \
    if (failingTests > 0) \
    { \
        std::cout << "[  PASSED  ] " << passingTests << " test" << (passingTests != 1 ? "s" : "") << "\n"; \
        std::cout << "[  FAILED  ] " << failingTests << " test" << (failingTests != 1 ? "s" : "") << "\n"; \
        std::cout << "\n"; \
        std::cout << failingTests << " FAILED TEST" << (failingTests != 1 ? "S" : "") << "\n"; \
        return 1; \
    } \
    else \
    { \
        std::cout << passingTests << " test" << (passingTests != 1 ? "s" : "") << " passed, 0 tests failed\n"; \
        return 0; \
    }
