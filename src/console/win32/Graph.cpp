// Graph.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "Graph.h"
#include <math.h>

#define ROW_DATA(row, dt)  ((dt == DCI_DT_STRING) ? strtod(row->value.szString, NULL) : \
                            (((dt == DCI_DT_INT) || (dt == DCI_DT_UINT)) ? row->value.dwInt32 : \
                             (((dt == DCI_DT_INT64) || (dt == DCI_DT_UINT64)) ? (INT64)row->value.qwInt64 : \
                              ((dt == DCI_DT_FLOAT) ? row->value.dFloat : 0) \
                             ) \
                            ) \
                           )

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGraph

CGraph::CGraph()
{
   m_dMaxValue = 100;
   m_bAutoScale = TRUE;
   m_bShowGrid = TRUE;
   m_bShowRuler = TRUE;
   m_dwNumItems = 0;
   m_rgbBkColor = RGB(0,0,0);
   m_rgbGridColor = RGB(64, 64, 64);
   m_rgbAxisColor = RGB(127, 127, 127);
   m_rgbTextColor = RGB(255, 255, 255);
   m_rgbLabelBkColor = RGB(255, 255, 170);
   m_rgbLabelTextColor = RGB(85, 0, 0);

   m_rgbLineColors[0] = RGB(0, 255, 0);
   m_rgbLineColors[1] = RGB(255, 255, 0);
   m_rgbLineColors[2] = RGB(0, 255, 255);
   m_rgbLineColors[3] = RGB(0, 0, 255);
   m_rgbLineColors[4] = RGB(255, 0, 255);
   m_rgbLineColors[5] = RGB(255, 0, 0);
   m_rgbLineColors[6] = RGB(0, 128, 128);
   m_rgbLineColors[7] = RGB(0, 128, 0);
   m_rgbLineColors[8] = RGB(128, 128, 255);
   m_rgbLineColors[9] = RGB(255, 128, 0);
   m_rgbLineColors[10] = RGB(128, 128, 0);
   m_rgbLineColors[11] = RGB(128, 0, 255);
   m_rgbLineColors[12] = RGB(255, 255, 128);
   m_rgbLineColors[13] = RGB(0, 128, 64);
   m_rgbLineColors[14] = RGB(0, 128, 255);
   m_rgbLineColors[15] = RGB(192, 192, 192);

   memset(m_pData, 0, sizeof(NXC_DCI_DATA *) * MAX_GRAPH_ITEMS);
   
   m_bIsActive = FALSE;
}

CGraph::~CGraph()
{
   DWORD i;

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      if (m_pData[i] != NULL)
         NXCDestroyDCIData(m_pData[i]);
}


BEGIN_MESSAGE_MAP(CGraph, CWnd)
	//{{AFX_MSG_MAP(CGraph)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraph message handlers


BOOL CGraph::Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, int nId)
{
   return CWnd::Create(NULL, "", dwStyle, rect, pwndParent, nId);
}


//
// Overloaded PreCreateWindow()
//

BOOL CGraph::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, 
      ::LoadCursor(NULL, IDC_ARROW), ::CreateSolidBrush(RGB(0, 0, 0)), NULL);

	return TRUE;
}


//
// WM_PAINT message handler
//

void CGraph::OnPaint() 
{
	CPaintDC sdc(this);  // original device context for painting
   CDC dc;              // In-memory dc
   CBitmap *pOldBitmap;
   RECT rect;

   GetClientRect(&rect);

   dc.CreateCompatibleDC(&sdc);
   pOldBitmap = dc.SelectObject(&m_bmpGraph);

   // Draw ruler
   if (m_bShowRuler && m_bIsActive && PtInRect(&m_rectGraph, m_ptCurrMousePos))
   {
      CPen pen, *pOldPen;
      CBitmap bitmap;         // Bitmap for in-memory drawing
      CBitmap *pOldBitmap2;
      CDC dc2;

      // Create another one in-memory DC to draw ruler on top of graph image
      dc2.CreateCompatibleDC(&sdc);
      bitmap.CreateCompatibleBitmap(&sdc, rect.right, rect.bottom);
      pOldBitmap2 = dc2.SelectObject(&bitmap);

      // Move drawing from in-memory DC to in-memory DC #2
      dc2.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);

      pen.CreatePen(PS_DOT, 1, m_rgbAxisColor);
      pOldPen = dc2.SelectObject(&pen);

      dc2.SetBkColor(m_rgbBkColor);
      dc2.MoveTo(m_rectGraph.left, m_ptCurrMousePos.y);
      dc2.LineTo(m_rectGraph.right, m_ptCurrMousePos.y);
      dc2.MoveTo(m_ptCurrMousePos.x, m_rectGraph.top);
      dc2.LineTo(m_ptCurrMousePos.x, m_rectGraph.bottom);

      dc2.SelectObject(pOldPen);
      pen.DeleteObject();

      // Move drawing from in-memory DC #2 to screen
      sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc2, 0, 0, SRCCOPY);

      dc2.SelectObject(pOldBitmap2);
      bitmap.DeleteObject();
      dc2.DeleteDC();
   }
   else
   {
      // Move drawing from in-memory DC to screen
      sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);
   }

   // Cleanup
   dc.SelectObject(pOldBitmap);
   dc.DeleteDC();
}


//
// Set time frame this graph covers
//

void CGraph::SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo)
{
   m_dwTimeFrom = dwTimeFrom;
   m_dwTimeTo = dwTimeTo;
}


//
// Set data for specific item
//

void CGraph::SetData(DWORD dwIndex, NXC_DCI_DATA *pData)
{
   if (dwIndex < MAX_GRAPH_ITEMS)
   {
      if (m_pData[dwIndex] != NULL)
         NXCDestroyDCIData(m_pData[dwIndex]);
      m_pData[dwIndex] = pData;
   }
}


//
// Draw single line
//

void CGraph::DrawLineGraph(CDC &dc, NXC_DCI_DATA *pData, COLORREF rgbColor, int nGridSize)
{
   DWORD i;
   int x;
   CPen pen, *pOldPen;
   NXC_DCI_ROW *pRow;
   double dScale;

   if (pData->dwNumRows < 2)
      return;  // Nothing to draw

   pen.CreatePen(PS_SOLID, 2, rgbColor);
   pOldPen = dc.SelectObject(&pen);

   // Calculate scale factor for values
   dScale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
               (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / m_dCurrMaxValue;

   // Move to first position
   pRow = pData->pRows;
   dc.MoveTo(m_rectGraph.right, 
             (int)(m_rectGraph.bottom - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1));
   inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);

   for(i = 1; (i < pData->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); i++)
   {
      // Calculate timestamp position on graph
      x = m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel);
      dc.LineTo(x, (int)(m_rectGraph.bottom - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1));
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
   }

   dc.SelectObject(pOldPen);
}


//
// WM_MOUSEMOVE message handler
//

void CGraph::OnMouseMove(UINT nFlags, CPoint point) 
{
   BOOL bChanged;

   CWnd::OnMouseMove(nFlags, point);
   if (!m_bIsActive)
      return;

   if (PtInRect(&m_rectGraph, point))
   {
      DWORD dwTimeStamp;
      double dValue;

      dwTimeStamp = m_dwTimeFrom + (DWORD)((point.x - m_rectGraph.left) * m_dSecondsPerPixel);
      dValue = m_dCurrMaxValue / (m_rectGraph.bottom - m_rectGraph.top - 
                  (m_rectGraph.bottom - m_rectGraph.top) % m_nLastGridSizeY) * 
                  (m_rectGraph.bottom - point.y);
      GetParent()->SendMessage(NXCM_UPDATE_GRAPH_POINT, dwTimeStamp, (LPARAM)&dValue);
      bChanged = TRUE;
   }
   else
   {
      GetParent()->SendMessage(NXCM_UPDATE_GRAPH_POINT, 0, 0);
      bChanged = PtInRect(&m_rectGraph, m_ptCurrMousePos);
   }
   m_ptCurrMousePos = point;
   if (bChanged)
      InvalidateRect(&m_rectGraph, FALSE);
}


//
// WM_SETFOCUS message handler
//

void CGraph::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
   m_bIsActive = TRUE;
}


//
// WM_KILLFOCUS message handler
//

void CGraph::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);
   m_bIsActive = FALSE;
   if (m_bShowRuler && PtInRect(&m_rectGraph, m_ptCurrMousePos))
   {
      m_ptCurrMousePos = CPoint(0, 0);
      InvalidateRect(&m_rectGraph, FALSE);
   }
}


//
// Draw entire graph on bitmap in memory
//

/* Select appropriate style for ordinate marks */
#define SELECT_ORDINATE_MARKS \
   if (m_dCurrMaxValue > 100000000000) \
   { \
      szModifier[0] = 'G'; \
      szModifier[1] = 0; \
      nDivider = 1000000000; \
      bIntMarks = TRUE; \
   } \
   else if (m_dCurrMaxValue > 100000000) \
   { \
      szModifier[0] = 'M'; \
      szModifier[1] = 0; \
      nDivider = 1000000; \
      bIntMarks = TRUE; \
   } \
   else if (m_dCurrMaxValue > 100000) \
   { \
      szModifier[0] = 'K'; \
      szModifier[1] = 0; \
      nDivider = 1000; \
      bIntMarks = TRUE; \
   } \
   else \
   { \
      szModifier[0] = 0; \
      nDivider = 1; \
      for(i = 0, bIntMarks = TRUE; i < MAX_GRAPH_ITEMS; i++) \
      { \
         if (m_pData[i] != NULL) \
         { \
            if (m_pData[i]->wDataType == DCI_DT_FLOAT) \
            { \
               bIntMarks = FALSE; \
               break; \
            } \
         } \
      } \
   }

void CGraph::DrawGraphOnBitmap()
{
   CDC *pdc, dc;           // Window dc and in-memory dc
   CBitmap *pOldBitmap;
   CPen pen, *pOldPen;
   CFont font, *pOldFont;
   CBrush brush;
   RECT rect;
   CSize textSize;
   DWORD i, dwTimeStamp;
   int iLeftMargin, iBottomMargin, iRightMargin = 5, iTopMargin = 5;
   int x, y, iTimeLen, iStep, iGraphLen, nDivider;
   int nGridSizeX, nGridSizeY, nGrids, nDataAreaHeight;
   double dStep, dMark;
   char szBuffer[256], szModifier[4];
   BOOL bIntMarks;

   GetClientRect(&rect);

   // Create compatible DC and bitmap for painting
   pdc = GetDC();
   dc.CreateCompatibleDC(pdc);
   m_bmpGraph.DeleteObject();
   m_bmpGraph.CreateCompatibleBitmap(pdc, rect.right, rect.bottom);
   ReleaseDC(pdc);

   // Initial DC setup
   pOldBitmap = dc.SelectObject(&m_bmpGraph);
   dc.SetBkColor(m_rgbBkColor);

   // Fill background
   brush.CreateSolidBrush(m_rgbBkColor);
   dc.FillRect(&rect, &brush);

   // Setup text parameters
   font.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                   0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                   VARIABLE_PITCH | FF_DONTCARE, "Verdana");
   pOldFont = dc.SelectObject(&font);
   dc.SetTextColor(m_rgbTextColor);

   // Calculate text size and left margin
   textSize = dc.GetTextExtent("00000.000");
   iTimeLen = dc.GetTextExtent("00:00:00").cx;
   iLeftMargin = textSize.cx + 10;
   iBottomMargin = textSize.cy + 8;

   // Draw all parameters
   ///////////////////////////////////

   // Calculate data rectangle
   memcpy(&m_rectGraph, &rect, sizeof(RECT));
   m_rectGraph.left += iLeftMargin;
   m_rectGraph.top += iTopMargin;
   m_rectGraph.right -= iRightMargin;
   m_rectGraph.bottom -= iBottomMargin;
   iGraphLen = m_rectGraph.right - m_rectGraph.left + 1;   // Actual data area length in pixels
   m_dSecondsPerPixel = (double)(m_dwTimeTo - m_dwTimeFrom) / (double)iGraphLen;
   nDataAreaHeight = rect.bottom - iBottomMargin - textSize.cy / 2 - iTopMargin - textSize.cy / 2;

   // Calculate max graph value
   if (m_bAutoScale)
   {
      for(i = 0, m_dCurrMaxValue = 0; i < MAX_GRAPH_ITEMS; i++)
         if (m_pData[i] != NULL)
         {
            NXC_DCI_ROW *pRow;
            double dCurrValue;
            DWORD j;

            pRow = m_pData[i]->pRows;
            for(j = 0; (j < m_pData[i]->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); j++)
            {
               dCurrValue = (double)ROW_DATA(pRow, m_pData[i]->wDataType);
               if (dCurrValue > m_dCurrMaxValue)
                  m_dCurrMaxValue = dCurrValue;
               inc_ptr(pRow, m_pData[i]->wRowSize, NXC_DCI_ROW);
            }
         }

      if (m_dCurrMaxValue == 0)
         m_dCurrMaxValue = 1;

      // Round max value
      for(double d = 0.00001; d < 10000000000000000000; d *= 10)
         if ((m_dCurrMaxValue >= d) && (m_dCurrMaxValue <= d * 10))
         {
            m_dCurrMaxValue -= fmod(m_dCurrMaxValue, d);
            m_dCurrMaxValue += d;

            SELECT_ORDINATE_MARKS;
   
            // For integer values, Y axis step cannot be less than 1
            if (bIntMarks && (d < 1))
               d = 1;

            // Calculate grid size for Y axis
            nGridSizeY = (int)(nDataAreaHeight / (m_dCurrMaxValue / d));
            if (nGridSizeY > 2)
            {
               if (bIntMarks)
               {
                  nGrids = nDataAreaHeight / (nGridSizeY / 2);
                  while((nGridSizeY >= 50) && ((INT64)m_dCurrMaxValue % nGrids == 0))
                  {
                     nGridSizeY /= 2;
                     nGrids = nDataAreaHeight / (nGridSizeY / 2);
                  }
               }
               else
               {
                  while(nGridSizeY >= 50)
                     nGridSizeY /= 2;
               }
            }
            else
            {
               nGridSizeY = 2;
            }
            break;
         }
   }
   else
   {
      m_dCurrMaxValue = m_dMaxValue;

      SELECT_ORDINATE_MARKS;
      
      nGridSizeY = 40;
   }

   // Calculate grid size for X axis
   //for(nGridSizeX = 25; (nGridSizeX < 50) && (fmod(m_dSecondsPerPixel * nGridSizeX, 15) != 0); nGridSizeX++);
   nGridSizeX = 40;

   // Draw grid
   if (m_bShowGrid)
   {
      pen.CreatePen(PS_ALTERNATE | PS_COSMETIC, 0, m_rgbGridColor);
      pOldPen = dc.SelectObject(&pen);
      for(x = iLeftMargin + nGridSizeX; x < rect.right - iRightMargin; x += nGridSizeX)
      {
         dc.MoveTo(x, rect.bottom - iBottomMargin);
         dc.LineTo(x, iTopMargin);
      }
      for(y = rect.bottom - iBottomMargin - nGridSizeY; y > iTopMargin; y -= nGridSizeY)
      {
         dc.MoveTo(iLeftMargin, y);
         dc.LineTo(rect.right - iRightMargin, y);
      }
      dc.SelectObject(pOldPen);
      pen.DeleteObject();
   }

   // Draw each parameter
   CRgn rgn;
   rgn.CreateRectRgn(m_rectGraph.left, m_rectGraph.top, m_rectGraph.right, m_rectGraph.bottom);
   dc.SelectClipRgn(&rgn);
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      if (m_pData[i] != NULL)
         DrawLineGraph(dc, m_pData[i], m_rgbLineColors[i], nGridSizeY);
   dc.SelectClipRgn(NULL);
   rgn.DeleteObject();

   // Paint ordinates
   pen.CreatePen(PS_SOLID, 3, m_rgbAxisColor);
   pOldPen = dc.SelectObject(&pen);
   dc.MoveTo(iLeftMargin, rect.bottom - iBottomMargin);
   dc.LineTo(iLeftMargin, iTopMargin);
   dc.MoveTo(iLeftMargin, rect.bottom - iBottomMargin);
   dc.LineTo(rect.right - iRightMargin, rect.bottom - iBottomMargin);
   dc.SelectObject(pOldPen);
   pen.DeleteObject();

   // Display ordinate marks
   dStep = m_dCurrMaxValue / ((rect.bottom - iBottomMargin - iTopMargin) / nGridSizeY);
   for(y = rect.bottom - iBottomMargin - textSize.cy / 2, dMark = 0; y > iTopMargin; y -= nGridSizeY, dMark += dStep)
   {
      if (bIntMarks)
         sprintf(szBuffer, INT64_FMT "%s", (INT64)dMark / nDivider, szModifier);
      else
         sprintf(szBuffer, "%5.3f%s", dMark, szModifier);
      CSize cz = dc.GetTextExtent(szBuffer);
      dc.TextOut(iLeftMargin - cz.cx - 5, y, szBuffer);
   }

   // Display absciss marks
   y = rect.bottom - iBottomMargin + 3;
   iStep = iTimeLen / nGridSizeX + 1;    // How many grid lines we should skip
   for(x = iLeftMargin; x < rect.right - iRightMargin; x += nGridSizeX * iStep)
   {
      dwTimeStamp = m_dwTimeFrom + (DWORD)((double)(x - iLeftMargin) * m_dSecondsPerPixel);
      FormatTimeStamp(dwTimeStamp, szBuffer, TS_LONG_TIME);
      dc.TextOut(x, y, szBuffer);
   }

   // Cleanup
   dc.SelectObject(pOldFont);
   dc.SelectObject(pOldBitmap);
   font.DeleteObject();
   dc.DeleteDC();

   // Save used grid size
   m_nLastGridSizeY = nGridSizeY;
}


//
// Repaint graph entirely
//

void CGraph::Update()
{
   DrawGraphOnBitmap();
   Invalidate(FALSE);
}


//
// WM_SIZE message handler
//

void CGraph::OnSize(UINT nType, int cx, int cy) 
{
   DrawGraphOnBitmap();
	CWnd::OnSize(nType, cx, cy);
}
