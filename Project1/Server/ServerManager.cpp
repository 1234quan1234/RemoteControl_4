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

string ServerManager::getServerName() {
	return gmail.getServerName();
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
        //logActivity("Processing commands");
    }
    else {
	    this->currentCommand.content = "";
		this->currentCommand.from = "";
		this->currentCommand.message = "";
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
        this->currentCommand.content = subject.substr(subject.find("Command") + 9);
        
        string subject = command["Subject"].asString();
        string fromEmail = command["From"].asString();
		this->currentCommand.from = fromEmail;


        // Xử lý request access trước
        if (this->currentCommand.content == "requestAccess") {
            this->currentCommand.from = fromEmail;
            return;
        }

        // Sau đó mới check quyền truy cập cho các lệnh khác
        if (!isEmailApproved(fromEmail)) {
            gmail.sendSimpleEmail(fromEmail, "Access Denied",
                "You need to request access first.");
            this->currentCommand.message = "Access denied. Request access first.";            

            return;
        }

        else if (this->currentCommand.content == "listProcess") {
            handleProcessListCommand(command);
            return; 
        }
		else if (this->currentCommand.content == "startProcess") {
			handleStartProcess(command);
			return;
		}
        else if (this->currentCommand.content == "endProcess") {
            handleEndProcess(command);
            return;
        }
        else if (this->currentCommand.content == "readRecentEmails") {
            handleReadRecentEmailsCommand(command);
            return;
        }
        else if (this->currentCommand.content == "captureScreen") {
            handleCaptureScreen(command);
            return;
        }
        else if (this->currentCommand.content == "captureWebcam") {
            handleCaptureWebcam(command);
            return;
        }
        else if (this->currentCommand.content == "trackKeyboard") {
            handleTrackKeyboard(command);
            return;
        }
        else if (this->currentCommand.content == "listService") {
            handleListService(command);
            return;
        }
        else if (this->currentCommand.content == "startService") {
            handleStartService(command);
            return;
        }
        else if (this->currentCommand.content == "endService") {
			handleEndService(command);
            return;
        }
        else if (this->currentCommand.content == "listFile") {
            handleListFile(command);
            return;
        }
        else if (this->currentCommand.content == "Shutdown" || this->currentCommand.content == "Restart" || this->currentCommand.content == "Sleep" || this->currentCommand.content == "Lock" || this->currentCommand.content == "Hibernate") {
			handlePowerCommand(command);
			return;
        }
        else {
            // Handle unknown command
            response["message"] = "Unknown command";
			this->currentCommand.message = "Unknown command";
        }
    }
    else {
        response["message"] = "Invalid command format";
		this->currentCommand.message = "Invalid command format";
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
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

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
    string subject = "Process List";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
        cout << "Process list sent successfully via email" << endl;
		this->currentCommand.message = "Process list sent successfully via email";
    }
    else {
        cout << "Failed to send process list via email" << endl;
		this->currentCommand.message = "Failed to send process list via email";
    }
}

void ServerManager::handleStartProcess(const Json::Value& command) {
    cout << "Handling start process command..." << endl;

    // Get sender email
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Get process names from content
    vector<string> processesToStart;
    string content = command["Content"].asString();

    // Split content by spaces
    istringstream iss(content);
    string process;
    while (iss >> process) {
        processesToStart.push_back(process);
    }

    // Generate log filename
    string logFileName = "D:\\process_start_" + to_string(time(nullptr)) + ".txt";

    // Start processes and log results
    RunningApps::startAppsFromShortcuts(processesToStart, logFileName);

    // Send email with results
    string subject = "Process Start Results";
    string body = "Process start operation log attached.";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, logFileName)) {
        cout << "Start operation results sent successfully via email" << endl;
        this->currentCommand.message = "Start operation results sent successfully";
    }
    else {
        cout << "Failed to send start operation results via email" << endl;
        this->currentCommand.message = "Failed to send start operation results";
    }
}

void ServerManager::handleEndProcess(const Json::Value& command) {
    cout << "Handling end process command..." << endl;

    // Get sender email
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Get process names from content
    vector<string> processesToEnd;
    string content = command["Content"].asString();

    // Split content by spaces
    istringstream iss(content);
    string process;
    while (iss >> process) {
        processesToEnd.push_back(process);
    }

    // Generate log filename
    string logFileName = "D:\\process_end_" + to_string(time(nullptr)) + ".txt";

    // End processes and log results
    RunningApps::endSelectedTasks(processesToEnd, logFileName);

    // Send email with results
    string subject = "Process Termination";
    string body = "Process termination log attached.";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, logFileName)) {
        cout << "Termination results sent successfully via email" << endl;
        this->currentCommand.message = "Termination results sent successfully";
    }
    else {
        cout << "Failed to send termination results via email" << endl;
        this->currentCommand.message = "Failed to send termination results";
    }
}

void ServerManager::handleReadRecentEmailsCommand(const Json::Value& command) {
    cout << "Handling read recent emails command..." << endl;
    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();

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

    string subject = "Recent emails";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
        cout << "Recent received emails sent successfully via email" << endl;
		this->currentCommand.message = "Recent received emails sent successfully via email";
    }
    else {
        cout << "Failed to send recent received emails via email" << endl;
		this->currentCommand.message = "Failed to send recent received emails via email";
    }
}

void ServerManager::handleCaptureWebcam(const Json::Value& command) {
    // Tạo đối tượng WebcamCapture
    WebcamCapture webcamCapture;

    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    string path = "D:\\webcam_capture_" + to_string(time(nullptr)) + ".jpg";

    // Gọi hàm captureImage
    if (webcamCapture.captureImage(path.c_str())) {
        cout << "Webcam captured successfully. Saved in: " << path << endl;
		this->currentCommand.message = "Webcam captured successfully. Saved in: " + path;
    }
    else {
        cout << "Webcam capture failed" << endl;
		this->currentCommand.message = "Webcam capture failed";
    }

    string subject = "Webcam Capture";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, path)) {
        cout << "Webcam capture sent successfully via email" << endl;
		this->currentCommand.message += "\nWebcam capture sent successfully via email";
    }
    else {
        cout << "Failed to send webcam capture via email" << endl;
		this->currentCommand.message += "\nFailed to send webcam capture via email";
    }
}

void ServerManager::handleCaptureScreen(const Json::Value& command) {
    // Create screenshot handler
    ScreenshotHandler screenshotHandler;

    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

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

    string subject = "Screen Capture";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
        cout << "Screen capture sent successfully via email" << endl;
    }
    else {
        cout << "Failed to send screen capture via email" << endl;
    }
}

void ServerManager::handleTrackKeyboard(const Json::Value& command) {
    int duration = command.get("duration", 5).asInt();

    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Create results directory using Windows API
    if (!CreateDirectoryA("results", NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        std::cout << "Failed to create results directory" << std::endl;
		this->currentCommand.message = "Failed to create results directory";
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
		this->currentCommand.message = "Keyboard tracking completed. Log saved to: " + filename;
    }

    string subject = "Keyboard Tracking";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
		cout << "Keyboard tracking log sent successfully via email" << endl;
		this->currentCommand.message += "\nKeyboard tracking log sent successfully via email";

    }
    else {
        cout << "Failed to send screen capture via email" << endl;
		this->currentCommand.message += "\nFailed to send screen capture via email";
    }
}

void ServerManager::handleListService(const Json::Value& command) {

    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

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
		this->currentCommand.message = "Services list saved to: " + filename;
    }
    else {
        std::cout << "Failed to save services list" << std::endl;
		this->currentCommand.message = "Failed to save services list";
    }

	string subject = "List of Services";
    string body = "";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
        cout << "Screen capture sent successfully via email" << endl;
		this->currentCommand.message += "\nScreen capture sent successfully via email";
    }
    else {
        cout << "Failed to send screen capture via email" << endl;
		this->currentCommand.message += "\nFailed to send screen capture via email";
    }
}

void ServerManager::handleStartService(const Json::Value& command) {
    cout << "Handling start service command..." << endl;

    // Get sender email
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Get service names from content
    vector<string> servicesToStart;
    string content = command["Content"].asString();

    // Split content by spaces
    istringstream iss(content);
    string service;
    while (iss >> service) {
        servicesToStart.push_back(service);
    }

    // Generate log filename
    string logFileName = "D:\\service_start_" + to_string(time(nullptr)) + ".txt";

    // Start services and log results
    ServiceList serviceList;
    serviceList.startService(servicesToStart, logFileName);

    // Send email with results
    string subject = "Service Start Results";
    string body = "Service start operation log attached.";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, logFileName)) {
        cout << "Service start results sent successfully via email" << endl;
        this->currentCommand.message = "Service start results sent successfully";
    }
    else {
        cout << "Failed to send service start results via email" << endl;
        this->currentCommand.message = "Failed to send service start results";
    }
}

void ServerManager::handleEndService(const Json::Value& command) {
    cout << "Handling stop service command..." << endl;

    // Get sender email
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Get service names from content
    vector<string> servicesToStop;
    string content = command["Content"].asString();

    // Split content by spaces
    istringstream iss(content);
    string service;
    while (iss >> service) {
        servicesToStop.push_back(service);
    }

    // Generate log filename
    string logFileName = "D:\\service_stop_" + to_string(time(nullptr)) + ".txt";

    // Stop services and log results
    ServiceList serviceList;
    serviceList.stopService(servicesToStop, logFileName);

    // Send email with results
    string subject = "Service Stop Results";
    string body = "Service stop operation log attached.";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, logFileName)) {
        cout << "Service stop results sent successfully via email" << endl;
        this->currentCommand.message = "Service stop results sent successfully";
    }
    else {
        cout << "Failed to send service stop results via email" << endl;
        this->currentCommand.message = "Failed to send service stop results";
    }
}

void ServerManager::handleListFile(const Json::Value& command) {
    // Get the sender's email address
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

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
		this->currentCommand.message = "Files list saved to: " + filename;
    }
    else {
        std::cout << "Failed to save files list" << std::endl;
		this->currentCommand.message = "Failed to save files list";
    }

    string subject = "Hello!";
    string body = "Email Content!";

    if (gmail.sendEmail(this->currentCommand.from, subject, body, filename)) {
        cout << "Screen capture sent successfully via email" << endl;
		this->currentCommand.message += "\nScreen capture sent successfully via email";
    }
    else {
        cout << "Failed to send screen capture via email" << endl;
		this->currentCommand.message += "\nFailed to send screen capture via email";
    }
}

void ServerManager::handlePowerCommand(const Json::Value& command) {
    // Get sender's email
    this->currentCommand.from = command["From"].asString();
    cout << "Sender email: " << this->currentCommand.from << endl;

    // Get power action type from subject
    string actionType = command["Subject"].asString();
    actionType = actionType.substr(actionType.find("Command") + 9); // Skip "Command::"

    bool success = false;
    string resultMessage;

    // Execute power action based on command
    if (actionType == "Shutdown") {
        success = PowerManager::Shutdown(false);
        resultMessage = "Shutdown command executed";
		this->currentCommand.message = "Shutdown command executed";
    }
    else if (actionType == "Restart") {
        success = PowerManager::Restart(false);
        resultMessage = "Restart command executed";
		this->currentCommand.message = "Restart command executed";
    }
    else if (actionType == "Hibernate") {
        success = PowerManager::Hibernate();
        resultMessage = "Hibernate command executed";
		this->currentCommand.message = "Hibernate command executed";
    }
    else if (actionType == "Sleep") {
        success = PowerManager::Sleep();
        resultMessage = "Sleep command executed";
		this->currentCommand.message = "Sleep command executed";
    }
    else if (actionType == "Lock") {
        success = PowerManager::Lock();
        resultMessage = "Lock command executed";
		this->currentCommand.message = "Lock command executed";
    }

    // Send response email
    string subject = "Power Command Result";
    string body = success ? "Successfully executed: " + actionType : "Failed to execute: " + actionType;

    gmail.sendEmail(this->currentCommand.from, subject, body, "");
    cout << "Power command response sent to: " << this->currentCommand.from << endl;
	this->currentCommand.message += "\nPower command response sent to: " + this->currentCommand.from;
}