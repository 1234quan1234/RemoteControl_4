#include <wx/wx.h>
#include <wx/hyperlink.h>
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include "Server/ServerManager.h"
#include "RemoteControl/SystemInfo.h"


// Add these constants at the top of your file
namespace UIColors {
    const wxColour PRIMARY(0, 87, 183);           // HCMUS Blue
    const wxColour SECONDARY(255, 255, 255);      // White
    const wxColour ACCENT(241, 196, 15);          // Gold
    const wxColour BACKGROUND(245, 245, 245);     // Light Gray
    const wxColour NORMALTEXT(51, 51, 51);          // Dark Gray
    const wxColour STATUS_GREEN(39, 174, 96);    // Success green
    const wxColour STATUS_YELLOW(241, 196, 15);  // Warning yellow
    const wxColour STATUS_RED(231, 76, 60);      // Error red
    const wxColour PANEL_BG(250, 250, 250);      // Light panel background
    const wxColour BORDER(229, 229, 229);        // Border color
    const wxColour EMAIL_COLOR(41, 128, 185);    // Blue for email addresses
    const wxColour MESSAGE_COLOR(52, 73, 94);    // Dark gray for message content
}

// Add this helper function for styled buttons
wxButton* styledButton(wxWindow* parent, wxWindowID id, const wxString& label) {
    wxButton* button = new wxButton(parent, id, label);
    button->SetBackgroundColour(UIColors::PRIMARY);
    button->SetForegroundColour(UIColors::SECONDARY);
    button->SetMinSize(wxSize(120, 35));  // Consistent button size

    // Bind hover events for interactive effect
    button->Bind(wxEVT_ENTER_WINDOW, [](wxMouseEvent& evt) {
        wxButton* btn = (wxButton*)evt.GetEventObject();
        btn->SetBackgroundColour(UIColors::ACCENT);
        btn->Refresh();
        evt.Skip();
        });

    button->Bind(wxEVT_LEAVE_WINDOW, [](wxMouseEvent& evt) {
        wxButton* btn = (wxButton*)evt.GetEventObject();
        btn->SetBackgroundColour(UIColors::PRIMARY);
        btn->Refresh();
        evt.Skip();
        });

    return button;
}

// Helper function for creating styled panels
wxPanel* createStyledPanel(wxWindow* parent) {
    wxPanel* panel = new wxPanel(parent, wxID_ANY);
    panel->SetBackgroundColour(UIColors::PANEL_BG);
    return panel;
}

// Helper function for creating styled text
wxStaticText* styledText(wxWindow* parent, const wxString& label, bool isBold = false) {
    wxStaticText* text = new wxStaticText(parent, wxID_ANY, label);
    wxFont font = text->GetFont();
    if (isBold) {
        font.SetWeight(wxFONTWEIGHT_BOLD);
    }
    text->SetFont(font);
    return text;
}

// Add new custom event for access request
wxDECLARE_EVENT(CUSTOM_ACCESS_REQUEST_EVENT, wxCommandEvent);
wxDEFINE_EVENT(CUSTOM_ACCESS_REQUEST_EVENT, wxCommandEvent);

class AuthenticationFrame : public wxFrame {
private:
    wxTextCtrl* m_authCodeCtrl;
    GmailAPI& m_api;
    wxHyperlinkCtrl* m_authUrlLink;
    wxButton* m_manualAuthBtn;
    wxButton* m_autoAuthBtn;
    wxPanel* m_manualAuthPanel;
    wxStaticBitmap* m_logoImage;
    wxStaticText* m_titleText;

    void OnAuthenticate(wxCommandEvent& event);        // Event handler for manual authentication
    void OnCopyURL(wxHyperlinkEvent& event);          // Event handler for URL copying
    void OnAutoAuthenticate(wxCommandEvent& event);    // Event handler for automatic authentication
    void OnManualAuthenticate(wxCommandEvent& event);  // Event handler for showing manual controls
    void ShowManualAuthControls(bool show);           // Helper method to show/hide manual controls

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
    //wxStaticText* m_commandFromLabel;
    //wxStaticText* m_commandMessageLabel;

    wxStaticText* m_fromLabelText;
    wxStaticText* m_fromContentText;
    wxStaticText* m_messageLabelText;
    wxStaticText* m_messageContentText;

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
    m_yesButton = styledButton(panel, wxID_YES, "Yes");
    m_noButton = styledButton(panel, wxID_NO, "No");

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

/*AuthenticationFrame::AuthenticationFrame(GmailAPI& api)
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control - Authentication",
        wxDefaultPosition, wxSize(500, 300)),
    m_api(api) {

    wxImage::AddHandler(new wxPNGHandler());
    wxIcon appIcon;
    if (appIcon.LoadFile("imgs/hcmus-logo.png", wxBITMAP_TYPE_PNG))
        AuthenticationFrame::SetIcon(appIcon);


    SetBackgroundColour(UIColors::BACKGROUND);

    // Main panel and sizer
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Add logo
    wxImage hcmusLogo(L"imgs/hcmus-logo.png", wxBITMAP_TYPE_PNG);
    if (hcmusLogo.IsOk()) {
        int targetSize = 150;
        double scale = std::min((double)targetSize / hcmusLogo.GetWidth(),
            (double)targetSize / hcmusLogo.GetHeight());
        wxImage scaledLogo = hcmusLogo.Scale(
            hcmusLogo.GetWidth() * scale,
            hcmusLogo.GetHeight() * scale,
            wxIMAGE_QUALITY_HIGH);

        m_logoImage = new wxStaticBitmap(mainPanel, wxID_ANY,
            wxBitmap(scaledLogo));
        mainSizer->AddSpacer(20);
        mainSizer->Add(m_logoImage, 0, wxALIGN_CENTER_HORIZONTAL);
    }

    // Title with larger, bold font
    m_titleText = new wxStaticText(mainPanel, wxID_ANY, "Gmail Remote Control");
    wxFont titleFont = m_titleText->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() * 1.5);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_titleText->SetFont(titleFont);
    m_titleText->SetForegroundColour(UIColors::PRIMARY);
    mainSizer->AddSpacer(15);
    mainSizer->Add(m_titleText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

    // Subtitle
    wxStaticText* subtitle = new wxStaticText(mainPanel, wxID_ANY,
        "Please choose your authentication method:");
    subtitle->SetForegroundColour(UIColors::NORMALTEXT);
    mainSizer->Add(subtitle, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

    // Authentication choice card
    wxPanel* choicePanel = new wxPanel(mainPanel);
    choicePanel->SetBackgroundColour(UIColors::SECONDARY);
    wxBoxSizer* choiceSizer = new wxBoxSizer(wxVERTICAL);

    // Auto auth button with icon
    m_autoAuthBtn = styledButton(choicePanel, wxID_ANY, "Automatic Authentication");
    m_autoAuthBtn->SetMinSize(wxSize(250, 40));
    choiceSizer->Add(m_autoAuthBtn, 0, wxALIGN_CENTER | wxALL, 10);

    // Separator line
    wxStaticLine* line = new wxStaticLine(choicePanel);
    choiceSizer->Add(line, 0, wxEXPAND | wxALL, 10);

    // Manual auth button
    m_manualAuthBtn = styledButton(choicePanel, wxID_ANY, "Manual Authentication");
    m_manualAuthBtn->SetMinSize(wxSize(250, 40));
    choiceSizer->Add(m_manualAuthBtn, 0, wxALIGN_CENTER | wxALL, 10);

    choicePanel->SetSizer(choiceSizer);
    mainSizer->Add(choicePanel, 0, wxEXPAND | wxALL, 20);

    // Manual authentication panel
    m_manualAuthPanel = new wxPanel(mainPanel);
    m_manualAuthPanel->SetBackgroundColour(UIColors::SECONDARY);
    wxStaticBoxSizer* manualSizer = new wxStaticBoxSizer(wxVERTICAL, m_manualAuthPanel, "");

    // Instructions with improved formatting
    wxArrayString instructions;
    instructions.Add("To authenticate manually, please follow these steps:");
    instructions.Add("1. Click the authorization URL below or copy it to your browser");
    instructions.Add("2. Sign in with your Google account and grant permissions");
    instructions.Add("3. Copy the authorization code and paste it below");

    for (const auto& instruction : instructions) {
        wxStaticText* step = new wxStaticText(m_manualAuthPanel, wxID_ANY, instruction);
        step->SetForegroundColour(UIColors::NORMALTEXT);
        manualSizer->Add(step, 0, wxALL, 5);
    }

    // URL display and copy section
    wxPanel* urlPanel = new wxPanel(m_manualAuthPanel);
    urlPanel->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* urlSizer = new wxBoxSizer(wxHORIZONTAL);

    m_authUrlLink = new wxHyperlinkCtrl(urlPanel, wxID_ANY,
        m_api.getAuthorizationUrl(), m_api.getAuthorizationUrl());
    wxButton* copyUrlBtn = styledButton(urlPanel, wxID_ANY, "Copy URL");

    urlSizer->Add(m_authUrlLink, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    urlSizer->Add(copyUrlBtn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    urlPanel->SetSizer(urlSizer);
    manualSizer->Add(urlPanel, 0, wxEXPAND | wxALL, 10);

    // Auth code input section
    wxPanel* authPanel = new wxPanel(m_manualAuthPanel);
    authPanel->SetBackgroundColour(*wxWHITE);
    wxBoxSizer* authSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* authLabel = new wxStaticText(authPanel, wxID_ANY,
        "Authorization Code:");
    m_authCodeCtrl = new wxTextCtrl(authPanel, wxID_ANY, "",
        wxDefaultPosition, wxSize(-1, 35));
    wxButton* authenticateBtn = styledButton(authPanel, wxID_ANY, "Authenticate");
    authenticateBtn->SetMinSize(wxSize(120, 35));

    authSizer->Add(authLabel, 0, wxALL, 5);
    authSizer->Add(m_authCodeCtrl, 0, wxEXPAND | wxALL, 5);
    authSizer->Add(authenticateBtn, 0, wxALIGN_RIGHT | wxALL, 5);

    authPanel->SetSizer(authSizer);
    manualSizer->Add(authPanel, 0, wxEXPAND | wxALL, 10);

    m_manualAuthPanel->SetSizer(manualSizer);
    mainSizer->Add(m_manualAuthPanel, 0, wxEXPAND | wxALL, 20);

    // Bind events
    copyUrlBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(m_api.getAuthorizationUrl()));
            wxTheClipboard->Close();
        }
        });

    authenticateBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnAuthenticate, this);
    m_autoAuthBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnAutoAuthenticate, this);
    m_manualAuthBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnManualAuthenticate, this);

    mainPanel->SetSizer(mainSizer);

    // Initially hide manual authentication controls
    ShowManualAuthControls(false);

    // Set minimum size and center
    SetMinSize(wxSize(500, 600));
    Center();
}*/

AuthenticationFrame::AuthenticationFrame(GmailAPI& api)
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control - Authentication",
        wxDefaultPosition, wxSize(500, 600)),
    m_api(api) {

    wxImage::AddHandler(new wxPNGHandler());
    wxIcon appIcon;
    if (appIcon.LoadFile("imgs/hcmus-logo.png", wxBITMAP_TYPE_PNG))
        AuthenticationFrame::SetIcon(appIcon);

    // Basic debug output
    wxLogDebug("Starting AuthenticationFrame initialization");

    SetBackgroundColour(UIColors::BACKGROUND);

    // Initialize image handlers only once
    static bool handlersInitialized = false;
    if (!handlersInitialized) {
        wxImage::AddHandler(new wxPNGHandler());
        handlersInitialized = true;
    }

    // Create base panel and sizer
    wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND);

    // Panel sizer
    wxBoxSizer* panelSizer = new wxBoxSizer(wxVERTICAL);
    mainPanel->SetSizer(panelSizer);

    // Try to load logo
    wxImage hcmusLogo;
    if (hcmusLogo.LoadFile("imgs/hcmus-logo.png", wxBITMAP_TYPE_PNG)) {
        int targetSize = 150;
        double scale = std::min((double)targetSize / hcmusLogo.GetWidth(),
            (double)targetSize / hcmusLogo.GetHeight());
        wxImage scaledLogo = hcmusLogo.Scale(
            hcmusLogo.GetWidth() * scale,
            hcmusLogo.GetHeight() * scale,
            wxIMAGE_QUALITY_HIGH);

        m_logoImage = new wxStaticBitmap(mainPanel, wxID_ANY,
            wxBitmap(scaledLogo));
        panelSizer->AddSpacer(20);
        panelSizer->Add(m_logoImage, 0, wxALIGN_CENTER_HORIZONTAL);
    }
    else {
        wxLogDebug("Failed to load logo image");
    }

    // Title
    wxStaticText* titleText = new wxStaticText(mainPanel, wxID_ANY, "Gmail Remote Control",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont titleFont = titleText->GetFont();
    titleFont.SetPointSize(14);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    titleText->SetFont(titleFont);
    titleText->SetForegroundColour(UIColors::PRIMARY);
    panelSizer->AddSpacer(15);
    panelSizer->Add(titleText, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

    // Subtitle
    wxStaticText* subtitle = new wxStaticText(mainPanel, wxID_ANY,
        "Please choose your authentication method:",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    subtitle->SetForegroundColour(UIColors::NORMALTEXT);
    panelSizer->Add(subtitle, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 10);

    // Authentication buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_autoAuthBtn = styledButton(mainPanel, wxID_ANY, "Automatic Authentication");
    m_manualAuthBtn = styledButton(mainPanel, wxID_ANY, "Manual Authentication");

    buttonSizer->Add(m_autoAuthBtn, 1, wxALL | wxEXPAND, 5);
    buttonSizer->AddSpacer(10);
    buttonSizer->Add(m_manualAuthBtn, 1, wxALL | wxEXPAND, 5);
    panelSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);

    // Manual authentication panel
    m_manualAuthPanel = new wxPanel(mainPanel, wxID_ANY);
    wxBoxSizer* manualSizer = new wxBoxSizer(wxVERTICAL);
    m_manualAuthPanel->SetSizer(manualSizer);

    // Instructions
    wxStaticText* instructLabel = new wxStaticText(m_manualAuthPanel, wxID_ANY,
        "To authenticate, please follow these steps:");
    wxStaticText* step1Label = new wxStaticText(m_manualAuthPanel, wxID_ANY,
        "1. Click the link below to open the authorization page in your browser.");
    wxStaticText* step2Label = new wxStaticText(m_manualAuthPanel, wxID_ANY,
        "2. Copy the authorization code from the browser and paste it in the box below.");
    wxStaticText* step3Label = new wxStaticText(m_manualAuthPanel, wxID_ANY,
        "3. Click Authenticate to complete the process.");

    manualSizer->Add(instructLabel, 0, wxALL | wxCENTER, 10);
    manualSizer->Add(step1Label, 0, wxALL | wxCENTER, 10);
    manualSizer->Add(step2Label, 0, wxALL | wxCENTER, 10);
    manualSizer->Add(step3Label, 0, wxALL | wxCENTER, 10);

    // URL Controls
    wxBoxSizer* urlSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* urlLabel = new wxStaticText(m_manualAuthPanel, wxID_ANY, "Authorization URL: ");
    m_authUrlLink = new wxHyperlinkCtrl(m_manualAuthPanel, wxID_ANY,
        m_api.getAuthorizationUrl(), m_api.getAuthorizationUrl());

    urlSizer->Add(urlLabel, 0, wxALIGN_CENTER_VERTICAL);
    urlSizer->Add(m_authUrlLink, 0, wxALIGN_CENTER_VERTICAL);
    manualSizer->Add(urlSizer, 0, wxALL | wxCENTER, 10);

    // Copy URL Button
    wxButton* copyUrlBtn = styledButton(m_manualAuthPanel, wxID_ANY, "Copy URL");
    manualSizer->Add(copyUrlBtn, 0, wxALL | wxCENTER, 10);

    // Auth code input
    wxBoxSizer* authSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* authLabel = new wxStaticText(m_manualAuthPanel, wxID_ANY, "Enter Authorization Code:");
    m_authCodeCtrl = new wxTextCtrl(m_manualAuthPanel, wxID_ANY, "",
        wxDefaultPosition, wxSize(200, -1));
    wxButton* authenticateBtn = styledButton(m_manualAuthPanel, wxID_ANY, "Authenticate");

    authSizer->Add(authLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    authSizer->Add(m_authCodeCtrl, 1, wxALIGN_CENTER_VERTICAL);
    authSizer->Add(authenticateBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    manualSizer->Add(authSizer, 0, wxALL | wxEXPAND, 10);

    panelSizer->Add(m_manualAuthPanel, 1, wxEXPAND | wxALL, 10);

    // Bind events
    copyUrlBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(m_api.getAuthorizationUrl()));
            wxTheClipboard->Close();
            wxMessageBox("URL copied to clipboard!", "Success", wxOK | wxICON_INFORMATION);
        }
        });

    authenticateBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnAuthenticate, this);
    m_autoAuthBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnAutoAuthenticate, this);
    m_manualAuthBtn->Bind(wxEVT_BUTTON, &AuthenticationFrame::OnManualAuthenticate, this);

    // Initially hide manual authentication controls
    ShowManualAuthControls(false);

    // Ensure layout is correct
    panelSizer->Layout();
    mainSizer->Layout();

    // Set minimum size and center
    SetMinSize(wxSize(500, 400));
    Center();

    wxLogDebug("AuthenticationFrame initialization complete");
}

void AuthenticationFrame::ShowManualAuthControls(bool show) {
    m_manualAuthPanel->Show(show);
    GetSizer()->Layout();
    Refresh();
    Update();
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
        ShowManualAuthControls(true); // Show manual controls in case of error
    }
}

void AuthenticationFrame::OnAutoAuthenticate(wxCommandEvent& event) {
    wxBusyCursor wait;
    m_autoAuthBtn->Disable();
    m_manualAuthBtn->Disable();

    try {
        if (m_api.authenticateAutomatically()) {
            // Create and show ServerMonitorFrame
            SystemInfo* sysInfo = new SystemInfo();
            ServerManager* server = new ServerManager(m_api);

            ServerMonitorFrame* monitorFrame = new ServerMonitorFrame(m_api, *server, *sysInfo);
            monitorFrame->Show(true);

            // Close authentication frame
            Close();
        }
        else {
            wxMessageBox("Automatic authentication failed. Please try manual authentication.",
                "Authentication Failed", wxOK | wxICON_ERROR);
            ShowManualAuthControls(true);
        }
    }
    catch (const std::exception& e) {
        wxMessageBox(wxString::Format("Authentication Failed: %s\nPlease try manual authentication.", e.what()),
            "Error", wxOK | wxICON_ERROR);
        ShowManualAuthControls(true);
    }

    m_autoAuthBtn->Enable();
    m_manualAuthBtn->Enable();
}

void AuthenticationFrame::OnManualAuthenticate(wxCommandEvent& event) {
    ShowManualAuthControls(true);
}


// Event table for ServerMonitorFrame
wxBEGIN_EVENT_TABLE(ServerMonitorFrame, wxFrame)
EVT_TIMER(wxID_ANY, ServerMonitorFrame::OnUpdateTimer)
EVT_COMMAND(wxID_ANY, CUSTOM_ACCESS_REQUEST_EVENT, ServerMonitorFrame::OnAccessRequest)
wxEND_EVENT_TABLE()

// ServerMonitorFrame implementation
ServerMonitorFrame::ServerMonitorFrame(GmailAPI& api, ServerManager& server, SystemInfo& sysInfo)
    : wxFrame(nullptr, wxID_ANY, "Gmail Remote Control - Server Monitor",
        wxDefaultPosition, wxSize(800, 600)),
    m_api(api), m_server(server), m_sysInfo(sysInfo) {

    wxImage::AddHandler(new wxPNGHandler());
    wxIcon appIcon;
    if (appIcon.LoadFile("imgs/hcmus-logo.png", wxBITMAP_TYPE_PNG))
        ServerMonitorFrame::SetIcon(appIcon);


    // Set frame background and create main panel
    SetBackgroundColour(UIColors::BACKGROUND);
    wxPanel* mainPanel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Status Bar Panel
    wxPanel* statusPanel = createStyledPanel(mainPanel);
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);

    // Server status indicator (colored circle)
    wxPanel* statusIndicator = new wxPanel(statusPanel, wxID_ANY, wxDefaultPosition, wxSize(12, 12));
    statusIndicator->SetBackgroundColour(UIColors::STATUS_GREEN);
    statusSizer->Add(statusIndicator, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);

    // Status text
    wxStaticText* statusText = styledText(statusPanel, "Server's running", true);
    statusText->SetForegroundColour(UIColors::STATUS_GREEN);
    statusSizer->Add(statusText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    statusPanel->SetSizer(statusSizer);
    mainSizer->Add(statusPanel, 0, wxALL | wxEXPAND, 10);

    // Server Info Card
    wxPanel* infoCard = createStyledPanel(mainPanel);
    wxStaticBoxSizer* infoSizer = new wxStaticBoxSizer(wxVERTICAL, infoCard, "");
    infoCard->SetBackgroundColour(UIColors::SECONDARY);

    // Info grid
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 2, 15, 20);

    // Labels with icons (you can add actual icons using wxBitmap)
    m_hostnameLabel = styledText(infoCard, wxString::Format("%s", m_sysInfo.hostname));
    m_localIPLabel = styledText(infoCard, wxString::Format("%s", m_sysInfo.localIP));
    m_gmailNameLabel = styledText(infoCard, wxString::Format("%s", m_api.getServerName()));

    gridSizer->Add(styledText(infoCard, "Hostname:", true));
    gridSizer->Add(m_hostnameLabel);
    gridSizer->Add(styledText(infoCard, "Local IP:", true));
    gridSizer->Add(m_localIPLabel);
    gridSizer->Add(styledText(infoCard, "Server's Gmail:", true));
    gridSizer->Add(m_gmailNameLabel);

    infoSizer->Add(gridSizer, 1, wxALL | wxEXPAND, 15);
    infoCard->SetSizer(infoSizer);
    mainSizer->Add(infoCard, 0, wxALL | wxEXPAND, 10);

    // Command Monitor Card
    wxPanel* commandCard = createStyledPanel(mainPanel);
    wxStaticBoxSizer* commandSizer = new wxStaticBoxSizer(wxVERTICAL, commandCard, "");

    // Current command section
    wxPanel* commandStatusPanel = createStyledPanel(commandCard);
    wxBoxSizer* commandStatusSizer = new wxBoxSizer(wxVERTICAL);

    // Current command label
    m_currentCommandLabel = new wxStaticText(commandStatusPanel, wxID_ANY, "Waiting for command");
    wxFont boldFont = m_currentCommandLabel->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_currentCommandLabel->SetFont(boldFont.Scale(1.5));

    // From line with horizontal sizer
    wxBoxSizer* fromSizer = new wxBoxSizer(wxHORIZONTAL);
    m_fromLabelText = new wxStaticText(commandStatusPanel, wxID_ANY, "From:     ");
    wxFont boldFont1 = m_fromLabelText->GetFont();
    boldFont1.SetWeight(wxFONTWEIGHT_BOLD);
    m_fromLabelText->SetFont(boldFont1);
    m_fromContentText = new wxStaticText(commandStatusPanel, wxID_ANY, "");
    m_fromContentText->SetForegroundColour(UIColors::EMAIL_COLOR);
    fromSizer->Add(m_fromLabelText, 0, wxALIGN_CENTER_VERTICAL);
    fromSizer->Add(m_fromContentText, 0, wxALIGN_CENTER_VERTICAL);

    // Message line with horizontal sizer
    wxBoxSizer* messageSizer = new wxBoxSizer(wxHORIZONTAL);
    m_messageLabelText = new wxStaticText(commandStatusPanel, wxID_ANY, "Message:   ");
    m_messageLabelText->SetFont(boldFont1);
    m_messageContentText = new wxStaticText(commandStatusPanel, wxID_ANY, "");
    m_messageContentText->SetForegroundColour(UIColors::MESSAGE_COLOR);
    messageSizer->Add(m_messageLabelText, 0, wxALIGN_CENTER_VERTICAL);
    messageSizer->Add(m_messageContentText, 0, wxALIGN_CENTER_VERTICAL);

    commandStatusSizer->Add(m_currentCommandLabel, 0, wxALL, 5);
    commandStatusSizer->Add(fromSizer, 0, wxALL, 5);
    commandStatusSizer->Add(messageSizer, 0, wxALL, 5);

    commandStatusPanel->SetSizer(commandStatusSizer);
    commandSizer->Add(commandStatusPanel, 1, wxALL | wxEXPAND, 10);

    commandCard->SetSizer(commandSizer);
    mainSizer->Add(commandCard, 1, wxALL | wxEXPAND, 10);

    mainPanel->SetSizer(mainSizer);

    // Setup update timer
    m_updateTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &ServerMonitorFrame::OnUpdateTimer, this, m_updateTimer->GetId());
    m_maxBlinkCount = 10;
    m_updateTimer->Start(5000 / m_maxBlinkCount);
    m_blinkCounter = 0;

    // Set minimum size and center the frame
    SetMinSize(wxSize(600, 400));
    Center();
}

void ServerMonitorFrame::UpdateCommandInfo() {
    if (m_blinkCounter >= m_maxBlinkCount) {
        m_blinkCounter = 0;
        m_server.processCommands();
        m_accessRequesting = false;
    }

    if (!m_server.currentCommand.content.empty()) {
        // Update command display with animation
        m_currentCommandLabel->SetForegroundColour(UIColors::PRIMARY);

        if (m_server.currentCommand.content == "requestAccess") {
            // Handle access request UI updates
            m_currentCommandLabel->SetLabel("Access Request");
            m_currentCommandLabel->SetForegroundColour(UIColors::STATUS_YELLOW);
			m_fromLabelText->SetLabel("From: ");
            m_fromContentText->SetLabel(m_server.currentCommand.from);


			//check if the email is already approved
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
                m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
                m_currentCommandLabel->SetForegroundColour(UIColors::STATUS_YELLOW);
				m_messageLabelText->SetLabel("Message: ");
				m_messageContentText->SetLabel(m_server.currentCommand.message);



                // Clear the current command
                m_server.currentCommand.content.clear();
                m_blinkCounter = 0;
                m_accessRequesting = false;

                return;
            }

            if (!m_accessRequesting) {
                wxCommandEvent accessRequestEvent(CUSTOM_ACCESS_REQUEST_EVENT);
                QueueEvent(accessRequestEvent.Clone());
            }
        }
        else {
            // Regular command updates
            m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
			m_fromLabelText->SetLabel("From: ");
			m_fromContentText->SetLabel(m_server.currentCommand.from);
			m_messageLabelText->SetLabel("Message: ");
			m_messageContentText->SetLabel(m_server.currentCommand.message);
        }
    }
    
    // Waiting animation
    if (m_blinkCounter < m_maxBlinkCount) {
        if (m_server.currentCommand.content.empty()) {
            wxString waitingText = "Waiting for command";

            for (int i = 0; i <= m_blinkCounter % 3; i++) {
                waitingText += ".";
            }

            m_currentCommandLabel->SetLabel(waitingText);
            m_currentCommandLabel->SetForegroundColour(UIColors::NORMALTEXT);
			m_fromLabelText->SetLabel("");
            m_fromContentText->SetLabel("");
			m_messageLabelText->SetLabel("");
			m_messageContentText->SetLabel("");
            
        }
        m_blinkCounter++;
    }

    

    // Refresh the frame to ensure smooth updates
    Refresh();
    Update();
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
        m_currentCommandLabel->SetForegroundColour(UIColors::STATUS_GREEN);
		m_fromLabelText->SetLabel("From: ");
        m_fromContentText->SetLabel(m_server.currentCommand.from);
		m_messageLabelText->SetLabel("Message: ");
		m_messageContentText->SetLabel(m_server.currentCommand.message);
    }
    else {
        m_server.gmail.sendSimpleEmail(m_server.currentCommand.from, "Access Denied",
            "Your access request was denied.");

        // Explicitly update labels
		m_server.currentCommand.content = "Access request (Denied)";
		m_server.currentCommand.message = "Access request was denied";
        m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
        m_currentCommandLabel->SetForegroundColour(UIColors::STATUS_RED);
        m_fromLabelText->SetLabel("From: ");
        m_fromContentText->SetLabel(m_server.currentCommand.from);
        m_messageLabelText->SetLabel("Message: ");
        m_messageContentText->SetLabel(m_server.currentCommand.message);
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

// Keep the wxWidgets application implementation
wxIMPLEMENT_APP(RemoteControlApp);