#include "TestRunner.h"
#include "XBDM.h"
#include "XbdmServerMock.h"
#include "Utils.h"

#include "TestServer.h"

#include <thread>

#define TARGET_HOST "127.0.0.1"

namespace fs = std::filesystem;

int main()
{
    /* runner.AddTest("Send file", []() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("resultFile.txt");
        fs::path pathOnClient = Utils::GetFixtureDir().append("fileOnClient.txt");
        std::thread thread(XbdmServerMock::SendFile, pathOnServer.string(), pathOnClient.string());
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        console.SendFile(pathOnServer.string(), pathOnClient.string());

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);

        TEST_EQ(fs::exists(pathOnServer), true);
        TEST_EQ(Utils::CompareFiles(pathOnServer, pathOnClient), true);

        fs::remove(pathOnServer);
    });

    runner.AddTest("Delete file", []() {
        std::string fakePath = "Hdd:\\Fake\\Path\\To\\File";
        std::thread thread(XbdmServerMock::DeleteFile, fakePath, false);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        console.DeleteFile(fakePath, false);

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    });

    runner.AddTest("Delete directory", []() {
        std::string fakePath = "Hdd:\\Fake\\Path\\To\\Directory";
        std::thread thread(XbdmServerMock::DeleteFile, fakePath, true);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        console.DeleteFile(fakePath, true);

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    });

    runner.AddTest("Create directory", []() {
        std::string fakePath = "Hdd:\\Fake\\Path\\To\\Directory";
        std::thread thread(XbdmServerMock::CreateDirectory, fakePath);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        console.CreateDirectory(fakePath);

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    });

    runner.AddTest("Rename file", []() {
        std::string fakeOldPath = "Hdd:\\Fake\\Old\\Path";
        std::string fakeNewPath = "Hdd:\\Fake\\New\\Path";
        std::thread thread(XbdmServerMock::RenameFile, fakeOldPath, fakeNewPath);
        XbdmServerMock::WaitForServerToListen();

        XBDM::Console console(TARGET_HOST);
        bool connectionSuccess = console.OpenConnection();
        console.RenameFile(fakeOldPath, fakeNewPath);

        XbdmServerMock::SendRequestToShutdownServer();
        thread.join();

        TEST_EQ(connectionSuccess, true);
    }); */

    // Set the testing environment up
    TestServer server;
    TestRunner runner;

    std::thread thread(std::bind(&TestServer::Start, &server));
    server.WaitForServerToListen();

    XBDM::Console console(TARGET_HOST);
    bool connectionSuccess = console.OpenConnection();

    // The tests
    runner.AddTest("Connect to server", [&]() {
        TEST_EQ(connectionSuccess, true);
    });

    runner.AddTest("Get console name", [&]() {
        std::string consoleName = console.GetName();

        TEST_EQ(consoleName, "TestXDK");
    });

    runner.AddTest("Get drive list", [&]() {
        std::vector<XBDM::Drive> drives = console.GetDrives();

        TEST_EQ(drives.size(), 2);

        TEST_EQ(drives[0].Name, "HDD:");
        TEST_EQ(drives[0].FriendlyName, "Retail Hard Drive Emulation (HDD:)");
        TEST_EQ(drives[0].FreeBytesAvailable, 10);
        TEST_EQ(drives[0].TotalBytes, 11);
        TEST_EQ(drives[0].TotalFreeBytes, 12);
        TEST_EQ(drives[0].TotalUsedBytes, 1);

        TEST_EQ(drives[1].Name, "Z:");
        TEST_EQ(drives[1].FriendlyName, "Devkit Drive (Z:)");
        TEST_EQ(drives[1].FreeBytesAvailable, 10);
        TEST_EQ(drives[1].TotalBytes, 11);
        TEST_EQ(drives[1].TotalFreeBytes, 12);
        TEST_EQ(drives[1].TotalUsedBytes, 1);
    });

    runner.AddTest("Get directory contents", [&]() {
        std::string directoryPath = "Hdd:\\Path\\To\\Game";
        std::set<XBDM::File> files = console.GetDirectoryContents(directoryPath);

        TEST_EQ(files.size(), 4);

        auto file1 = std::next(files.begin(), 0);
        TEST_EQ(file1->Name, "dir1");
        TEST_EQ(file1->Size, 0);
        TEST_EQ(file1->IsDirectory, true);
        TEST_EQ(file1->IsXex, false);

        auto file2 = std::next(files.begin(), 1);
        TEST_EQ(file2->Name, "dir2");
        TEST_EQ(file2->Size, 0);
        TEST_EQ(file2->IsDirectory, true);
        TEST_EQ(file2->IsXex, false);

        auto file3 = std::next(files.begin(), 2);
        TEST_EQ(file3->Name, "file1.txt");
        TEST_EQ(file3->Size, 10);
        TEST_EQ(file3->IsDirectory, false);
        TEST_EQ(file3->IsXex, false);

        auto file4 = std::next(files.begin(), 3);
        TEST_EQ(file4->Name, "file2.xex");
        TEST_EQ(file4->Size, 11);
        TEST_EQ(file4->IsDirectory, false);
        TEST_EQ(file4->IsXex, true);
    });

    runner.AddTest("Start XEX", [&]() {
        std::string xexPath = "Hdd:\\Path\\To\\Game\\default.xex";
        console.LaunchXex(xexPath);

        // No value to check here, we just make sure Console::LaunchXex doesn't throw
    });

    runner.AddTest("Receive file", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("fileOnServer.txt");
        fs::path pathOnClient = Utils::GetFixtureDir().append("resultFile.txt");

        console.ReceiveFile(pathOnServer.string(), pathOnClient.string());

        TEST_EQ(fs::exists(pathOnClient), true);
        TEST_EQ(Utils::CompareFiles(pathOnServer, pathOnClient), true);

        fs::remove(pathOnClient);
    });

    // Running the tests and shuting down the server
    bool finalResult = runner.RunTests();
    server.RequestShutdown();
    thread.join();

    return static_cast<int>(!finalResult);
}
