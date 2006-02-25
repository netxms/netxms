#if !defined(AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_)
#define AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_

#include "Graph.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GraphFrame.h : header file
//


//
// Time units
//

#define TIME_UNIT_MINUTE   0
#define TIME_UNIT_HOUR     1
#define TIME_UNIT_DAY      2


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
	void RestoreFromServer(TCHAR *pszParams);
	void SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo);
	void AddItem(DWORD dwNodeId, NXC_DCI *pItem);
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
	void Preset(int nTimeUnit, DWORD dwNumUnits);
	DWORD m_dwTimeFrame;
	DWORD m_dwNumTimeUnits;
	int m_iTimeUnit;
	int m_iTimeFrameType;
	TCHAR m_szSubTitle[256];
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
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnGraphPresetsLasthour();
	afx_msg void OnGraphPresetsLast2hours();
	afx_msg void OnGraphPresetsLast4hours();
	afx_msg void OnGraphPresetsLastday();
	afx_msg void OnGraphPresetsLastweek();
	afx_msg void OnGraphPresetsLast10minutes();
	afx_msg void OnGraphPresetsLast30minutes();
	afx_msg void OnGraphRuler();
	afx_msg void OnUpdateGraphRuler(CCmdUI* pCmdUI);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnGraphLegend();
	afx_msg void OnUpdateGraphLegend(CCmdUI* pCmdUI);
	afx_msg void OnGraphPresetsLastmonth();
	afx_msg void OnGraphPresetsLastyear();
	//}}AFX_MSG
   afx_msg LRESULT OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo);
   afx_msg void OnUpdateGraphPoint(DWORD dwTimeStamp, double *pdValue);
	DECLARE_MESSAGE_MAP()
private:
	int m_iStatusBarHeight;
	DWORD m_dwRefreshInterval;
	DWORD m_dwFlags;
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	//DWORD m_pdwItemId[MAX_GRAPH_ITEMS];
	//DWORD m_pdwNodeId[MAX_GRAPH_ITEMS];
	DWORD m_dwNumItems;
   DCIInfo *m_ppItems[MAX_GRAPH_ITEMS];
	CGraph m_wndGraph;

public:
   void SetSubTitle(TCHAR *pszText) { nx_strncpy(m_szSubTitle, pszText, 256); }
   TCHAR *GetSubTitle(void) { return m_szSubTitle; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHFRAME_H__A3700362_0913_4191_B681_0A21C0D4A1EF__INCLUDED_)
