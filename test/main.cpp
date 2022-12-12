#include "TestRunner.h"
#include "XBDM.h"
#include "TestServer.h"
#include "Utils.h"

#include <thread>

#define TARGET_HOST "127.0.0.1"

namespace fs = std::filesystem;

int main()
{
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
        TEST_EQ(drives[0].FriendlyName, "Retail Hard Drive Emulation");
        TEST_EQ(drives[0].FreeBytesAvailable, 10);
        TEST_EQ(drives[0].TotalBytes, 11);
        TEST_EQ(drives[0].TotalFreeBytes, 12);
        TEST_EQ(drives[0].TotalUsedBytes, 1);

        TEST_EQ(drives[1].Name, "Z:");
        TEST_EQ(drives[1].FriendlyName, "Devkit Drive");
        TEST_EQ(drives[1].FreeBytesAvailable, 10);
        TEST_EQ(drives[1].TotalBytes, 11);
        TEST_EQ(drives[1].TotalFreeBytes, 12);
        TEST_EQ(drives[1].TotalUsedBytes, 1);
    });

    runner.AddTest("Get directory contents", [&]() {
        std::set<XBDM::File> files = console.GetDirectoryContents(Utils::GetFixtureDir().string());

        // This value is hardcoded in the test server instead of the actual file dates for simplicity
        const time_t creationAndModificationDate = 1447599192;

        TEST_EQ(files.size(), 3);

        auto file1 = std::next(files.begin(), 0);
        TEST_EQ(file1->Name, "client");
        TEST_EQ(file1->Size, 0);
        TEST_EQ(file1->IsDirectory, true);
        TEST_EQ(file1->IsXex, false);
        TEST_EQ(file1->CreationDate, creationAndModificationDate);
        TEST_EQ(file1->ModificationDate, creationAndModificationDate);

        auto file2 = std::next(files.begin(), 1);
        TEST_EQ(file2->Name, "server");
        TEST_EQ(file2->Size, 0);
        TEST_EQ(file2->IsDirectory, true);
        TEST_EQ(file2->IsXex, false);
        TEST_EQ(file2->CreationDate, creationAndModificationDate);
        TEST_EQ(file2->ModificationDate, creationAndModificationDate);

        auto file3 = std::next(files.begin(), 2);
        TEST_EQ(file3->Name, "file.xex");
        TEST_EQ(file3->Size, 14);
        TEST_EQ(file3->IsDirectory, false);
        TEST_EQ(file3->IsXex, true);
        TEST_EQ(file3->CreationDate, creationAndModificationDate);
        TEST_EQ(file3->ModificationDate, creationAndModificationDate);
    });

    runner.AddTest("Get directory contents of inexistant directory", [&]() {
        fs::path inexistantDirectory = Utils::GetFixtureDir() /= "inexistant";
        bool throws = false;

        try
        {
            console.GetDirectoryContents(inexistantDirectory.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Invalid directory path: " + inexistantDirectory.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Start XEX", [&]() {
        fs::path xexPath = Utils::GetFixtureDir() /= "file.xex";
        console.LaunchXex(xexPath.string());

        // No value to check here, we just make sure Console::LaunchXex doesn't throw
    });

    runner.AddTest("Get active title", [&]() {
        std::string activeTitle = console.GetActiveTitle();

        TEST_EQ(activeTitle, "\\Device\\Harddisk0\\SystemExtPartition\\20449700\\dash.xex");
    });

    runner.AddTest("Get console type", [&]() {
        std::string consoleType = console.GetType();

        TEST_EQ(consoleType, "reviewerkit");
    });

    runner.AddTest("Synchronize time", [&]() {
        console.SynchronizeTime();

        // No value to check here, we just make sure Console::SynchronizeTime doesn't throw
    });

    runner.AddTest("Cold reboot", [&]() {
        console.Reboot();

        // No value to check here, we just make sure Console::Reboot doesn't throw
    });

    runner.AddTest("Reboot to dashboard", [&]() {
        console.GoToDashboard();

        // No value to check here, we just make sure Console::GoToDashboard doesn't throw
    });

    runner.AddTest("Restart active title", [&]() {
        console.RestartActiveTitle();

        // No value to check here, we just make sure Console::RestartActiveTitle doesn't throw
    });

    runner.AddTest("Receive file", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("server").append("file.txt");
        fs::path pathOnClient = Utils::GetFixtureDir().append("client").append("result.txt");

        console.ReceiveFile(pathOnServer.string(), pathOnClient.string());

        TEST_EQ(fs::exists(pathOnClient), true);
        TEST_EQ(Utils::CompareFiles(pathOnServer, pathOnClient), true);

        fs::remove(pathOnClient);
    });

    runner.AddTest("Receive directory", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("server");
        fs::path pathOnClient = Utils::GetFixtureDir().append("clientTmp");

        console.ReceiveDirectory(pathOnServer.string(), pathOnClient.string());

        TEST_EQ(fs::exists(pathOnClient), true);
        TEST_EQ(Utils::CompareFiles(pathOnServer / "file.txt", pathOnClient / "file.txt"), true);

        fs::remove_all(pathOnClient);
    });

    runner.AddTest("Receive inexistant file", [&]() {
        fs::path inexistantPathOnServer = Utils::GetFixtureDir().append("server").append("inexistant.txt");
        fs::path pathOnClient = Utils::GetFixtureDir().append("client").append("result.txt");
        bool throws = false;

        try
        {
            console.ReceiveFile(inexistantPathOnServer.string(), pathOnClient.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Invalid remote path: " + inexistantPathOnServer.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Send file", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("server").append("result.txt");
        fs::path pathOnClient = Utils::GetFixtureDir().append("client").append("file.txt");

        console.SendFile(pathOnServer.string(), pathOnClient.string());

        TEST_EQ(fs::exists(pathOnServer), true);
        TEST_EQ(Utils::CompareFiles(pathOnServer, pathOnClient), true);

        fs::remove(pathOnServer);
    });

    runner.AddTest("Send inexistant file", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("server").append("result.txt");
        fs::path inexistantPathOnClient = Utils::GetFixtureDir().append("client").append("inexistant.txt");
        bool throws = false;

        try
        {
            console.SendFile(pathOnServer.string(), inexistantPathOnClient.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Invalid local path: " + inexistantPathOnClient.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Delete file", [&]() {
        // This won't actually delete the file
        fs::path pathOnServer = Utils::GetFixtureDir().append("server").append("file.txt");
        console.DeleteFile(pathOnServer.string(), false);

        // No value to check here, we just make sure Console::DeleteFile doesn't throw
    });

    runner.AddTest("Delete inexistant file", [&]() {
        fs::path inexistantPathOnServer = Utils::GetFixtureDir().append("server").append("inexistant.txt");
        bool throws = false;

        try
        {
            console.DeleteFile(inexistantPathOnServer.string(), false);
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Couldn't delete " + inexistantPathOnServer.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Delete directory", [&]() {
        // This won't actually delete the fixture directory
        console.DeleteFile(Utils::GetFixtureDir().string(), true);

        // No value to check here, we just make sure Console::DeleteFile doesn't throw
    });

    runner.AddTest("Delete inexistant directory", [&]() {
        fs::path inexistantPathOnServer = Utils::GetFixtureDir().append("server").append("inexistant");
        bool throws = false;

        try
        {
            console.DeleteFile(inexistantPathOnServer.string(), true);
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Invalid directory path: " + inexistantPathOnServer.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Create directory", [&]() {
        // This won't actually create a new directory
        console.CreateDirectory(Utils::GetFixtureDir().append("newDirectory").string());

        // No value to check here, we just make sure Console::CreateDirectory doesn't throw
    });

    runner.AddTest("Create already existing directory", [&]() {
        fs::path existingDirectoryPath = Utils::GetFixtureDir() /= "server";
        bool throws = false;

        try
        {
            console.CreateDirectory(existingDirectoryPath.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Couldn't create directory " + existingDirectoryPath.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Rename file", [&]() {
        // This won't actually rename the file
        fs::path oldPathOnServer = Utils::GetFixtureDir().append("server").append("file.txt");
        fs::path newPathOnServer = Utils::GetFixtureDir().append("server").append("newFile.txt");
        console.RenameFile(oldPathOnServer.string(), newPathOnServer.string());

        // No value to check here, we just make sure Console::RenameFile doesn't throw
    });

    runner.AddTest("Rename inexistant file", [&]() {
        fs::path inexistantPathOnServer = Utils::GetFixtureDir().append("server").append("inexistant.txt");
        fs::path newPathOnServer = Utils::GetFixtureDir().append("server").append("newFile.txt");
        bool throws = false;

        try
        {
            console.RenameFile(inexistantPathOnServer.string(), newPathOnServer.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Couldn't rename " + inexistantPathOnServer.string());
        }

        TEST_EQ(throws, true);
    });

    runner.AddTest("Rename file to already existing file", [&]() {
        fs::path pathOnServer = Utils::GetFixtureDir().append("server").append("file.txt");
        bool throws = false;

        try
        {
            console.RenameFile(pathOnServer.string(), pathOnServer.string());
        }
        catch (const std::exception &exception)
        {
            throws = true;
            TEST_EQ(exception.what(), "Couldn't rename " + pathOnServer.string());
        }

        TEST_EQ(throws, true);
    });

    // Running the tests and shuting down the server
    bool finalResult = runner.RunTests();
    server.RequestShutdown();
    thread.join();

    return static_cast<int>(!finalResult);
}
