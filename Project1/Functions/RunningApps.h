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
    static void startAppsFromShortcuts(const vector<string>& appNames, const string& logFileName);
    static void endSelectedTasks(const vector<string>& appNames, const string& logFileName);
private:
    static SIZE_T getProcessMemoryUsage(HANDLE process);
    static std::string WCharToString(const WCHAR* wchar);
    
    static wstring FindShortcutPath(const string& appName);

    // Common locations for shortcuts
    static const vector<wstring> shortcutLocations;
};

