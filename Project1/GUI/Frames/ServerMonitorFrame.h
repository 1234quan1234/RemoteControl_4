#pragma once

#ifndef SERVERMONITORFRAME_H
#define SERVERMONITORFRAME_H

#include <wx/wx.h>
#include "../../Server/ServerManager.h"
#include "../../RemoteControl/SystemInfo.h"
#include "../Styles/UIStyles.h"
#include "../Dialogs/AccessRequestDialog.h"

extern const wxEventTypeTag<wxCommandEvent> CUSTOM_ACCESS_REQUEST_EVENT;


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

#endif