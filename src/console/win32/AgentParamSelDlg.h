#if !defined(AFX_AGENTPARAMSELDLG_H__04E7FF90_ABEA_40BC_A931_6AB70708E4C3__INCLUDED_)
#define AFX_AGENTPARAMSELDLG_H__04E7FF90_ABEA_40BC_A931_6AB70708E4C3__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AgentParamSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAgentParamSelDlg dialog

class CAgentParamSelDlg : public CDialog
{
// Construction
public:
	NXC_OBJECT *m_pNode;
	DWORD m_dwSelectionIndex;
	NXC_AGENT_PARAM *m_pParamList;
	DWORD m_dwNumParams;
	CAgentParamSelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAgentParamSelDlg)
	enum { IDD = IDD_SELECT_AGENT_PARAM };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentParamSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAgentParamSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonGet();
	afx_msg void OnDblclkListParams(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTPARAMSELDLG_H__04E7FF90_ABEA_40BC_A931_6AB70708E4C3__INCLUDED_)
