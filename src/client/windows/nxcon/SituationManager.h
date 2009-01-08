#if !defined(AFX_SITUATIONMANAGER_H__55E503C1_F283_4668_A23E_9F59BC7AEB12__INCLUDED_)
#define AFX_SITUATIONMANAGER_H__55E503C1_F283_4668_A23E_9F59BC7AEB12__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SituationManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSituationManager frame

class CSituationManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CSituationManager)
protected:
	CSituationManager();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSituationManager)
	//}}AFX_VIRTUAL

// Implementation
protected:
	HTREEITEM m_root;
	CImageList m_imageList;
	CTreeCtrl m_wndTreeCtrl;
	virtual ~CSituationManager();

	// Generated message map functions
	//{{AFX_MSG(CSituationManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnDestroy();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSituationCreate();
	afx_msg void OnSituationDelete();
	afx_msg void OnUpdateSituationDelete(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT OnSituationChange(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SITUATIONMANAGER_H__55E503C1_F283_4668_A23E_9F59BC7AEB12__INCLUDED_)
