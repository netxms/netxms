#if !defined(AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_)
#define AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleList.h : header file
//

#include "RuleHeader.h"

#define MAX_COLUMN_NAME    64


//
// Column flags
//

#define CF_CENTER          0x0001
#define CF_TITLE_COLOR     0x0002


//
// Row flags
//

#define RF_SELECTED        0x0001
#define RF_DISABLED        0x0002


//
// Supplementary classes
//

class RL_Cell
{
public:
   int m_iNumLines;
   char **m_pszTextList;
   HICON *m_phIconList;

   RL_Cell();
   ~RL_Cell();

   int AddLine(char *pszText, HICON hIcon);
};

class RL_Row
{
public:
   int m_iNumCells;
   int m_iHeight;
   RL_Cell **m_ppCellList;
   DWORD m_dwFlags;

   RL_Row(int iNumCells);
   ~RL_Row();

   void InsertCell(int iPos);
   void RecalcHeight(int iTextHeight);
};

struct RL_COLUMN
{
   char m_szName[MAX_COLUMN_NAME];
   int m_iWidth;
   DWORD m_dwFlags;
};

/////////////////////////////////////////////////////////////////////////////
// CRuleList window

class CRuleList : public CWnd
{
// Construction
public:
	CRuleList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleList)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ClearSelection(BOOL bRedraw = TRUE);
	int RowFromPoint(int x, int y);
	COLORREF m_rgbActiveBkColor;
	int AddItem(int iRow, int iColumn, char *pszText, HICON hIcon = NULL);
	int InsertColumn(int iInsertBefore, char *pszText, int iWidth, DWORD dwFlags = 0);
	int InsertRow(int iInsertBefore);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, UINT nId);
	virtual ~CRuleList();

	// Generated message map functions
protected:
	void OnScroll(void);
	int CalculateNewScrollPos(UINT nScrollBar, UINT nSBCode, UINT nPos);
	void UpdateScrollBars(void);
	CRuleHeader m_wndHeader;
	//{{AFX_MSG(CRuleList)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
   afx_msg void OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderEndTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	int m_iYOrg;
	int m_iXOrg;
	int m_iEdgeCX;
   int m_iEdgeCY;
	BOOL m_bVScroll;
	COLORREF m_rgbTextColor;
	COLORREF m_rgbTitleTextColor;
	COLORREF m_rgbNormalBkColor;
	COLORREF m_rgbTitleBkColor;
	int m_iTextHeight;
	CFont m_fontNormal;
	void DrawShadowLine(int x1, int y1, int x2, int y2);
	int GetColumnStartPos(int iColumn);
	void RecalcWidth(void);
	void RecalcHeight(void);
	int m_iNumRows;
   int m_iNumColumns;
   int m_iTotalHeight;
   int m_iTotalWidth;

   RL_COLUMN *m_pColList;
   RL_Row **m_ppRowList;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_)
