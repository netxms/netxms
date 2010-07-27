#if !defined(AFX_NODEPERFVIEW_H__66B55EB4_974D_401D_B1CF_15C4D134347D__INCLUDED_)
#define AFX_NODEPERFVIEW_H__66B55EB4_974D_401D_B1CF_15C4D134347D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodePerfView.h : header file
//

#include <nxqueue.h>
#include "Graph.h"


typedef struct
{
	CGraph *pWnd;
	DWORD dwItemId[MAX_GRAPH_ITEMS];
} PERF_GRAPH;

/////////////////////////////////////////////////////////////////////////////
// CNodePerfView window

class CNodePerfView : public CWnd
{
// Construction
public:
	CNodePerfView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodePerfView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void WorkerThread(void);
	virtual ~CNodePerfView();

	// Generated message map functions
protected:
	DWORD FindItemByName(NXC_PERFTAB_DCI *pItemList, DWORD dwNumItems, TCHAR *pszName);
	int m_nViewHeight;
	int m_nOrigin;
	int m_nTotalHeight;
	void AdjustView(void);
	CFont m_fontTitle;
	int m_nTitleHeight;
	BOOL CreateGraph(NXC_PERFTAB_DCI *pItemList, DWORD dwNumItems, TCHAR *pszParam,
	                 TCHAR *pszTitle, RECT &rect, BOOL bArea);
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	void UpdateAllGraphs(void);
	UINT_PTR m_nTimer;
	Queue m_workerQueue;
	THREAD m_hWorkerThread;
	DWORD m_dwNumGraphs;
	PERF_GRAPH *m_pGraphList;
	int m_nState;
	NXC_OBJECT * m_pObject;
	//{{AFX_MSG(CNodePerfView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	//}}AFX_MSG
   afx_msg LRESULT OnSetObject(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRequestCompleted(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateFinished(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGraphData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	void CreateCustomGraph(NXC_PERFTAB_DCI *dci, RECT &rect);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPERFVIEW_H__66B55EB4_974D_401D_B1CF_15C4D134347D__INCLUDED_)
