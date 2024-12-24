#include "RemoteControlApp.h"

// App Initialization
bool RemoteControlApp::OnInit() {

    // Read client secrets
    auto secrets = GmailAPI::ReadClientSecrets("C:\\Users\\GIGABYTE\\Downloads\\Client3.json");
    m_api = new GmailAPI(
        secrets["installed"]["client_id"].asString(),
        secrets["installed"]["client_secret"].asString(),
        "http://localhost:8080"  // Update redirect URI for automatic authentication
    );

    // Try to load saved tokens
    try {
        m_api->loadSavedTokens();

        // If valid tokens exist, go directly to Server Monitor
        if (m_api->hasValidToken()) {
            m_sysInfo = new SystemInfo();
            m_server = new ServerManager(*m_api);

            ServerMonitorFrame* monitorFrame = new ServerMonitorFrame(*m_api, *m_server, *m_sysInfo);
            monitorFrame->Show(true);
        }
        else {
            // If no valid tokens, show Authentication frame
            AuthenticationFrame* authFrame = new AuthenticationFrame(*m_api);
            authFrame->Show(true);
        }
    }
    catch (const std::exception& e) {
        // If any error in loading tokens, show Authentication frame
        AuthenticationFrame* authFrame = new AuthenticationFrame(*m_api);
        authFrame->Show(true);
    }

    return true;
}