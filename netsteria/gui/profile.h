#ifndef PROFILE_H
#define PROFILE_H

struct ProfilePanel : public wxPanel
{
	ProfilePanel(wxWindow *parent);

private:
	wxTextCtrl *fullNameCtrl;

	wxTextCtrl *prefixCtrl;
	wxTextCtrl *firstNameCtrl;
	wxTextCtrl *middleNameCtrl;
	wxTextCtrl *lastNameCtrl;
	wxTextCtrl *suffixCtrl;

	DECLARE_EVENT_TABLE();


	void OnNameChanged(wxCommandEvent &ev);
};

#endif	// PROFILE_H
