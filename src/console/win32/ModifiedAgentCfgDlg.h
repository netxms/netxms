#if !defined(AFX_MODIFIEDAGENTCFGDLG_H__0D076092_2B35_4E7D_AEDF_5D5BBFCA914C__INCLUDED_)
#define AFX_MODIFIEDAGENTCFGDLG_H__0D076092_2B35_4E7D_AEDF_5D5BBFCA914C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModifiedAgentCfgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CModifiedAgentCfgDlg dialog

class CModifiedAgentCfgDlg : public CDialog
{
// Construction
public:
	CModifiedAgentCfgDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CModifiedAgentCfgDlg)
	enum { IDD = IDD_SAVE_AGENT_CFG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModifiedAgentCfgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CModifiedAgentCfgDlg)
	afx_msg void OnSave();
	afx_msg void OnDiscard();
	afx_msg void OnApply();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODIFIEDAGENTCFGDLG_H__0D076092_2B35_4E7D_AEDF_5D5BBFCA914C__INCLUDED_)
