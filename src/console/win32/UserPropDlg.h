#if !defined(AFX_USERPROPDLG_H__4F7FEF4B_E193_46D4_87E6_810864495585__INCLUDED_)
#define AFX_USERPROPDLG_H__4F7FEF4B_E193_46D4_87E6_810864495585__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserPropDlg dialog

class CUserPropDlg : public CDialog
{
// Construction
public:
	CUserPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUserPropDlg)
	enum { IDD = IDD_USER_PROPERTIES };
	BOOL	m_bAccountDisabled;
	BOOL	m_bDropConn;
	BOOL	m_bEditConfig;
	BOOL	m_bEditEventDB;
	BOOL	m_bManageUsers;
	BOOL	m_bChangePassword;
	BOOL	m_bViewConfig;
	BOOL	m_bViewEventDB;
	CString	m_strDescription;
	CString	m_strLogin;
	CString	m_strFullName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUserPropDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERPROPDLG_H__4F7FEF4B_E193_46D4_87E6_810864495585__INCLUDED_)
