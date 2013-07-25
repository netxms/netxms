#if !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
#define AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_

#include "globals.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Graph.h : header file
//

#include <math.h>


#define MAX_GRAPH_ITEMS    16
#define ZOOM_HISTORY_SIZE  16

#define GRAPH_TYPE_LINE		0
#define GRAPH_TYPE_AREA		1
#define GRAPH_TYPE_STACKED	2

#define GCS_CLASSIC			0
#define GCS_LIGHT				1

#define DEFAULT_LINE_WIDTH 2

struct ZOOM_INFO
{
   DWORD dwTimeFrom;
   DWORD dwTimeTo;
};

struct GRAPH_ITEM_STYLE
{
	int nType;	// Line. area, or stacked
	COLORREF rgbColor;
	int nLineWidth;
	BOOL bShowAverage;
	BOOL bShowThresholds;
	BOOL bShowTrend;
};


/////////////////////////////////////////////////////////////////////////////
// CGraph window

class CGraph : public CWnd
{
// Construction
public:
	CGraph();

// Attributes
public:
	BOOL m_bShowGrid;
	BOOL m_bAutoScale;
	BOOL m_bShowLegend;
	COLORREF m_rgbGridColor;
	COLORREF m_rgbBkColor;
	COLORREF m_rgbAxisColor;
	COLORREF m_rgbTextColor;
	COLORREF m_rgbSelRectColor;
	COLORREF m_rgbRulerColor;
	GRAPH_ITEM_STYLE m_graphItemStyles[MAX_GRAPH_ITEMS];

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGraph)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void DrawGraphOnBitmap(CBitmap &bmpGraph, RECT &rect);
	void Update();
	BOOL m_bShowRuler;
	void SetData(DWORD dwIndex, NXC_DCI_DATA *pData);
	void SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo);
	void SetThresholds(DWORD index, DWORD numThresholds, NXC_DCI_THRESHOLD *thresholds);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, int nId);
	virtual ~CGraph();

	// Generated message map functions
protected:
	int m_nZeroLine;
	TCHAR m_szTitle[MAX_DB_STRING];
   ZOOM_INFO m_zoomInfo[ZOOM_HISTORY_SIZE];
   int m_nZoomLevel;
	POINT m_ptMouseOpStart;
	int m_nState;
	RECT m_rcSelection;
	int NextMonthOffset(DWORD dwTimeStamp);
	BOOL m_bIsActive;
	CBitmap m_bmpGraph;
	CPoint m_ptCurrMousePos;
	//{{AFX_MSG(CGraph)
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	double ROW_DATA(NXC_DCI_ROW *row, int dt);

private:
	DCIInfo **m_ppItems;
	RECT m_rectGraph;
	double m_dSecondsPerPixel;
	void DrawLineGraph(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize);
	void DrawAreaGraph(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize);
	void DrawAverage(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize);
	void DrawTrendLine(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize);
	void DrawThresholds(CDC &dc, NXC_DCI_THRESHOLD *thresholds, DWORD numThresholds, GRAPH_ITEM_STYLE *style, int nGridSize);
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	DWORD m_dwNumItems;
	double m_dMaxValue;
	double m_dCurrMaxValue;
	double m_dMinValue;
	double m_dCurrMinValue;
	NXC_DCI_DATA *m_pData[MAX_GRAPH_ITEMS];
	NXC_DCI_THRESHOLD *m_thresholds[MAX_GRAPH_ITEMS];
	DWORD m_numThresholds[MAX_GRAPH_ITEMS];
   int m_nLastGridSizeY;

public:
	BOOL m_bLogarithmicScale;
	BOOL m_bShowHostNames;
	BOOL m_bUpdating;
	BOOL m_bShowTitle;
	BOOL m_bSet3DEdge;
	void SetColorScheme(int nScheme);
	void UpdateData(DWORD dwIndex, NXC_DCI_DATA *pData);
	BOOL GetTimeFrameForUpdate(DWORD dwIndex, DWORD *pdwFrom, DWORD *pdwTo);
	BOOL m_bEnableZoom;
	void ClearZoomHistory(void);
	BOOL CanZoomOut(void);
	BOOL CanZoomIn(void);
	void ZoomOut(void);
	void ZoomIn(RECT &rect);
   void SetDCIInfo(DCIInfo **ppInfo) { m_ppItems = ppInfo; }
   CBitmap *GetBitmap(void) { return &m_bmpGraph; }
	void SetTitle(const TCHAR *pszTitle) { nx_strncpy(m_szTitle, pszTitle, MAX_DB_STRING); }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

inline double CGraph::ROW_DATA(NXC_DCI_ROW *row, int dt)
{
	double value = ((dt == DCI_DT_STRING) ? _tcstod(row->value.szString, NULL) :
	                ((dt == DCI_DT_INT) ? *((LONG *)(&row->value.dwInt32)) :
	                 ((dt == DCI_DT_UINT) ? row->value.dwInt32 :
	                  (((dt == DCI_DT_INT64) || (dt == DCI_DT_UINT64)) ? (INT64)row->value.ext.v64.qwInt64 :
	                   ((dt == DCI_DT_FLOAT) ? row->value.ext.v64.dFloat : 0)
	                  )
	                 )
	                )
	               );
	return m_bLogarithmicScale ? log10(value) : value;
}


#endif // !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
