#if !defined(AFX_SERVERCFGEDITOR_H__1422748F_ED2C_4207_B3B5_F0A8BDA792BF__INCLUDED_)
#define AFX_SERVERCFGEDITOR_H__1422748F_ED2C_4207_B3B5_F0A8BDA792BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ServerCfgEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServerCfgEditor frame

class CServerCfgEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CServerCfgEditor)
protected:
	CServerCfgEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerCfgEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddItem(NXC_SERVER_VARIABLE *pVar);
	CListCtrl m_wndListCtrl;
	virtual ~CServerCfgEditor();

	// Generated message map functions
	//{{AFX_MSG(CServerCfgEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	afx_msg void OnVariableEdit();
	afx_msg void OnUpdateVariableDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateVariableEdit(CCmdUI* pCmdUI);
	afx_msg void OnVariableNew();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnVariableDelete();
	//}}AFX_MSG
   afx_msg void OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERCFGEDITOR_H__1422748F_ED2C_4207_B3B5_F0A8BDA792BF__INCLUDED_)
