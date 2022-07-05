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

    runner.AddTest("Get drive list", []() {
        std::thread thread(XbdmServerMock::DriveResponse);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        std::vector<XBDM::Drive> drives = console.GetDrives();

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
        TEST_EQ(drives.size(), 2);

        TEST_EQ(drives[0].Name, "HDD");
        TEST_EQ(drives[0].FriendlyName, "Retail Hard Drive Emulation (HDD:)");
        TEST_EQ(drives[0].FreeBytesAvailable, 10);
        TEST_EQ(drives[0].TotalBytes, 11);
        TEST_EQ(drives[0].TotalFreeBytes, 12);
        TEST_EQ(drives[0].TotalUsedBytes, 1);

        TEST_EQ(drives[1].Name, "Z");
        TEST_EQ(drives[1].FriendlyName, "Devkit Drive (Z:)");
        TEST_EQ(drives[1].FreeBytesAvailable, 10);
        TEST_EQ(drives[1].TotalBytes, 11);
        TEST_EQ(drives[1].TotalFreeBytes, 12);
        TEST_EQ(drives[1].TotalUsedBytes, 1);
    });

    runner.AddTest("Get directory contents", []() {
        std::string directoryPath = "Hdd:\\Path\\To\\Game";
        std::thread thread(XbdmServerMock::DirectoryContentsResponse, directoryPath);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();

        std::set<XBDM::File> files = console.GetDirectoryContents(directoryPath);

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
        TEST_EQ(files.size(), 4);

        XBDM::File file1 = *std::next(files.begin(), 0);
        TEST_EQ(file1.Name, "dir1");
        TEST_EQ(file1.Size, 0);
        TEST_EQ(file1.IsDirectory, true);
        TEST_EQ(file1.IsXEX, false);

        XBDM::File file2 = *std::next(files.begin(), 1);
        TEST_EQ(file2.Name, "dir2");
        TEST_EQ(file2.Size, 0);
        TEST_EQ(file2.IsDirectory, true);
        TEST_EQ(file2.IsXEX, false);

        XBDM::File file3 = *std::next(files.begin(), 2);
        TEST_EQ(file3.Name, "file1.txt");
        TEST_EQ(file3.Size, 10);
        TEST_EQ(file3.IsDirectory, false);
        TEST_EQ(file3.IsXEX, false);

        XBDM::File file4 = *std::next(files.begin(), 3);
        TEST_EQ(file4.Name, "file2.xex");
        TEST_EQ(file4.Size, 11);
        TEST_EQ(file4.IsDirectory, false);
        TEST_EQ(file4.IsXEX, true);
    });

    return runner.RunTests() ? 0 : 1;
}
