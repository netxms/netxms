#if !defined(AFX_NEWUSERDLG_H__7C8775CB_49D4_429B_B052_A71BB6D1F9CC__INCLUDED_)
#define AFX_NEWUSERDLG_H__7C8775CB_49D4_429B_B052_A71BB6D1F9CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewUserDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewUserDlg dialog

class CNewUserDlg : public CDialog
{
// Construction
public:
	CString m_strTitle;
	CNewUserDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewUserDlg)
	enum { IDD = IDD_NEW_USER };
	BOOL	m_bDefineProperties;
	CString	m_strName;
	CString	m_strHeader;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewUserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewUserDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWUSERDLG_H__7C8775CB_49D4_429B_B052_A71BB6D1F9CC__INCLUDED_)
