#if !defined(AFX_SETTINGSDLG_H__D71A958C_F19C_49E9_8763_060B6CBDF888__INCLUDED_)
#define AFX_SETTINGSDLG_H__D71A958C_F19C_49E9_8763_060B6CBDF888__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SettingsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg dialog

class CSettingsDlg : public CDialog
{
// Construction
public:
	CSettingsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSettingsDlg)
	enum { IDD = IDD_SETTINGS };
	BOOL	m_bAutoLogin;
	CString	m_strPassword;
	CString	m_strServer;
	CString	m_strUser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableControls(void);

	// Generated message map functions
	//{{AFX_MSG(CSettingsDlg)
	afx_msg void OnCheckAutologin();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTINGSDLG_H__D71A958C_F19C_49E9_8763_060B6CBDF888__INCLUDED_)
