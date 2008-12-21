#if !defined(AFX_SYSLOGPARSERCFG_H__896AD4A6_C35E_4040_908C_ECA84C5BE322__INCLUDED_)
#define AFX_SYSLOGPARSERCFG_H__896AD4A6_C35E_4040_908C_ECA84C5BE322__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SyslogParserCfg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSyslogParserCfg frame

class CSyslogParserCfg : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CSyslogParserCfg)
protected:
	CSyslogParserCfg();           // protected constructor used by dynamic creation
	void WriteStatusMsg(const TCHAR *pszMsg);

// Attributes
protected:
	CScintillaCtrl m_wndEditor;
	CMenu *m_pCtxMenu;
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	int m_iMsgTimer;
	DWORD m_dwTimer;

public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSyslogParserCfg)
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL SaveConfig();
	virtual ~CSyslogParserCfg();

	// Generated message map functions
	//{{AFX_MSG(CSyslogParserCfg)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnParserSave();
	afx_msg void OnUpdateParserSave(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg void OnEditCtrlChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSLOGPARSERCFG_H__896AD4A6_C35E_4040_908C_ECA84C5BE322__INCLUDED_)
