#if !defined(AFX_CONNECTIONPAGE_H__0FA3939A_B41E_4595_B4F8_80AD6BD5D4D4__INCLUDED_)
#define AFX_CONNECTIONPAGE_H__0FA3939A_B41E_4595_B4F8_80AD6BD5D4D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConnectionPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectionPage dialog

class CConnectionPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CConnectionPage)

// Construction
public:
	CConnectionPage();
	~CConnectionPage();

// Dialog Data
	//{{AFX_DATA(CConnectionPage)
	enum { IDD = IDD_CONNECTION_PAGE };
	BOOL	m_bAutoLogin;
	BOOL	m_bEncrypt;
	CString	m_strLogin;
	CString	m_strPassword;
	CString	m_strServer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CConnectionPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetItemState(void);
	// Generated message map functions
	//{{AFX_MSG(CConnectionPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckAutologin();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTIONPAGE_H__0FA3939A_B41E_4595_B4F8_80AD6BD5D4D4__INCLUDED_)
