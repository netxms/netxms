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
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetEditMode(DWORD dwScriptId);
	void SetListMode(CTreeCtrl &wndTreeCtrl, HTREEITEM hRoot);
	virtual ~CScriptView();

	// Generated message map functions
protected:
	int m_nMode;
	CImageList m_imageList;
	CEdit m_wndEditor;
	CListCtrl m_wndListCtrl;
	//{{AFX_MSG(CScriptView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRIPTVIEW_H__94A483F2_760D_44BD_A6E0_230B202CAE13__INCLUDED_)
