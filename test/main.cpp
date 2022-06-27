#include "XBDM.h"

#include "Macros.h"

bool PassingTest()
{
    return true;
}

bool FailingTest()
{
    return false;
}

int main()
{
    TEST_INIT();

    TEST_F(PassingTest, "First test should pass");
    TEST_F(PassingTest, "Second test should pass");
    TEST_F(FailingTest, "Third test should pass");
    TEST_F(FailingTest, "Third test should pass");
    TEST_F(PassingTest, "Fourth test should pass");

    TEST_SHUTDOWN();
}
