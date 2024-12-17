#pragma once
#include "..\Libs\Header.h"

class ServerManager;
class GmailAPI;
class ServerConfig;

class EmailMonitor {
private:
    GmailAPI& gmail;
    ServerConfig& config;
    ServerManager& server; 

public:
    EmailMonitor(GmailAPI& api, ServerConfig& cfg, ServerManager& srv);
    bool checkForCommands();
};