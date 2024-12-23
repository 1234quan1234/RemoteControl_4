// GUI/App/RemoteControlApp.h
#pragma once
#include <wx/wx.h>
#include "../../Server/ServerManager.h"
#include "../../RemoteControl/SystemInfo.h"
#include "../Frames/AuthenticationFrame.h"
#include "../Frames/ServerMonitorFrame.h"

class RemoteControlApp : public wxApp {
private:
    GmailAPI* m_api;
    ServerManager* m_server;
    SystemInfo* m_sysInfo;

public:
    virtual bool OnInit() override;
};
