// RuleList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define RULE_HEADER_HEIGHT    30
#define EMPTY_ROW_HEIGHT      30


/////////////////////////////////////////////////////////////////////////////
// Supplementary classes


//
// RL_Row
//

RL_Row::RL_Row(int iNumCells)
{
   int i;

   m_iHeight = EMPTY_ROW_HEIGHT;
   m_iNumCells = iNumCells;
   m_ppCellList = (RL_Cell **)malloc(sizeof(RL_Cell *) * m_iNumCells);
   memset(m_ppCellList, 0, sizeof(RL_Cell *) * m_iNumCells);
   for(i = 0; i < m_iNumCells; i++)
      m_ppCellList[i] = new RL_Cell;
}

RL_Row::~RL_Row()
{
   int i;

   for(i = 0; i < m_iNumCells; i++)
      delete m_ppCellList[i];
   safe_free(m_ppCellList);
}

void RL_Row::InsertCell(int iPos)
{
   m_ppCellList = (RL_Cell **)realloc(m_ppCellList, sizeof(RL_Cell *) * (m_iNumCells + 1));
   if (iPos < m_iNumCells)
      memmove(&m_ppCellList[iPos + 1], &m_ppCellList[iPos], 
              sizeof(RL_Cell *) * (m_iNumCells - iPos));
   m_iNumCells++;
   m_ppCellList[iPos] = new RL_Cell;

}

//
// RL_Cell
//

RL_Cell::RL_Cell()
{
   m_iNumLines = 0;
   m_pszTextList = NULL;
   m_phIconList = NULL;
}

RL_Cell::~RL_Cell()
{
   int i;

   for(i = 0; i < m_iNumLines; i++)
      safe_free(m_pszTextList[i]);
    safe_free(m_pszTextList);
    safe_free(m_phIconList);
}

/////////////////////////////////////////////////////////////////////////////
// CRuleList

CRuleList::CRuleList()
{
   m_pColList = NULL;
   m_ppRowList = NULL;
   m_iNumColumns = 0;
   m_iNumRows = 0;
   m_iTotalWidth = 0;
   m_iTotalHeight = 0;
}

CRuleList::~CRuleList()
{
   safe_free(m_pColList);
   safe_free(m_ppRowList);
}


BEGIN_MESSAGE_MAP(CRuleList, CWnd)
	//{{AFX_MSG_MAP(CRuleList)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
   ON_NOTIFY(HDN_BEGINTRACK, ID_HEADER_CTRL, OnHeaderBeginTrack)
   ON_NOTIFY(HDN_TRACK, ID_HEADER_CTRL, OnHeaderTrack)
   ON_NOTIFY(HDN_ENDTRACK, ID_HEADER_CTRL, OnHeaderEndTrack)
END_MESSAGE_MAP()


//
// Simplified Create() method
//

BOOL CRuleList::Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, UINT nId)
{
   return CWnd::Create(NULL, "", dwStyle, rect, pwndParent, nId);
}


//
// Redefined PreCreateWindow()
//

BOOL CRuleList::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CRuleList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
	
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font for elements
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "Verdana");

   GetClientRect(&rect);
   rect.bottom = RULE_HEADER_HEIGHT;
   m_wndHeader.Create(WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | HDS_HORZ, rect, this, ID_HEADER_CTRL);
	return 0;
}


//
// WM_PAINT message handler
//

void CRuleList::OnPaint() 
{
   RECT rect, rcClient;
   int i, j;
   CFont *pOldFont;

	CPaintDC dc(this); // device context for painting

   // Setup DC
   pOldFont = dc.SelectObject(&m_fontNormal);
	
   // Calculate drawing rect
   GetClientRect(&rcClient);
   rcClient.top = RULE_HEADER_HEIGHT;

   // Draw frames
   for(i = 0, rect.bottom = rcClient.top; i < m_iNumRows; i++)
   {
      rect.top = rect.bottom;
      rect.bottom += m_ppRowList[i]->m_iHeight;
      for(j = 0, rect.right = rcClient.left; j < m_iNumColumns; j++)
      {
         rect.left = rect.right;
         rect.right += m_pColList[j].m_iWidth;
         dc.DrawEdge(&rect, BDR_SUNKENOUTER | BDR_SUNKENINNER, BF_RECT);
char szBuffer[256];
sprintf(szBuffer,"%d %d", i,j);
         dc.DrawText(szBuffer, strlen(szBuffer), &rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
      }
      rect.left = rect.right;
//      rect.right++;
      dc.DrawEdge(&rect, BDR_SUNKENOUTER | BDR_SUNKENINNER, BF_LEFT);
   }
   rect.left = rcClient.left;
   rect.top = rect.bottom;
   rect.bottom++;
   rect.right += 2;
   dc.DrawEdge(&rect, BDR_SUNKENOUTER | BDR_SUNKENINNER, BF_TOP);

   // Cleanup
   dc.SelectObject(pOldFont);
}


//
// Insert new column
//

int CRuleList::InsertColumn(int iInsertBefore, char *pszText, int iWidth)
{
   int i, iNewCol;
   HDITEM hdi;

   // Calculate position for new item
   iNewCol = (iInsertBefore >= m_iNumColumns) ? m_iNumColumns : 
               (iInsertBefore >= 0 ? iInsertBefore : 0);

   // Insert new item into internal column list
   m_pColList = (RL_COLUMN *)realloc(m_pColList, sizeof(RL_COLUMN) * (m_iNumColumns + 1));
   if (iNewCol < m_iNumColumns)
      memmove(&m_pColList[iNewCol + 1], &m_pColList[iNewCol], 
              sizeof(RL_COLUMN) * (m_iNumColumns - iNewCol));
   m_iNumColumns++;
   m_pColList[iNewCol].m_iWidth = iWidth;
   strncpy(m_pColList[iNewCol].m_szName, pszText, MAX_COLUMN_NAME);

   // Insert new item into header control
   hdi.mask = HDI_TEXT | HDI_WIDTH;
   hdi.pszText = pszText;
   hdi.cxy = iWidth;
   m_wndHeader.InsertItem(iNewCol, &hdi);

   // Update all existing rows
   for(i = 0; i < m_iNumRows; i++)
      m_ppRowList[i]->InsertCell(iNewCol);
   
   RecalcWidth();
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth + 2, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   return iNewCol;
}


//
// Insert new row
//

int CRuleList::InsertRow(int iInsertBefore)
{
   int iNewRow;

   // Calculate position for new item
   iNewRow = (iInsertBefore >= m_iNumRows) ? m_iNumRows : 
               (iInsertBefore >= 0 ? iInsertBefore : 0);

   // Insert new item into internal row list
   m_ppRowList = (RL_Row **)realloc(m_ppRowList, sizeof(RL_Row *) * (m_iNumRows + 1));
   if (iNewRow < m_iNumRows)
      memmove(&m_ppRowList[iNewRow + 1], &m_ppRowList[iNewRow], 
              sizeof(RL_Row *) * (m_iNumRows - iNewRow));
   m_iNumRows++;
   m_ppRowList[iNewRow] = new RL_Row(m_iNumColumns);

   RecalcHeight();

   return iNewRow;
}


//
// Recalculate control's width
//

void CRuleList::RecalcWidth()
{
   int i;

   m_iTotalWidth = 0;
   for(i = 0; i < m_iNumColumns; i++)
      m_iTotalWidth += m_pColList[i].m_iWidth;
}


//
// Recalculate control's height
//

void CRuleList::RecalcHeight()
{
   int i;

   m_iTotalHeight = 0;
   for(i = 0; i < m_iNumRows; i++)
      m_iTotalHeight += m_ppRowList[i]->m_iHeight;
}


//
// WM_SIZE message handler
//

void CRuleList::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth + 2, RULE_HEADER_HEIGHT, SWP_NOZORDER);
}


//
// Process HDN_BEGINTRACK notification from header control
//

void CRuleList::OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   theApp.DebugPrintf("HDN_BEGINTRACK %d %d", pHdrInfo->iItem, pHdrInfo->pitem->cxy);
   *pResult = 0;
}


//
// Process HDN_TRACK notification from header control
//

void CRuleList::OnHeaderTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   int x, sx;

   // Draw line at old and new column width
   sx = GetColumnStartPos(pHdrInfo->iItem);
   x = sx + m_pColList[pHdrInfo->iItem].m_iWidth;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);
   x = sx + pHdrInfo->pitem->cxy;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);

   // New column width
   m_pColList[pHdrInfo->iItem].m_iWidth = pHdrInfo->pitem->cxy;

   *pResult = 0;
}


//
// Process HDN_ENDTRACK notification from header control
//

void CRuleList::OnHeaderEndTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   RecalcWidth();
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth + 2, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   InvalidateRect(NULL);
}


//
// Get start X position for column
//

int CRuleList::GetColumnStartPos(int iColumn)
{
   int i, x;

   for(i = 0, x = 0; i < iColumn; i++)
      x += m_pColList[i].m_iWidth;
   return x;
}


//
// Draw shadow line
//

void CRuleList::DrawShadowLine(int x1, int y1, int x2, int y2)
{
   CDC *pDC;
   int iDrawMode;

   pDC = GetDC();
   iDrawMode = pDC->SetROP2(R2_NOT);
   pDC->MoveTo(x1, y1);
   pDC->LineTo(x2, y2);
   pDC->SetROP2(iDrawMode);
   ReleaseDC(pDC);
}
