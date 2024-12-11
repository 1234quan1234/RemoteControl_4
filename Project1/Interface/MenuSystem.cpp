#include "MenuSystem.h"
#include "..\Functions\RunningApps.h"

MenuSystem::MenuSystem(GmailAPI& api) : api(api) {}

void MenuSystem::clearScreen() const {
    system("cls");  // Windows-specific clear screen command
}

void MenuSystem::displayMenu() const {
    cout << "\n=== Gmail Reader ===\n"
        << "1. Read Recent Emails\n"
        << "2. Take Screenshot\n"
        << "3. List Running Apps\n"
        << "4. Exit\n"
        << "Enter choice: ";
}

void MenuSystem::readEmails() const {
    cout << "\nFetching recent emails...\n";
    auto emails = api.getRecentEmails();

    if (emails.empty()) {
        cout << "No emails found in the last 7 days.\n";
        return;
    }

    cout << "\nFound " << emails.size() << " recent emails:\n";
    for (size_t i = 0; i < emails.size(); ++i) {
        cout << "\n=== Email " << (i + 1) << " ===\n"
            << "----------------------------------------\n"
            << emails[i] << "\n"
            << "----------------------------------------\n";
    }
}

void MenuSystem::listRunningApps() const {
    cout << "\nGetting running applications...\n\n";
    auto apps = RunningApps::getRunningApps();

    cout << "Found " << apps.size() << " running applications:\n\n";
    for (const auto& app : apps) {
        cout << "Name: " << app.name << "\n"
            << "PID: " << app.processId << "\n"
            << "Memory Usage: " << (app.memoryUsage / 1024.0 / 1024.0) << " MB\n"
            << "------------------------\n";
    }
}

void MenuSystem::waitForInput() const {
    cout << "\nPress Enter to continue...";
    cin.get();
}


void MenuSystem::run() {
    while (true) {
        clearScreen();
        displayMenu();

        string input;
        getline(cin, input);

        try {
            int choice = stoi(input);

            switch (choice) {
            case 1: {
                clearScreen();
                readEmails();
                cout << "\nPress Enter to return to menu...";
                cin.get();
                break;
            }
            case 2: {
                clearScreen();
                cout << "Taking screenshot...\n";
                if (screenshotHandler.captureWindow("screenshot.png")) {
                    cout << "Screenshot saved as screenshot.png\n";
                }
                else {
                    cout << "Failed to capture screenshot\n";
                }
                cout << "\nPress Enter to return to menu...";
                cin.get();
                break;
            }
            case 3: {
                clearScreen();
                listRunningApps();
                cout << "\nPress Enter to return to menu...";
                cin.get();
                break;
            }
            case 4:
                cout << "\nGoodbye!\n";
                return;
            default:
                cout << "\nPlease enter 1-4\n";
                cout << "Press Enter to continue...";
                cin.get();
            }
        }
        catch (...) {
            cout << "\nPlease enter a valid number\n";
            cout << "Press Enter to continue...";
            cin.get();
        }
    }
}