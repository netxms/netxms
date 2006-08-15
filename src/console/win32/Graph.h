#if !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
#define AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#include "globals.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Graph.h : header file
//

#define MAX_GRAPH_ITEMS    16
#define ZOOM_HISTORY_SIZE  16

struct ZOOM_INFO
{
   DWORD dwTimeFrom;
   DWORD dwTimeTo;
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
	COLORREF m_rgbLabelTextColor;
	COLORREF m_rgbLabelBkColor;
	COLORREF m_rgbLineColors[MAX_GRAPH_ITEMS];
	COLORREF m_rgbGridColor;
	COLORREF m_rgbBkColor;
	COLORREF m_rgbAxisColor;
	COLORREF m_rgbTextColor;
	COLORREF m_rgbSelRectColor;

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
	void Update(void);
	BOOL m_bShowRuler;
	void SetData(DWORD dwIndex, NXC_DCI_DATA *pData);
	void SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, int nId);
	virtual ~CGraph();

	// Generated message map functions
protected:
   ZOOM_INFO m_zoomInfo[ZOOM_HISTORY_SIZE];
   int m_nZoomLevel;
	POINT m_ptMouseOpStart;
	int m_nState;
	RECT m_rcSelection;
	int NextMonthOffset(DWORD dwTimeStamp);
	BOOL m_bIsActive;
	void DrawGraphOnBitmap(void);
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
private:
	DCIInfo **m_ppItems;
	RECT m_rectGraph;
	double m_dCurrMaxValue;
	double m_dSecondsPerPixel;
	void DrawLineGraph(CDC &dc, NXC_DCI_DATA *pData, COLORREF rgbColor, int nGridSize);
	DWORD m_dwTimeTo;
	DWORD m_dwTimeFrom;
	DWORD m_dwNumItems;
	double m_dMaxValue;
	NXC_DCI_DATA *m_pData[MAX_GRAPH_ITEMS];
   int m_nLastGridSizeY;

public:
	BOOL m_bEnableZoom;
	void ClearZoomHistory(void);
	BOOL CanZoomOut(void);
	BOOL CanZoomIn(void);
	void ZoomOut(void);
	void ZoomIn(RECT &rect);
   void SetDCIInfo(DCIInfo **ppInfo) { m_ppItems = ppInfo; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPH_H__021ACC81_A267_4464_889B_5718D2985D19__INCLUDED_)
