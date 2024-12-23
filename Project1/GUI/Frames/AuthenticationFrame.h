#pragma once
#include <wx/wx.h>
#include <wx/hyperlink.h>
#include <wx/clipbrd.h>
#include "../../Server/ServerManager.h"
#include "../../RemoteControl/SystemInfo.h"
#include "../Styles/UIStyles.h"
#include "ServerMonitorFrame.h"

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