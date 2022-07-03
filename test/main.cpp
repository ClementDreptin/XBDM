#include "TestRunner.h"
#include "XBDM.h"
#include "XbdmServerMock.h"

#include <thread>

#define TARGET_HOST "127.0.0.1"

int main()
{
    TestRunner runner;

    runner.AddTest("Connect to socket and shutdown right after", []() {
        std::thread thread(XbdmServerMock::ConnectRespondAndShutdown);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    });

    return runner.RunTests() ? 0 : 1;
}
