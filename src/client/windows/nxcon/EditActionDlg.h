#if !defined(AFX_EDITACTIONDLG_H__C1450122_B42F_452B_9F7C_E7D46C630564__INCLUDED_)
#define AFX_EDITACTIONDLG_H__C1450122_B42F_452B_9F7C_E7D46C630564__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditActionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditActionDlg dialog

class CEditActionDlg : public CDialog
{
// Construction
public:
	CEditActionDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditActionDlg)
	enum { IDD = IDD_ACTION_PROPERTIES };
	CString	m_strData;
	CString	m_strName;
	CString	m_strRcpt;
	CString	m_strSubject;
	int		m_iType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditActionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void OnTypeChange(void);

	// Generated message map functions
	//{{AFX_MSG(CEditActionDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioEmail();
	afx_msg void OnRadioExec();
	afx_msg void OnRadioRexec();
	afx_msg void OnRadioSms();
	afx_msg void OnRadioForward();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITACTIONDLG_H__C1450122_B42F_452B_9F7C_E7D46C630564__INCLUDED_)
