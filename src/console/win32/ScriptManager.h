#if !defined(AFX_SCRIPTMANAGER_H__CC6DCB34_1291_4DC4_9137_47CFCFF95555__INCLUDED_)
#define AFX_SCRIPTMANAGER_H__CC6DCB34_1291_4DC4_9137_47CFCFF95555__INCLUDED_

#include "AdvSplitter.h"	// Added by ClassView
#include "ScriptView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScriptManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScriptManager frame

class CScriptManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CScriptManager)
protected:
	CScriptManager();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScriptManager)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void InsertScript(DWORD dwId, TCHAR *pszName);
	void SetMode(int nMode);
	int m_nMode;
	CSimpleListCtrl m_wndListCtrl;
	HTREEITEM m_hRootItem;
	void BuildScriptName(HTREEITEM hItem, CString &strName);
	CImageList m_imageList;
	CScriptView m_wndScriptView;
	CTreeCtrl m_wndTreeCtrl;
	CAdvSplitter m_wndSplitter;
	virtual ~CScriptManager();

	// Generated message map functions
	//{{AFX_MSG(CScriptManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnScriptViewaslist();
	afx_msg void OnUpdateScriptViewaslist(CCmdUI* pCmdUI);
	afx_msg void OnScriptViewastree();
	afx_msg void OnUpdateScriptViewastree(CCmdUI* pCmdUI);
	afx_msg void OnScriptNew();
	//}}AFX_MSG
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult);
   afx_msg void OnTreeViewSelChanging(LPNMTREEVIEW lpnmt, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRIPTMANAGER_H__CC6DCB34_1291_4DC4_9137_47CFCFF95555__INCLUDED_)
