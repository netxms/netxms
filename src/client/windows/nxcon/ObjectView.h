#if !defined(AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_)
#define AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_

#include "ObjectOverview.h"	// Added by ClassView
#include "AlarmView.h"	// Added by ClassView
#include "ClusterView.h"	// Added by ClassView
#include "NodePerfView.h"
#include "ExtEditCtrl.h"
#include "NodeLastValuesView.h"	// Added by ClassView
#include "ObjectSubordinateView.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectView.h : header file
//

#define MAX_TABS     16


/////////////////////////////////////////////////////////////////////////////
// CObjectView window

class CObjectView : public CWnd
{
// Construction
public:
	CObjectView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ShowSearchBar(BOOL bShow);
   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
	void SetCurrentObject(NXC_OBJECT *pObject);
	void Refresh(void);
	virtual ~CObjectView();

	// Generated message map functions
protected:
	CObjectSubordinateView m_wndSubordinateView;
	CNodeLastValuesView m_wndLastValuesView;
	int m_nSearchTextOffset;
	CImageList m_imageListSearch;
	CToolBarCtrl m_wndSearchButtons;
	CExtEditCtrl m_wndSearchText;
	CStatic m_wndSearchHint;
	int m_nTitleBarOffset;
	void AdjustView(void);
	BOOL m_bShowSearchBar;
	CNodePerfView m_wndPerfView;
	CClusterView m_wndClusterView;
	CAlarmView m_wndAlarms;
	void CreateTab(int nIndex, TCHAR *pszName, int nImage, CWnd *pWnd);
	CWnd *m_pTabWnd[MAX_TABS];
	CObjectOverview m_wndOverview;
	CImageList m_imageList;
	CFont m_fontHeader;
	CFont m_fontTabs;
	CTabCtrl m_wndTabCtrl;
	NXC_OBJECT * m_pObject;
	//{{AFX_MSG(CObjectView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	//}}AFX_MSG
   afx_msg void OnTabChange(LPNMHDR lpnmh, LRESULT *pResult);
	afx_msg void OnSearchFindFirst();
	afx_msg void OnSearchFindNext();
	afx_msg void OnSearchNextInstance();
	afx_msg void OnSearchClose();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_)
