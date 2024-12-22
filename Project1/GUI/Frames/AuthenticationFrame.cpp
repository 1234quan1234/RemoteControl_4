#include "AuthenticationFrame.h"

//Authentication Frame Implementation
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
    m_autoAuthBtn = UIStyles::styledButton(mainPanel, wxID_ANY, "Automatic Authentication");
    m_manualAuthBtn = UIStyles::styledButton(mainPanel, wxID_ANY, "Manual Authentication");

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
    wxButton* copyUrlBtn = UIStyles::styledButton(m_manualAuthPanel, wxID_ANY, "Copy URL");
    manualSizer->Add(copyUrlBtn, 0, wxALL | wxCENTER, 10);

    // Auth code input
    wxBoxSizer* authSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* authLabel = new wxStaticText(m_manualAuthPanel, wxID_ANY, "Enter Authorization Code:");
    m_authCodeCtrl = new wxTextCtrl(m_manualAuthPanel, wxID_ANY, "",
        wxDefaultPosition, wxSize(200, -1));
    wxButton* authenticateBtn = UIStyles::styledButton(m_manualAuthPanel, wxID_ANY, "Authenticate");

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