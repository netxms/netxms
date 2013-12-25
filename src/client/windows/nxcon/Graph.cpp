// Graph.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "Graph.h"
#include <math.h>

#define LEGEND_TEXT_SPACING   4

#define STATE_NORMAL          0
#define STATE_SELECTING       1


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Get month from timestamp
//

static int MonthFromTS(DWORD dwTimeStamp)
{
   time_t t = dwTimeStamp;
	struct tm ltm;
	errno_t err;
	
	err = localtime_s(&ltm, &t);
   return (err == 0) ? ltm.tm_mon : -1;
}


/////////////////////////////////////////////////////////////////////////////
// CGraph

CGraph::CGraph()
{
   m_dMaxValue = 100;
   m_dMinValue = 0;
   m_bAutoScale = TRUE;
   m_bShowGrid = TRUE;
   m_bShowRuler = TRUE;
   m_bShowLegend = TRUE;
   m_bEnableZoom = TRUE;
	m_bSet3DEdge = TRUE;
	m_bShowTitle = FALSE;
	m_bShowHostNames = FALSE;
	m_bUpdating = FALSE;
	m_bLogarithmicScale = FALSE;
   m_dwNumItems = 0;
	m_szTitle[0] = 0;
	m_dwTimeFrom = 0;
	m_dwTimeTo = 0;

	memset(m_graphItemStyles, 0, sizeof(GRAPH_ITEM_STYLE) * MAX_GRAPH_ITEMS);
	SetColorScheme(GCS_CLASSIC);

   memset(m_pData, 0, sizeof(NXC_DCI_DATA *) * MAX_GRAPH_ITEMS);
   memset(m_thresholds, 0, sizeof(NXC_DCI_THRESHOLD *) * MAX_GRAPH_ITEMS);
   memset(m_numThresholds, 0, sizeof(DWORD) * MAX_GRAPH_ITEMS);
   m_ppItems = NULL;
   
   m_bIsActive = FALSE;
   m_nState = STATE_NORMAL;
   m_nZoomLevel = 0;
}

CGraph::~CGraph()
{
   DWORD i;

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
      if (m_pData[i] != NULL)
         NXCDestroyDCIData(m_pData[i]);
		safe_free(m_thresholds[i]);
	}
}


BEGIN_MESSAGE_MAP(CGraph, CWnd)
	//{{AFX_MSG_MAP(CGraph)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGraph message handlers


BOOL CGraph::Create(DWORD dwStyle, const RECT &rect, CWnd *pwndParent, int nId)
{
   return CWnd::Create(NULL, _T(""), dwStyle, rect, pwndParent, nId);
}


//
// Overloaded PreCreateWindow()
//

BOOL CGraph::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	if (m_bSet3DEdge)
	{
		cs.dwExStyle |= WS_EX_CLIENTEDGE;
		cs.style &= ~WS_BORDER;
	}
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, 
      ::LoadCursor(NULL, IDC_ARROW), NULL, NULL);

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
   if (m_bShowRuler && m_bIsActive && (m_nState == STATE_NORMAL) &&
       PtInRect(&m_rectGraph, m_ptCurrMousePos))
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

      pen.CreatePen(PS_DOT, 1, m_rgbRulerColor);
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
      if (m_nState == STATE_SELECTING)
      {
         CPen pen, *pOldPen;
         CBrush brush, *pOldBrush;
         CDC dcTemp, dcGraph;
         CBitmap bitmap, bmpGraph, *pOldTempBitmap, *pOldGraphBitmap;
         int cx, cy;
         BLENDFUNCTION bf;

         // Create map copy
         dcGraph.CreateCompatibleDC(&sdc);
         bmpGraph.CreateCompatibleBitmap(&sdc, rect.right, rect.bottom);
         pOldGraphBitmap = dcGraph.SelectObject(&bmpGraph);
         dcGraph.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);

         // Calculate selection width and height
         cx = m_rcSelection.right - m_rcSelection.left - 1;
         cy = m_rcSelection.bottom - m_rcSelection.top - 1;

         // Create temporary bitmap
         dcTemp.CreateCompatibleDC(&sdc);
         bitmap.CreateCompatibleBitmap(&sdc, cx, cy);
         pOldTempBitmap = dcTemp.SelectObject(&bitmap);

         // Fill temporary bitmap with selection color
         brush.DeleteObject();
         brush.CreateSolidBrush(m_rgbSelRectColor);
         pen.CreatePen(PS_SOLID, 1, m_rgbSelRectColor);
         pOldBrush = dcTemp.SelectObject(&brush);
         pOldPen = dcTemp.SelectObject(&pen);
         dcTemp.Rectangle(0, 0, cx + 1, cy + 1);
         dcTemp.SelectObject(pOldPen);
         dcTemp.SelectObject(pOldBrush);

         // Copy temporary bitmap to main bitmap with transparency
         bf.AlphaFormat = 0;
         bf.BlendFlags = 0;
         bf.BlendOp = AC_SRC_OVER;
         bf.SourceConstantAlpha = 32;
         AlphaBlend(dcGraph.m_hDC, m_rcSelection.left, m_rcSelection.top,
                    cx, cy, dcTemp.m_hDC, 0, 0, cx, cy, bf);
      
         // Draw solid rectangle around selection area
         brush.DeleteObject();
         brush.CreateStockObject(NULL_BRUSH);
         pOldBrush = dcGraph.SelectObject(&brush);
         pOldPen = dcGraph.SelectObject(&pen);
         dcGraph.Rectangle(&m_rcSelection);
         dcGraph.SelectObject(pOldPen);
         dcGraph.SelectObject(pOldBrush);

         dcTemp.SelectObject(pOldTempBitmap);
         dcTemp.DeleteDC();

         // Move drawing from in-memory bitmap to screen
         sdc.BitBlt(0, 0, rect.right, rect.bottom, &dcGraph, 0, 0, SRCCOPY);

         dcGraph.SelectObject(pOldGraphBitmap);
         dcGraph.DeleteDC();
      }
      else
      {
         // Move drawing from in-memory DC to screen
         sdc.BitBlt(0, 0, rect.right, rect.bottom, &dc, 0, 0, SRCCOPY);
      }
   }

	// Draw "Updating..." message
	if (m_bUpdating)
	{
		RECT rcText;
		CSize size;

		size = sdc.GetTextExtent(_T("Updating..."), 11);
		rcText.left = 10;
		rcText.top = 10;
		rcText.right = rcText.left + size.cx + 10;
		rcText.bottom = rcText.top + size.cy + 8;
		sdc.SetBkColor(RGB(64, 0, 0));
		sdc.SetTextColor(RGB(255, 0, 0));
		sdc.FillSolidRect(&rcText, RGB(64, 0, 0));
		sdc.DrawText(_T("Updating..."), 11, &rcText, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
		sdc.Draw3dRect(&rcText, RGB(255, 0, 0), RGB(255, 0, 0));
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
   struct tm lt;
   time_t t = dwTimeFrom;

	if (dwTimeFrom > dwTimeTo)
		return;

   m_dwTimeTo = dwTimeTo;

   // Round boundaries
   localtime_s(&lt, &t);
   if (dwTimeTo - dwTimeFrom >= 5184000)   // 60 days
   {
      // Align to month boundary
      lt.tm_mday = 1;
      lt.tm_hour = 0;
      lt.tm_min = 0;
      lt.tm_sec = 0;
   }
   else if (dwTimeTo - dwTimeFrom >= 432000)   // 5 days
   {
      // Align to day boundary
      lt.tm_hour = 0;
      lt.tm_min = 0;
      lt.tm_sec = 0;
   }
   else if (dwTimeTo - dwTimeFrom >= 86400)
   {
      lt.tm_min = 0;
      lt.tm_sec = 0;
   }
   else
   {
      lt.tm_sec = 0;
   }
   m_dwTimeFrom = (DWORD)mktime(&lt);
}


//
// Set data for specific item
//

void CGraph::SetData(DWORD dwIndex, NXC_DCI_DATA *pData)
{
   if (dwIndex >= MAX_GRAPH_ITEMS)
		return;

   if (m_pData[dwIndex] != NULL)
      NXCDestroyDCIData(m_pData[dwIndex]);
   m_pData[dwIndex] = pData;
}


//
// Update data (add new items to top and remove old from bottom)
//

void CGraph::UpdateData(DWORD dwIndex, NXC_DCI_DATA *pData)
{
   NXC_DCI_ROW *pCurr;
   DWORD dwSize;

   if (dwIndex >= MAX_GRAPH_ITEMS)
      return;

   // Delete records older than m_dwTimeFrom
   if (m_pData[dwIndex]->dwNumRows > 0)
   {
      pCurr = (NXC_DCI_ROW *)((char *)m_pData[dwIndex]->pRows + (m_pData[dwIndex]->dwNumRows - 1) * m_pData[dwIndex]->wRowSize);
      while((pCurr->dwTimeStamp < m_dwTimeFrom) && (m_pData[dwIndex]->dwNumRows > 0))
      {
         m_pData[dwIndex]->dwNumRows--;
         pCurr = (NXC_DCI_ROW *)((char *)pCurr - m_pData[dwIndex]->wRowSize);
      }
   }

   // Insert new records
   dwSize = m_pData[dwIndex]->dwNumRows * m_pData[dwIndex]->wRowSize;
   m_pData[dwIndex]->dwNumRows += pData->dwNumRows;
   m_pData[dwIndex]->pRows = (NXC_DCI_ROW *)realloc(m_pData[dwIndex]->pRows, m_pData[dwIndex]->dwNumRows * m_pData[dwIndex]->wRowSize);
   memmove((char *)m_pData[dwIndex]->pRows + pData->dwNumRows * m_pData[dwIndex]->wRowSize, m_pData[dwIndex]->pRows, dwSize);
   memcpy(m_pData[dwIndex]->pRows, pData->pRows, pData->dwNumRows * m_pData[dwIndex]->wRowSize);

   NXCDestroyDCIData(pData);
}


//
// Draw single line
//

void CGraph::DrawLineGraph(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize)
{
   DWORD i;
   int x;
   CPen pen, *pOldPen;
   NXC_DCI_ROW *pRow;
   double dScale;

   if (pData->dwNumRows < 2)
      return;  // Nothing to draw

   pen.CreatePen(PS_SOLID, (style->nLineWidth == 0) ? DEFAULT_LINE_WIDTH : style->nLineWidth, style->rgbColor);
   pOldPen = dc.SelectObject(&pen);

   // Calculate scale factor for values
   dScale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
               (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / (m_dCurrMaxValue - m_dCurrMinValue);

   // Move to first position
   pRow = pData->pRows;
   for(i = 0; (i < pData->dwNumRows) && (pRow->dwTimeStamp > m_dwTimeTo); i++)
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
   if (i < pData->dwNumRows)
   {
      dc.MoveTo(m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel),
                (int)(m_nZeroLine - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1));
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);

      for(i++; (i < pData->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); i++)
      {
         // Calculate timestamp position on graph
         x = m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel);
         dc.LineTo(x, (int)(m_nZeroLine - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1));
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }
   }

   dc.SelectObject(pOldPen);
}


//
// Draw single area
//

void CGraph::DrawAreaGraph(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize)
{
   DWORD i;
   CPen pen, *pOldPen;
	CBrush brush, *pOldBrush;
	POINT pts[4];
   NXC_DCI_ROW *pRow;
   double dScale;

   if (pData->dwNumRows < 2)
      return;  // Nothing to draw

   pen.CreatePen(PS_SOLID, (style->nLineWidth == 0) ? DEFAULT_LINE_WIDTH : style->nLineWidth, style->rgbColor);
   pOldPen = dc.SelectObject(&pen);
	brush.CreateHatchBrush(HS_DIAGCROSS, style->rgbColor);
	pOldBrush = dc.SelectObject(&brush);

   // Calculate scale factor for values
   dScale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
               (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / (m_dCurrMaxValue - m_dCurrMinValue);

   // Move to first position
   pRow = pData->pRows;
   for(i = 0; (i < pData->dwNumRows) && (pRow->dwTimeStamp > m_dwTimeTo); i++)
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
   if (i < pData->dwNumRows)
   {
		pts[0].x = m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel);
		pts[0].y = m_nZeroLine;
		pts[1].x = pts[0].x;
		pts[1].y = (int)(m_nZeroLine - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1);
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);

      for(i++; (i < pData->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); i++)
      {
         // Calculate timestamp position on graph
         pts[2].x = m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel);
			pts[2].y = (int)(m_nZeroLine - (double)ROW_DATA(pRow, pData->wDataType) * dScale - 1);
			pts[3].x = pts[2].x;
			pts[3].y = m_nZeroLine;
			dc.Polygon(pts, 4);
			memcpy(&pts[0], &pts[3], sizeof(POINT));
			memcpy(&pts[1], &pts[2], sizeof(POINT));
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }
   }

   dc.SelectObject(pOldPen);
	dc.SelectObject(pOldBrush);
}


//
// Draw average mark
//

void CGraph::DrawAverage(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize)
{
	if (!style->bShowAverage)
		return;

	CPen pen, *oldPen;
   NXC_DCI_ROW *pRow;
	DWORD i;

   pRow = pData->pRows;
   for(i = 0; (i < pData->dwNumRows) && (pRow->dwTimeStamp > m_dwTimeTo); i++)
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
   if (i < pData->dwNumRows)
   {
		double sum = 0;
		int samples = 0;

      for(; (i < pData->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); i++)
      {
			sum += (double)ROW_DATA(pRow, pData->wDataType);
			samples++;
         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }

		if (samples > 0)
		{
			double avg = sum / samples;
		   double scale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
			                  (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / (m_dCurrMaxValue - m_dCurrMinValue);
			int y = (int)(m_nZeroLine - avg * scale - 1);

			pen.CreatePen(PS_DASH, 1, style->rgbColor);
			oldPen = dc.SelectObject(&pen);
			dc.MoveTo(m_rectGraph.left, y);
			dc.LineTo(m_rectGraph.right, y);
			dc.SelectObject(oldPen);
			pen.DeleteObject();
		}
   }
}


//
// Draw trend line
//

void CGraph::DrawTrendLine(CDC &dc, NXC_DCI_DATA *pData, GRAPH_ITEM_STYLE *style, int nGridSize)
{
	if (!style->bShowTrend)
		return;

	CPen pen, *oldPen;
   NXC_DCI_ROW *pRow;
	DWORD i;

   pRow = pData->pRows;
   for(i = 0; (i < pData->dwNumRows) && (pRow->dwTimeStamp > m_dwTimeTo); i++)
      inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
   if (i < pData->dwNumRows)
   {
		double scale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
			               (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / (m_dCurrMaxValue - m_dCurrMinValue);

		INT64 xSum = 0, ySum = 0, xxSum = 0, xySum = 0, count = 0;

      for(; (i < pData->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); i++)
      {
         int x = m_rectGraph.right - (int)((double)(m_dwTimeTo - pRow->dwTimeStamp) / m_dSecondsPerPixel);
			int y = (int)(m_nZeroLine - (double)ROW_DATA(pRow, pData->wDataType) * scale - 1);
			
			xSum += x;
			ySum += y;
			xySum += x * y;
			xxSum += x * x;
			count++;

         inc_ptr(pRow, pData->wRowSize, NXC_DCI_ROW);
      }

		if (count > 1)
		{
			double xx = (double)(xxSum * count - xSum * xSum);
			double slope = ((xx != 0) ? ((double)(xySum * count - xSum * ySum) / xx) : 0);
			double intercept = ((double)ySum - (double)xSum * slope) / (double)count;

			int yStart = (int)(slope * (double)m_rectGraph.left + intercept);
			int yEnd = (int)(slope * (double)m_rectGraph.right + intercept);

			//theApp.DebugPrintf(_T("DrawTrendLine: count=%I64d yStart=%d yEnd=%d slope=%f intercept=%d"), count, yStart, yEnd, slope, intercept);

			pen.CreatePen(PS_DOT, 1, style->rgbColor);
//			pen.CreatePen(PS_SOLID, 10, style->rgbColor);
			oldPen = dc.SelectObject(&pen);
			dc.MoveTo(m_rectGraph.left, yStart);
			dc.LineTo(m_rectGraph.right, yEnd);
			dc.SelectObject(oldPen);
			pen.DeleteObject();
		}
   }
}


//
// Draw thresholds
//

void CGraph::DrawThresholds(CDC &dc, NXC_DCI_THRESHOLD *thresholds, DWORD numThresholds, GRAPH_ITEM_STYLE *style, int nGridSize)
{
	if ((!style->bShowThresholds) || (thresholds == NULL))
		return;

	CPen pen, *oldPen;

	pen.CreatePen(PS_DOT, 1, style->rgbColor);
	oldPen = dc.SelectObject(&pen);

	for(DWORD i = 0; i < numThresholds; i++)
	{
	   double scale = (double)(m_rectGraph.bottom - m_rectGraph.top - 
		                  (m_rectGraph.bottom - m_rectGraph.top) % nGridSize) / (m_dCurrMaxValue - m_dCurrMinValue);
		int y = (int)(m_nZeroLine - _tcstod(thresholds[i].value, NULL) * scale - 1);

		dc.MoveTo(m_rectGraph.left, y);
		dc.LineTo(m_rectGraph.right, y);
	}
	dc.SelectObject(oldPen);
	pen.DeleteObject();
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
   if (dMaxAbsValue > 100000000000) \
   { \
      szModifier[0] = 'G'; \
      szModifier[1] = 0; \
      nDivider = 1000000000; \
      bIntMarks = TRUE; \
   } \
   else if (dMaxAbsValue > 100000000) \
   { \
      szModifier[0] = 'M'; \
      szModifier[1] = 0; \
      nDivider = 1000000; \
      bIntMarks = TRUE; \
   } \
   else if (dMaxAbsValue > 100000) \
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
            if ((m_pData[i]->wDataType == DCI_DT_FLOAT) || \
                (m_pData[i]->wDataType == DCI_DT_STRING)) \
            { \
					/* Strings can contain any numbers, including floating point numbers */ \
               bIntMarks = FALSE; \
               break; \
            } \
         } \
      } \
   }

/* Correct next month offset */
#define CORRECT_MONTH_OFFSET \
   { \
      int nMonth; \
\
      dwTimeStamp = m_dwTimeFrom + (DWORD)((double)(x - iLeftMargin) * m_dSecondsPerPixel); \
      nMonth = MonthFromTS(dwTimeStamp); \
		if (nMonth != -1) \
			while(1) \
			{ \
				dwTimeStamp = m_dwTimeFrom + (DWORD)((double)(x - iLeftMargin - 1) * m_dSecondsPerPixel); \
				if (MonthFromTS(dwTimeStamp) != nMonth) \
					break; \
				x--; \
			} \
   }

/* Display Y axis mark */
#define DISPLAY_Y_MARK \
	{ \
		if (m_bLogarithmicScale) \
		{ \
			bool useInt = true; \
			const TCHAR *modifier = _T(""); \
			double m = pow(10.0, dMark); \
			if (m > 100000000000) \
			{ \
				m /= 1000000000; \
				modifier = _T("G"); \
			} \
			else if (m > 100000000) \
			{ \
				m /= 1000000; \
				modifier = _T("M"); \
			} \
			else if (m > 100000) \
			{ \
				m /= 1000; \
				modifier = _T("K"); \
			} \
			else \
			{ \
				if (m != floor(m)) \
					useInt = false; \
			} \
			if (useInt) \
				_sntprintf_s(szBuffer, 256, _TRUNCATE, INT64_FMT _T("%s"), (INT64)m, modifier); \
			else \
				_sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%5.3f"), m); \
		} \
		else \
		{ \
			if (bIntMarks) \
				_sntprintf_s(szBuffer, 256, _TRUNCATE, INT64_FMT _T("%s"), (INT64)dMark / nDivider, szModifier); \
			else \
				_sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%5.3f%s"), dMark, szModifier); \
		} \
		CSize cz = dc.GetTextExtent(szBuffer); \
		dc.TextOut(iLeftMargin - cz.cx - 5, y, szBuffer); \
	}


void CGraph::DrawGraphOnBitmap(CBitmap &bmpGraph, RECT &rect)
{
   CDC *pdc, dc;           // Window dc and in-memory dc
   CBitmap *pOldBitmap;
   CPen pen, *pOldPen;
   CFont font, *pOldFont;
   CBrush brush, *pOldBrush;
   CSize textSize;
   DWORD i, dwTimeStamp;
   int iLeftMargin, iBottomMargin, iRightMargin = 5, iTopMargin = 5;
   int x, y, iTimeLen, iStep, iGraphLen, nDivider;
   int nGridSizeX, nGridSizeY, nGrids, nDataAreaHeight;
   int nColSize, nCols, nCurrCol, nTimeLabel;
   double dStep, dMark, dMaxAbsValue;
   TCHAR szBuffer[256], szModifier[4];
   BOOL bIntMarks;
   static double nSecPerMonth[12] = { 2678400, 2419200, 2678400, 2592000,
                                      2678400, 2592000, 2678400, 2678400,
                                      2592000, 2678400, 2592000, 2678400 };

   // Create compatible DC and bitmap for painting
   pdc = GetDC();
	if (pdc == NULL)
		return;	// Abort drawing if GetDC() failed

   dc.CreateCompatibleDC(pdc);
   bmpGraph.DeleteObject();
   bmpGraph.CreateCompatibleBitmap(pdc, rect.right, rect.bottom);
   ReleaseDC(pdc);

   // Initial DC setup
   pOldBitmap = dc.SelectObject(&bmpGraph);
   dc.SetBkColor(m_rgbBkColor);

   // Fill background
   brush.CreateSolidBrush(m_rgbBkColor);
   dc.FillRect(&rect, &brush);
   brush.DeleteObject();

   // Setup text parameters
   font.CreateFont(-MulDiv(7, GetDeviceCaps(dc.m_hDC, LOGPIXELSY), 72),
                   0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                   VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
   pOldFont = dc.SelectObject(&font);
   dc.SetTextColor(m_rgbTextColor);

   // Calculate text size and left margin
   textSize = dc.GetTextExtent(_T("0000.000"));
   iLeftMargin = textSize.cx + 10;
   if (m_bShowLegend)
   {
      CSize size;

      for(i = 0, nColSize = 0; i < MAX_GRAPH_ITEMS; i++)
      {
         if (m_ppItems != NULL)
            if (m_ppItems[i] != NULL)
            {
					if (m_bShowHostNames)
					{
						NXC_OBJECT *object;
						TCHAR temp[1024];

						object = NXCFindObjectById(g_hSession, m_ppItems[i]->m_dwNodeId);
						_sntprintf_s(temp, 1024, _TRUNCATE, _T("%s - %s"), (object != NULL) ? object->szName : _T("(null)"),
						             m_ppItems[i]->m_pszDescription);
						size = dc.GetTextExtent(temp, (int)_tcslen(temp));
					}
					else
					{
						size = dc.GetTextExtent(m_ppItems[i]->m_pszDescription,
														(int)_tcslen(m_ppItems[i]->m_pszDescription));
					}
               if (size.cx > nColSize)
                  nColSize = size.cx;
            }
      }
      nColSize += textSize.cy + 20;
      nCols = (rect.right - rect.left - iLeftMargin - iRightMargin) / nColSize;
      if (nCols == 0)
         nCols = 1;

      iBottomMargin = textSize.cy + 12;
      for(i = 0, nCurrCol = 0; i < MAX_GRAPH_ITEMS; i++)
         if (m_pData[i] != NULL)
         {
            if (nCurrCol == 0)
               iBottomMargin += textSize.cy + LEGEND_TEXT_SPACING;
            nCurrCol++;
            if (nCurrCol == nCols)
               nCurrCol = 0;
         }
   }
   else
   {
      iBottomMargin = textSize.cy + 8;
   }

	// Draw title if needed and adjust top margin
	if (m_bShowTitle)
	{
		RECT rcTitle;

		memcpy(&rcTitle, &rect, sizeof(RECT));
		rcTitle.left += iLeftMargin;
		rcTitle.top += iTopMargin;
		rcTitle.right -= iRightMargin;
		rcTitle.bottom = rcTitle.top + textSize.cy;
		dc.DrawText(m_szTitle, (int)_tcslen(m_szTitle), &rcTitle, DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
		iTopMargin += textSize.cy + 7;
	}

   // Calculate data rectangle
   memcpy(&m_rectGraph, &rect, sizeof(RECT));
   m_rectGraph.left += iLeftMargin;
   m_rectGraph.top += iTopMargin;
   m_rectGraph.right -= iRightMargin;
   m_rectGraph.bottom -= iBottomMargin;
   iGraphLen = m_rectGraph.right - m_rectGraph.left + 1;   // Actual data area length in pixels
   nDataAreaHeight = rect.bottom - iBottomMargin - textSize.cy / 2 - iTopMargin - textSize.cy / 2;

   // Calculate how many seconds represent each pixel
   // and select time stamp label's style
   m_dSecondsPerPixel = (double)(m_dwTimeTo - m_dwTimeFrom) / (double)iGraphLen;
   if (m_dwTimeTo - m_dwTimeFrom >= 10368000)   // 120 days
   {
      iTimeLen = dc.GetTextExtent(_T("MMM")).cx;
      nTimeLabel = TS_MONTH;
   }
   else if (m_dwTimeTo - m_dwTimeFrom >= 432000)   // 5 days
   {
      iTimeLen = dc.GetTextExtent(_T("MMM/00")).cx;
      nTimeLabel = TS_DAY_AND_MONTH;
      nGridSizeX = (int)ceil(86400.0 / m_dSecondsPerPixel);
   }
   else
   {
      iTimeLen = dc.GetTextExtent(_T("00:00:00")).cx;
      nTimeLabel = TS_LONG_TIME;
      nGridSizeX = 40;
   }

   // Calculate min and max graph value
   if (m_bAutoScale)
   {
      for(i = 0, m_dCurrMaxValue = 0, m_dCurrMinValue = 0, dMaxAbsValue = 0; i < MAX_GRAPH_ITEMS; i++)
         if (m_pData[i] != NULL)
         {
            NXC_DCI_ROW *pRow;
            double dCurrValue;
            DWORD j;

				// Include thresholds into evaluation
				if ((m_graphItemStyles[i].bShowThresholds) && (m_thresholds[i] != NULL))
				{
					for(j = 0; j < m_numThresholds[i]; j++)
					{
						dCurrValue = _tcstod(m_thresholds[i][j].value, NULL);
						if (dCurrValue > 0)
						{
							if (dCurrValue > m_dCurrMaxValue)
								m_dCurrMaxValue = dCurrValue;
							if (dCurrValue > dMaxAbsValue)
								dMaxAbsValue = dCurrValue;
						}
						else
						{
							if (dCurrValue < m_dCurrMinValue)
								m_dCurrMinValue = dCurrValue;
							if (-dCurrValue > dMaxAbsValue)
								dMaxAbsValue = -dCurrValue;
						}
					}
				}

            // Skip values beyond right graph border
            pRow = m_pData[i]->pRows;
            for(j = 0; (j < m_pData[i]->dwNumRows) && (pRow->dwTimeStamp > m_dwTimeTo); j++)
               inc_ptr(pRow, m_pData[i]->wRowSize, NXC_DCI_ROW);

            for(; (j < m_pData[i]->dwNumRows) && (pRow->dwTimeStamp >= m_dwTimeFrom); j++)
            {
               dCurrValue = (double)ROW_DATA(pRow, m_pData[i]->wDataType);
					if (dCurrValue > 0)
					{
						if (dCurrValue > m_dCurrMaxValue)
							m_dCurrMaxValue = dCurrValue;
						if (dCurrValue > dMaxAbsValue)
							dMaxAbsValue = dCurrValue;
					}
					else
					{
						if (dCurrValue < m_dCurrMinValue)
							m_dCurrMinValue = dCurrValue;
						if (-dCurrValue > dMaxAbsValue)
							dMaxAbsValue = -dCurrValue;
					}
               inc_ptr(pRow, m_pData[i]->wRowSize, NXC_DCI_ROW);
            }
         }

      if (m_dCurrMaxValue == 0)
         m_dCurrMaxValue = 1;

		if (dMaxAbsValue == 0)
			dMaxAbsValue = 1;

		theApp.DebugPrintf(_T("DrawGraph: currMinValue=%f currMaxValue=%f maxAbsValue=%f"),
		                   m_dCurrMinValue, m_dCurrMaxValue, dMaxAbsValue);

      // Round max value
      for(double d = 0.00001; d < 10000000000000000000; d *= 10)
		{
         if ((dMaxAbsValue >= d) && (dMaxAbsValue <= d * 10))
         {
            m_dCurrMaxValue -= fmod(m_dCurrMaxValue, d);
            m_dCurrMaxValue += d;

				if (m_dCurrMinValue < 0)
				{
					m_dCurrMinValue += fmod(m_dCurrMinValue, d);
					m_dCurrMinValue -= d;
				}

            SELECT_ORDINATE_MARKS;
   
            // For integer values, Y axis step cannot be less than 1
            if (bIntMarks && (d < 1))
               d = 1;

            // Calculate grid size for Y axis
            nGridSizeY = (int)(nDataAreaHeight / ((m_dCurrMaxValue - m_dCurrMinValue) / d));
            if (nGridSizeY > 2)
            {
               if (bIntMarks)
               {
                  nGrids = nDataAreaHeight / (nGridSizeY / 2);
                  while((nGridSizeY >= 50) && ((INT64)(m_dCurrMaxValue - m_dCurrMinValue) % nGrids == 0))
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
		theApp.DebugPrintf(_T("DrawGraph: gridSize=%d; rounded currMinValue=%f currMaxValue=%f"), nGridSizeY, m_dCurrMinValue, m_dCurrMaxValue);
   }
   else
   {
      m_dCurrMaxValue = m_dMaxValue;
		m_dCurrMinValue = m_dMinValue;
		dMaxAbsValue = max(m_dCurrMaxValue, fabs(m_dCurrMinValue));

      SELECT_ORDINATE_MARKS;
      
      nGridSizeY = 40;
   }

	// Determine zero line
	m_nZeroLine = m_rectGraph.bottom + (int)((double)(m_rectGraph.bottom - m_rectGraph.top) * (m_dCurrMinValue / (m_dCurrMaxValue - m_dCurrMinValue)));
	theApp.DebugPrintf(_T("DrawGraph: zeroLine=%d top=%d bottom=%d"), m_nZeroLine, m_rectGraph.top, m_rectGraph.bottom);

   // Draw grid
   if (m_bShowGrid)
   {
      pen.CreatePen(PS_ALTERNATE | PS_COSMETIC, 0, m_rgbGridColor);
      pOldPen = dc.SelectObject(&pen);
      if (nTimeLabel == TS_MONTH)
      {
         x = iLeftMargin + NextMonthOffset(m_dwTimeFrom);
      }
      else
      {
         x = iLeftMargin + nGridSizeX;
      }
      while(x < rect.right - iRightMargin)
      {
         dc.MoveTo(x, rect.bottom - iBottomMargin);
         dc.LineTo(x, iTopMargin);
         if (nTimeLabel == TS_MONTH)
         {
            dwTimeStamp = m_dwTimeFrom + (DWORD)((double)(x - iLeftMargin) * m_dSecondsPerPixel);
            x += NextMonthOffset(dwTimeStamp);
            CORRECT_MONTH_OFFSET;
         }
         else
         {
            x += nGridSizeX;
         }
      }
      for(y = m_nZeroLine; y >= iTopMargin; y -= nGridSizeY)
      {
         dc.MoveTo(iLeftMargin, y);
         dc.LineTo(rect.right - iRightMargin, y);
      }
      for(y = m_nZeroLine; y <= rect.bottom - iBottomMargin; y += nGridSizeY)
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
	{
      if (m_pData[i] != NULL)
		{
			switch(m_graphItemStyles[i].nType)
			{
				case GRAPH_TYPE_LINE:
					DrawLineGraph(dc, m_pData[i], &m_graphItemStyles[i], nGridSizeY);
					break;
				case GRAPH_TYPE_AREA:
					DrawAreaGraph(dc, m_pData[i], &m_graphItemStyles[i], nGridSizeY);
					break;
				default:
					break;
			}

			DrawAverage(dc, m_pData[i], &m_graphItemStyles[i], nGridSizeY);
			DrawTrendLine(dc, m_pData[i], &m_graphItemStyles[i], nGridSizeY);
			DrawThresholds(dc, m_thresholds[i], m_numThresholds[i], &m_graphItemStyles[i], nGridSizeY);
		}
	}
   dc.SelectClipRgn(NULL);
   rgn.DeleteObject();

   // Paint ordinates
   pen.CreatePen(PS_SOLID, 3, m_rgbAxisColor);
   pOldPen = dc.SelectObject(&pen);
   dc.MoveTo(iLeftMargin, rect.bottom - iBottomMargin);
   dc.LineTo(iLeftMargin, iTopMargin);
   dc.MoveTo(iLeftMargin, m_nZeroLine);
   dc.LineTo(rect.right - iRightMargin, m_nZeroLine);
   dc.SelectObject(pOldPen);
   pen.DeleteObject();

   // Display ordinate marks
   dStep = (m_dCurrMaxValue - m_dCurrMinValue) / ((rect.bottom - iBottomMargin - iTopMargin) / nGridSizeY);
   for(y = m_nZeroLine - textSize.cy / 2, dMark = 0; y > iTopMargin; y -= nGridSizeY, dMark += dStep)
   {
		DISPLAY_Y_MARK;
   }
   for(y = m_nZeroLine + nGridSizeY - textSize.cy / 2, dMark = -dStep; y < rect.bottom - iBottomMargin; y += nGridSizeY, dMark -= dStep)
   {
		DISPLAY_Y_MARK;
   }

   // Display absciss marks
   y = rect.bottom - iBottomMargin + 3;
	if (nTimeLabel != TS_MONTH)
		iStep = iTimeLen / nGridSizeX + 1;    // How many grid lines we should skip
   for(x = iLeftMargin; x < rect.right - iRightMargin;)
   {
      dwTimeStamp = m_dwTimeFrom + (DWORD)((double)(x - iLeftMargin) * m_dSecondsPerPixel);
      FormatTimeStamp(dwTimeStamp, szBuffer, nTimeLabel);
      dc.TextOut(x, y, szBuffer);
      if (nTimeLabel == TS_MONTH)
      {
         x += NextMonthOffset(dwTimeStamp);
         CORRECT_MONTH_OFFSET;
      }
      else
      {
         x += nGridSizeX * iStep;
      }
   }

   // Draw legend
   if (m_bShowLegend)
   {
      pen.CreatePen(PS_SOLID, 0, m_rgbTextColor);
      pOldPen = dc.SelectObject(&pen);
      for(i = 0, nCurrCol = 0, x = m_rectGraph.left,
               y = m_rectGraph.bottom + textSize.cy + 8;
          i < MAX_GRAPH_ITEMS; i++)
         if (m_pData[i] != NULL)
         {
            brush.CreateSolidBrush(m_graphItemStyles[i].rgbColor);
            pOldBrush = dc.SelectObject(&brush);
            dc.Rectangle(x, y, x + textSize.cy, y + textSize.cy);
            dc.SelectObject(pOldBrush);
            brush.DeleteObject();

            if (m_ppItems != NULL)
               if (m_ppItems[i] != NULL)
               {
						if (m_bShowHostNames)
						{
							NXC_OBJECT *object;
							TCHAR temp[1024];

							object = NXCFindObjectById(g_hSession, m_ppItems[i]->m_dwNodeId);
							_sntprintf_s(temp, 1024, _TRUNCATE, _T("%s - %s"), (object != NULL) ? object->szName : _T("(null)"),
										 m_ppItems[i]->m_pszDescription);
							dc.TextOut(x + 14, y, temp, (int)_tcslen(temp));
						}
						else
						{
							dc.TextOut(x + 14, y, m_ppItems[i]->m_pszDescription,
										  (int)_tcslen(m_ppItems[i]->m_pszDescription));
						}
                  nCurrCol++;
                  if (nCurrCol == nCols)
                  {
                     nCurrCol = 0;
                     x = m_rectGraph.left;
                     y += textSize.cy + LEGEND_TEXT_SPACING;
                  }
                  else
                  {
                     x += nColSize;
                  }
               }
         }
      dc.SelectObject(pOldPen);
      pen.DeleteObject();
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
   RECT rect;

   GetClientRect(&rect);
   DrawGraphOnBitmap(m_bmpGraph, rect);
   Invalidate(FALSE);
}


//
// WM_SIZE message handler
//

void CGraph::OnSize(UINT nType, int cx, int cy) 
{
   RECT rect;

   GetClientRect(&rect);
   DrawGraphOnBitmap(m_bmpGraph, rect);
	CWnd::OnSize(nType, cx, cy);
}


//
// Calculate offset (in pixels) of next month start
//

int CGraph::NextMonthOffset(DWORD dwTimeStamp)
{
   static double nSecPerMonth[12] = { 2678400, 2419200, 2678400, 2592000,
                                      2678400, 2592000, 2678400, 2678400,
                                      2592000, 2678400, 2592000, 2678400 };
   struct tm lt;
   time_t t = dwTimeStamp;

   localtime_s(&lt, &t);
   if ((lt.tm_year % 4 == 0) && (lt.tm_mon == 1))
      return (int)ceil(2505600.0 / m_dSecondsPerPixel) + 1;
   else
      return (int)ceil(nSecPerMonth[lt.tm_mon] / m_dSecondsPerPixel) + 1;
}


//
// WM_MOUSEMOVE message handler
//

void CGraph::OnMouseMove(UINT nFlags, CPoint point) 
{
   BOOL bChanged;
   RECT rcOldSel, rcUpdate;

   if (!m_bIsActive)
      return;

   if (PtInRect(&m_rectGraph, point))
   {
      DWORD dwTimeStamp;
      double dValue;

      dwTimeStamp = m_dwTimeFrom + (DWORD)((point.x - m_rectGraph.left) * m_dSecondsPerPixel);
      dValue = (m_dCurrMaxValue - m_dCurrMinValue) / (m_rectGraph.bottom - m_rectGraph.top - 
                  (m_rectGraph.bottom - m_rectGraph.top) % m_nLastGridSizeY) * 
                  (m_nZeroLine - point.y);
		if (m_bLogarithmicScale)
			dValue = pow(10.0, dValue);
      GetParent()->SendMessage(NXCM_UPDATE_GRAPH_POINT, dwTimeStamp, (LPARAM)&dValue);
      bChanged = TRUE;
   }
   else
   {
      bChanged = PtInRect(&m_rectGraph, m_ptCurrMousePos);
      if (bChanged)
         GetParent()->SendMessage(NXCM_UPDATE_GRAPH_POINT, 0, 0);
   }
   m_ptCurrMousePos = point;

   switch(m_nState)
   {
      case STATE_NORMAL:
         if (bChanged)
            InvalidateRect(&m_rectGraph, FALSE);
         break;
      case STATE_SELECTING:
         CopyRect(&rcOldSel, &m_rcSelection);

         // Calculate normalized selection rectangle
         if (point.x < m_ptMouseOpStart.x)
         {
            m_rcSelection.left = point.x;
            m_rcSelection.right = m_ptMouseOpStart.x;
         }
         else
         {
            m_rcSelection.left = m_ptMouseOpStart.x;
            m_rcSelection.right = point.x;
         }
         if (point.y < m_ptMouseOpStart.y)
         {
            m_rcSelection.top = point.y;
            m_rcSelection.bottom = m_ptMouseOpStart.y;
         }
         else
         {
            m_rcSelection.top = m_ptMouseOpStart.y;
            m_rcSelection.bottom = point.y;
         }

         // Validate selection rectangle
         if (m_rcSelection.left < m_rectGraph.left)
            m_rcSelection.left = m_rectGraph.left;
         if (m_rcSelection.top < m_rectGraph.top)
            m_rcSelection.top = m_rectGraph.top;
         if (m_rcSelection.right > m_rectGraph.right)
            m_rcSelection.right = m_rectGraph.right;
         if (m_rcSelection.bottom > m_rectGraph.bottom)
            m_rcSelection.bottom = m_rectGraph.bottom;
         
         // Update changed region
         UnionRect(&rcUpdate, &rcOldSel, &m_rcSelection);
         InvalidateRect(&rcUpdate, FALSE);
         break;
      default:
         break;
   }
}


//
// WM_LBUTTONDOWN message handler
//

void CGraph::OnLButtonDown(UINT nFlags, CPoint point) 
{
   if (m_bEnableZoom)
   {
      m_ptMouseOpStart = point;
      m_rcSelection.left = m_rcSelection.right = point.x;
      m_rcSelection.top = m_rcSelection.bottom = point.y;
      m_nState = STATE_SELECTING;
      InvalidateRect(&m_rectGraph, FALSE);
      SetCapture();
   }
}


//
// WM_LBUTTONUP message handler
//

void CGraph::OnLButtonUp(UINT nFlags, CPoint point) 
{
   if (m_nState == STATE_SELECTING)
   {
      ReleaseCapture();
      m_nState = STATE_NORMAL;
      if ((m_rcSelection.right - m_rcSelection.left > 2) && 
          (m_rcSelection.bottom - m_rcSelection.top > 2))
      {
         ZoomIn(m_rcSelection);
      }
      else
      {
         InvalidateRect(&m_rectGraph, FALSE);
      }
   }
}


//
// Zoom graph based on selection rectangle
// If graph mode is set to autoscale, Y axis is ignored
//

void CGraph::ZoomIn(RECT &rect)
{
   if (m_nZoomLevel >= ZOOM_HISTORY_SIZE)
      return;

   m_zoomInfo[m_nZoomLevel].dwTimeFrom = m_dwTimeFrom;
   m_zoomInfo[m_nZoomLevel].dwTimeTo = m_dwTimeTo;
   m_nZoomLevel++;
   SetTimeFrame(m_dwTimeFrom + (DWORD)((rect.left - m_rectGraph.left) * m_dSecondsPerPixel),
                m_dwTimeFrom + (DWORD)((rect.right - m_rectGraph.left) * m_dSecondsPerPixel));
   Update();
   GetParent()->PostMessage(NXCM_GRAPH_ZOOM_CHANGED, m_nZoomLevel, 0);
}


//
// Restore graph to state before last zoom-in operation
//

void CGraph::ZoomOut()
{
   if (m_nZoomLevel == 0)
      return;

   m_nZoomLevel--;
   SetTimeFrame(m_zoomInfo[m_nZoomLevel].dwTimeFrom, m_zoomInfo[m_nZoomLevel].dwTimeTo);
   Update();
   GetParent()->PostMessage(NXCM_GRAPH_ZOOM_CHANGED, m_nZoomLevel, 0);
}


//
// Returns TRUE if graph can be zoomed in
//

BOOL CGraph::CanZoomIn()
{
   return m_bEnableZoom && (m_nZoomLevel < ZOOM_HISTORY_SIZE);
}


//
// Returns TRUE if graph can be zoomed out
//

BOOL CGraph::CanZoomOut()
{
   return m_nZoomLevel > 0;
}


//
// Clear zoom history
//

void CGraph::ClearZoomHistory()
{
   m_nZoomLevel = 0;
   GetParent()->PostMessage(NXCM_GRAPH_ZOOM_CHANGED, 0, 0);
}


//
// Get time frame for data that need to be updated
//

BOOL CGraph::GetTimeFrameForUpdate(DWORD dwIndex, DWORD *pdwFrom, DWORD *pdwTo)
{
   if (dwIndex >= MAX_GRAPH_ITEMS)
      return FALSE;

   if (m_pData[dwIndex] == NULL)
      return FALSE;

   if (m_pData[dwIndex]->dwNumRows == 0)
      return FALSE;
      
   *pdwFrom = m_pData[dwIndex]->pRows->dwTimeStamp + 1;
   *pdwTo = m_dwTimeTo;

   return TRUE;
}


//
// Set predefined color scheme
//

void CGraph::SetColorScheme(int nScheme)
{
	switch(nScheme)
	{
		case GCS_CLASSIC:
			m_rgbBkColor = RGB(0, 0, 0);
			m_rgbGridColor = RGB(64, 64, 64);
			m_rgbAxisColor = RGB(127, 127, 127);
			m_rgbRulerColor = RGB(127, 127, 127);
			m_rgbTextColor = RGB(255, 255, 255);
			m_rgbSelRectColor = RGB(0, 255, 255);
			m_graphItemStyles[0].rgbColor = RGB(0, 255, 0);
			m_graphItemStyles[1].rgbColor = RGB(255, 255, 0);
			m_graphItemStyles[2].rgbColor = RGB(0, 255, 255);
			m_graphItemStyles[3].rgbColor = RGB(0, 0, 255);
			m_graphItemStyles[4].rgbColor = RGB(255, 0, 255);
			m_graphItemStyles[5].rgbColor = RGB(255, 0, 0);
			m_graphItemStyles[6].rgbColor = RGB(0, 128, 128);
			m_graphItemStyles[7].rgbColor = RGB(0, 128, 0);
			m_graphItemStyles[8].rgbColor = RGB(128, 128, 255);
			m_graphItemStyles[9].rgbColor = RGB(255, 128, 0);
			m_graphItemStyles[10].rgbColor = RGB(128, 128, 0);
			m_graphItemStyles[11].rgbColor = RGB(128, 0, 255);
			m_graphItemStyles[12].rgbColor = RGB(255, 255, 128);
			m_graphItemStyles[13].rgbColor = RGB(0, 128, 64);
			m_graphItemStyles[14].rgbColor = RGB(0, 128, 255);
			m_graphItemStyles[15].rgbColor = RGB(192, 192, 192);
			break;
		case GCS_LIGHT:
			m_rgbBkColor = RGB(255, 255, 255);
			m_rgbGridColor = RGB(56, 142, 142);
			m_rgbAxisColor = RGB(56, 142, 142);
			m_rgbRulerColor = RGB(0, 0, 0);
			m_rgbTextColor = RGB(0, 0, 0);
			m_rgbSelRectColor = RGB(0, 0, 94);
			m_graphItemStyles[0].rgbColor = RGB(0, 147, 0);
			m_graphItemStyles[1].rgbColor = RGB(165, 42, 0);
			m_graphItemStyles[2].rgbColor = RGB(0, 64, 64);
			m_graphItemStyles[3].rgbColor = RGB(0, 0, 255);
			m_graphItemStyles[4].rgbColor = RGB(255, 0, 255);
			m_graphItemStyles[5].rgbColor = RGB(255, 0, 0);
			m_graphItemStyles[6].rgbColor = RGB(0, 128, 128);
			m_graphItemStyles[7].rgbColor = RGB(0, 128, 0);
			m_graphItemStyles[8].rgbColor = RGB(128, 128, 255);
			m_graphItemStyles[9].rgbColor = RGB(255, 128, 0);
			m_graphItemStyles[10].rgbColor = RGB(128, 128, 0);
			m_graphItemStyles[11].rgbColor = RGB(128, 0, 255);
			m_graphItemStyles[12].rgbColor = RGB(0, 0, 0);
			m_graphItemStyles[13].rgbColor = RGB(0, 128, 64);
			m_graphItemStyles[14].rgbColor = RGB(0, 128, 255);
			m_graphItemStyles[15].rgbColor = RGB(192, 192, 192);
			break;
		default:
			break;
	}
}


//
// Set thresholds information
//

void CGraph::SetThresholds(DWORD index, DWORD numThresholds, NXC_DCI_THRESHOLD *thresholds)
{
	safe_free(m_thresholds[index]);
	m_thresholds[index] = thresholds;
	m_numThresholds[index] = numThresholds;
}
