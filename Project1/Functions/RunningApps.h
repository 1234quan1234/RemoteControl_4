#pragma once
#include "..\Libs\Header.h"

struct ProcessInfo {
    string name;
    DWORD processId;
    SIZE_T memoryUsage;
};

class RunningApps {
public:
    static vector<ProcessInfo> getRunningApps();
private:
    static SIZE_T getProcessMemoryUsage(HANDLE process);
};


