#pragma once
#include "..\Libs\Header.h"

class SystemInfo {
private:
    string hostname;
    
    void initializeWinsock();
    

public:
    string localIP;
    SystemInfo();
    ~SystemInfo();
    void getSystemInfo();
    void display() const;
    void waitForInput() const;
};