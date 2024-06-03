
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <array>
#include <stdio.h>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <sys/mount.h>
#include <sys/stat.h>
#include "boost/asio.hpp"
#include "parsedata.h"
#include "ce_time.h"
#include "ini_parser.h"


using namespace boost::asio;
using ip::udp;

class UdpClient {
public:
    UdpClient(io_service& ioService, const std::string& serverAddress, unsigned short serverPort, unsigned short LocalPort)
        : socket_(ioService, udp::endpoint(udp::v4(), LocalPort)), serverEndpoint_(ip::address::from_string(serverAddress), serverPort)
    {
        startReceive();
    }
    void startSend(const std::string& message) {
        socket_.async_send_to(buffer(message), serverEndpoint_, [this](const boost::system::error_code& error, std::size_t /*bytes_sent*/) {
            if (!error) {
                std::cout << "Message sent successfully." << std::endl;
            } else {
                std::cerr << "Error sending message: " << error.message() << std::endl;
            }
        });
    }
 private:
    void startReceive() {
        socket_.async_receive_from(buffer(data_, max_length), senderEndpoint_, [this](const boost::system::error_code& error, std::size_t bytes_received) {
            if (!error) {
                std::cout << "Received response from " << senderEndpoint_ << ": " << std::string(data_, bytes_received) << std::endl;
                ProcessData(data_, bytes_received);
               // startSend();  // Continue with the next send operation
            } else {
                std::cerr << "Error receiving message: " << error.message() << std::endl;
            }

            startReceive();  // Continue with the next receive operation
        });
    } 

public:
    udp::socket socket_;
    udp::endpoint serverEndpoint_;
    udp::endpoint senderEndpoint_;
    enum { max_length = 1024 };
    char data_[max_length];
    void ProcessData(const char*data, std::size_t length);
    bool firmwareUpdate(const std::string& firmwarePath, const std::string& firmwareName);
    bool killProcess(const std::string& processName);
    bool isProcessRunning(const std::string& processName);
    bool copyFirmware(const std::string& appName, const std::string& mountPoint, const std::string& sharedFolderPath, 
                        const std::string& username, const std::string& password, const std::string& outputFolderPath);
    std::string findExecutableFilePath(const std::string& processName);
    std::string findExecutableDirectoryPath(const std::string& path);
    void rollbackOldFirmware(const std::string& executableAppPath, const std::string& backupAppName, const std::string& originalAppName, const std::string& command);
    std::string getIPAddress();
    void sendMsg2Server(std::string cmdCode, std::string data);
    bool startStationSoftware();
};

class Watchdog
{

public:
    Watchdog(boost::asio::io_service& io_service, int watchdogPort)
        : socket_(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), watchdogPort)),
        timer_(io_service),
        ackFailureCount_(0)
    {
    }

    void start()
    {
        startReceive();
        startTimer();
    }

private:

    void startReceive()
    {
        socket_.async_receive_from(boost::asio::buffer(recvBuffer_), remoteEndpoint_,
            [&](const boost::system::error_code& error, std::size_t bytes_transferred) {
                if (!error)
                {
                    std::string receivedMessage(recvBuffer_.data(), bytes_transferred);
                    if (receivedMessage == "Heartbeat")
                    {
                        std::cout << "Received heartbeat." << std::endl;
                        ackFailureCount_ = 0;

                        // Reset the timer for next heartbeat
                        restartTimer();
                    }
                    else
                    {
                        std::cerr << "Received unexpected message: " << receivedMessage << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Error receiving message: " << error.message() << std::endl;
                }

                startReceive();
            });
    }

    void startTimer()
    {
        timer_.expires_after(std::chrono::minutes(2));
        std::cout << "Timer started." << std::endl;
        timer_.async_wait([this](const boost::system::error_code& error) {
            if (!error)
            {
                handleTimeout();
            }
            else
            {
                handleTimerError(error);
            }
        });
    }

    void restartTimer()
    {
        std::cout << "Restarting timer." << std::endl;
        timer_.cancel();
        startTimer();
    }

    void handleTimeout()
    {
        std::cerr << "Timeout: No heartbeat received within 1 minute. Perform restart action..." << std::endl;
        ackFailureCount_++;
        if (ackFailureCount_ >= 1)
        {
            ackFailureCount_ = 0;
            startStationSoftware();
        }

        // Restart the timer to keep checking for the next timeout
        startTimer();
    }

    void handleTimerError(const boost::system::error_code& error)
    {
        // Attempt to recover from the error by resetting the timer
        if (error == boost::asio::error::operation_aborted) {
            // Timer was cancelled, possibly by design. No need to take further action.
            return;
        }

        try
        {
            std::cerr << "Timer Error: " << error.message() << std::endl;
            startTimer();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception while handling timer error: " << e.what() << std::endl;
        }
    }

    bool isProcessRunning(const std::string& processName)
    {
        std::string command = "sudo pgrep " + processName + " > /dev/null";
        int result = system(command.c_str());
        return (result == 0);
    }

    bool killProcess(const std::string& processName)
    {
        std::string command = "sudo pkill -15 ";
        command += processName;
        int result = system(command.c_str());

        if (result == 0)
        {
            std::cout << "Successfully sent SIGTERM signal to the application." << std::endl;
        }
        else
        {
            std::cout << "Failed to send SIGTERM signal." << std::endl;
        }

        return (result == 0);
    }

    std::string findExecutableFilePath(const std::string& processName)
    {
        std::array<char, 128> buffer;
        std::string result;

        std::string command = "find / -type f -executable -name " + processName + " 2>/dev/null";

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe)
        {
            std::cout << "popen() failed!" << std::endl;
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
            result.pop_back();
        }

        return result;
    }

    bool startStationSoftware()
    {
        std::string appName = "linuxpbs";
        std::string executableAppPath = findExecutableFilePath(appName);
        //std::string command = "x-terminal-emulator -e " + executableAppPath + " &";
        std::string command = executableAppPath + " &";

        if (isProcessRunning(appName))
        {
            std::cout << "Process is running and kill the process." << std::endl;
            if (killProcess(appName))
            {
                usleep(1000000);
            }
            else
            {
                std::cout << "Unable to kill the process." << std::endl;
            }
        }

        if (!isProcessRunning(appName))
        {
            mode_t permissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
            std::cout << "Updating the executable file permissions..." << std::endl;
            int retPermissions = chmod(executableAppPath.c_str(), permissions);
            if (retPermissions == 0)
            {
                std::cout << "Successfully updated the executable file permissions." << std::endl;
                std::cout << "Running the new firmware..." << std::endl;
                int result = std::system(command.c_str());
                if (result == 0)
                {
                    usleep(20000000);
                    if (isProcessRunning(appName))
                    {
                        std::cout << "Process is running." << std::endl;
                        return true;
                    }
                    else
                    {
                        std::cout << "Process not running." << std::endl;
                        return false;
                    }
                }
                else
                {
                    std::cout << "Unable to use command to start station software." << std::endl;
                    return false;
                }
            }
            else
            {
                std::cout << "Unable to change the executable file permission." << std::endl;
                return false;
            }
            
        }
        else
        {
            std::cout << "Process not running." << std::endl;
            return false;
        }
    }

    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint remoteEndpoint_;
    std::array<char, 1024> recvBuffer_;
    boost::asio::steady_timer timer_;
    int ackFailureCount_;
};

int main() {

    try {
        io_service ioService;
        IniParser::getInstance()->FnReadIniFile();
        UdpClient udpClient(ioService, IniParser::getInstance()->FnGetCentralDBServer(), 2010, 2009);
		udpClient.startSend("Admin is starting");
        Watchdog watchdog(ioService, 9000);
        watchdog.start();
        ioService.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}

void UdpClient::ProcessData (const char* data, std::size_t length) 
{
    // Your processing logic here
    // Example: Print the received data
    int n,i, result;
    int rxcmd;

    std::cout << "Received data12: " << std::string(data, length) << std::endl;
     
    ParseData pField('[',']','|');
    n=pField.Parse(data);

    if (n < 4)
    {
        return;
    }

    rxcmd = stoi(pField.Field(2));
    const char* AppName = "main";
    
    switch(rxcmd)
    {
        // start station software
        case 10:
        {
            std::cout << "Running... Start Station Software" << std::endl;
            bool ret = startStationSoftware();
            if (ret)
            {
                std::cout << "Start the station software successfully." << std::endl;
                sendMsg2Server("10", "99");
            }
            else
            {
                std::cout << "Failed to start the station software." << std::endl;
                sendMsg2Server("10", "98");
            }
            break;
        }
        // Restart Station PC 
        case 12:
        {
            std::cout << "Running... Restart Station PC" << std::endl;
            result = std::system("sudo shutdown -r now");
            // Check the result of the system call
            if (result == 0) {
                std:: cout << "Restart station PC executed successfully" << std::endl;
                sendMsg2Server("12", "99");
            } else {
                std:: cout << "There was an issue executing restart" << std::endl; 
            } 
            break;
        }
        // shutdown station PC
        case 14:
        {
            std::cout << "Running... Shutdown Station PC" << std::endl;
            result = std::system("sudo shutdown -h -F now");
            if (result == 0) {
                std:: cout << "shutdown executed successfully" << std::endl;
                sendMsg2Server("14", "99");
            } else {
                std:: cout << "unable to execut shutdown command" << std::endl; 
            } 
            break;
        }
        // stop sunpark admin 
        case 16:
        {
            std::cout << "Running... Stop Admin Software" << std::endl;
            std::exit(0);
            break;
        }
        // update firmware
        case 307:
        {
            std::cout << "Running... Upgrade the Station Software" << std::endl;
            std::vector<std::string> dataTokens;
			std::stringstream ss(pField.Field(3));
			std::string token;

			while (std::getline(ss, token, ','))
			{
				dataTokens.push_back(token);
			}

			if (dataTokens.size() == 2)
            {
                std::replace(dataTokens[0].begin(), dataTokens[0].end(), '\\', '/');
                std::cout << "Path : " << dataTokens[0] << std::endl;
                std::cout << "File name : " << dataTokens[1] << std::endl;
                if (firmwareUpdate(dataTokens[0], dataTokens[1]))
                {
                    std:: cout << "Firmware update successfully." << std::endl;
                    sendMsg2Server("308", "99");
                }
                else
                {
                    std:: cout << "Firmware update failed." << std::endl;
                    sendMsg2Server("308", "98");
                }
            }
            else
            {
                std:: cout << "Invalid parameters in received data." << std::endl;
                sendMsg2Server("308", "98");
            }
            break;
        }
        default:
        {
            std::cout << "URX: unknown command" << std::endl;
            break;
        }
    }
}

bool UdpClient::isProcessRunning(const std::string& processName)
{
    std::string command = "sudo pgrep " + processName + " > /dev/null";
    int result = system(command.c_str());
    return (result == 0);
}

bool UdpClient::killProcess(const std::string& processName)
{
    std::string command = "sudo pkill -15 ";
    command += processName;
    int result = system(command.c_str());

    if (result == 0)
    {
        std::cout << "Successfully sent SIGTERM signal to the application." << std::endl;
    }
    else
    {
        std::cout << "Failed to send SIGTERM signal." << std::endl;
    }

    return (result == 0);
}

std::string UdpClient::findExecutableFilePath(const std::string& processName)
{
    std::array<char, 128> buffer;
    std::string result;

    std::string command = "find / -type f -executable -name " + processName + " 2>/dev/null";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
        std::cout << "popen() failed!" << std::endl;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
        result.pop_back();
    }

    return result;
}

std::string UdpClient::findExecutableDirectoryPath(const std::string& path)
{
    std::string directoryPath = "";

    std::size_t lastSlashIdx = path.find_last_of("/");
    if (lastSlashIdx != std::string::npos)
    {
        directoryPath = path.substr(0, lastSlashIdx);
    }
    return directoryPath;
}

void UdpClient::rollbackOldFirmware(const std::string& executableAppPath, const std::string& backupAppName, const std::string& originalAppName, const std::string& command)
{
    // 4. if unable to run the new app, then rollback the old app (rename back and run)
    std::cout << "Failed to run the new firmware, removing the new firmware and rolling back to old firmware." << std::endl;
    std::filesystem::remove(executableAppPath);
    std::rename(backupAppName.c_str(), executableAppPath.c_str());
    int ret = std::system(command.c_str());

    if (ret == 0)
    {
        usleep(20000000);
        if (isProcessRunning(originalAppName))
        {
            std::cout << "Successfully rollback the old firmware and run." << std::endl;
        }
        else
        {
            std::cout << "Failed to run old firmware." << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to rollback the old firmware and run." << std::endl;
    }
}

bool UdpClient::copyFirmware(const std::string& appName, const std::string& mountPoint, const std::string& sharedFolderPath, 
                        const std::string& username, const std::string& password, const std::string& outputFolderPath)
{
    // Create the mount poin directory if doesn't exist
    if (!std::filesystem::exists(mountPoint))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(mountPoint, ec))
        {
            std::cout << "Failed to create " << mountPoint << " directory : " << ec.message() << std::endl;
            return false;
        }
        else
        {
            std::cout << "Successfully to create " << mountPoint << " directory." << std::endl;
        }
    }
    else
    {
        std::cout << "Mount point directory: " << mountPoint << " exists." << std::endl;
    }

    // Mount the shared folder
    std::string mountCommand = "sudo mount -t cifs " + sharedFolderPath + " " + mountPoint +
                                " -o username=" + username + ",password=" + password;
    int mountStatus = std::system(mountCommand.c_str());
    if (mountStatus != 0)
    {
        std::cout << "Failed to mount " << mountPoint << std::endl;
        return false;
    }
    else
    {
        std::cout << "Successfully to mount " << mountPoint << std::endl;
    }

    // Create the output folder if it doesn't exist
    if (!std::filesystem::exists(outputFolderPath))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(outputFolderPath, ec))
        {
            std::cout << "Failed to create " << outputFolderPath << " directory : " << ec.message() << std::endl;
            umount(mountPoint.c_str()); // Unmount if folder creation fails
            return false;
        }
        else
        {
            std::cout << "Successfully to create " << outputFolderPath << " directory." << std::endl;
        }
    }
    else
    {
        std::cout << "Output folder directory : " << outputFolderPath << " exists." << std::endl;
    }

    // Copy files to mount point
    bool foundNewFirmware = false;
    std::filesystem::path folder(mountPoint);
    if (std::filesystem::exists(folder) && std::filesystem::is_directory(folder))
    {
        for (const auto& entry : std::filesystem::directory_iterator(folder))
        {
            std::string filename = entry.path().filename().string();

            if (filename == appName)
            {
                foundNewFirmware = true;
                std::filesystem::path dest_file = outputFolderPath / entry.path().filename();
                std::filesystem::copy(entry.path(), dest_file, std::filesystem::copy_options::overwrite_existing);

                std::cout << "Copy " << entry.path() << " to " << dest_file << " successfully" << std::endl;
                break;
            }
        }
    }
    else
    {
        std::cout << "Folder doesn't exist or is not a directory." << std::endl;
        umount(mountPoint.c_str());
        return false;
    }

    // Unmount the shared folder
    std::string unmountCommand = "sudo umount " + mountPoint;
    int unmountStatus = std::system(unmountCommand.c_str());
    if (unmountStatus != 0)
    {
        std::cout << "Failed to unmount " << mountPoint << std::endl;
        return false;
    }
    else
    {
        std::cout << "Successfully to unmount " << mountPoint << std::endl;
    }

    if (!foundNewFirmware)
    {
        std::cout << " file not found." << std::endl;
        return false;
    }

    return true;
}

bool UdpClient::firmwareUpdate(const std::string& firmwarePath, const std::string& firmwareName)
{
    if (firmwarePath.empty())
    {
        std::cout << "Firmware file path is empty." << std::endl;
        return false;
    }

    std::string originalAppName = firmwareName;

    std::string executableAppPath  = findExecutableFilePath(originalAppName);
    std::cout << "Executable path : " << executableAppPath << std::endl;

    std::cout << "Check whether process is running." << std::endl;
    if (!executableAppPath.empty())
    {
        if (isProcessRunning(originalAppName))
        {
            std::cout << "Process is running and kill the process." << std::endl;
            if (killProcess(originalAppName))
            {
                usleep(1000000);
            }
            else
            {
                std::cout << "Unable to kill the process." << std::endl;
            }
        }

        // Check again the process to ensure it is stopped running
        if (!isProcessRunning(originalAppName))
        {
            std::cout << "Process status : stopped run." << std::endl;

            std::string executableAppDirectoryPath = findExecutableDirectoryPath(executableAppPath);
            std::cout << "Executable directory path : " << executableAppDirectoryPath << std::endl;

            // 1. Rename appName to appName + datetime
            CE_Time dt;
            std::string dtStr= dt.DateTimeNumberOnlyString();
            std::string backupAppName = executableAppPath + dtStr.substr(0,12);

            std::rename(executableAppPath.c_str(), backupAppName.c_str());
            std::cout << "Rename existing app for backup : " << backupAppName << std::endl;
            std::string mountPoint = "/mnt/firmware";
            std::string username = "sunpark";
            std::string password = "Tdxh638*";
            std::string outputFolderPath = executableAppDirectoryPath;

            //std::string command = "x-terminal-emulator -e " + executableAppPath + " &";
            std::string command = executableAppPath + " &";

            // 2. Copy the firmware from other PC
            if (copyFirmware(firmwareName, mountPoint, firmwarePath, username, password, outputFolderPath))
            {
                std::cout << "Successfully copy the firmware." << std::endl;

                // Define the desired permissions (e.g., 0755 for read/execute permissions for owner, group, and others)
                mode_t permissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
                std::cout << "Updating the executable file permissions..." << std::endl;
                int retPermissions = chmod(executableAppPath.c_str(), permissions);
                if (retPermissions == 0)
                {
                    std::cout << "Successfully updated the executable file permissions." << std::endl;
                    std::cout << "Running the new firmware..." << std::endl;
                    int result = std::system(command.c_str());
                    if (result == 0)
                    {
                        usleep(20000000);
                        if (isProcessRunning(originalAppName))
                        {
                            // 3. Run the new app
                            std::cout << "Successfully run the new firmware." << std::endl;
                            std::cout << "Move backup firmware to backup folder." << std::endl;
                            std::string directoryPath = "/home/root/carpark/FirmwareBackUp";

                            std::size_t lastSlashIdx = backupAppName.find_last_of("/");
                            std::string destPath = directoryPath + backupAppName.substr(lastSlashIdx);

                            std::cout << "Backup Firmware is stored at : " << destPath << std::endl;
                            
                            std::filesystem::create_directories(directoryPath);
                            std::filesystem::rename(backupAppName.c_str(), destPath.c_str());
                        }
                        else
                        {
                            std::cout << "New firmware not running." << std::endl;
                            rollbackOldFirmware(executableAppPath, backupAppName, originalAppName, command);
                            return false;
                        }
                    }
                    else
                    {
                        std::cout << "Failed to run new firmware." << std::endl;
                        rollbackOldFirmware(executableAppPath, backupAppName, originalAppName, command);
                        return false;
                    }
                }
                else
                {
                    std::cout << "Failed to update the file permissions." << std::endl;
                    rollbackOldFirmware(executableAppPath, backupAppName, originalAppName, command);
                    return false;
                }
            }
            else
            {
                std::cout << "Failed to copy the firmware." << std::endl;
                rollbackOldFirmware(executableAppPath, backupAppName, originalAppName, command);
                return false;
            }
        }
        else
        {
            std::cout << "Process status : running. Unable to upgrade firmware." << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "No executable file path found. Unable to upgrade firmware." << std::endl;
        return false;
    }

    return true;
}

std::string UdpClient::getIPAddress() 
{
    FILE* pipe = popen("ifconfig", "r");
    if (!pipe) {
        std::cerr << "Error in popen\n";
        return "";
    }

    char buffer[128];
    std::string result = "";
    int iRet = 0;

    // Read the output of the 'ifconfig' command
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
         if (iRet == 1) {
             size_t pos = result.find("inet addr:");
             result = result.substr(pos + 10);
             result.erase(result.find(' '));
            break;
         }

         if (result.find("eth0") != std::string::npos) {
             iRet = 1;
         }
    }
    
    pclose(pipe);

    // Output the result
    return result;

}

void UdpClient::sendMsg2Server(std::string cmdCode, std::string data)
{
    std::string dStr = "[" + getIPAddress() + "|" + IniParser::getInstance()->FnGetStationID() + "|" + cmdCode + "|" + data + "|]";
    std::cout << "Data string : " << dStr << std::endl;
    startSend(dStr);
}


bool UdpClient::startStationSoftware()
{
    std::string appName = "linuxpbs";
    std::string executableAppPath = findExecutableFilePath(appName);
    //std::string command = "x-terminal-emulator -e " + executableAppPath + " &";
    std::string command = executableAppPath + " &";

    if (isProcessRunning(appName))
    {
        std::cout << "Process is running and kill the process." << std::endl;
        if (killProcess(appName))
        {
            usleep(1000000);
        }
        else
        {
            std::cout << "Unable to kill the process." << std::endl;
        }
    }

    if (!isProcessRunning(appName))
    {
        mode_t permissions = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        std::cout << "Updating the executable file permissions..." << std::endl;
        int retPermissions = chmod(executableAppPath.c_str(), permissions);
        if (retPermissions == 0)
        {
            std::cout << "Successfully updated the executable file permissions." << std::endl;
            std::cout << "Running the new firmware..." << std::endl;
            int result = std::system(command.c_str());
            if (result == 0)
            {
                usleep(20000000);
                if (isProcessRunning(appName))
                {
                    std::cout << "Process is running." << std::endl;
                    return true;
                }
                else
                {
                    std::cout << "Process not running." << std::endl;
                    return false;
                }
            }
            else
            {
                std::cout << "Unable to use command to start station software." << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Unable to change the executable file permission." << std::endl;
            return false;
        }
        
    }
    else
    {
        std::cout << "Process not running." << std::endl;
        return false;
    }
}
