#if !defined(AFX_TRAPEDITOR_H__608EDB5F_7897_413D_916D_9061147CBF34__INCLUDED_)
#define AFX_TRAPEDITOR_H__608EDB5F_7897_413D_916D_9061147CBF34__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrapEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrapEditor frame

class CTrapEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CTrapEditor)
protected:
	CTrapEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList *m_pImageList;
	void UpdateItem(int iItem, DWORD dwIndex);
	int AddItem(DWORD dwIndex);
	CListCtrl m_wndListCtrl;
	virtual ~CTrapEditor();

	// Generated message map functions
	//{{AFX_MSG(CTrapEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateTrapDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTrapEdit(CCmdUI* pCmdUI);
	afx_msg void OnTrapNew();
	afx_msg void OnTrapDelete();
	afx_msg void OnTrapEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	NXC_TRAP_CFG_ENTRY *m_pTrapList;
	DWORD m_dwNumTraps;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPEDITOR_H__608EDB5F_7897_413D_916D_9061147CBF34__INCLUDED_)
