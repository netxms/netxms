#if !defined(AFX_CONSOLEUPGRADEDLG_H__E34F275F_BC69_41A4_BB34_57F5ADBD12C6__INCLUDED_)
#define AFX_CONSOLEUPGRADEDLG_H__E34F275F_BC69_41A4_BB34_57F5ADBD12C6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConsoleUpgradeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConsoleUpgradeDlg dialog

class CConsoleUpgradeDlg : public CDialog
{
// Construction
public:
	CConsoleUpgradeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConsoleUpgradeDlg)
	enum { IDD = IDD_UPGRADE };
	CString	m_strURL;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsoleUpgradeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConsoleUpgradeDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONSOLEUPGRADEDLG_H__E34F275F_BC69_41A4_BB34_57F5ADBD12C6__INCLUDED_)
