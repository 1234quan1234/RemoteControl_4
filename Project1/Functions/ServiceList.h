#pragma once
#include "..\Libs\Header.h"

class ServiceList {
public:
    ServiceList();
    ~ServiceList();
    bool writeServicesToFile(const std::string& filename);
    bool startService(const wchar_t* serviceName);
    bool stopService(const wchar_t* serviceName);

private:
    SC_HANDLE schSCManager;
    std::string getServiceStatusString(DWORD dwCurrentState);
    std::string wcharToString(LPWSTR wstr);  // Add declaration
    const std::vector<std::wstring> CRITICAL_SERVICES = {
        L"wuauserv",      // Windows Update
        L"WinDefend",     // Windows Defender
        L"Dhcp",          // DHCP Client
        L"Dnscache",      // DNS Cache
        L"LanmanServer",  // Server
        L"LanmanWorkstation", // Workstation
        L"nsi",           // Network Store Interface
        L"W32Time",       // Windows Time
        L"EventLog"       // Event Log
    };
    bool isCriticalService(const wchar_t* serviceName);
};