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
#define EMPTY_ROW_HEIGHT      20
#define CELL_TEXT_Y_MARGIN    5
#define CELL_TEXT_Y_SPACING   4
#define CELL_TEXT_X_MARGIN    8
#define SCROLL_STEP           30
#define ITEM_IMAGE_SIZE       16


//
// Custom scrolling codes
//

#define SB_WHEELUP            'MWUP'
#define SB_WHEELDOWN          'MWDN'


/////////////////////////////////////////////////////////////////////////////
// Supplementary classes


//
// RL_Row
//

RL_Row::RL_Row(int iNumCells)
{
   int i;

   m_dwFlags = 0;
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

void RL_Row::RecalcHeight(int iTextHeight, RL_COLUMN *pColInfo, CFont *pFont)
{
   int i, iCellHeight;

   for(i = 0, m_iHeight = 0; i < m_iNumCells; i++)
   {
      iCellHeight = m_ppCellList[i]->CalculateHeight(iTextHeight, pColInfo[i].m_iWidth, 
                                                     pColInfo[i].m_dwFlags & CF_TEXTBOX,
                                                     pFont);
      if (iCellHeight > m_iHeight)
         m_iHeight = iCellHeight;
   }
}

//
// RL_Cell
//

RL_Cell::RL_Cell()
{
   m_iNumLines = 0;
   m_pszTextList = NULL;
   m_piImageList = NULL;
   m_pSelectFlags = NULL;
   m_bHasImages = FALSE;
   m_bSelectable = TRUE;
   m_pszText = NULL;
}

RL_Cell::~RL_Cell()
{
   int i;

   for(i = 0; i < m_iNumLines; i++)
      safe_free(m_pszTextList[i]);
   safe_free(m_pszTextList);
   safe_free(m_piImageList);
   safe_free(m_pSelectFlags);
   safe_free(m_pszText);
}

void RL_Cell::Recalc(void)
{
   int i;

   // Check if cell has at least one image
   for(i = 0, m_bHasImages = FALSE; i < m_iNumLines; i++)
      if (m_piImageList[i] != -1)
      {
         m_bHasImages = TRUE;
         break;
      }
}

int RL_Cell::AddLine(char *pszText, int iImage)
{
   int iPos;

   iPos = m_iNumLines++;
   m_pszTextList = (char **)realloc(m_pszTextList, sizeof(char *) * m_iNumLines);
   m_piImageList = (int *)realloc(m_piImageList, sizeof(int) * m_iNumLines);
   m_pSelectFlags = (BYTE *)realloc(m_pSelectFlags, sizeof(BYTE) * m_iNumLines);
   m_pszTextList[iPos] = strdup(pszText);
   m_piImageList[iPos] = iImage;
   m_pSelectFlags[iPos] = 0;
   Recalc();
   return iPos;
}

BOOL RL_Cell::ReplaceLine(int iLine, char *pszText, int iImage)
{
   if ((iLine < 0) || (iLine >= m_iNumLines))
      return FALSE;  // Invalid line number, no changes

   safe_free(m_pszTextList[iLine]);
   m_pszTextList[iLine] = strdup(pszText);
   m_piImageList[iLine] = iImage;
   Recalc();
   return TRUE;
}

void RL_Cell::Clear(void)
{
   int i;

   for(i = 0; i < m_iNumLines; i++)
      safe_free(m_pszTextList[i]);
   safe_free(m_pszTextList);
   safe_free(m_piImageList);
   safe_free(m_pSelectFlags);
   m_iNumLines = 0;
   m_pszTextList = NULL;
   m_piImageList = NULL;
   m_pSelectFlags = NULL;
   m_bHasImages = FALSE;
}

void RL_Cell::ClearSelection(void)
{
   memset(m_pSelectFlags, 0, sizeof(BYTE) * m_iNumLines);
}

BOOL RL_Cell::DeleteLine(int iLine)
{
   if ((iLine < 0) || (iLine >= m_iNumLines))
      return FALSE;  // Invalid line number, no changes

   safe_free(m_pszTextList[iLine]);
   m_iNumLines--;
   memmove(&m_pszTextList[iLine], &m_pszTextList[iLine + 1], sizeof(char *) * (m_iNumLines - iLine));
   memmove(&m_piImageList[iLine], &m_piImageList[iLine + 1], sizeof(int) * (m_iNumLines - iLine));
   memmove(&m_pSelectFlags[iLine], &m_pSelectFlags[iLine + 1], sizeof(BYTE) * (m_iNumLines - iLine));
   Recalc();
   return TRUE;
}

void RL_Cell::SetText(char *pszText)
{
   int iLen;

   ASSERT(pszText != NULL);
   safe_free(m_pszText);
   iLen = strlen(pszText);
   m_pszText = (char *)malloc(iLen + 3);
   strcpy(m_pszText, pszText);

   // Text should end with CR/LF pair for proper cell size calculation
   if (iLen < 2)
   {
      strcpy(&m_pszText[iLen], "\r\n");
   }
   else
   {
      if (memcmp(&m_pszText[iLen - 2], "\r\n", 2))
         strcpy(&m_pszText[iLen], "\r\n");
   }
}

int RL_Cell::CalculateHeight(int iTextHeight, int iWidth, BOOL bIsTextBox, CFont *pFont)
{
   int iHeight;

   if (bIsTextBox)
   {
      RECT rect;
      CDC *pDC;
      CFont *pOldFont;

      rect.left = CELL_TEXT_Y_MARGIN;
      rect.right = iWidth - CELL_TEXT_Y_MARGIN;
      rect.top = 0;
      rect.bottom = 0;

      pDC = theApp.m_pMainWnd->GetDC();
      pOldFont = pDC->SelectObject(pFont);
      pDC->DrawText(m_pszText, strlen(m_pszText), &rect, DT_WORDBREAK | DT_CALCRECT);
      pDC->SelectObject(pOldFont);
      theApp.m_pMainWnd->ReleaseDC(pDC);
      iHeight = rect.bottom;
   }
   else
   {
      iHeight = (m_iNumLines == 0) ? EMPTY_ROW_HEIGHT :
                 (CELL_TEXT_Y_MARGIN * 2 + m_iNumLines * iTextHeight +
                    (m_iNumLines - 1) * CELL_TEXT_Y_SPACING);
   }
   return iHeight;
}

/////////////////////////////////////////////////////////////////////////////
// CRuleList

CRuleList::CRuleList()
{
   m_pImageList = NULL;

   m_pColList = NULL;
   m_ppRowList = NULL;
   m_iNumColumns = 0;
   m_iNumRows = 0;
   m_iTotalWidth = 0;
   m_iTotalHeight = 0;
   m_iMouseWheelDelta = 0;

   m_iEdgeCX = GetSystemMetrics(SM_CXEDGE);
   m_iEdgeCY = GetSystemMetrics(SM_CYEDGE);

   m_iXOrg = 0;
   m_iYOrg = 0;

   m_iCurrRow = -1;
   m_iCurrColumn = -1;
   m_iCurrItem = -1;

   // Default colors
   m_rgbActiveBkColor = RGB(255, 255, 255);
   m_rgbNormalBkColor = RGB(206, 206, 206);
   m_rgbTextColor = RGB(0, 0, 0);
   m_rgbTitleBkColor = RGB(0, 115, 230);
   m_rgbTitleTextColor = RGB(255, 255, 255);
   m_rgbCrossColor = RGB(255, 0, 0);
   m_rgbDisabledBkColor = RGB(128, 128, 255);
   m_rgbSelectedTextColor = RGB(255, 255, 255);
   m_rgbSelectedBkColor = RGB(0, 0, 128);
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
	ON_WM_LBUTTONDOWN()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
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
   cs.style |= WS_HSCROLL | WS_VSCROLL;
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CRuleList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CDC *pDC;
	
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font for elements
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "Verdana");
   pDC = GetDC();
   m_iTextHeight = max(pDC->GetTextExtent("gqhXQ|", 6).cy, ITEM_IMAGE_SIZE);
   ReleaseDC(pDC);

   // Create header
   GetClientRect(&rect);
   rect.bottom = RULE_HEADER_HEIGHT;
   m_wndHeader.SetColors(m_rgbTitleTextColor, m_rgbTitleBkColor);
   m_wndHeader.Create(WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | HDS_HORZ, rect, this, ID_HEADER_CTRL);

   // Disable scroll bars
   SetScrollRange(SB_HORZ, 0, 0);
   SetScrollRange(SB_VERT, 0, 0);

   return 0;
}


//
// WM_PAINT message handler
//

void CRuleList::OnPaint() 
{
   RECT rect, rcClient, rcText;
   int i, j, iLine;
   CFont *pOldFont;
   CBrush brTitle, brActive, brNormal, brDisabled, *pOldBrush;
   CPen pen, *pOldPen;
   COLORREF rgbTextColor, rgbBkColor;

	CPaintDC dc(this); // device context for painting

   // Create brushes and pens
   brTitle.CreateSolidBrush(m_rgbTitleBkColor);
   brActive.CreateSolidBrush(m_rgbActiveBkColor);
   brNormal.CreateSolidBrush(m_rgbNormalBkColor);
   brDisabled.CreateSolidBrush(m_rgbDisabledBkColor);
   pen.CreatePen(PS_SOLID, 2, m_rgbCrossColor);

   // Setup DC
   dc.SetWindowOrg(m_iXOrg, m_iYOrg);
   pOldFont = dc.SelectObject(&m_fontNormal);
   pOldBrush = dc.GetCurrentBrush();
	
   // Calculate drawing rect
   GetClientRect(&rcClient);
   rcClient.top = RULE_HEADER_HEIGHT;

   // Draw table
   for(i = 0, rect.bottom = rcClient.top; i < m_iNumRows; i++)
   {
      rect.top = rect.bottom;
      rect.bottom += m_ppRowList[i]->m_iHeight;
      if (rect.top > rcClient.bottom + m_iYOrg)
         break;   // This and following rows are not visible, stop painting
      if (rect.bottom - m_iYOrg > RULE_HEADER_HEIGHT)
      {
         // Draw only visible rows
         for(j = 0, rect.right = rcClient.left; j < m_iNumColumns; j++)
         {
            // Draw background and bounding rectangle for the cell
            rect.left = rect.right;
            rect.right += m_pColList[j].m_iWidth;
            dc.SelectObject((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? &brTitle : 
                              ((m_ppRowList[i]->m_dwFlags & RLF_SELECTED) ? &brActive : 
                                 ((m_ppRowList[i]->m_dwFlags & RLF_DISABLED) ? &brDisabled : 
                                    &brNormal)));
            dc.Rectangle(&rect);
            dc.Draw3dRect(&rect, RGB(128, 128, 128), RGB(255, 255, 255));

            // Prepare for drawing cell text
            memcpy(&rcText, &rect, sizeof(RECT));
            dc.SetTextColor((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? m_rgbTitleTextColor : 
                              m_rgbTextColor);
            dc.SetBkColor((m_pColList[j].m_dwFlags & CF_TITLE_COLOR) ? m_rgbTitleBkColor :
                            ((m_ppRowList[i]->m_dwFlags & RLF_SELECTED) ? m_rgbActiveBkColor : 
                               ((m_ppRowList[i]->m_dwFlags & RLF_DISABLED) ? m_rgbDisabledBkColor : 
                                  m_rgbNormalBkColor)));

            // Different drawing depend on CF_TEXTBOX flag
            if (m_pColList[j].m_dwFlags & CF_TEXTBOX)
            {
               // This cell has single multiline text item without image
               memcpy(&rcText, &rect, sizeof(RECT));
               rcText.top += CELL_TEXT_Y_MARGIN;
               rcText.bottom -= CELL_TEXT_Y_MARGIN;
               rcText.left += CELL_TEXT_X_MARGIN;
               rcText.right -= CELL_TEXT_X_MARGIN;

               dc.DrawText(m_ppRowList[i]->m_ppCellList[j]->m_pszText,
                           strlen(m_ppRowList[i]->m_ppCellList[j]->m_pszText),
                           &rcText, DT_WORDBREAK | (m_pColList[j].m_dwFlags & CF_CENTER ? DT_CENTER : DT_LEFT));
            }
            else  // Multiline cell
            {
               int iCellHeight;

               iCellHeight = (m_ppRowList[i]->m_ppCellList[j]->m_iNumLines == 0) ? EMPTY_ROW_HEIGHT :
                              (CELL_TEXT_Y_MARGIN * 2 + m_ppRowList[i]->m_ppCellList[j]->m_iNumLines * m_iTextHeight +
                                 (m_ppRowList[i]->m_ppCellList[j]->m_iNumLines - 1) * CELL_TEXT_Y_SPACING);
            
               rcText.top += CELL_TEXT_Y_MARGIN + ((rect.bottom - rect.top) - iCellHeight) / 2;
               rcText.bottom = rcText.top + m_iTextHeight;
               rcText.left += CELL_TEXT_X_MARGIN;
               if (m_ppRowList[i]->m_ppCellList[j]->m_bHasImages)
                  rcText.left += ITEM_IMAGE_SIZE + 3;
               rcText.right -= CELL_TEXT_X_MARGIN;

               // Walk through cell's items
               for(iLine = 0; iLine < m_ppRowList[i]->m_ppCellList[j]->m_iNumLines; iLine++)
               {
                  // Change text colors if current item is selected
                  if (m_ppRowList[i]->m_ppCellList[j]->m_pSelectFlags[iLine])
                  {
                     rgbTextColor = dc.SetTextColor(m_rgbSelectedTextColor);
                     rgbBkColor = dc.SetBkColor(m_rgbSelectedBkColor);
                  }

                  if (m_ppRowList[i]->m_ppCellList[j]->m_piImageList[iLine] != -1)
                     m_pImageList->Draw(&dc, m_ppRowList[i]->m_ppCellList[j]->m_piImageList[iLine],
                                        CPoint(rcText.left - ITEM_IMAGE_SIZE, rcText.top), ILD_TRANSPARENT);
                  dc.DrawText(m_ppRowList[i]->m_ppCellList[j]->m_pszTextList[iLine],
                              strlen(m_ppRowList[i]->m_ppCellList[j]->m_pszTextList[iLine]),
                              &rcText, DT_SINGLELINE | DT_VCENTER | 
                                          (m_pColList[j].m_dwFlags & CF_CENTER ? DT_CENTER : DT_LEFT));
               
                  // Restore colors
                  if (m_ppRowList[i]->m_ppCellList[j]->m_pSelectFlags[iLine])
                  {
                     dc.SetTextColor(rgbTextColor);
                     dc.SetBkColor(rgbBkColor);
                  }

                  rcText.top = rcText.bottom + CELL_TEXT_Y_SPACING;
                  rcText.bottom = rcText.top + m_iTextHeight;
               }
            }

            if ((m_ppRowList[i]->m_dwFlags & RLF_DISABLED) && 
                (m_pColList[j].m_dwFlags & CF_TITLE_COLOR))
            {
               pOldPen = dc.SelectObject(&pen);
               dc.MoveTo(rect.left + 2, rect.top + 2);
               dc.LineTo(rect.right - 2, rect.bottom - 2);
               dc.MoveTo(rect.right - 2, rect.top + 2);
               dc.LineTo(rect.left + 2, rect.bottom - 2);
               dc.SelectObject(pOldPen);
            }
         }
      }
   }

   // Draw frame around the table
   rect.left = rcClient.left;
   rect.top = 0;
   rect.bottom++;
   rect.right = m_iTotalWidth + 2;
   dc.DrawEdge(&rect, EDGE_RAISED, BF_BOTTOMRIGHT);

   // Cleanup
   dc.SelectObject(pOldFont);
   dc.SelectObject(pOldBrush);
}


//
// Insert new column
//

int CRuleList::InsertColumn(int iInsertBefore, char *pszText, int iWidth, DWORD dwFlags)
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
   m_pColList[iNewCol].m_dwFlags = dwFlags;

   // Insert new item into header control
   hdi.mask = HDI_TEXT | HDI_WIDTH;
   hdi.pszText = pszText;
   hdi.cxy = iWidth;
   m_wndHeader.InsertItem(iNewCol, &hdi);

   // Update all existing rows
   for(i = 0; i < m_iNumRows; i++)
      m_ppRowList[i]->InsertCell(iNewCol);
   
   RecalcWidth();
   UpdateScrollBars();
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   return iNewCol;
}


//
// Insert new row
//

int CRuleList::InsertRow(int iInsertBefore)
{
   int i, iNewRow;

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

   // Do necssary changes on new cells
   for(i = 0; i < m_iNumColumns; i++)
   {
      // Set non-selectable flag on cells
      if (m_pColList[i].m_dwFlags & CF_NON_SELECTABLE)
         m_ppRowList[iNewRow]->m_ppCellList[i]->m_bSelectable = FALSE;

      // Set empty text for text box columns
      if (m_pColList[i].m_dwFlags & CF_TEXTBOX)
         m_ppRowList[iNewRow]->m_ppCellList[i]->SetText("");
   }

   RecalcHeight();
   UpdateScrollBars();

   return iNewRow;
}


//
// Add new item to cell
//

int CRuleList::AddItem(int iRow, int iColumn, char *pszText, int iImage)
{
   int iPos;

   if ((iRow < 0) || (iRow >= m_iNumRows) || (iColumn < 0) || (iColumn >= m_iNumColumns))
      return -1;

   if (m_pColList[iColumn].m_dwFlags & CF_TEXTBOX)
      return -1;  // SetCellText() should be used for text box columns

   iPos = m_ppRowList[iRow]->m_ppCellList[iColumn]->AddLine(pszText, iImage);
   m_ppRowList[iRow]->RecalcHeight(m_iTextHeight, m_pColList, &m_fontNormal);
   RecalcHeight();
   UpdateScrollBars();
   InvalidateList();
   return iPos;
}


//
// Replace item in a cell
//

void CRuleList::ReplaceItem(int iRow, int iColumn, int iItem, char *pszText, int iImage)
{
   if ((iRow < 0) || (iRow >= m_iNumRows) || (iColumn < 0) || (iColumn >= m_iNumColumns))
      return;
   if (m_pColList[iColumn].m_dwFlags & CF_TEXTBOX)
      return;  // SetCellText() should be used for text box columns
   if (m_ppRowList[iRow]->m_ppCellList[iColumn]->ReplaceLine(iItem, pszText, iImage))
      InvalidateRow(iRow);
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
// Process HDN_BEGINTRACK notification from header control
//

void CRuleList::OnHeaderBeginTrack(NMHEADER *pHdrInfo, LRESULT *pResult)
{
   int x;

   x = GetColumnStartPos(pHdrInfo->iItem) + pHdrInfo->pitem->cxy;
   DrawShadowLine(x, RULE_HEADER_HEIGHT, x, m_iTotalHeight + RULE_HEADER_HEIGHT);
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
   m_wndHeader.SetWindowPos(NULL, 0, 0, m_iTotalWidth, RULE_HEADER_HEIGHT, SWP_NOZORDER);
   UpdateScrollBars();
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


//
// Clear selection flag from all rows
//

void CRuleList::ClearSelection(BOOL bRedraw)
{
   int i;

   for(i = 0; i < m_iNumRows; i++)
      m_ppRowList[i]->m_dwFlags &= ~RLF_SELECTED;
   if (bRedraw)
      InvalidateList();
}


//
// Get row from given point
//

int CRuleList::RowFromPoint(int x, int y, int *piRowStart)
{
   int i, cy;

   if ((x >= m_iTotalWidth - m_iXOrg) || 
       (y >= m_iTotalHeight + RULE_HEADER_HEIGHT - m_iYOrg) ||
       (y <= RULE_HEADER_HEIGHT - m_iYOrg))
      return - 1;

   for(i = 0, cy = RULE_HEADER_HEIGHT - m_iYOrg; i < m_iNumRows; i++)
   {
      if ((y >= cy) && (y < cy + m_ppRowList[i]->m_iHeight))
      {
         if (piRowStart != NULL)
            *piRowStart = cy;
         return i;
      }
      cy += m_ppRowList[i]->m_iHeight;
   }
   return -1;
}


//
// Get column from given point
//

int CRuleList::ColumnFromPoint(int x, int y, int *piColStart)
{
   int i, cx;

   if ((x >= m_iTotalWidth - m_iXOrg) || (y >= m_iTotalHeight + RULE_HEADER_HEIGHT - m_iYOrg))
      return - 1;

   for(i = 0, cx = -m_iXOrg; i < m_iNumColumns; i++)
   {
      if ((x >= cx) && (x < cx + m_pColList[i].m_iWidth))
      {
         if (piColStart != NULL)
            *piColStart = cx;
         return i;
      }
      cx += m_pColList[i].m_iWidth;
   }
   return -1;
}


//
// Item from point
//

int CRuleList::ItemFromPoint(int x, int y)
{
   int i, cy, iRow, iCol, iCellX, iCellY, iCellHeight;

   iRow = RowFromPoint(x, y, &iCellY);
   iCol = ColumnFromPoint(x, y, &iCellX);
   if ((iRow != -1) && (iCol != -1))
   {
      iCellHeight = (m_ppRowList[iRow]->m_ppCellList[iCol]->m_iNumLines == 0) ? EMPTY_ROW_HEIGHT :
                     (CELL_TEXT_Y_MARGIN * 2 + m_ppRowList[iRow]->m_ppCellList[iCol]->m_iNumLines * m_iTextHeight +
                        (m_ppRowList[iRow]->m_ppCellList[iCol]->m_iNumLines - 1) * CELL_TEXT_Y_SPACING);

      cy = iCellY + (m_ppRowList[iRow]->m_iHeight - iCellHeight) / 2;
      for(i = 0; i < m_ppRowList[iRow]->m_ppCellList[iCol]->m_iNumLines; i++)
      {
         if ((y >= cy) && (y < cy + m_iTextHeight))
            return i;
         cy += m_iTextHeight + CELL_TEXT_Y_SPACING;
      }
   }
   return -1;
}


//
// WM_SIZE message handler
//

void CRuleList::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   UpdateScrollBars();
}


//
// Update scroll bars state and position
//

void CRuleList::UpdateScrollBars(void)
{
   RECT rect;
   SCROLLINFO si;
   int iCX, iCY;
   BOOL bUpdateWindow = FALSE;

   GetClientRect(&rect);
   si.cbSize = sizeof(SCROLLINFO);
   si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
   si.nMin = 0;

   iCX = m_iTotalWidth + m_iEdgeCX;
   iCY = m_iTotalHeight + RULE_HEADER_HEIGHT + m_iEdgeCY;

   // Set scroll bars ranges
   si.nMax = (rect.right < iCX) ? iCX : 0;
   si.nPage = rect.right;
   if (si.nMax == 0)
   {
      if (m_iXOrg > 0)
      {
         m_iXOrg = 0;
         bUpdateWindow = TRUE;
      }
   }
   else
   {
      if (m_iXOrg > (int)(si.nMax - si.nPage))
      {
         m_iXOrg = si.nMax - si.nPage;
         bUpdateWindow = TRUE;
      }
   }
   si.nPos = m_iXOrg;
   SetScrollInfo(SB_HORZ, &si);

   si.nMax = (rect.bottom < iCY) ? iCY : 0;
   si.nPage = rect.bottom;
   if (si.nMax == 0)
   {
      if (m_iYOrg > 0)
      {
         m_iYOrg = 0;
         bUpdateWindow = TRUE;
      }
   }
   else
   {
      if (m_iYOrg > (int)(si.nMax - si.nPage))
      {
         m_iYOrg = si.nMax - si.nPage;
         bUpdateWindow = TRUE;
      }
   }
   si.nPos = m_iYOrg;
   SetScrollInfo(SB_VERT, &si);

   if (bUpdateWindow)
      OnScroll(FALSE);
}


//
// WM_VSCROLL message handler
//

void CRuleList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   int iNewPos;

   // Calculate new position
   iNewPos = CalculateNewScrollPos(SB_VERT, nSBCode, nPos);

   // Update Y origin
   if (iNewPos != -1)
   {
      SetScrollPos(SB_VERT, iNewPos);
      m_iYOrg = iNewPos;
      OnScroll(FALSE);
   }
}


//
// WM_HSCROLL message handler
//

void CRuleList::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
   int iNewPos;

   // Calculate new position
   iNewPos = CalculateNewScrollPos(SB_HORZ, nSBCode, nPos);

   // Update X origin
   if (iNewPos != -1)
   {
      SetScrollPos(SB_HORZ, iNewPos);
      m_iXOrg = iNewPos;
      OnScroll(TRUE);
   }
}


//
// Calculate new scroll bar position
//

int CRuleList::CalculateNewScrollPos(UINT nScrollBar, UINT nSBCode, UINT nPos)
{
   int iNewPos;
   RECT rect;

   GetClientRect(&rect);
   switch(nSBCode)
   {
      case SB_THUMBPOSITION:
         iNewPos = nPos;
         break;
      case SB_TOP:
         iNewPos = 0;
         break;
      case SB_BOTTOM:
         iNewPos = (nScrollBar == SB_HORZ) ? m_iTotalWidth + m_iEdgeCX - rect.right :
                        m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom;
         break;
      case SB_PAGEUP:
         iNewPos = (nScrollBar == SB_HORZ) ? m_iXOrg - rect.right : m_iYOrg - rect.bottom;
         if (iNewPos < 0)
            iNewPos = 0;
         break;
      case SB_PAGEDOWN:
         iNewPos = (nScrollBar == SB_HORZ) ? 
               min(m_iXOrg + rect.right, m_iTotalWidth + m_iEdgeCX - rect.right) :
               min(m_iYOrg + rect.bottom, m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom);
         break;
      case SB_LINEUP:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_iXOrg > SCROLL_STEP) ? m_iXOrg - SCROLL_STEP : 0;
         else
            iNewPos = (m_iYOrg > SCROLL_STEP) ? m_iYOrg - SCROLL_STEP : 0;
         break;
      case SB_LINEDOWN:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_iXOrg + SCROLL_STEP < m_iTotalWidth + m_iEdgeCX - rect.right) ? 
               (m_iXOrg + SCROLL_STEP) : (m_iTotalWidth + m_iEdgeCX - rect.right);
         else
            iNewPos = (m_iYOrg + SCROLL_STEP < m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom - 1) ? 
               (m_iYOrg + SCROLL_STEP) : (m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom - 1);
         break;
      case SB_WHEELUP:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_iXOrg > (int)(SCROLL_STEP * nPos)) ? m_iXOrg - SCROLL_STEP * nPos : 0;
         else
            iNewPos = (m_iYOrg > (int)(SCROLL_STEP * nPos)) ? m_iYOrg - SCROLL_STEP * nPos : 0;
         break;
      case SB_WHEELDOWN:
         if (nScrollBar == SB_HORZ)
            iNewPos = (m_iXOrg + (int)(SCROLL_STEP * nPos) < m_iTotalWidth + m_iEdgeCX - rect.right) ? 
               (m_iXOrg + (int)(SCROLL_STEP * nPos)) : (m_iTotalWidth + m_iEdgeCX - rect.right);
         else
            iNewPos = (m_iYOrg + (int)(SCROLL_STEP * nPos) < m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom - 1) ? 
               (m_iYOrg + (int)(SCROLL_STEP * nPos)) : (m_iTotalHeight + m_iEdgeCY + RULE_HEADER_HEIGHT - rect.bottom - 1);
         break;
      default:
         iNewPos = -1;  // Ignore other codes
   }
   return iNewPos;
}


//
// Update window on scroll
//

void CRuleList::OnScroll(BOOL bRedrawHeader)
{
   m_wndHeader.SetWindowPos(NULL, -m_iXOrg, 0, 0, 0, 
                            SWP_NOSIZE | SWP_NOZORDER | (bRedrawHeader ? 0 : SWP_NOREDRAW));
   InvalidateList();
}


//
// WM_MOUSEWHEEL message handler
//

BOOL CRuleList::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
   SCROLLINFO si;
   int iSteps;

   si.cbSize = sizeof(SCROLLINFO);
   GetScrollInfo(SB_VERT, &si);
   if (si.nMax > 0)  // Vertical scroll bar enabled, do scroll
   {
      m_iMouseWheelDelta += zDelta;
      iSteps = m_iMouseWheelDelta / WHEEL_DELTA;
      if (iSteps != 0)
      {
         OnVScroll((iSteps > 0) ? SB_WHEELUP : SB_WHEELDOWN, abs(iSteps), NULL);
         m_iMouseWheelDelta -= iSteps * WHEEL_DELTA;
      }
   }
   return TRUE;
}


//
// Common code for processing mouse buttons
//

void CRuleList::OnMouseButtonDown(UINT nFlags, CPoint point)
{
   int iRow, iCol;

   iRow = RowFromPoint(point.x, point.y);
   iCol = ColumnFromPoint(point.x, point.y);

   // Clear cell text selection if needed
   if ((iRow != m_iCurrRow) || (iCol != m_iCurrColumn))
      if ((m_iCurrRow != -1) && (m_iCurrColumn != -1))
         m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->ClearSelection();

   m_iCurrRow = iRow;
   m_iCurrColumn = iCol;

   m_iCurrItem = ItemFromPoint(point.x, point.y);
}


//
// WM_RBUTTONDOWN message handler
//

void CRuleList::OnRButtonDown(UINT nFlags, CPoint point) 
{
   OnMouseButtonDown(nFlags, point);
   if (m_iCurrRow != -1)
   {
      if (!(m_ppRowList[m_iCurrRow]->m_dwFlags & RLF_SELECTED))
      {
         // Right click on non-selected row
         ClearSelection(FALSE);
         m_ppRowList[m_iCurrRow]->m_dwFlags |= RLF_SELECTED;

         // Select item
         if (m_iCurrColumn != -1)
         {
            m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->ClearSelection();
            if ((m_iCurrItem != -1) &&
                (m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_bSelectable))
               m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_pSelectFlags[m_iCurrItem] = 1;
         }
         
         InvalidateList();
      }
   }
	CWnd::OnRButtonDown(nFlags, point);
}


//
// WM_LBUTTONDOWN message handler
//

void CRuleList::OnLButtonDown(UINT nFlags, CPoint point) 
{
   OnMouseButtonDown(nFlags, point);
   if (m_iCurrRow != -1)
   {
      if (nFlags & MK_CONTROL)
      {
         m_ppRowList[m_iCurrRow]->m_dwFlags |= RLF_SELECTED;

         // Select item
         if (m_iCurrColumn != -1)
            if ((m_iCurrItem != -1) &&
                (m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_bSelectable))
               m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_pSelectFlags[m_iCurrItem] = 1;
      }
      else if (nFlags & MK_SHIFT)
      {
      }
      else
      {
         // Select row
         ClearSelection(FALSE);
         m_ppRowList[m_iCurrRow]->m_dwFlags |= RLF_SELECTED;
         
         // Select item
         if (m_iCurrColumn != -1)
         {
            m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->ClearSelection();
            if ((m_iCurrItem != -1) && 
                (m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_bSelectable))
               m_ppRowList[m_iCurrRow]->m_ppCellList[m_iCurrColumn]->m_pSelectFlags[m_iCurrItem] = 1;
         }
      }
      InvalidateList();
   }
   else
   {
      ClearSelection();
   }
}


//
// Get next row with matched flags
//

int CRuleList::GetNextRow(int iStartAfter, DWORD dwFlags)
{
   int i;

   for(i = iStartAfter + 1; i < m_iNumRows; i++)
      if ((m_ppRowList[i]->m_dwFlags & dwFlags) == dwFlags)
         return i;
   return -1;
}


//
// Enable or disable row
//

void CRuleList::EnableRow(int iRow, BOOL bEnable)
{
   if ((iRow >= 0) && (iRow < m_iNumRows))
      if (bEnable)
         m_ppRowList[iRow]->m_dwFlags &= ~RLF_DISABLED;
      else
         m_ppRowList[iRow]->m_dwFlags |= RLF_DISABLED;
   InvalidateRow(iRow);
}


//
// Get number of items in given cell
//

int CRuleList::GetNumItems(int iRow, int iCell)
{
   if ((iRow < 0) || (iRow >= m_iNumRows) || (iCell < 0) || (iCell >= m_iNumColumns))
      return 0;
   return m_ppRowList[iRow]->m_ppCellList[iCell]->m_iNumLines;
}


//
// Remove all lines from cell
//

void CRuleList::ClearCell(int iRow, int iCell)
{
   if ((iRow < 0) || (iRow >= m_iNumRows) || (iCell < 0) || (iCell >= m_iNumColumns))
      return;
   if (m_pColList[iCell].m_dwFlags & CF_TEXTBOX)
      return;  // SetCellText() should be used for text box columns
   m_ppRowList[iRow]->m_ppCellList[iCell]->Clear();
}


//
// Invalidate rectangle with visible part of the list
//

void CRuleList::InvalidateList()
{
   RECT rect;

   GetClientRect(&rect);
   rect.top = RULE_HEADER_HEIGHT;
   if (m_iTotalWidth + m_iEdgeCX < rect.right)
      rect.right = m_iTotalWidth + m_iEdgeCX;
   if (m_iTotalHeight  + m_iEdgeCY < rect.bottom - RULE_HEADER_HEIGHT)
      rect.bottom = m_iTotalHeight + RULE_HEADER_HEIGHT + m_iEdgeCY;
   InvalidateRect(&rect, FALSE);
}


//
// Invalidate rectangle containing single row
//

void CRuleList::InvalidateRow(int iRow)
{
   RECT rect;
   int i, y;

   GetClientRect(&rect);

   // Calculate row starting Y position
   for(i = 0, y = RULE_HEADER_HEIGHT - m_iYOrg; i < iRow; i++)
      y+= m_ppRowList[i]->m_iHeight;
   
   // Create rectangle for row
   if ((y + m_ppRowList[iRow]->m_iHeight < RULE_HEADER_HEIGHT) || (y > rect.bottom))
      return;  // Row isn't visible at all
   rect.top = (y < RULE_HEADER_HEIGHT) ? RULE_HEADER_HEIGHT : y;
   if (y + m_ppRowList[iRow]->m_iHeight < rect.bottom)
      rect.bottom = y + m_ppRowList[iRow]->m_iHeight;
   if (m_iTotalWidth < rect.right)
      rect.right = m_iTotalWidth;
   InvalidateRect(&rect, FALSE);
}


//
// Delete item from cell
//

void CRuleList::DeleteItem(int iRow, int iCell, int iItem)
{
   if (m_pColList[iCell].m_dwFlags & CF_TEXTBOX)
      return;  // SetCellText() should be used for text box columns

   if ((iRow >= 0) && (iRow < m_iNumRows) &&
       (iCell >= 0) && (iCell < m_iNumColumns))
   {
      if (m_ppRowList[iRow]->m_ppCellList[iCell]->DeleteLine(iItem))
      {
         if ((iRow == m_iCurrRow) && (iCell == m_iCurrColumn))
         {
            if (iItem == m_iCurrItem)
               m_iCurrItem = -1;
            else
               if (iItem < m_iCurrItem)
                  m_iCurrItem--;
         }
         m_ppRowList[iRow]->RecalcHeight(m_iTextHeight, m_pColList, &m_fontNormal);
         RecalcHeight();
         UpdateScrollBars();
         InvalidateRect(NULL);
      }
   }
}


//
// Enable or disable item selection in cell
//

BOOL CRuleList::EnableCellSelection(int iRow, int iCell, BOOL bEnable)
{
   BOOL bOldValue = FALSE;

   if ((iRow >= 0) && (iRow < m_iNumRows) &&
       (iCell >= 0) && (iCell < m_iNumColumns))
   {
      bOldValue = m_ppRowList[iRow]->m_ppCellList[iCell]->m_bSelectable;
      m_ppRowList[iRow]->m_ppCellList[iCell]->m_bSelectable = bEnable;
      if (!bEnable)
         m_ppRowList[iRow]->m_ppCellList[iCell]->ClearSelection();
   }
   return bOldValue;
}


//
// Delete row
//

BOOL CRuleList::DeleteRow(int iRow)
{
   BOOL bResult = FALSE;

   if ((iRow >= 0) && (iRow < m_iNumRows))
   {
      if (iRow == m_iCurrRow)
         m_iCurrRow = -1;
      delete m_ppRowList[iRow];
      m_iNumRows--;
      memmove(&m_ppRowList[iRow], &m_ppRowList[iRow + 1], sizeof(RL_Row *) * (m_iNumRows - iRow));
      RecalcHeight();
      UpdateScrollBars();
      InvalidateRect(NULL);
      bResult = TRUE;
   }
   return bResult;
}


//
// Set text for textbox cells
//

void CRuleList::SetCellText(int iRow, int iColumn, char *pszText)
{
   if (m_pColList[iColumn].m_dwFlags & CF_TEXTBOX)
   {
      m_ppRowList[iRow]->m_ppCellList[iColumn]->SetText(pszText != NULL ? pszText : "");
      m_ppRowList[iRow]->RecalcHeight(m_iTextHeight, m_pColList, &m_fontNormal);
      RecalcHeight();
      UpdateScrollBars();
      InvalidateRect(NULL);
   }
}
