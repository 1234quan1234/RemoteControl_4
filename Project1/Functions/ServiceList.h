#pragma once
#include "..\Libs\Header.h"

class ServiceList {
public:
    ServiceList();
    ~ServiceList();
    bool writeServicesToFile(const std::string& filename);
    bool startService(const vector<string>& serviceNames, const string& logFileName);
    bool stopService(const vector<string>& serviceNames, const string& logFileName);

private:
    SC_HANDLE schSCManager;
    bool isAdmin;  // Add flag to track admin status
    bool checkAdminRights();  // Add function to check admin rights
    bool elevatePrivileges(); // Add function to elevate privileges
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