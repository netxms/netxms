#if !defined(AFX_RULESEVERITYDLG_H__E16143DF_14D3_4CD2_9B8C_77CA0C916897__INCLUDED_)
#define AFX_RULESEVERITYDLG_H__E16143DF_14D3_4CD2_9B8C_77CA0C916897__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleSeverityDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleSeverityDlg dialog

class CRuleSeverityDlg : public CDialog
{
// Construction
public:
	CRuleSeverityDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleSeverityDlg)
	enum { IDD = IDD_EDIT_RULE_SEVERITY };
	BOOL	m_bCritical;
	BOOL	m_bMajor;
	BOOL	m_bMinor;
	BOOL	m_bNormal;
	BOOL	m_bWarning;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleSeverityDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRuleSeverityDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULESEVERITYDLG_H__E16143DF_14D3_4CD2_9B8C_77CA0C916897__INCLUDED_)
