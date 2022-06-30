#include "Macros.h"
#include "XBDM.h"
#include "XbdmServerMock.h"

#include <thread>

#define TARGET_HOST "127.0.0.1"

static bool ConnectToSocketAndShutdown()
{
    std::thread thread(XbdmServerMock::ConnectRespondAndShutdown);
    XBDM::Console console(TARGET_HOST);

    bool connectionSuccess = console.OpenConnection();
    thread.join();

    return connectionSuccess;
}

int main()
{
    TEST_INIT();

    TEST_F(ConnectToSocketAndShutdown, "ConnectToSocketAndShutdown should pass");

    TEST_SHUTDOWN();
}
