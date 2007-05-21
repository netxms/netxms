#if !defined(AFX_RULEALARMDLG_H__D15B3CCA_6C75_4CFE_9CC2_E1BB449F665A__INCLUDED_)
#define AFX_RULEALARMDLG_H__D15B3CCA_6C75_4CFE_9CC2_E1BB449F665A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleAlarmDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleAlarmDlg dialog

class CRuleAlarmDlg : public CDialog
{
// Construction
public:
	DWORD m_dwTimeoutEvent;
	CRuleAlarmDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleAlarmDlg)
	enum { IDD = IDD_EDIT_RULE_ALARM };
	int		m_iSeverity;
	CString	m_strMessage;
	CString	m_strKey;
	CString	m_strAckKey;
	DWORD	m_dwTimeout;
	//}}AFX_DATA
	int m_nMode;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleAlarmDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableControls(int nMode);

	// Generated message map functions
	//{{AFX_MSG(CRuleAlarmDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioNone();
	afx_msg void OnRadioNewalarm();
	afx_msg void OnRadioTerminate();
	afx_msg void OnSelectEvent();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULEALARMDLG_H__D15B3CCA_6C75_4CFE_9CC2_E1BB449F665A__INCLUDED_)
