// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__6AC60510_4AD4_48EA_AFF7_F15510399DB3__INCLUDED_)
#define AFX_MAINFRM_H__6AC60510_4AD4_48EA_AFF7_F15510399DB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

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
	void BroadcastMessage(UINT msg, WPARAM wParam, LPARAM lParam, BOOL bUsePost);
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CReBar      m_wndReBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateEventList();
	afx_msg void OnClose();
	//}}AFX_MSG
   afx_msg void OnStateChange(WPARAM wParam, LPARAM lParam);
   afx_msg void OnObjectChange(WPARAM wParam, LPARAM lParam);
   afx_msg void OnUserDBChange(WPARAM wParam, LPARAM lParam);
   afx_msg void OnAlarmUpdate(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__6AC60510_4AD4_48EA_AFF7_F15510399DB3__INCLUDED_)
