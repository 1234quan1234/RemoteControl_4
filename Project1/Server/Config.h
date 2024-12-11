#pragma once
#include "../Libs/Header.h"

struct ServerConfig {
    string authorizedEmail;
    string serverIP;
    int serverPort;
    int checkInterval;  // milliseconds
    string logFile;
};