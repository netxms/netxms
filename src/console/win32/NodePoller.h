#if !defined(AFX_NODEPOLLER_H__E530007B_DAA0_485D_A9B0_60B0284FFAF6__INCLUDED_)
#define AFX_NODEPOLLER_H__E530007B_DAA0_485D_A9B0_60B0284FFAF6__INCLUDED_

#include "globals.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePoller.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodePoller frame

class CNodePoller : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CNodePoller)
protected:
	CNodePoller();           // protected constructor used by dynamic creation

public:

// Operations
public:
	DWORD m_dwObjectId;
	int m_iPollType;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodePoller)
	//}}AFX_VIRTUAL

// Implementation
protected:
	RqData m_data;
	void PrintMsg(TCHAR *pszMsg);
	BOOL m_bPollingStopped;
	CFont m_font;
	CScintillaCtrl m_wndMsgArea;
	DWORD m_dwResult;
	virtual ~CNodePoller();

	// Generated message map functions
	//{{AFX_MSG(CNodePoller)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPollRestart();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPollerCopytoclipboard();
	afx_msg void OnUpdatePollerCopytoclipboard(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectAll();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
   afx_msg void OnRequestCompleted(WPARAM wParam, LPARAM lParam);
   afx_msg void OnPollerMessage(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPOLLER_H__E530007B_DAA0_485D_A9B0_60B0284FFAF6__INCLUDED_)
