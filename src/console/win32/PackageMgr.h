#if !defined(AFX_PACKAGEMGR_H__870772A7_A79F_43FE_ACDB_22C1FCFF6791__INCLUDED_)
#define AFX_PACKAGEMGR_H__870772A7_A79F_43FE_ACDB_22C1FCFF6791__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PackageMgr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPackageMgr frame

class CPackageMgr : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CPackageMgr)
protected:
	CPackageMgr();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPackageMgr)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	CImageList m_imageList;
   DWORD m_dwNumPackages;
   NXC_PACKAGE_INFO *m_pPackageList;
	virtual ~CPackageMgr();
	void AddListItem(NXC_PACKAGE_INFO *pInfo, BOOL bSelect);

	// Generated message map functions
	//{{AFX_MSG(CPackageMgr)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnViewRefresh();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	afx_msg void OnPackageInstall();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdatePackageRemove(CCmdUI* pCmdUI);
	afx_msg void OnPackageRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PACKAGEMGR_H__870772A7_A79F_43FE_ACDB_22C1FCFF6791__INCLUDED_)
