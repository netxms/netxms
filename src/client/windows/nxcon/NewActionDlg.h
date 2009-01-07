#if !defined(AFX_NEWACTIONDLG_H__C445DD8E_1569_4F71_9A97_FEC8D909D952__INCLUDED_)
#define AFX_NEWACTIONDLG_H__C445DD8E_1569_4F71_9A97_FEC8D909D952__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewActionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewActionDlg dialog

class CNewActionDlg : public CDialog
{
// Construction
public:
	CNewActionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewActionDlg)
	enum { IDD = IDD_NEW_ACTION };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewActionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewActionDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWACTIONDLG_H__C445DD8E_1569_4F71_9A97_FEC8D909D952__INCLUDED_)
