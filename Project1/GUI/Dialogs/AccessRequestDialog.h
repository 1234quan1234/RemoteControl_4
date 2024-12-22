#pragma once
#include <wx/wx.h>
#include "../Styles/UIStyles.h"

class AccessRequestDialog : public wxDialog {
private:
    wxStaticText* m_requestDetailsText;
    wxButton* m_yesButton;
    wxButton* m_noButton;

public:
	AccessRequestDialog(wxWindow* parent, const wxString& fromEmail, bool& accessRequesting);
    int ShowModal() override;
};