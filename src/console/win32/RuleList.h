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
#define CF_NON_SELECTABLE  0x0004
#define CF_TEXTBOX         0x0008


//
// Row flags
//

#define RLF_SELECTED       0x0001
#define RLF_DISABLED       0x0002


//
// Supplementary classes
//

struct RL_COLUMN
{
   TCHAR m_szName[MAX_COLUMN_NAME];
   int m_iWidth;
   DWORD m_dwFlags;
};

class RL_Cell
{
public:
   int m_iNumLines;
   TCHAR **m_pszTextList;
   int *m_piImageList;
   BYTE *m_pSelectFlags;
   BOOL m_bHasImages;
   BOOL m_bSelectable;
   BOOL m_bNegate;
   TCHAR *m_pszText;    // Text for CF_TEXTBOX cells

   RL_Cell();
   ~RL_Cell();

   void Recalc(void);
   int CalculateHeight(int iTextHeight, int iWidth, BOOL bIsTextBox, CFont *pFont);
   int AddLine(TCHAR *pszText, int iImage = -1);
   BOOL ReplaceLine(int iLine, TCHAR *pszText, int iImage = -1);
   BOOL DeleteLine(int iLine);
   void Clear(void);
   void ClearSelection(void);
   void SetText(TCHAR *pszText);
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
   void RecalcHeight(int iTextHeight, RL_COLUMN *pColInfo, CFont *pFont);
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
	public:
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ClearSelection(BOOL bRedraw = TRUE);
	COLORREF m_rgbActiveBkColor;
	int AddItem(int iRow, int iColumn, TCHAR *pszText, int iImage = -1);
	int InsertColumn(int iInsertBefore, TCHAR *pszText, int iWidth, DWORD dwFlags = 0);
	int InsertRow(int iInsertBefore);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, UINT nId);
	virtual ~CRuleList();

	// Generated message map functions
protected:
	int m_nLastSelectedRow;
	void InvalidateRow(int iRow);
	void InvalidateList(void);
	void OnMouseButtonDown(UINT nFlags, CPoint point);
	void OnScroll(BOOL bRedrawHeader);
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
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
   afx_msg void OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderEndTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	COLORREF m_rgbEmptyBkColor;
	int m_iMouseWheelDelta;
	COLORREF m_rgbSelectedBkColor;
	COLORREF m_rgbSelectedTextColor;
	COLORREF m_rgbDisabledBkColor;
	COLORREF m_rgbCrossColor;
	int m_iCurrItem;
	int m_iCurrColumn;
	int m_iCurrRow;
	CImageList *m_pImageList;
   CImageList m_imgListInternal;
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
	int GetRowStartPos(int nRow);
	void RecalcWidth(void);
	void RecalcHeight(void);
	int m_iNumRows;
   int m_iNumColumns;
   int m_iTotalHeight;
   int m_iTotalWidth;

   RL_COLUMN *m_pColList;
   RL_Row **m_ppRowList;

public:
	void RestoreColumns(TCHAR *pszSection, TCHAR *pszPrefix);
	void SaveColumns(TCHAR *pszSection, TCHAR *pszPrefix);
	int GetSelectionCount(void);
	void SetNegationFlag(int nRow, int nCol, BOOL bNegate);
	void SetCellText(int iRow, int iColumn, TCHAR *pszText);
	BOOL DeleteRow(int iRow);
	BOOL EnableCellSelection(int iRow, int iCell, BOOL bEnable);
	void DeleteItem(int iRow, int iCell, int iItem);
	int RowFromPoint(int x, int y, int *piRowStart = NULL);
	int ColumnFromPoint(int x, int y, int *piColStart = NULL);
	int ItemFromPoint(int x, int y);
	void ClearCell(int iRow, int iCell);
	int GetNumItems(int iRow, int iCell);
	void EnableRow(int iRow, BOOL bEnable = TRUE);
	int GetNextRow(int iStartAfter, DWORD dwFlags);
	void ReplaceItem(int iRow, int iColumn, int iItem, TCHAR *pszText, int iImage = -1);
	void SetImageList(CImageList *pImageList) { m_pImageList = pImageList; }
   int GetNumRows(void) { return m_iNumRows; }
   int GetCurrentRow(void) { return m_iCurrRow; }
   int GetCurrentColumn(void) { return m_iCurrColumn; }
   int GetCurrentItem(void) { return m_iCurrItem; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_)
