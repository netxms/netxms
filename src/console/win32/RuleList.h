#if !defined(AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_)
#define AFX_RULELIST_H__DB5A19FC_7403_4763_8985_71746416FE15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleList.h : header file
//

#include "RuleHeader.h"

#define MAX_COLUMN_NAME    64

class RL_Cell
{
public:
   int m_iNumLines;
   char **m_pszTextList;
   HICON *m_phIconList;

   RL_Cell();
   ~RL_Cell();
};

class RL_Row
{
public:
   int m_iNumCells;
   int m_iHeight;
   RL_Cell **m_ppCellList;

   RL_Row(int iNumCells);
   ~RL_Row();

   void InsertCell(int iPos);
};

struct RL_COLUMN
{
   char m_szName[MAX_COLUMN_NAME];
   int m_iWidth;
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
	int InsertColumn(int iInsertBefore, char *pszText, int iWidth);
	int InsertRow(int iInsertBefore);
	BOOL Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, UINT nId);
	virtual ~CRuleList();

	// Generated message map functions
protected:
	CRuleHeader m_wndHeader;
	//{{AFX_MSG(CRuleList)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
   afx_msg void OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
   afx_msg void OnHeaderEndTrack(NMHEADER *pHdrInfo, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
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
