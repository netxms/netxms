#if !defined(AFX_EVENTBROWSER_H__02BA6E09_9B47_47DF_BD7F_5D39FB02A78C__INCLUDED_)
#define AFX_EVENTBROWSER_H__02BA6E09_9B47_47DF_BD7F_5D39FB02A78C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventBrowser frame

class CEventBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEventBrowser)
protected:
	CEventBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void EnableDisplay(BOOL bEnable);
	void AddEvent(NXC_EVENT *pEvent);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	virtual ~CEventBrowser();

	// Generated message map functions
	//{{AFX_MSG(CEventBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTBROWSER_H__02BA6E09_9B47_47DF_BD7F_5D39FB02A78C__INCLUDED_)
