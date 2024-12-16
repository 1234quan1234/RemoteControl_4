#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"

int main() {
    try {
        cout << "=== Gmail Remote Control System ===\n";

        // Initialize API and check/refresh token
        cout << "\nInitializing Gmail API...\n";

        cout << "Loading client secrets...\n";
        auto secrets = GmailAPI::ReadClientSecrets("\\Resources\\ClientSecrets.json");
        cout << "Client secrets loaded.\n";

        cout << "Initializing Gmail API..." << endl;
        GmailAPI api(
            secrets["installed"]["client_id"].asString(),
            secrets["installed"]["client_secret"].asString(),
            secrets["installed"]["redirect_uris"][0].asString()
        );
        cout << "Gmail API initialized" << endl;

        // Check and handle token at startup
        try {
            api.loadSavedTokens();
            if (!api.hasValidToken()) {
                cout << "\nFor Authentication. Please visit:\n"
                    << api.getAuthorizationUrl() << endl;

                string authCode;
                cout << "\nEnter authorization code: ";
                getline(cin, authCode);
                api.authenticate(authCode);
                cout << "Authentication successful!\n" << endl;
            }
        }
        catch (const exception& e) {
            cout << "\nAuthentication required. Please visit:\n"
                << api.getAuthorizationUrl() << endl;

            string authCode;
            cout << "\nEnter authorization code: ";
            getline(cin, authCode);
            api.authenticate(authCode);
            cout << "Authentication successful!\n" << endl;
        }

        SystemInfo sysInfo;
        cout << "=== System Information ===" << endl;
        cout << "Hostname: " << sysInfo.hostname << endl;
        cout << "Local IP: " << sysInfo.localIP << endl;
        cout << "Server's gmail name: " << api.getServerName() << endl;
        cout << "\nServer starting...\n";
        ServerManager server(api);

        cout << "\nServer is running. Press 'Q' to quit.\n";
        bool running = true;
        while (running) {
            server.processCommands();
            if (server.currentCommand != "") {
                cout << "Current Command: " << server.currentCommand << endl;
            }
            else {
                cout << "Waiting for command..." << endl;
            }
            //Nhận lệnh xong thì không thể nhận thêm lệnh nữa, rất nà kỳ cục! mai sửa
            Sleep(5000);
            if (GetAsyncKeyState('Q') & 0x8000) {
                running = false;
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        cout << "Exception: " << e.what() << endl;
    }
    catch (...) {
        cout << "Unknown exception occurred.\n";
    }
}