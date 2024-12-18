#pragma once
#include "..\Libs\Header.h"

class SystemInfo {
private:
    void initializeWinsock();

public:
    string hostname;
    string localIP;
    SystemInfo();
    ~SystemInfo();
    void getSystemInfo();
    void waitForInput() const;
};