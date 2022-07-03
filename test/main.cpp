#include "TestRunner.h"
#include "XBDM.h"
#include "XbdmServerMock.h"

#include <thread>

#define TARGET_HOST "127.0.0.1"

int main()
{
    TestRunner runner;

    runner.AddTest("Try to connect while server is not running", []() {
        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        TEST_EQ(connectionSuccess, false);
    });

    runner.AddTest("Connect to server but no response", []() {
        std::thread thread(XbdmServerMock::NoResponse);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, false);
    });

    runner.AddTest("Connect to server but only received partial response", []() {
        std::thread thread(XbdmServerMock::PartialConnectResponse);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, false);
    });

    runner.AddTest("Connect to socket and shutdown right after", []() {
        std::thread thread(XbdmServerMock::ConnectRespondAndShutdown);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    });

    runner.AddTest("Get console name", []() {
        std::thread thread(XbdmServerMock::ConsoleNameResponse);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        std::string consoleName = console.GetName();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
        TEST_EQ(consoleName, "TestXDK");
    });

    return runner.RunTests() ? 0 : 1;
}
