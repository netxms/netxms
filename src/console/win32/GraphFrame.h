#if !defined(AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_)
#define AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_

#include "Graph.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphFrame.h : header file
//


class CGraphStatusBar : public CStatusBarCtrl
{
public:
   DWORD m_dwMaxValue;

protected:
   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
};


//
// Flags
//

#define GF_AUTOUPDATE      0x0001


/////////////////////////////////////////////////////////////////////////////
// CGraphFrame frame

class CGraphFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CGraphFrame)
protected:

// Attributes
public:

// Operations
public:
	void SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo);
	void AddItem(DWORD dwNodeId, DWORD dwItemId);
	CGraphFrame();           // default constructor
	virtual ~CGraphFrame();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD m_dwSeconds;
	CGraphStatusBar m_wndStatusBar;
	UINT m_hTimer;

	// Generated message map functions
	//{{AFX_MSG(CGraphFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnGraphProperties();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int m_iStatusBarHeight;
	DWORD m_dwRefreshInterval;
	DWORD m_dwFlags;
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	DWORD m_pdwItemId[MAX_GRAPH_ITEMS];
	DWORD m_pdwNodeId[MAX_GRAPH_ITEMS];
	DWORD m_dwNumItems;
	CGraph m_wndGraph;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_)
