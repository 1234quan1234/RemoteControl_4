#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/listbox.h>
#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"

class ServerWindow : public wxFrame {
private:
    // Main components
    wxTextCtrl* m_authCodeCtrl;
    wxButton* m_authenticateButton;
    wxStaticText* m_statusText;
    wxListBox* m_logListBox;

    // Backend components
    GmailAPI* m_api;
    ServerManager* m_server;
    SystemInfo* m_sysInfo;

    // Authentication URL
    wxString m_authUrl;

public:
    ServerWindow() : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control System", wxDefaultPosition, wxSize(1800, 900)) {
        // Create main panel
        wxPanel* panel = new wxPanel(this, wxID_ANY);

        // Create sizers
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* authSizer = new wxBoxSizer(wxHORIZONTAL);

        // Authentication URL display
        wxStaticText* urlLabel = new wxStaticText(panel, wxID_ANY, "Authentication URL:");
        mainSizer->Add(urlLabel, 0, wxALL, 10);

        // Status text for displaying messages
        m_statusText = new wxStaticText(panel, wxID_ANY, "Initializing...");
        mainSizer->Add(m_statusText, 0, wxALL | wxEXPAND, 10);

        // Authentication process
        try {
            // Read client secrets
            auto secrets = GmailAPI::ReadClientSecrets("/Resources/ClientSecrets.json");

            // Initialize API
            m_api = new GmailAPI(
                secrets["installed"]["client_id"].asString(),
                secrets["installed"]["client_secret"].asString(),
                secrets["installed"]["redirect_uris"][0].asString()
            );

            // Get authentication URL
            m_authUrl = m_api->getAuthorizationUrl();

            // Create URL display text control (read-only)
            wxTextCtrl* urlDisplay = new wxTextCtrl(panel, wxID_ANY, m_authUrl,
                wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
            mainSizer->Add(urlDisplay, 0, wxALL | wxEXPAND, 10);

            // Authentication code input
            wxStaticText* codeLabel = new wxStaticText(panel, wxID_ANY, "Enter Authorization Code:");
            mainSizer->Add(codeLabel, 0, wxALL, 10);

            m_authCodeCtrl = new wxTextCtrl(panel, wxID_ANY);
            authSizer->Add(m_authCodeCtrl, 1, wxALL | wxEXPAND, 10);

            // Authenticate button
            m_authenticateButton = new wxButton(panel, wxID_ANY, "Authenticate");
            m_authenticateButton->Bind(wxEVT_BUTTON, &ServerWindow::OnAuthenticate, this);
            authSizer->Add(m_authenticateButton, 0, wxALL, 10);

            mainSizer->Add(authSizer, 0, wxEXPAND);

            // Log ListBox
            wxStaticText* logLabel = new wxStaticText(panel, wxID_ANY, "Server Logs:");
            mainSizer->Add(logLabel, 0, wxALL, 10);

            m_logListBox = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 200));
            mainSizer->Add(m_logListBox, 1, wxALL | wxEXPAND, 10);

            // Server control buttons
            wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

            wxButton* startServerBtn = new wxButton(panel, wxID_ANY, "Start Server");
            startServerBtn->Bind(wxEVT_BUTTON, &ServerWindow::OnStartServer, this);
            buttonSizer->Add(startServerBtn, 0, wxALL, 10);

            wxButton* stopServerBtn = new wxButton(panel, wxID_ANY, "Stop Server");
            stopServerBtn->Bind(wxEVT_BUTTON, &ServerWindow::OnStopServer, this);
            buttonSizer->Add(stopServerBtn, 0, wxALL, 10);

            mainSizer->Add(buttonSizer, 0, wxCENTER);

            // Set panel sizer
            panel->SetSizer(mainSizer);
            mainSizer->Fit(this);

            // Update status
            m_statusText->SetLabel("Ready to authenticate");
        }
        catch (const std::exception& e) {
            wxMessageBox(wxString::Format("Initialization Error: %s", e.what()),
                "Error", wxOK | wxICON_ERROR);
        }

        // Center the window
        Centre();
    }

    void OnAuthenticate(wxCommandEvent& event) {
        try {
            wxString authCode = m_authCodeCtrl->GetValue();
            if (authCode.IsEmpty()) {
                wxMessageBox("Please enter the authorization code", "Error", wxOK | wxICON_WARNING);
                return;
            }

            // Authenticate
            m_api->authenticate(std::string(authCode.mb_str()));

            // Create Client_Gmail after successful authentication
            auto secrets = GmailAPI::ReadClientSecrets("/Resources/ClientSecrets.json");

            // Log the authentication
            m_logListBox->Append("Authentication Successful!");
            m_statusText->SetLabel("Authentication Complete");
            m_authenticateButton->Enable(false);
        }
        catch (const std::exception& e) {
            wxMessageBox(wxString::Format("Authentication Error: %s", e.what()),
                "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnStartServer(wxCommandEvent& event) {
        try {
            // Ensure authentication is complete
            if (!m_api) {
                wxMessageBox("Please complete authentication first",
                    "Server Start Error", wxOK | wxICON_WARNING);
                return;
            }

            // Initialize system info
            m_sysInfo = new SystemInfo();

            // Start server
            m_server = new ServerManager(*m_api);

            // Log server start
            m_logListBox->Append("Server Started Successfully");
            m_statusText->SetLabel("Server Running");

            // Start a timer to process commands periodically
            Connect(wxID_ANY, wxEVT_TIMER, wxTimerEventHandler(ServerWindow::OnProcessCommands));
            m_timer = new wxTimer(this);
            m_timer->Start(5000); // 5-second interval
        }
        catch (const std::exception& e) {
            wxMessageBox(wxString::Format("Server Start Error: %s", e.what()),
                "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnStopServer(wxCommandEvent& event) {
        try {
            // Stop the timer
            if (m_timer) {
                m_timer->Stop();
                delete m_timer;
                m_timer = nullptr;
            }

            // Clean up resources
            if (m_server) {
                delete m_server;
                m_server = nullptr;
            }

            if (m_sysInfo) {
                delete m_sysInfo;
                m_sysInfo = nullptr;
            }

            // Log server stop
            m_logListBox->Append("Server Stopped");
            m_statusText->SetLabel("Server Stopped");
        }
        catch (const std::exception& e) {
            wxMessageBox(wxString::Format("Server Stop Error: %s", e.what()),
                "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnProcessCommands(wxTimerEvent& event) {
        try {
            if (m_server) {
                m_server->processCommands();
                m_logListBox->Append("Processing Commands...");
            }
        }
        catch (const std::exception& e) {
            m_logListBox->Append(wxString::Format("Command Processing Error: %s", e.what()));
        }
    }

    ~ServerWindow() {
        // Clean up resources
        if (m_api) delete m_api;
        if (m_server) delete m_server;
        if (m_sysInfo) delete m_sysInfo;
        if (m_timer) delete m_timer;
    }

private:
    wxTimer* m_timer = nullptr;
};

class ServerApp : public wxApp {
public:
    bool OnInit() {
        ServerWindow* window = new ServerWindow();
        window->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ServerApp);