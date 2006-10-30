#if !defined(AFX_OBJECTCOMMENTSEDITOR_H__7585F850_CC4B_4AE5_80C1_0264B69BD0ED__INCLUDED_)
#define AFX_OBJECTCOMMENTSEDITOR_H__7585F850_CC4B_4AE5_80C1_0264B69BD0ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectCommentsEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectCommentsEditor frame

class CObjectCommentsEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CObjectCommentsEditor)
protected:
	CObjectCommentsEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CObjectCommentsEditor(DWORD dwObjectId);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectCommentsEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SaveComments(void);
	CRichEditCtrl m_wndEdit;
	DWORD m_dwObjectId;
	virtual ~CObjectCommentsEditor();

	// Generated message map functions
	//{{AFX_MSG(CObjectCommentsEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnClose();
	afx_msg void OnCommentsSave();
	afx_msg void OnUpdateCommentsSave(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTCOMMENTSEDITOR_H__7585F850_CC4B_4AE5_80C1_0264B69BD0ED__INCLUDED_)
