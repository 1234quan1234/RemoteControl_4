#include "RunningApps.h"

SIZE_T RunningApps::getProcessMemoryUsage(HANDLE process) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
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