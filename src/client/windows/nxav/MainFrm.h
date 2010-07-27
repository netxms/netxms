// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__8840A3A5_A463_43B9_8540_F0C3A797AB06__INCLUDED_)
#define AFX_MAINFRM_H__8840A3A5_A463_43B9_8540_F0C3A797AB06__INCLUDED_

#include "InfoLine.h"	// Added by ClassView
#include "AlarmBrowser.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	UINT_PTR m_nTimer;
	void SortAlarms(void);
	int m_iNumAlarms[5];
	DWORD m_dwNumAlarms;
	NXC_ALARM *m_pAlarmList;
	void GenerateHtml(CString &strHTML);
	CAlarmBrowser *m_pwndAlarmView;
	void AddAlarm(NXC_ALARM *pAlarm, CString &strHTML, BOOL bColoredLine);
	CInfoLine m_wndInfoLine;
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnCmdExit();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
   afx_msg LRESULT OnAlarmUpdate(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnDisableAlarmSound(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__8840A3A5_A463_43B9_8540_F0C3A797AB06__INCLUDED_)
