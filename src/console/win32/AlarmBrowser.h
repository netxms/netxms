#if !defined(AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_)
#define AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser frame

class CAlarmBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CAlarmBrowser)
protected:
	CAlarmBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iNumAlarms[5];
	void UpdateStatusBar(void);
	int m_iStatusBarHeight;
	CStatusBarCtrl m_wndStatusBar;
	int FindAlarmRecord(DWORD dwAlarmId);
	CFont m_fontNormal;
	void AddAlarm(NXC_ALARM *pAlarm);
	BOOL m_bShowAllAlarms;
	CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	virtual ~CAlarmBrowser();

	// Generated message map functions
	//{{AFX_MSG(CAlarmBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnAlarmAcknowlege();
	afx_msg void OnUpdateAlarmAcknowlege(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMBROWSER_H__0378A627_F763_4D57_87DC_76A9EF2560D3__INCLUDED_)
