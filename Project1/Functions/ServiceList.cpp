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

bool ServiceList::startService(const wchar_t* serviceName) {
    if (isCriticalService(serviceName)) {
        std::wcout << L"Cannot modify critical service: " << serviceName << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenServiceW(schSCManager, serviceName,
        SERVICE_START | SERVICE_QUERY_STATUS);
    if (!schService) {
        std::cout << "Failed to open service. Error: " << GetLastError() << "\n";
        return false;
    }

    bool result = ::StartServiceW(schService, 0, NULL);
    if (!result) {
        std::cout << "Failed to start service. Error: " << GetLastError() << "\n";
    }

    CloseServiceHandle(schService);
    return result;
}

bool ServiceList::stopService(const wchar_t* serviceName) {
    if (isCriticalService(serviceName)) {
        std::wcout << L"Cannot modify critical service: " << serviceName << std::endl;
        return false;
    }

    SC_HANDLE schService = OpenServiceW(schSCManager, serviceName,
        SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!schService) {
        std::cout << "Failed to open service. Error: " << GetLastError() << "\n";
        return false;
    }

    SERVICE_STATUS status;
    bool result = ControlService(schService, SERVICE_CONTROL_STOP, &status);
    if (!result) {
        std::cout << "Failed to stop service. Error: " << GetLastError() << "\n";
    }

    CloseServiceHandle(schService);
    return result;
}