#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"
#include "Client/Client_Gmail.h"

int main() {
    try {
        cout << "=== Gmail Remote Control System ===\n";

        // Initialize API and check/refresh token
        cout << "\nInitializing Gmail API..." << endl;
        auto secrets = GmailAPI::ReadClientSecrets("C:\\Users\\GIGABYTE\\Downloads\\Client3.json");
        GmailAPI api(
            secrets["installed"]["client_id"].asString(),
            secrets["installed"]["client_secret"].asString(),
            secrets["installed"]["redirect_uris"][0].asString()
        );

        // Check and handle token at startup
        try {
            api.loadSavedTokens();
            if (!api.hasValidToken()) {
                cout << "\nNeed authentication. Please visit:\n"
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

        Client_Gmail client(
            secrets["installed"]["client_id"].asString(),
            secrets["installed"]["client_secret"].asString(),
            secrets["installed"]["redirect_uris"][0].asString()
        );

        /*try {
            
            cout << "\nNeed authentication. Please visit:\n"
                << client.getAuthorizationUrl() << endl;

            string authCode;
            cout << "\nEnter authorization code: ";
            getline(cin, authCode);
            client.authenticate(authCode);
            cout << "Authentication successful!\n" << endl;
        }
        catch (const exception& e) {
            cout << "\nAuthentication required. Please visit:\n"
                << client.getAuthorizationUrl() << endl;

            string authCode;
            cout << "\nEnter authorization code: ";
            getline(cin, authCode);
            client.authenticate(authCode);
            cout << "Authentication successful!\n" << endl;
        }*/

        SystemInfo sysInfo;
        sysInfo.display();
        cout << "\nServer starting...\n";
        ServerManager server(api);

        cout << "\nServer is running. Press 'Q' to quit.\n";
        bool running = true;
        while (running) {
            server.processCommands();
            //Nhận lệnh xong thì không thể nhận thêm lệnh nữa, rất nà kỳ cục! mai sửa
            Sleep(5000);
            if (GetAsyncKeyState('Q') & 0x8000) {
                running = false;
            }
        }

        return 0;
    }
    catch (const exception& e) {
        cerr << "\nFatal error: " << e.what() << endl;
        return 1;
    }
}