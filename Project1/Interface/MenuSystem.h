#pragma once
#include "..\Libs\Header.h"
#include "..\GmailAPI\GmailAPI.h"
#include "..\Functions\ScreenshotHandler.h"

class MenuSystem {
private:
    GmailAPI& api;
    ScreenshotHandler screenshotHandler;
    void displayMenu() const;
    void readEmails() const;
    void clearScreen() const;
    void listRunningApps() const;
public:
    MenuSystem(GmailAPI& api);
    void run();
    void waitForInput() const;
};