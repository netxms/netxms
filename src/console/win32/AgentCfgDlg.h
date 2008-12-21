#if !defined(AFX_AGENTCFGDLG_H__F9633D24_CCCC_40A5_8EF1_CC9D804C25FC__INCLUDED_)
#define AFX_AGENTCFGDLG_H__F9633D24_CCCC_40A5_8EF1_CC9D804C25FC__INCLUDED_

#include "..\NXUILIB\ScintillaCtrl.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AgentCfgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgDlg dialog

class CAgentCfgDlg : public CDialog
{
// Construction
public:
	CString m_strFilter;
	CString m_strText;
	CAgentCfgDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAgentCfgDlg();

// Dialog Data
	//{{AFX_DATA(CAgentCfgDlg)
	enum { IDD = IDD_AGENT_CONFIG };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentCfgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CScintillaCtrl m_wndEditText;
	CScintillaCtrl m_wndEditFilter;

	// Generated message map functions
	//{{AFX_MSG(CAgentCfgDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTCFGDLG_H__F9633D24_CCCC_40A5_8EF1_CC9D804C25FC__INCLUDED_)
