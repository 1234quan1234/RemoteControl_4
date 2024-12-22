#include "ServiceList.h"

ServiceList::ServiceList() {
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
}

ServiceList::~ServiceList() {
    if (schSCManager) CloseServiceHandle(schSCManager);
}

std::string ServiceList::getServiceStatusString(DWORD dwCurrentState) {
    switch (dwCurrentState) {
    case SERVICE_STOPPED: return "Stopped";
    case SERVICE_START_PENDING: return "Starting";
    case SERVICE_STOP_PENDING: return "Stopping";
    case SERVICE_RUNNING: return "Running";
    case SERVICE_CONTINUE_PENDING: return "Continuing";
    case SERVICE_PAUSE_PENDING: return "Pausing";
    case SERVICE_PAUSED: return "Paused";
    default: return "Unknown";
    }
}

// Implement wcharToString
std::string ServiceList::wcharToString(LPWSTR wstr) {
    if (!wstr) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size, nullptr, nullptr);
    return str;
}

bool ServiceList::writeServicesToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;

    EnumServicesStatusEx(schSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_STATE_ALL, NULL, 0, &bytesNeeded, &servicesReturned,
        &resumeHandle, NULL);

    LPBYTE buffer = new BYTE[bytesNeeded];
    LPENUM_SERVICE_STATUS_PROCESS services =
        (LPENUM_SERVICE_STATUS_PROCESS)buffer;

    if (EnumServicesStatusEx(schSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_STATE_ALL, buffer, bytesNeeded, &bytesNeeded,
        &servicesReturned, &resumeHandle, NULL)) {

        file << "=== Windows Services List ===\n\n";
        for (DWORD i = 0; i < servicesReturned; i++) {
            SC_HANDLE hService = OpenServiceW(schSCManager,
                services[i].lpServiceName,
                SERVICE_QUERY_CONFIG);

            if (hService) {
                DWORD bytesNeeded;
                QueryServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION,
                    NULL, 0, &bytesNeeded);
                LPSERVICE_DESCRIPTION psd = (LPSERVICE_DESCRIPTION)LocalAlloc(
                    LPTR, bytesNeeded);

                if (QueryServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION,
                    (LPBYTE)psd, bytesNeeded, &bytesNeeded)) {

                    file << "Service #" << (i + 1) << "\n";
                    file << "==================\n";
                    file << "System Name: " << wcharToString(services[i].lpServiceName) << "\n";
                    file << "Display Name: " << wcharToString(services[i].lpDisplayName) << "\n";
                    file << "Status: " << getServiceStatusString(
                        services[i].ServiceStatusProcess.dwCurrentState) << "\n";
                    file << "Description: " << (psd->lpDescription ?
                        wcharToString(psd->lpDescription) : "No description available") << "\n";
                    file << "Process ID: " <<
                        services[i].ServiceStatusProcess.dwProcessId << "\n";
                    file << "------------------\n\n";
                }

                LocalFree(psd);
                CloseServiceHandle(hService);
            }
        }
    }

    delete[] buffer;
    file.close();
    return true;
}

bool ServiceList::isCriticalService(const wchar_t* serviceName) {
    std::wstring service(serviceName);
    return std::find(CRITICAL_SERVICES.begin(), CRITICAL_SERVICES.end(), service)
        != CRITICAL_SERVICES.end();
}

bool ServiceList::startService(const vector<string>& serviceNames, const string& logFileName) {
    ofstream logFile(logFileName, ios::app);
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    logFile << "\n=== Service Start Operation Log " << timeStr << "===\n";

    bool allSuccess = true;

    for (const auto& serviceName : serviceNames) {
        wstring wServiceName(serviceName.begin(), serviceName.end());
        logFile << "Attempting to start service: " << serviceName << "\n";

        if (isCriticalService(wServiceName.c_str())) {
            logFile << "Cannot modify critical service: " << serviceName << "\n";
            allSuccess = false;
            continue;
        }

        SC_HANDLE schService = OpenServiceW(schSCManager, wServiceName.c_str(),
            SERVICE_START | SERVICE_QUERY_STATUS);
        if (!schService) {
            DWORD error = GetLastError();
            logFile << "Failed to open service. Error code: " << error << "\n";
            allSuccess = false;
            continue;
        }

        if (::StartServiceW(schService, 0, NULL)) {
            logFile << "Successfully started service: " << serviceName << "\n";
        }
        else {
            DWORD error = GetLastError();
            logFile << "Failed to start service. Error code: " << error << "\n";
            allSuccess = false;
        }

        CloseServiceHandle(schService);
    }

    logFile << "=== End of Log ===\n\n";
    logFile.close();
    return allSuccess;
}

bool ServiceList::stopService(const vector<string>& serviceNames, const string& logFileName) {
    ofstream logFile(logFileName, ios::app);
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);
    logFile << "\n=== Service Stop Operation Log " << timeStr << "===\n";

    bool allSuccess = true;

    for (const auto& serviceName : serviceNames) {
        wstring wServiceName(serviceName.begin(), serviceName.end());
        logFile << "Attempting to stop service: " << serviceName << "\n";

        if (isCriticalService(wServiceName.c_str())) {
            logFile << "Cannot modify critical service: " << serviceName << "\n";
            allSuccess = false;
            continue;
        }

        SC_HANDLE schService = OpenServiceW(schSCManager, wServiceName.c_str(),
            SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (!schService) {
            DWORD error = GetLastError();
            logFile << "Failed to open service. Error code: " << error << "\n";
            allSuccess = false;
            continue;
        }

        SERVICE_STATUS status;
        if (ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
            logFile << "Successfully stopped service: " << serviceName << "\n";
        }
        else {
            DWORD error = GetLastError();
            logFile << "Failed to stop service. Error code: " << error << "\n";
            allSuccess = false;
        }

        CloseServiceHandle(schService);
    }

    logFile << "=== End of Log ===\n\n";
    logFile.close();
    return allSuccess;
}