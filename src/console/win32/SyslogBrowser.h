#if !defined(AFX_SYSLOGBROWSER_H__743A897E_9FF2_4815_BE2F_B66A2E6436B8__INCLUDED_)
#define AFX_SYSLOGBROWSER_H__743A897E_9FF2_4815_BE2F_B66A2E6436B8__INCLUDED_

#include "WaitView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SyslogBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSyslogBrowser frame

class CSyslogBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CSyslogBrowser)
protected:
	CSyslogBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSyslogBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void AddRecord(NXC_SYSLOG_RECORD *pRec, BOOL bAppend);
	CImageList *m_pImageList;
	BOOL m_bIsBusy;
	CWaitView m_wndWaitView;
	CListCtrl m_wndListCtrl;
	virtual ~CSyslogBrowser();

	// Generated message map functions
	//{{AFX_MSG(CSyslogBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
   afx_msg void OnRequestCompleted(void);
   afx_msg void OnSyslogRecord(WPARAM wParam, NXC_SYSLOG_RECORD *pRec);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSLOGBROWSER_H__743A897E_9FF2_4815_BE2F_B66A2E6436B8__INCLUDED_)
