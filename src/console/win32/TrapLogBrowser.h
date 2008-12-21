#if !defined(AFX_TRAPLOGBROWSER_H__4BF65716_21B1_42CB_AAD4_37E933410142__INCLUDED_)
#define AFX_TRAPLOGBROWSER_H__4BF65716_21B1_42CB_AAD4_37E933410142__INCLUDED_

#include "WaitView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrapLogBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrapLogBrowser frame

class CTrapLogBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CTrapLogBrowser)
protected:
	CTrapLogBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapLogBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CWaitView m_wndWaitView;
	BOOL m_bIsBusy;
	CListCtrl m_wndListCtrl;
	virtual ~CTrapLogBrowser();

   void AddRecord(NXC_SNMP_TRAP_LOG_RECORD *pRec, BOOL bAppend);

	// Generated message map functions
	//{{AFX_MSG(CTrapLogBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
   afx_msg void OnRequestCompleted(void);
   afx_msg void OnTrapLogRecord(WPARAM wParam, NXC_SNMP_TRAP_LOG_RECORD *pRec);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPLOGBROWSER_H__4BF65716_21B1_42CB_AAD4_37E933410142__INCLUDED_)
