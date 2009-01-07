#if !defined(AFX_DESKTOPMANAGER_H__0E45DD9B_4C40_46C4_BDC4_3CB64E1832D5__INCLUDED_)
#define AFX_DESKTOPMANAGER_H__0E45DD9B_4C40_46C4_BDC4_3CB64E1832D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DesktopManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDesktopManager frame

class CDesktopManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDesktopManager)
protected:
	CDesktopManager();           // protected constructor used by dynamic creation

// Attributes
public:
	BOOL m_bEnableCopy;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDesktopManager)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void MoveOrCopyDesktop(BOOL bMove);
	TCHAR m_szCurrConf[MAX_DB_STRING];
	DWORD m_dwCurrUser;
	CImageList m_imageList;
	CTreeCtrl m_wndTreeCtrl;
	virtual ~CDesktopManager();

	// Generated message map functions
	//{{AFX_MSG(CDesktopManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDesktopDelete();
	afx_msg void OnUpdateDesktopDelete(CCmdUI* pCmdUI);
	afx_msg void OnDesktopMove();
	afx_msg void OnUpdateDesktopMove(CCmdUI* pCmdUI);
	afx_msg void OnDesktopCopy();
	afx_msg void OnUpdateDesktopCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG
   afx_msg void OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DESKTOPMANAGER_H__0E45DD9B_4C40_46C4_BDC4_3CB64E1832D5__INCLUDED_)
