#include "RunningApps.h"

const vector<wstring> RunningApps::shortcutLocations = {
    L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs",
    L"C:\\Users\\Default\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs",
    L"C:\\Users\\Public\\Desktop"
};

SIZE_T RunningApps::getProcessMemoryUsage(HANDLE process) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

std::string RunningApps::WCharToString(const WCHAR* wchar) {
    int size = WideCharToMultiByte(CP_UTF8, 0, wchar, -1, nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wchar, -1, &str[0], size, nullptr, nullptr);
    return str;
}

wstring RunningApps::FindShortcutPath(const string& appName) {
    // Expand environment variables in paths
    wchar_t expandedPath[MAX_PATH];

    for (const auto& location : shortcutLocations) {
        // Expand environment variables
        ExpandEnvironmentStringsW(location.c_str(), expandedPath, MAX_PATH);

        // Check access rights
        DWORD attributes = GetFileAttributesW(expandedPath);
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            DWORD error = GetLastError();
            continue; // Skip inaccessible locations
        }

        wstring searchPattern = expandedPath + wstring(L"\\*") +
            wstring(appName.begin(), appName.end()) + L"*.lnk";

        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            wstring fullPath = expandedPath + wstring(L"\\") + findData.cFileName;
            FindClose(hFind);
            return fullPath;
        }
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose(hFind);
        }
    }
    return L"";
}

vector<ProcessInfo> RunningApps::getRunningApps() {
    vector<ProcessInfo> apps;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE) {
        return apps;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(processEntry);

    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, processEntry.th32ProcessID);

            if (processHandle) {
                ProcessInfo info;
                info.processId = processEntry.th32ProcessID;
                info.name = string(begin(processEntry.szExeFile),
                    end(processEntry.szExeFile));
                info.memoryUsage = getProcessMemoryUsage(processHandle);

                // Convert process name from wide string
                char processName[MAX_PATH];
                size_t numChars;
                wcstombs_s(&numChars, processName, MAX_PATH,
                    processEntry.szExeFile, wcslen(processEntry.szExeFile));
                info.name = processName;

                apps.push_back(info);
                CloseHandle(processHandle);
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);

    // Sort by memory usage
    sort(apps.begin(), apps.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.memoryUsage > b.memoryUsage;
        });

    return apps;
}

void RunningApps::startAppsFromShortcuts(const vector<string>& appNames, const string& logFileName) {
    ofstream logFile(logFileName, ios::app);
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    logFile << "\n=== App Launch Log " << timeStr << "===\n";

    for (const auto& appName : appNames) {
        wstring shortcutPath = FindShortcutPath(appName);

        if (shortcutPath.empty()) {
            logFile << "Failed to find shortcut for: " << appName << "\n";
            continue;
        }

        HINSTANCE result = ShellExecuteW(
            NULL,           // Parent window
            L"open",       // Operation
            shortcutPath.c_str(),  // File to execute
            NULL,          // Parameters
            NULL,          // Working directory
            SW_SHOWNORMAL  // Show command
        );

        if ((INT_PTR)result > 32) {
            logFile << "Successfully launched: " << appName << "\n";
        }
        else {
            DWORD error = GetLastError();
            logFile << "Failed to launch: " << appName
                << " (Error code: " << error << ")\n";
        }
    }

    logFile << "=== End of Log ===\n\n";
    logFile.close();
}

void RunningApps::endSelectedTasks(const vector<string>& appNames, const string& logFileName) {
    ofstream logFile(logFileName, ios::app);
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    logFile << "\n=== Task Termination Log " << timeStr << "===\n";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        logFile << "Failed to create process snapshot\n";
        return;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(processEntry);

    if (Process32FirstW(snapshot, &processEntry)) {
        do {
            string processName = WCharToString(processEntry.szExeFile);

            for (const auto& appName : appNames) {
                if (_stricmp(processName.c_str(), appName.c_str()) == 0) {
                    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);

                    if (processHandle) {
                        if (TerminateProcess(processHandle, 1)) {
                            logFile << "Successfully terminated " << appName
                                << " (PID: " << processEntry.th32ProcessID << ")\n";
                        }
                        else {
                            DWORD error = GetLastError();
                            logFile << "Failed to terminate " << appName
                                << " (PID: " << processEntry.th32ProcessID
                                << ") - Error code: " << error << "\n";
                        }
                        CloseHandle(processHandle);
                    }
                    else {
                        DWORD error = GetLastError();
                        logFile << "Failed to open process " << appName
                            << " (PID: " << processEntry.th32ProcessID
                            << ") - Error code: " << error << "\n";
                    }
                }
            }
        } while (Process32NextW(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    logFile << "=== End of Log ===\n\n";
    logFile.close();
}
