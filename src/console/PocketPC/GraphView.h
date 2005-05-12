#if !defined(AFX_GRAPHVIEW_H__BD76B70C_E3AE_42A2_B317_0D07EC27140B__INCLUDED_)
#define AFX_GRAPHVIEW_H__BD76B70C_E3AE_42A2_B317_0D07EC27140B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphView.h : header file
//

#include "DynamicView.h"
#include "Graph.h"


//
// Time units
//

#define TIME_UNIT_MINUTE   0
#define TIME_UNIT_HOUR     1
#define TIME_UNIT_DAY      2

#define MAX_TIME_UNITS     3


//
// Graph view initialization structure
//

struct GRAPH_VIEW_INIT
{
   DWORD dwNodeId;
   DWORD dwNumItems;
   DWORD pdwItemList[MAX_GRAPH_ITEMS];
};


/////////////////////////////////////////////////////////////////////////////
// CGraphView window

class CGraphView : public CDynamicView
{
// Construction
public:
	CGraphView();

// Attributes
public:

// Operations
public:
	virtual void InitView(void *pData);
	virtual QWORD GetFingerprint(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraphView)
	//}}AFX_VIRTUAL

protected:
   void Preset(int nTimeUnit, DWORD dwNumUnits);

// Implementation
public:
	virtual ~CGraphView();

	// Generated message map functions
protected:
   DWORD m_dwRefreshInterval;
	DWORD m_dwTimeFrame;
	DWORD m_dwNumTimeUnits;
	int m_iTimeUnit;
	int m_iTimeFrameType;
	DWORD m_dwSeconds;
	CGraph m_wndGraph;
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	DWORD m_dwNodeId;
	DWORD m_pdwItemList[MAX_GRAPH_ITEMS];
	DWORD m_dwNumItems;
	DWORD m_dwSum2;
	DWORD m_dwSum1;
	//{{AFX_MSG(CGraphView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	afx_msg void OnGraphPresetsLast10minutes();
	afx_msg void OnGraphPresetsLast30minutes();
	afx_msg void OnGraphPresetsLasthour();
	afx_msg void OnGraphPresetsLast2hours();
	afx_msg void OnGraphPresetsLast4hours();
	afx_msg void OnGraphPresetsLastday();
	afx_msg void OnGraphPresetsLastweek();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGraphFullscreen();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHVIEW_H__BD76B70C_E3AE_42A2_B317_0D07EC27140B__INCLUDED_)
