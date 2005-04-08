#if !defined(AFX_LOGINDIALOG_H__C97D464C_47D1_4262_88B9_A2AC4EC0D4F9__INCLUDED_)
#define AFX_LOGINDIALOG_H__C97D464C_47D1_4262_88B9_A2AC4EC0D4F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoginDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginDialog dialog

class CLoginDialog : public CDialog
{
// Construction
public:
	CLoginDialog(CWnd* pParent = NULL);   // standard constructor
   virtual ~CLoginDialog();

// Dialog Data
	//{{AFX_DATA(CLoginDialog)
	enum { IDD = IDD_LOGIN };
	CString	m_szLogin;
	CString	m_szPassword;
	CString	m_szServer;
	int		m_iEncryption;
	BOOL	m_bClearCache;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CFont m_font;
	HBRUSH m_hNullBrush;

	// Generated message map functions
	//{{AFX_MSG(CLoginDialog)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINDIALOG_H__C97D464C_47D1_4262_88B9_A2AC4EC0D4F9__INCLUDED_)
