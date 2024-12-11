#include "SystemInfo.h"

SystemInfo::SystemInfo() {
    initializeWinsock();
    getSystemInfo();
}

SystemInfo::~SystemInfo() {
    WSACleanup();
}

void SystemInfo::initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw runtime_error("WSAStartup failed");
    }
}

void SystemInfo::getSystemInfo() {
    char hostnameBuffer[256];
    gethostname(hostnameBuffer, sizeof(hostnameBuffer));
    hostname = hostnameBuffer;

    struct addrinfo hints = {}, * addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname.c_str(), NULL, &hints, &addrs) == 0) {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)addrs->ai_addr;
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip, INET_ADDRSTRLEN);
        localIP = ip;
        freeaddrinfo(addrs);
    }
}

void SystemInfo::display() const {
    cout << "=== System Information ===" << endl;
    cout << "Hostname: " << hostname << endl;
    cout << "Local IP: " << localIP << endl;
}

void SystemInfo::waitForInput() const {
    cout << "\nPress Enter to continue...";
    cin.get();
}