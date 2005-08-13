#if !defined(AFX_AGENTCFGEDITOR_H__0E494DF9_D52B_4AA6_9B4C_118C59609CEC__INCLUDED_)
#define AFX_AGENTCFGEDITOR_H__0E494DF9_D52B_4AA6_9B4C_118C59609CEC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AgentCfgEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAgentCfgEditor frame

class CAgentCfgEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CAgentCfgEditor)
protected:
	CAgentCfgEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CAgentCfgEditor(DWORD dwNodeId);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentCfgEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CFont m_font;
	CRichEditCtrl m_wndEdit;
	DWORD m_dwNodeId;
	virtual ~CAgentCfgEditor();

	// Generated message map functions
	//{{AFX_MSG(CAgentCfgEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTCFGEDITOR_H__0E494DF9_D52B_4AA6_9B4C_118C59609CEC__INCLUDED_)
