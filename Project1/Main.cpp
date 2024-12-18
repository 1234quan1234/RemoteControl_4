#include <wx/wx.h>
#include <wx/hyperlink.h>
#include <wx/clipbrd.h>
#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"

// Add new custom event for access request
wxDECLARE_EVENT(CUSTOM_ACCESS_REQUEST_EVENT, wxCommandEvent);
wxDEFINE_EVENT(CUSTOM_ACCESS_REQUEST_EVENT, wxCommandEvent);

class AuthenticationFrame : public wxFrame {
private:
    wxTextCtrl* m_authCodeCtrl;
    GmailAPI& m_api;
    wxHyperlinkCtrl* m_authUrlLink;

    void OnAuthenticate(wxCommandEvent& event);
    void OnCopyURL(wxHyperlinkEvent& event);

public:
    AuthenticationFrame(GmailAPI& api);
};

class AccessRequestDialog : public wxDialog {
private:
    wxStaticText* m_requestDetailsText;
    wxButton* m_yesButton;
    wxButton* m_noButton;

public:
	AccessRequestDialog(wxWindow* parent, const wxString& fromEmail, bool& accessRequesting);
    int ShowModal() override;
};

class ServerMonitorFrame : public wxFrame {
private:
    GmailAPI& m_api;
    ServerManager& m_server;
    SystemInfo& m_sysInfo;

    wxStaticText* m_hostnameLabel;
    wxStaticText* m_localIPLabel;
    wxStaticText* m_gmailNameLabel;

    wxStaticText* m_currentCommandLabel;
    wxStaticText* m_commandFromLabel;
    wxStaticText* m_commandMessageLabel;

	bool m_accessRequesting = false;

    wxTimer* m_updateTimer;
    int m_blinkCounter;
    int m_maxBlinkCount;

    void UpdateServerInfo();
    void UpdateCommandInfo();
    void OnUpdateTimer(wxTimerEvent& event);
    void OnAccessRequest(wxCommandEvent& event);

public:
    ServerMonitorFrame(GmailAPI& api, ServerManager& server, SystemInfo& sysInfo);
    ~ServerMonitorFrame();

    wxDECLARE_EVENT_TABLE();
};

class RemoteControlApp : public wxApp {
private:
    GmailAPI* m_api;
    ServerManager* m_server;
    SystemInfo* m_sysInfo;

public:
    virtual bool OnInit();
};

// Access Request Dialog Implementation
AccessRequestDialog::AccessRequestDialog(wxWindow* parent, const wxString& fromEmail, bool& accessRequesting)
    : wxDialog(parent, wxID_ANY, "Access Request", wxDefaultPosition, wxSize(400, 200)) {

	accessRequesting = true;

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxPanel* panel = new wxPanel(this, wxID_ANY);

    // Request details text
    m_requestDetailsText = new wxStaticText(panel, wxID_ANY,
        wxString::Format("Access request received from:\n%s\n\nDo you want to grant access?", fromEmail),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    mainSizer->Add(m_requestDetailsText, 0, wxALL | wxCENTER, 20);

    // Button sizer for Yes and No buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_yesButton = new wxButton(panel, wxID_YES, "Yes");
    m_noButton = new wxButton(panel, wxID_NO, "No");

    buttonSizer->Add(m_yesButton, 0, wxALL, 10);
    buttonSizer->Add(m_noButton, 0, wxALL, 10);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER);

    panel->SetSizer(mainSizer);
    mainSizer->Fit(this);
    Center();

	m_yesButton->Bind(wxEVT_BUTTON, [this, &accessRequesting](wxCommandEvent&) {
	    accessRequesting = false;
	    EndModal(wxID_YES);
	});

	m_noButton->Bind(wxEVT_BUTTON, [this, &accessRequesting](wxCommandEvent&) {
	    accessRequesting = false;
	    EndModal(wxID_NO);
	});
}

int AccessRequestDialog::ShowModal() {
    return wxDialog::ShowModal();
}

// Authentication Frame Implementation
AuthenticationFrame::AuthenticationFrame(GmailAPI& api)
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control - Authentication",
        wxDefaultPosition, wxSize(500, 250)),
    m_api(api) {

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Authentication URL with Hyperlink
    wxStaticText* instructLabel = new wxStaticText(panel, wxID_ANY,
        "To authenticate, please follow these steps:");
	wxStaticText* step1Label = new wxStaticText(panel, wxID_ANY,
		"1. Click the link below to open the authorization page in your browser.");
	wxStaticText* step2Label = new wxStaticText(panel, wxID_ANY,
		"2. Copy the authorization code from the browser and paste it in the box below.");
	wxStaticText* step3Label = new wxStaticText(panel, wxID_ANY,
		"3. Click Authenticate to complete the process.");

    mainSizer->Add(instructLabel, 0, wxALL | wxCENTER, 10);
	mainSizer->Add(step1Label, 0, wxALL | wxCENTER, 10);
	mainSizer->Add(step2Label, 0, wxALL | wxCENTER, 10);
	mainSizer->Add(step3Label, 0, wxALL | wxCENTER, 10);

    wxBoxSizer* urlSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* urlTextLabel = new wxStaticText(panel, wxID_ANY, "Authorization URL: ");
    m_authUrlLink = new wxHyperlinkCtrl(panel, wxID_ANY,
        m_api.getAuthorizationUrl(), m_api.getAuthorizationUrl());

    urlSizer->Add(urlTextLabel, 0, wxALIGN_CENTER_VERTICAL);
    urlSizer->Add(m_authUrlLink, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(urlSizer, 0, wxALL | wxCENTER, 10);

    // Copy URL Button
    wxButton* copyUrlBtn = new wxButton(panel, wxID_ANY, "Copy URL");
    copyUrlBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(m_api.getAuthorizationUrl()));
            wxTheClipboard->Close();
            wxMessageBox("URL copied to clipboard!", "Copied", wxOK | wxICON_INFORMATION);
        }
        });
    mainSizer->Add(copyUrlBtn, 0, wxALL | wxCENTER, 10);

    // Authorization Code Input
    wxBoxSizer* authSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* authLabel = new wxStaticText(panel, wxID_ANY, "Enter Authorization Code:");
    m_authCodeCtrl = new wxTextCtrl(panel, wxID_ANY);

    wxButton* authenticateBtn = new wxButton(panel, wxID_ANY, "Authenticate");
    authenticateBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnAuthenticate, this);

    authSizer->Add(authLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    authSizer->Add(m_authCodeCtrl, 1, wxALIGN_CENTER_VERTICAL);
    authSizer->Add(authenticateBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    mainSizer->Add(authSizer, 0, wxALL | wxEXPAND, 10);

    panel->SetSizer(mainSizer);
    mainSizer->Fit(this);
    Center();
}

void AuthenticationFrame::OnAuthenticate(wxCommandEvent& event) {
    try {
        m_api.authenticate(m_authCodeCtrl->GetValue().ToStdString());

        // Create and show ServerMonitorFrame
        SystemInfo* sysInfo = new SystemInfo();
        ServerManager* server = new ServerManager(m_api);

        ServerMonitorFrame* monitorFrame = new ServerMonitorFrame(m_api, *server, *sysInfo);
        monitorFrame->Show(true);

        // Close authentication frame
        Close();
    }
    catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Authentication Failed: %s", e.what()),
            "Error", wxOK | wxICON_ERROR);
    }
}

// Event table for ServerMonitorFrame
wxBEGIN_EVENT_TABLE(ServerMonitorFrame, wxFrame)
EVT_TIMER(wxID_ANY, ServerMonitorFrame::OnUpdateTimer)
EVT_COMMAND(wxID_ANY, CUSTOM_ACCESS_REQUEST_EVENT, ServerMonitorFrame::OnAccessRequest)
wxEND_EVENT_TABLE()

// Server Monitor Frame Implementation
ServerMonitorFrame::ServerMonitorFrame(GmailAPI& api, ServerManager& server, SystemInfo& sysInfo)
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control - Server Monitor",
        wxDefaultPosition, wxSize(600, 400)),
    m_api(api), m_server(server), m_sysInfo(sysInfo),
    m_hostnameLabel(nullptr), m_localIPLabel(nullptr), m_gmailNameLabel(nullptr) {

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Server Info Section
    wxStaticBoxSizer* serverInfoSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Server's Information");

    // Hostname
    m_hostnameLabel = new wxStaticText(panel, wxID_ANY,
        wxString::Format("Hostname: %s", m_sysInfo.hostname));
    serverInfoSizer->Add(m_hostnameLabel, 0, wxALL, 5);

    // Local IP
    m_localIPLabel = new wxStaticText(panel, wxID_ANY,
        wxString::Format("Local IP: %s", m_sysInfo.localIP));
    serverInfoSizer->Add(m_localIPLabel, 0, wxALL, 5);

    // Gmail Name
    m_gmailNameLabel = new wxStaticText(panel, wxID_ANY,
        wxString::Format("Server's Gmail: %s", m_api.getServerName()));
    serverInfoSizer->Add(m_gmailNameLabel, 0, wxALL, 5);

    mainSizer->Add(serverInfoSizer, 0, wxALL | wxEXPAND, 10);

    // Command Processing Section
    wxBoxSizer* commandSizer = new wxBoxSizer(wxHORIZONTAL);

    // Left side - Current Command
    wxStaticBoxSizer* leftSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Current Command");
    m_currentCommandLabel = new wxStaticText(panel, wxID_ANY, "Waiting for command...");
    leftSizer->Add(m_currentCommandLabel, 0, wxALL | wxEXPAND, 5);
    commandSizer->Add(leftSizer, 1, wxEXPAND | wxRIGHT, 5);

    // Right side - Command Details
    wxStaticBoxSizer* rightSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Command Details");
    m_commandFromLabel = new wxStaticText(panel, wxID_ANY, "");
    m_commandMessageLabel = new wxStaticText(panel, wxID_ANY, "");
    rightSizer->Add(m_commandFromLabel, 0, wxALL | wxEXPAND, 5);
    rightSizer->Add(m_commandMessageLabel, 0, wxALL | wxEXPAND, 5);
    commandSizer->Add(rightSizer, 1, wxEXPAND);

    mainSizer->Add(commandSizer, 1, wxALL | wxEXPAND, 10);

    panel->SetSizer(mainSizer);

    // Setup update timer
    m_updateTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &ServerMonitorFrame::OnUpdateTimer, this, m_updateTimer->GetId());
    m_maxBlinkCount = 10;
    m_updateTimer->Start(5000 / m_maxBlinkCount); // Update every 5 seconds
    m_blinkCounter = 0;

    Center();
}

void ServerMonitorFrame::UpdateCommandInfo() {
    if (m_blinkCounter >= m_maxBlinkCount) {
        m_blinkCounter = 0;
        m_server.processCommands();
		m_accessRequesting = false;
    }

    if (!m_server.currentCommand.content.empty()) {
		if (m_server.currentCommand.content == "requestAccess") {
            // Trigger custom event for access request
            m_currentCommandLabel->SetLabel("Access Request");
            m_commandFromLabel->SetLabel(wxString::Format("From: %s", m_server.currentCommand.from));
            string fromEmail = m_server.currentCommand.from;

			// Find if fromEmail already in approvedAccess.email
            m_server.loadAccessList();
			auto it = find_if(m_server.approvedAccess.begin(), m_server.approvedAccess.end(),
				[&fromEmail](const AccessInfo& access) { return access.email == fromEmail; });

            // If access exists and is still valid
			if (it != m_server.approvedAccess.end() && m_server.isAccessValid(*it)) {
                // Calculate remaining time
                time_t now = time(nullptr);
                time_t expiryTime = it->grantedTime + (AccessInfo::VALIDITY_HOURS * 3600);
                double hoursLeft = difftime(expiryTime, now) / 3600.0;

                // Format expiry time
                struct tm timeinfo;
                localtime_s(&timeinfo, &expiryTime);
                char timeStr[80];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

                // Send email and update UI
                m_server.gmail.sendSimpleEmail(fromEmail, "Access Info",
                    "You already have access.\nExpires at: " + string(timeStr) +
                    "\nHours remaining: " + to_string(static_cast<int>(hoursLeft)));

                // Update UI labels
				m_server.currentCommand.content = "Access Request (Already granted)";
				m_server.currentCommand.message = "Access already granted until: " + string(timeStr);
				m_commandMessageLabel->SetLabel(wxString::Format("Message: %s", m_server.currentCommand.message));

                // Clear the current command
                m_server.currentCommand.content.clear();
				m_blinkCounter = 0;
				m_accessRequesting = false;

                // Return to prevent further processing
                return;
            }

            // If no existing valid access, proceed with access request
            if (m_accessRequesting) return;
            wxCommandEvent accessRequestEvent(CUSTOM_ACCESS_REQUEST_EVENT);
            QueueEvent(accessRequestEvent.Clone());
        }
        else {
            m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
            m_commandFromLabel->SetLabel(wxString::Format("From: %s", m_server.currentCommand.from));
            m_commandMessageLabel->SetLabel(wxString::Format("Message: %s", m_server.currentCommand.message));
        }
    }

    // Rest of the existing blinking logic remains the same
    if (m_blinkCounter < m_maxBlinkCount) {
        if (m_server.currentCommand.content.empty()) {
            wxString waitingText = "Waiting for command";
            for (int i = 0; i <= m_blinkCounter % 4; i++) {
                waitingText += ".";
            }
            if (waitingText.length() > 24) {
                waitingText = "Waiting for command";
            }
            m_currentCommandLabel->SetLabel(waitingText);
            m_commandFromLabel->SetLabel("");
            m_commandMessageLabel->SetLabel("");
        }
        m_blinkCounter++;
    }
}

void ServerMonitorFrame::OnUpdateTimer(wxTimerEvent& event) {
	if (m_accessRequesting) return;
    UpdateCommandInfo();
}

void ServerMonitorFrame::OnAccessRequest(wxCommandEvent& event) {
    AccessRequestDialog dialog(this, m_server.currentCommand.from, m_accessRequesting);
    int result = dialog.ShowModal();

    if (result == wxID_YES) {
        AccessInfo access;
        access.email = m_server.currentCommand.from;
        access.grantedTime = time(nullptr);
        m_server.approvedAccess.push_back(access);
        m_server.saveAccessList();

        time_t expiryTime = access.grantedTime + (AccessInfo::VALIDITY_HOURS * 3600);
        struct tm timeinfo;
        localtime_s(&timeinfo, &expiryTime);
        char timeStr[80];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

        m_server.gmail.sendSimpleEmail(access.email, "Access Granted",
            "Access granted for 24 hours.\nExpires at: " + std::string(timeStr));

        // Explicitly update labels
		m_server.currentCommand.content = "Access request (Granted)";
		m_server.currentCommand.message = "Access granted until: " + std::string(timeStr);
		m_server.currentCommand.from = access.email;
		m_currentCommandLabel->SetLabel(m_server.currentCommand.content);    
		m_commandFromLabel->SetLabel(wxString::Format("From: %s", m_server.currentCommand.from));
		m_commandMessageLabel->SetLabel(wxString::Format("Message: %s", m_server.currentCommand.message));
    }
    else {
        m_server.gmail.sendSimpleEmail(m_server.currentCommand.from, "Access Denied",
            "Your access request was denied.");

        // Explicitly update labels
		m_server.currentCommand.content = "Access request (Denied)";
		m_server.currentCommand.message = "Access request was denied";
		m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
		m_commandFromLabel->SetLabel(wxString::Format("From: %s", m_server.currentCommand.from));
		m_commandMessageLabel->SetLabel(wxString::Format("Message: %s", m_server.currentCommand.message));
    }

    // Clear the current command after processing
    m_server.currentCommand.content.clear();
	m_blinkCounter = 0;
	m_accessRequesting = false;

	wxMilliSleep(5000);
}


ServerMonitorFrame::~ServerMonitorFrame() {
    if (m_updateTimer) {
        m_updateTimer->Stop();
        delete m_updateTimer;
    }
}

// App Initialization
bool RemoteControlApp::OnInit() {
    // Read client secrets
    auto secrets = GmailAPI::ReadClientSecrets("\\Resources\\ClientSecrets.json");
    m_api = new GmailAPI(
        secrets["installed"]["client_id"].asString(),
        secrets["installed"]["client_secret"].asString(),
        secrets["installed"]["redirect_uris"][0].asString()
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

// Implement the wxWidgets application
wxIMPLEMENT_APP(RemoteControlApp);