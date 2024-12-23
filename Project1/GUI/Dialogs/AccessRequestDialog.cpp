#include "AccessRequestDialog.h"

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
    m_yesButton = UIStyles::styledButton(panel, wxID_YES, "Yes");
    m_noButton = UIStyles::styledButton(panel, wxID_NO, "No");

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