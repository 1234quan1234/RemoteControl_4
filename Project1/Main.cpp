#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/hyperlink.h>
#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"

class GmailRemoteControlApp : public wxApp {
public:
    bool OnInit() override;
};

class MainFrame : public wxFrame {
private:
    GmailAPI* gmailApi;
    ServerManager* server;
    SystemInfo* sysInfo;

    // Authentication Section
    wxStaticText* authInstructionText;
    wxHyperlinkCtrl* authUrlLink;
    wxTextCtrl* authCodeInput;
    wxButton* authenticateButton;

    // System Info Section
    wxStaticText* hostnameLabel;
    wxStaticText* localIPLabel;
    wxStaticText* gmailNameLabel;

    // Command Section
    wxStaticText* currentCommandLabel;
    wxTimer* commandUpdateTimer;

    void OnAuthenticate(wxCommandEvent& event);
    void UpdateCommandDisplay(wxTimerEvent& event);
    void OnClose(wxCloseEvent& event);

public:
    MainFrame();
    ~MainFrame();

    void InitializeGmailAPI();
    void SetupAuthenticationUI();
    void SetupSystemInfoUI();
    void SetupCommandUI();
};

wxIMPLEMENT_APP(GmailRemoteControlApp);

bool GmailRemoteControlApp::OnInit() {
    MainFrame* frame = new MainFrame();
    frame->Show(true);
    return true;
}

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control System",
        wxDefaultPosition, wxSize(500, 400)) {

    // Create vertical box sizer for entire frame
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Setup sections
    SetupAuthenticationUI();
    SetupSystemInfoUI();
    SetupCommandUI();

    // Add timers and event handlers
    commandUpdateTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &MainFrame::UpdateCommandDisplay, this, commandUpdateTimer->GetId());
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

    SetSizer(mainSizer);
    Layout();
}

void MainFrame::SetupAuthenticationUI() {
    wxBoxSizer* authSizer = new wxBoxSizer(wxVERTICAL);

    // Authentication Section Header
    wxStaticBox* authBox = new wxStaticBox(this, wxID_ANY, "Gmail Authentication");
    wxStaticBoxSizer* authBoxSizer = new wxStaticBoxSizer(authBox, wxVERTICAL);

    // Authentication Instructions
    authInstructionText = new wxStaticText(this, wxID_ANY,
        "1. Click the link to open the authorization URL\n"
        "2. Copy the authorization code\n"
        "3. Paste the code in the text box and click Authenticate");
    authBoxSizer->Add(authInstructionText, 0, wxALL | wxEXPAND, 10);

    // Authorization URL Hyperlink
    authUrlLink = new wxHyperlinkCtrl(this, wxID_ANY,
        "Click to Open Authorization URL", "");
    authUrlLink->Bind(wxEVT_HYPERLINK, [this](wxHyperlinkEvent& event) {
        wxLaunchDefaultBrowser(this->gmailApi->getAuthorizationUrl());
        });
    authBoxSizer->Add(authUrlLink, 0, wxALL | wxCENTER, 10);

    // Authorization Code Input
    wxBoxSizer* codeInputSizer = new wxBoxSizer(wxHORIZONTAL);
    authCodeInput = new wxTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    authenticateButton = new wxButton(this, wxID_ANY, "Authenticate");

    codeInputSizer->Add(new wxStaticText(this, wxID_ANY, "Authorization Code:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    codeInputSizer->Add(authCodeInput, 1, wxALIGN_CENTER_VERTICAL);
    codeInputSizer->Add(authenticateButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

    authBoxSizer->Add(codeInputSizer, 0, wxALL | wxEXPAND, 10);

    // Bind authentication button
    authenticateButton->Bind(wxEVT_BUTTON, &MainFrame::OnAuthenticate, this);

    // Add to main sizer
    GetSizer()->Add(authBoxSizer, 0, wxALL | wxEXPAND, 10);
}

void MainFrame::SetupSystemInfoUI() {
    wxStaticBox* infoBox = new wxStaticBox(this, wxID_ANY, "System Information");
    wxStaticBoxSizer* infoBoxSizer = new wxStaticBoxSizer(infoBox, wxVERTICAL);

    hostnameLabel = new wxStaticText(this, wxID_ANY, "Hostname: ");
    localIPLabel = new wxStaticText(this, wxID_ANY, "Local IP: ");
    gmailNameLabel = new wxStaticText(this, wxID_ANY, "Server's Gmail Name: ");

    infoBoxSizer->Add(hostnameLabel, 0, wxALL, 5);
    infoBoxSizer->Add(localIPLabel, 0, wxALL, 5);
    infoBoxSizer->Add(gmailNameLabel, 0, wxALL, 5);

    GetSizer()->Add(infoBoxSizer, 0, wxALL | wxEXPAND, 10);
}

void MainFrame::SetupCommandUI() {
    wxStaticBox* commandBox = new wxStaticBox(this, wxID_ANY, "Current Command");
    wxStaticBoxSizer* commandBoxSizer = new wxStaticBoxSizer(commandBox, wxVERTICAL);

    currentCommandLabel = new wxStaticText(this, wxID_ANY, "Waiting for command...");
    commandBoxSizer->Add(currentCommandLabel, 0, wxALL | wxCENTER, 10);

    GetSizer()->Add(commandBoxSizer, 0, wxALL | wxEXPAND, 10);
}

void MainFrame::InitializeGmailAPI() {
    try {
        // Load client secrets (same as in original main.cpp)
        auto secrets = GmailAPI::ReadClientSecrets("\\Resources\\ClientSecrets.json");

        gmailApi = new GmailAPI(
            secrets["installed"]["client_id"].asString(),
            secrets["installed"]["client_secret"].asString(),
            secrets["installed"]["redirect_uris"][0].asString()
        );

        // Update authorization URL link
        authUrlLink->SetURL(gmailApi->getAuthorizationUrl());

        // Try to load saved tokens
        try {
            gmailApi->loadSavedTokens();
            if (gmailApi->hasValidToken()) {
                // Authentication successful, update UI
                authenticateButton->Enable(false);
                authCodeInput->Enable(false);
                authUrlLink->Enable(false);
            }
        }
        catch (const exception& e) {
            // No valid tokens, prompt for authentication
            wxMessageBox("Authentication required", "Login", wxOK | wxICON_INFORMATION);
        }

        // Initialize system info
        sysInfo = new SystemInfo();

        // Update system info labels
        hostnameLabel->SetLabel(wxString::Format("Hostname: %s", sysInfo->hostname));
        localIPLabel->SetLabel(wxString::Format("Local IP: %s", sysInfo->localIP));
        gmailNameLabel->SetLabel(wxString::Format("Server's Gmail Name: %s", gmailApi->getServerName()));

        // Initialize server
        server = new ServerManager(*gmailApi);

        // Start command update timer
        commandUpdateTimer->Start(5000);  // 5-second interval
    }
    catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Initialization Error: %s", e.what()),
            "Error", wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnAuthenticate(wxCommandEvent& event) {
    try {
        string authCode = authCodeInput->GetValue().ToStdString();
        gmailApi->authenticate(authCode);

        // Authentication successful
        wxMessageBox("Authentication Successful!", "Success", wxOK | wxICON_INFORMATION);

        // Disable authentication controls
        authenticateButton->Enable(false);
        authCodeInput->Enable(false);
        authUrlLink->Enable(false);

        // Update system info
        gmailNameLabel->SetLabel(wxString::Format("Server's Gmail Name: %s", gmailApi->getServerName()));
    }
    catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Authentication Failed: %s", e.what()),
            "Error", wxOK | wxICON_ERROR);
    }
}

void MainFrame::UpdateCommandDisplay(wxTimerEvent& event) {
    if (server) {
        server->processCommands();
        if (!server->currentCommand.empty()) {
            currentCommandLabel->SetLabel(wxString::Format("Current Command: %s", server->currentCommand));
        }
        else {
            currentCommandLabel->SetLabel("Waiting for command...");
        }
    }
}

void MainFrame::OnClose(wxCloseEvent& event) {
    // Cleanup resources
    if (commandUpdateTimer) {
        commandUpdateTimer->Stop();
        delete commandUpdateTimer;
    }

    if (server) delete server;
    if (gmailApi) delete gmailApi;
    if (sysInfo) delete sysInfo;

    event.Skip();  // Allow the window to close
}

MainFrame::~MainFrame() {
    // Additional cleanup if needed
}