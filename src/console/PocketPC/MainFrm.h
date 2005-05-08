// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__A3A56849_27C5_4B36_9F48_86C7088BF7BF__INCLUDED_)
#define AFX_MAINFRM_H__A3A56849_27C5_4B36_9F48_86C7088BF7BF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ObjectView.h"	// Added by ClassView
#include "SummaryView.h"	// Added by ClassView
#include "AlarmView.h"	// Added by ClassView
#include "DynamicView.h"	// Added by ClassView

// Array for the toolbar buttons

#if (_WIN32_WCE < 201)
static TBBUTTON g_arCBButtons[] = {
	{ 0,	ID_FILE_NEW,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  0},
	{ 1,    ID_FILE_OPEN,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  1},
	{ 2,	ID_FILE_SAVE,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  2},
	{ 0,	0,				TBSTATE_ENABLED, TBSTYLE_SEP,		0, 0, 0, -1},
	{ 3,    ID_EDIT_CUT,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  3},
	{ 4,	ID_EDIT_COPY,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  4},
	{ 5,	ID_EDIT_PASTE,	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0, 0, 0,  5}
};
#endif

#if defined(_WIN32_WCE_PSPC) && (_WIN32_WCE >= 212)
#define NUM_TOOL_TIPS 8
#endif


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
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ActivateView(CWnd *pwndView);
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CCeCommandBar	m_wndCommandBar;

// Generated message map functions
protected:
	DWORD FindViewInList(CWnd *pwndView);
	DWORD m_dwNumViews;
	CBitmapButton m_wndBtnNext;
	CBitmapButton m_wndBtnPrev;
	CToolBarCtrl m_wndToolBar;
	CBitmapButton m_wndBtnClose;
	CWnd *m_pwndViewList[MAX_DYNAMIC_VIEWS + 3];
	CAlarmView m_wndAlarmView;
	CWnd *m_pwndCurrView;
	CSummaryView m_wndSummaryView;
	CObjectView m_wndObjectView;
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnViewObjects();
	afx_msg void OnViewSummary();
	afx_msg void OnViewAlarms();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefreshAll();
	afx_msg void OnPaint();
	afx_msg void OnViewNext();
	afx_msg void OnViewPrev();
	//}}AFX_MSG
   afx_msg void OnObjectChange(WPARAM wParam, LPARAM lParam);
   afx_msg void OnAlarmUpdate(WPARAM wParam, LPARAM lParam);
	LPTSTR MakeString(UINT stringID);
	LPTSTR m_ToolTipsTable[NUM_TOOL_TIPS]; 

	DECLARE_MESSAGE_MAP()

public:
	void CreateView(CDynamicView *pwndView, TCHAR *pszTitle);
	void UnregisterView(CDynamicView *pView);
	BOOL RegisterView(CDynamicView *pView);
	COLORREF m_rgbTitleText;
	COLORREF m_rgbTitleBkgnd;
   int *GetAlarmStats(void) { return m_wndAlarmView.m_iNumAlarms; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__A3A56849_27C5_4B36_9F48_86C7088BF7BF__INCLUDED_)
