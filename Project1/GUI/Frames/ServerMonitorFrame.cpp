#include "ServerMonitorFrame.h"

// Event table for ServerMonitorFrame
const wxEventTypeTag<wxCommandEvent> CUSTOM_ACCESS_REQUEST_EVENT(wxNewEventType());
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
    wxPanel* statusPanel = UIStyles::createStyledPanel(mainPanel);
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);

    // Server status indicator (colored circle)
    wxPanel* statusIndicator = new wxPanel(statusPanel, wxID_ANY, wxDefaultPosition, wxSize(12, 12));
    statusIndicator->SetBackgroundColour(UIColors::STATUS_GREEN);
    statusSizer->Add(statusIndicator, 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);

    // Status text
    wxStaticText* statusText = UIStyles::styledText(statusPanel, "Server's running", true);
    statusText->SetForegroundColour(UIColors::STATUS_GREEN);
    statusSizer->Add(statusText, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

    statusPanel->SetSizer(statusSizer);
    mainSizer->Add(statusPanel, 0, wxALL | wxEXPAND, 10);

    // Server Info Card
    wxPanel* infoCard = UIStyles::createStyledPanel(mainPanel);
    wxStaticBoxSizer* infoSizer = new wxStaticBoxSizer(wxVERTICAL, infoCard, "");
    infoCard->SetBackgroundColour(UIColors::SECONDARY);

    // Info grid
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 2, 15, 20);

    // Labels with icons (you can add actual icons using wxBitmap)
    m_hostnameLabel = UIStyles::styledText(infoCard, wxString::Format("%s", m_sysInfo.hostname));
    m_localIPLabel = UIStyles::styledText(infoCard, wxString::Format("%s", m_sysInfo.localIP));
    m_gmailNameLabel = UIStyles::styledText(infoCard, wxString::Format("%s", m_api.getServerName()));

    gridSizer->Add(UIStyles::styledText(infoCard, "Hostname:", true));
    gridSizer->Add(m_hostnameLabel);
    gridSizer->Add(UIStyles::styledText(infoCard, "Local IP:", true));
    gridSizer->Add(m_localIPLabel);
    gridSizer->Add(UIStyles::styledText(infoCard, "Server's Gmail:", true));
    gridSizer->Add(m_gmailNameLabel);

    infoSizer->Add(gridSizer, 1, wxALL | wxEXPAND, 15);
    infoCard->SetSizer(infoSizer);
    mainSizer->Add(infoCard, 0, wxALL | wxEXPAND, 10);

    // Command Monitor Card
    wxPanel* commandCard = UIStyles::createStyledPanel(mainPanel);
    wxStaticBoxSizer* commandSizer = new wxStaticBoxSizer(wxVERTICAL, commandCard, "");

    // Current command section
    wxPanel* commandStatusPanel = UIStyles::createStyledPanel(commandCard);
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
                m_server.gmail.sendEmail(fromEmail, "Access Info",
                    "You already have access.\nExpires at: " + string(timeStr) +
                    "\nHours remaining: " + to_string(static_cast<int>(hoursLeft)), "Instruction.txt");

                // Update UI labels
                m_server.currentCommand.content = "Access Request (Already granted)";
                m_server.currentCommand.message = "Access already granted until: " + string(timeStr);
                m_currentCommandLabel->SetLabel(m_server.currentCommand.content);
                m_currentCommandLabel->SetForegroundColour(UIColors::STATUS_YELLOW);
				m_messageLabelText->SetLabel("Message: ");
				m_messageContentText->SetLabel(m_server.currentCommand.message);



                // Clear the current command
                //m_server.currentCommand.content.clear();
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

        m_server.gmail.sendEmail(access.email, "Access Granted",
            "Access granted for 24 hours.\nExpires at: " + std::string(timeStr), "Instruction.txt");

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
    //m_server.currentCommand.content.clear();
	m_blinkCounter = 0;
	m_accessRequesting = false;

	UpdateCommandInfo();
}


ServerMonitorFrame::~ServerMonitorFrame() {
    if (m_updateTimer) {
        m_updateTimer->Stop();
        delete m_updateTimer;
    }
}