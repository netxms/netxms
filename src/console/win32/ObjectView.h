#if !defined(AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_)
#define AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_

#include "ObjectOverview.h"	// Added by ClassView
#include "AlarmView.h"	// Added by ClassView
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
   void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
	void SetCurrentObject(NXC_OBJECT *pObject);
	void Refresh(void);
	virtual ~CObjectView();

	// Generated message map functions
protected:
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
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__53B1ADB5_D509_424F_A9C8_F3B8FBA61F1B__INCLUDED_)
