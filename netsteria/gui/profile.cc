
#include "wx/wx.h"
//#include "wx/datectrl.h"

#include "profile.h"

ProfilePanel::ProfilePanel(wxWindow *parent)
: wxPanel(parent)
{
	const int ngroups = 4;
	wxString groups[4] = {
		_T("Close Friends"),
		_T("Friends"),
		_T("Direct Contacts"),
		_T("Everyone"),
	};
	wxPoint pt(0,0);
	wxSize sz(-1,-1);

	wxFlexGridSizer *topSizer = new wxFlexGridSizer(3, 2, 2);
	topSizer->AddGrowableCol(1);


	// Column headings
	topSizer->Add(0, 0);
	topSizer->Add(0, 0);
	wxStaticText *visibleText = new wxStaticText(this, -1,
		_T("Visible to"));
	topSizer->Add(visibleText, 0, wxALL | wxALIGN_CENTER, 2);


	// Public nickname

	wxStaticText *nicknameText = new wxStaticText(this, -1,
		_T("Nickname"));
	wxTextCtrl *nicknameCtrl = new wxTextCtrl(this, -1);
	wxChoice *nicknameVis = new wxChoice(this, -1, pt, sz, ngroups, groups);
	nicknameVis->SetSelection(ngroups-1);
	nicknameVis->Disable();
	topSizer->Add(nicknameText, 0,
			wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 2);
	topSizer->Add(nicknameCtrl, 0, wxALL | wxGROW, 2);
	topSizer->Add(nicknameVis, 0, wxALL | wxALIGN_CENTER, 2);


	// Full name

	wxStaticText *fullNameText = new wxStaticText(this, -1,
		_T("Full Name"));
	fullNameCtrl = new wxTextCtrl(this, -1);
	wxChoice *fullNameVis = new wxChoice(this, -1, pt, sz, ngroups, groups);
	topSizer->Add(fullNameText, 0,
			wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 2);
	topSizer->Add(fullNameCtrl, 0, wxALL | wxGROW, 2);
	topSizer->Add(fullNameVis, 0, wxALL | wxALIGN_CENTER, 2);

	wxStaticText *prefixText = new wxStaticText(this, -1,
		_T("Prefix"));
	wxStaticText *firstNameText = new wxStaticText(this, -1,
		_T("First"));
	wxStaticText *middleNameText = new wxStaticText(this, -1,
		_T("Middle"));
	wxStaticText *lastNameText = new wxStaticText(this, -1,
		_T("Last"));
	wxStaticText *suffixText = new wxStaticText(this, -1,
		_T("Suffix"));
	prefixCtrl = new wxTextCtrl(this, -1);
	firstNameCtrl = new wxTextCtrl(this, -1);
	middleNameCtrl = new wxTextCtrl(this, -1);
	lastNameCtrl = new wxTextCtrl(this, -1);
	suffixCtrl = new wxTextCtrl(this, -1);
	wxFlexGridSizer *namesSizer = new wxFlexGridSizer(2, 5, 2, 2);
	namesSizer->Add(prefixText, 0, wxALIGN_CENTER);
	namesSizer->Add(firstNameText, 0, wxALIGN_CENTER);
	namesSizer->Add(middleNameText, 0, wxALIGN_CENTER);
	namesSizer->Add(lastNameText, 0, wxALIGN_CENTER);
	namesSizer->Add(suffixText, 0, wxALIGN_CENTER);
	namesSizer->Add(prefixCtrl, 1, wxGROW);
	namesSizer->Add(firstNameCtrl, 1, wxGROW);
	namesSizer->Add(middleNameCtrl, 1, wxGROW);
	namesSizer->Add(lastNameCtrl, 1, wxGROW);
	namesSizer->Add(suffixCtrl, 1, wxGROW);
	for (int i = 1; i <= 3; i++)
		namesSizer->AddGrowableCol(i);

	topSizer->Add(0, 0);
	topSizer->Add(namesSizer, 0, wxALL | wxGROW, 2);
	topSizer->Add(0, 0);


	// Address
	wxStaticText *addrText = new wxStaticText(this, -1,
		_T("Address"));
	wxTextCtrl *addrCtrl = new wxTextCtrl(this, -1,
		_(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	addrCtrl->SetSize(100, 50);
	wxChoice *addrVis = new wxChoice(this, -1, pt, sz, ngroups, groups);
	topSizer->Add(addrText, 0,
			wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 2);
	topSizer->Add(addrCtrl, 0, wxALL | wxGROW, 2);
	topSizer->Add(addrVis, 0, wxALL | wxALIGN_CENTER, 2);


	// Birthday
#if 0
	wxStaticText *birthdayText = new wxStaticText(this, -1,
		_T("Birth Date"));
	birthdayCtrl = new wxDatePickerCtrl(this, -1);
	wxChoice *birthdayVis = new wxChoice(this, -1, pt, sz, ngroups, groups);
	topSizer->Add(birthdayText, 0,
			wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 2);
	topSizer->Add(birthdayCtrl, 0, wxALL | wxGROW, 2);
	topSizer->Add(birthdayVis, 0, wxALL | wxALIGN_CENTER, 2);
#endif

	// Photo
	wxStaticText *photoText = new wxStaticText(this, -1,
		_T("Photo"));
	wxSize photoSize(256, 256);
	wxWindow *photoWin = new wxWindow(this, -1, pt, photoSize,
					wxSUNKEN_BORDER);
	wxButton *photoSet = new wxButton(this, -1, _T("Set"));
	wxBoxSizer *photoSizer = new wxBoxSizer(wxVERTICAL);
	photoSizer->Add(photoWin, 0, wxBOTTOM | wxALIGN_CENTER, 2);
	photoSizer->Add(photoSet, 0, wxTOP | wxALIGN_CENTER, 2);
	wxChoice *photoVis = new wxChoice(this, -1, pt, sz, ngroups, groups);
	topSizer->Add(photoText, 0,
			wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, 2);
	topSizer->Add(photoSizer, 0, wxALL | wxGROW, 2);
	topSizer->Add(photoVis, 0, wxALL | wxALIGN_CENTER, 2);


	SetSizer(topSizer);
}

void ProfilePanel::OnNameChanged(wxCommandEvent &ev)
{
	// Only parse the full name into components
	// if the user hasn't manually edited any of the component fields.
	if (prefixCtrl->IsModified() ||
			firstNameCtrl->IsModified() ||
			middleNameCtrl->IsModified() ||
			lastNameCtrl->IsModified() ||
			suffixCtrl->IsModified())
		return;

	wxString name = fullNameCtrl->GetValue();
	wxString prefix, first, middle, last, suffix;
	int i;

	// Break out the first token as the honorific prefix or first name
	i = name.Find(' ');
	if (i >= 0) {
		first = name.Left(i);
		name = name.Mid(i+1);
	} else {
		first = name;
		name.Clear();
	}

	// See if it's an honorific prefix
	if (first.Find('.') >= 0) {
		prefix = first;

		// Break out the next token as the first name
		i = name.Find(' ');
		if (i >= 0) {
			first = name.Left(i);
			name = name.Mid(i+1);
		} else {
			first = name;
			name.Clear();
		}
	}

	// Break out the last token as an honorific suffix or last name
	i = name.Find(' ', TRUE);
	if (i >= 0) {
		last = name.Mid(i+1);
		name = name.Left(i);
	} else {
		last = name;
		name.Clear();
	}

	// See if it's an honorific suffix
	if (last.Find('.') >= 0) {
		suffix = last;

		// Break out the last token as the last name
		i = name.Find(' ', TRUE);
		if (i >= 0) {
			last = name.Mid(i+1);
			name = name.Left(i);
		} else {
			last = name;
			name.Clear();
		}
	}


	// Take the remainder as any middle name(s)
	middle = name;

	// Set the new text control values
	prefixCtrl->SetValue(prefix);
	firstNameCtrl->SetValue(first);
	middleNameCtrl->SetValue(middle);
	lastNameCtrl->SetValue(last);
	suffixCtrl->SetValue(suffix);
}

BEGIN_EVENT_TABLE(ProfilePanel, wxPanel)
	EVT_TEXT(-1, ProfilePanel::OnNameChanged)
END_EVENT_TABLE()

