#include "..\Server\ServerManager.h"
#include "..\Functions\RunningApps.h"
#include "..\Functions\WebcamCapture.h"
#include "..\Functions\KeyboardTracker.h"
#include "..\Functions\ScreenshotHandler.h"
#include "..\Functions\ServiceList.h"
#include "..\Functions\FileList.h"
#include "..\Functions\Power.h"

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
        
        string subject = command["Subject"].asString();
        string fromEmail = command["From"].asString();

        // Xử lý request access trước
        if (commandStr == "requestAccess") {
            // Check if already has access
            auto it = std::find_if(approvedAccess.begin(), approvedAccess.end(),
                [&fromEmail](const AccessInfo& access) {
                    return access.email == fromEmail;
                });

            if (it != approvedAccess.end() && isAccessValid(*it)) {
                // Calculate remaining time
                time_t now = time(nullptr);
                time_t expiryTime = it->grantedTime + (AccessInfo::VALIDITY_HOURS * 3600);
                double hoursLeft = difftime(expiryTime, now) / 3600.0;

                // Format expiry time
                struct tm timeinfo;
                localtime_s(&timeinfo, &expiryTime);
                char timeStr[80];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

                gmail.sendSimpleEmail(fromEmail, "Access Info",
                    "You already have access.\nExpires at: " + string(timeStr) +
                    "\nHours remaining: " + std::to_string(static_cast<int>(hoursLeft)));
                return;
            }

            std::cout << "\nAccess Request from: " << fromEmail << "\n";
            std::cout << "Grant access? (y/n): ";
            char response;
            std::cin >> response;

            if (response == 'y' || response == 'Y') {
                AccessInfo access;
                access.email = fromEmail;
                access.grantedTime = time(nullptr);
                approvedAccess.push_back(access);
                saveAccessList();

                time_t expiryTime = access.grantedTime + (AccessInfo::VALIDITY_HOURS * 3600);
                struct tm timeinfo;
                localtime_s(&timeinfo, &expiryTime);
                char timeStr[80];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

                gmail.sendSimpleEmail(fromEmail, "Access Granted",
                    "Access granted for 24 hours.\nExpires at: " + string(timeStr));
            }
            else {
                gmail.sendSimpleEmail(fromEmail, "Access Denied",
                    "Your access request was denied.");
            }
            return;
        }

        // Sau đó mới check quyền truy cập cho các lệnh khác
        if (!isEmailApproved(fromEmail)) {
            gmail.sendSimpleEmail(fromEmail, "Access Denied",
                "You need to request access first.");
            return;
        }

        else if (commandStr == "listProcess") {
            handleProcessListCommand(command);
            return; 
        }
        else if (commandStr == "readRecentEmails") {
            handleReadRecentEmailsCommand(command);
            return;
        }
        else if (commandStr == "captureScreen") {
            handleCaptureScreen(command);
            return;
        }
        else if (commandStr == "recordScreen") {
            return;
        }
        else if (commandStr == "captureWebcam") {
            handleCaptureWebcam(command);
            return;
        }
        else if (commandStr == "trackKeyboard") {
            handleTrackKeyboard(command);
            return;
        }
        else if (commandStr == "listService") {
            handleListService(command);
            return;
        }
        else if (commandStr == "listFile") {
            handleListFile(command);
            return;
        }
        else if (commandStr == "Shutdown" || commandStr == "Restart" || commandStr == "Sleep" || commandStr == "Lock" || commandStr == "Hibernate") {
			handlePowerCommand(command);
			return;
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

bool ServerManager::isAccessValid(const AccessInfo& access) const {
    time_t now = time(nullptr);
    double hours = difftime(now, access.grantedTime) / 3600.0;
    return hours <= AccessInfo::VALIDITY_HOURS;
}

void ServerManager::cleanupExpiredAccess() {
    auto it = std::remove_if(approvedAccess.begin(), approvedAccess.end(),
        [this](const AccessInfo& access) { return !isAccessValid(access); });
    approvedAccess.erase(it, approvedAccess.end());
}

void ServerManager::saveAccessList() const {
    std::ofstream file("access_list.json");
    if (!file.is_open()) return;

    Json::Value root(Json::arrayValue);
    for (const auto& access : approvedAccess) {
        Json::Value entry;
        entry["email"] = access.email;
        entry["grantedTime"] = Json::Value::Int64(access.grantedTime);
        root.append(entry);
    }

    Json::StyledWriter writer;
    file << writer.write(root);
}

void ServerManager::loadAccessList() {
    std::ifstream file("access_list.json");
    if (!file.is_open()) return;

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(file, root)) return;

    approvedAccess.clear();

    for (const auto& entry : root) {
        AccessInfo access;
        access.email = entry["email"].asString();
        access.grantedTime = entry["grantedTime"].asInt64();
        approvedAccess.push_back(access);
    }
}

bool ServerManager::isEmailApproved(const string& email) {
    loadAccessList();
    
    auto it = std::find_if(approvedAccess.begin(), approvedAccess.end(),
        [&email](const AccessInfo& access) { return access.email == email; });

    if (it != approvedAccess.end()) {
        return isAccessValid(*it);
    }

    return false;
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

    // Get the sender's email address
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

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

    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(senderEmail, subject, body, filename)) {
        cout << "Screen capture sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send screen capture via email" << endl;
    }
}

void ServerManager::handleListService(const Json::Value& command) {

    // Get the sender's email address
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

    // Create timestamp for filename
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

    // Create filename
    std::string filename = "D:\\service_list_" + std::string(timestamp) + ".txt";

    // Create and use ServiceList
    ServiceList services;
    if (services.writeServicesToFile(filename)) {
        std::cout << "Services list saved to: " << filename << std::endl;
    }
    else {
        std::cout << "Failed to save services list" << std::endl;
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

void ServerManager::handleListFile(const Json::Value& command) {
	// Get the sender's email address
	string senderEmail = command["From"].asString();
	cout << "Sender email: " << senderEmail << endl;

	// Create timestamp for filename
	time_t now = time(nullptr);
	struct tm timeinfo;
	localtime_s(&timeinfo, &now);
	char timestamp[64];
	strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &timeinfo);

	// Create filename
	std::string filename = "D:\\file_list_" + std::string(timestamp) + ".txt";

	// Create and use FileList
	FileList files;
	if (files.writeFilesToFile(filename)) {
		std::cout << "Files list saved to: " << filename << std::endl;
	}
	else {
		std::cout << "Failed to save files list" << std::endl;
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

void ServerManager::handlePowerCommand(const Json::Value& command) {
    // Get sender's email
    string senderEmail = command["From"].asString();
    cout << "Sender email: " << senderEmail << endl;

    // Get power action type from subject
    string actionType = command["Subject"].asString();
    actionType = actionType.substr(actionType.find("Command") + 9); // Skip "Command::"

    bool success = false;
    string resultMessage;

    // Execute power action based on command
    if (actionType == "Shutdown") {
        success = PowerManager::Shutdown(false);
        resultMessage = "Shutdown command executed";
    }
    else if (actionType == "Restart") {
        success = PowerManager::Restart(false);
        resultMessage = "Restart command executed";
    }
    else if (actionType == "Hibernate") {
        success = PowerManager::Hibernate();
        resultMessage = "Hibernate command executed";
    }
    else if (actionType == "Sleep") {
        success = PowerManager::Sleep();
        resultMessage = "Sleep command executed";
    }
    else if (actionType == "Lock") {
        success = PowerManager::Lock();
        resultMessage = "Lock command executed";
    }

    // Send response email
    string subject = "Power Command Result";
    string body = success ? "Successfully executed: " + actionType : "Failed to execute: " + actionType;

    gmail.sendEmail(senderEmail, subject, body, "");
    cout << "Power command response sent to: " << senderEmail << endl;
}