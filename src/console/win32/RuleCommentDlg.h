#if !defined(AFX_RULECOMMENTDLG_H__A820A0B3_5122_4CDA_83CC_BEC2133A44D3__INCLUDED_)
#define AFX_RULECOMMENTDLG_H__A820A0B3_5122_4CDA_83CC_BEC2133A44D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleCommentDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleCommentDlg dialog

class CRuleCommentDlg : public CDialog
{
// Construction
public:
	CRuleCommentDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleCommentDlg)
	enum { IDD = IDD_EDIT_RULE_COMMENT };
	CString	m_strText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleCommentDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRuleCommentDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULECOMMENTDLG_H__A820A0B3_5122_4CDA_83CC_BEC2133A44D3__INCLUDED_)
