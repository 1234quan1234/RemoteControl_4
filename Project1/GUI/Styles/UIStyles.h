#pragma once
#include <wx/wx.h>
#include "UIColors.h"

class UIStyles {
public:
    static wxButton* styledButton(wxWindow* parent, wxWindowID id, const wxString& label) {
        wxButton* button = new wxButton(parent, id, label);
        button->SetBackgroundColour(UIColors::PRIMARY);
        button->SetForegroundColour(UIColors::SECONDARY);
        button->SetMinSize(wxSize(120, 35));

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

    static wxPanel* createStyledPanel(wxWindow* parent) {
        wxPanel* panel = new wxPanel(parent, wxID_ANY);
        panel->SetBackgroundColour(UIColors::PANEL_BG);
        return panel;
    }

    static wxStaticText* styledText(wxWindow* parent, const wxString& label, bool isBold = false) {
        wxStaticText* text = new wxStaticText(parent, wxID_ANY, label);
        wxFont font = text->GetFont();
        if (isBold) {
            font.SetWeight(wxFONTWEIGHT_BOLD);
        }
        text->SetFont(font);
        return text;
    }
};