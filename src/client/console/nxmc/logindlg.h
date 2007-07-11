#ifndef _logindlg_h_
#define _logindlg_h_

class nxLoginDialog : public wxDialog
{
protected:
	virtual bool TransferDataFromWindow(void);
	virtual bool TransferDataToWindow(void);

public:
	nxLoginDialog(wxWindow *parent);

	wxString m_login;
	wxString m_password;
	wxString m_server;
	bool m_isEncrypt;
	bool m_isCacheDisabled;
	bool m_isClearCache;
	bool m_isMatchVersion;

// Event handlers
protected:
	void OnInitDialog(wxInitDialogEvent &event);

	DECLARE_EVENT_TABLE()
};

#endif
