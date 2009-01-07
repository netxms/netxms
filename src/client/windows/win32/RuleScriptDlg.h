#if !defined(AFX_RULESCRIPTDLG_H__1FA0DC31_2D24_4230_81E2_960E5FB7A84A__INCLUDED_)
#define AFX_RULESCRIPTDLG_H__1FA0DC31_2D24_4230_81E2_960E5FB7A84A__INCLUDED_

#include "..\NXUILIB\ScintillaCtrl.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleScriptDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleScriptDlg dialog

class CRuleScriptDlg : public CDialog
{
// Construction
public:
	CString m_strText;
	CRuleScriptDlg(CWnd* pParent = NULL);   // standard constructor
   ~CRuleScriptDlg();

// Dialog Data
	//{{AFX_DATA(CRuleScriptDlg)
	enum { IDD = IDD_EDIT_RULE_SCRIPT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleScriptDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CScintillaCtrl m_wndEdit;

	// Generated message map functions
	//{{AFX_MSG(CRuleScriptDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULESCRIPTDLG_H__1FA0DC31_2D24_4230_81E2_960E5FB7A84A__INCLUDED_)
