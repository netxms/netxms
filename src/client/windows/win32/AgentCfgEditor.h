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
	DWORD m_dwTimer;
	int m_iMsgTimer;
	void WriteStatusMsg(TCHAR *pszMsg);
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	BOOL SaveConfig(BOOL bApply);
	CMenu *m_pCtxMenu;
	CScintillaCtrl m_wndEditor;
	DWORD m_dwNodeId;
	virtual ~CAgentCfgEditor();

	// Generated message map functions
	//{{AFX_MSG(CAgentCfgEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditDelete();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSelectAll();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnConfigSave();
	afx_msg void OnConfigSaveandapply();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnEditCtrlChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTCFGEDITOR_H__0E494DF9_D52B_4AA6_9B4C_118C59609CEC__INCLUDED_)
