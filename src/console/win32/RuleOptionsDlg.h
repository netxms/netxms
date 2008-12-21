#if !defined(AFX_RULEOPTIONSDLG_H__6A63AAB5_ED45_4FA3_A0A3_2ACC92A7EED1__INCLUDED_)
#define AFX_RULEOPTIONSDLG_H__6A63AAB5_ED45_4FA3_A0A3_2ACC92A7EED1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleOptionsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleOptionsDlg dialog

class CRuleOptionsDlg : public CDialog
{
// Construction
public:
	CRuleOptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleOptionsDlg)
	enum { IDD = IDD_EDIT_RULE_OPTIONS };
	BOOL	m_bStopProcessing;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRuleOptionsDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULEOPTIONSDLG_H__6A63AAB5_ED45_4FA3_A0A3_2ACC92A7EED1__INCLUDED_)
