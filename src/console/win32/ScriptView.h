#if !defined(AFX_SCRIPTVIEW_H__94A483F2_760D_44BD_A6E0_230B202CAE13__INCLUDED_)
#define AFX_SCRIPTVIEW_H__94A483F2_760D_44BD_A6E0_230B202CAE13__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScriptView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScriptView window

class CScriptView : public CWnd
{
// Construction
public:
	CScriptView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScriptView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL ValidateClose(void);
	void SetEditMode(DWORD dwScriptId, LPCTSTR pszScriptName);
	void SetListMode(CTreeCtrl &wndTreeCtrl, HTREEITEM hRoot);
	virtual ~CScriptView();

	// Generated message map functions
protected:
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	CString m_strScriptName;
	DWORD m_dwScriptId;
	CFlatButton m_wndButton;
	int m_nMode;
	CImageList m_imageList;
   CScintillaCtrl m_wndEditor;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CScriptView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnScriptEdit();
	afx_msg void OnUpdateScriptEdit(CCmdUI* pCmdUI);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
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
	afx_msg void OnEditSelectAll();
	afx_msg void OnUpdateEditSelectAll(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnEditorChange();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRIPTVIEW_H__94A483F2_760D_44BD_A6E0_230B202CAE13__INCLUDED_)
