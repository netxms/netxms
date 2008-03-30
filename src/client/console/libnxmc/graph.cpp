/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: graph.cpp
**
**/

#include "libnxmc.h"


//
// Constants and macros
//

#define ROW_DATA(row, dt)  ((dt == DCI_DT_STRING) ? _tcstod(row->value.szString, NULL) : \
                            (((dt == DCI_DT_INT) || (dt == DCI_DT_UINT)) ? row->value.dwInt32 : \
                             (((dt == DCI_DT_INT64) || (dt == DCI_DT_UINT64)) ? (INT64)row->value.qwInt64 : \
                              ((dt == DCI_DT_FLOAT) ? row->value.dFloat : 0) \
                             ) \
                            ) \
                           )

#define LEGEND_TEXT_SPACING   4


//
// Event table
//

BEGIN_EVENT_TABLE(nxGraph, wxWindow)
	EVT_PAINT(nxGraph::OnPaint)
	EVT_SET_FOCUS(nxGraph::OnSetFocus)
	EVT_KILL_FOCUS(nxGraph::OnKillFocus)
END_EVENT_TABLE()


//
// Get month from timestamp
//

static int MonthFromTS(time_t timeStamp)
{
	struct tm *ltm;
	
	ltm = localtime(&timeStamp);
   return ltm != NULL ? ltm->tm_mon : -1;
}


//
// Constructor
//

nxGraph::nxGraph(wxWindow *parent, const wxPoint& pos, const wxSize& size)
        : wxWindow(parent, wxID_ANY, pos, size)
{
	m_bitmap = NULL;

	m_flags = NXGF_SHOW_LEGEND | NXGF_SHOW_GRID | NXGF_AUTOSCALE;
   m_maxValue = 100;
   m_numItems = 0;
//	m_szTitle[0] = 0;
	m_timeFrom = 0;
	m_timeTo = 0;
   m_zoomLevel = 0;

	memset(m_graphItemStyles, 0, sizeof(GRAPH_ITEM_STYLE) * MAX_GRAPH_ITEMS);
	SetColorScheme(GCS_CLASSIC);

   memset(m_data, 0, sizeof(NXC_DCI_DATA *) * MAX_GRAPH_ITEMS);
   m_dciInfo = NULL;
}


//
// Destructor
//

nxGraph::~nxGraph()
{
	int i;

	delete m_bitmap;
   
	for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      if (m_data[i] != NULL)
         NXCDestroyDCIData(m_data[i]);
}


//
// Set predefined color scheme
//

void nxGraph::SetColorScheme(int scheme)
{
	switch(scheme)
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
// Paint event handler
//

void nxGraph::OnPaint(wxPaintEvent &event)
{
	wxPaintDC sdc(this);  // original device context for painting
   wxMemoryDC dc;              // In-memory dc
   wxSize size;

	if (m_bitmap == NULL)
		return;

   size = GetClientSize();

   //dc.CreateCompatibleDC(&sdc);
	dc.SelectObjectAsSource(*m_bitmap);

   // Draw ruler
   if (IsRulerVisible() && (m_flags & NXGF_IS_ACTIVE) && (!(m_flags & NXGF_IS_SELECTING)) &&
       m_rectGraph.Contains(m_currMousePos))
   {
      wxPen pen(m_rgbRulerColor, 1, wxDOT);
      wxBitmap bitmap;         // Bitmap for in-memory drawing
      wxMemoryDC dc2;

      // Create another one in-memory DC to draw ruler on top of graph image
      bitmap.Create(size.x, size.y);
      dc2.SelectObject(bitmap);

      // Move drawing from in-memory DC to in-memory DC #2
      dc2.Blit(0, 0, size.x, size.y, &dc, 0, 0);
      dc2.SetPen(pen);

//      dc2.SetBkColor(m_rgbBkColor);
      dc2.DrawLine(m_rectGraph.x, m_currMousePos.y, m_rectGraph.x + m_rectGraph.width, m_currMousePos.y);
      dc2.DrawLine(m_currMousePos.x, m_rectGraph.y, m_currMousePos.x, m_rectGraph.y + m_rectGraph.height);

      // Move drawing from in-memory DC #2 to screen
      sdc.Blit(0, 0, size.x, size.y, &dc2, 0, 0);
   }
   else
   {
      if (m_flags & NXGF_IS_SELECTING)
      {
/*         CPen pen, *pOldPen;
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
         sdc.Blit(0, 0, size.x, size.y, &dcGraph, 0, 0);

         dcGraph.SelectObject(pOldGraphBitmap);
         dcGraph.DeleteDC();*/
      }
      else
      {
         // Move drawing from in-memory DC to screen
         sdc.Blit(0, 0, size.x, size.y, &dc, 0, 0);
      }
   }

	// Draw "Updating..." message
	if (m_flags & NXGF_IS_UPDATING)
	{
		wxRect rcText;
		wxSize size;
		wxPen pen(wxColour(255, 0, 0));
		wxBrush brush(wxColour(64, 0, 0));

		size = sdc.GetTextExtent(_T("Updating..."));
		rcText.x = 10;
		rcText.y = 10;
		rcText.width = size.x + 10;
		rcText.height = size.y + 8;
		sdc.SetTextBackground(RGB(64, 0, 0));
		sdc.SetTextForeground(RGB(255, 0, 0));
		sdc.SetPen(pen);
		sdc.SetBrush(brush);
		sdc.DrawRectangle(rcText.x, rcText.y, rcText.width, rcText.height);
		rcText.Deflate(1, 1);
		sdc.DrawLabel(_T("Updating..."), rcText, wxALIGN_CENTER);
	}
}


//
// Handler for "set focus" event
//

void nxGraph::OnSetFocus(wxFocusEvent &event)
{
	m_flags |= NXGF_IS_ACTIVE;
	event.Skip();
}


//
// Handler for "kill focus" event
//

void nxGraph::OnKillFocus(wxFocusEvent &event)
{
	m_flags &= ~NXGF_IS_ACTIVE;
   if (IsRulerVisible() && m_rectGraph.Contains(m_currMousePos))
   {
      m_currMousePos = wxPoint(0, 0);
      RefreshRect(m_rectGraph, false);
   }
	event.Skip();
}


//
// Draw single line
//

void nxGraph::DrawLineGraph(wxMemoryDC &dc, NXC_DCI_DATA *data, wxColour rgbColor, int gridSize)
{
   DWORD i;
   int x, y, lastX, lastY;
   wxPen pen(rgbColor, 2);
   NXC_DCI_ROW *pRow;
   double dScale;

   if (data->dwNumRows < 2)
      return;  // Nothing to draw

   dc.SetPen(pen);

   // Calculate scale factor for values
   dScale = (double)(m_rectGraph.height - m_rectGraph.height % gridSize) / m_currMaxValue;

   // Move to first position
   pRow = data->pRows;
   for(i = 0; (i < data->dwNumRows) && ((time_t)pRow->dwTimeStamp > m_timeTo); i++)
      inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);
   if (i < data->dwNumRows)
   {
      lastX = m_rectGraph.GetRight() - (int)((double)(m_timeTo - (time_t)pRow->dwTimeStamp) / m_secondsPerPixel);
      lastY = (int)(m_rectGraph.GetBottom() - (double)ROW_DATA(pRow, data->wDataType) * dScale - 1);
      inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);

      for(i++; (i < data->dwNumRows) && ((time_t)pRow->dwTimeStamp >= m_timeFrom); i++)
      {
         // Calculate timestamp position on graph
         x = m_rectGraph.GetRight() - (int)((double)(m_timeTo - pRow->dwTimeStamp) / m_secondsPerPixel);
			y = (int)(m_rectGraph.GetBottom() - (double)ROW_DATA(pRow, data->wDataType) * dScale - 1);
         dc.DrawLine(lastX, lastY, x, y);
         inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);
			lastX = x;
			lastY = y;
      }
   }

   dc.SetPen(wxNullPen);
}


//
// Draw single area
//

void nxGraph::DrawAreaGraph(wxMemoryDC &dc, NXC_DCI_DATA *data, wxColour rgbColor, int gridSize)
{
   DWORD i;
   wxPen pen(rgbColor, 1);
	wxBrush brush(rgbColor);
	wxPoint pts[4];
   NXC_DCI_ROW *pRow;
   double dScale;

   if (data->dwNumRows < 2)
      return;  // Nothing to draw

   dc.SetPen(pen);
	dc.SetBrush(brush);

   // Calculate scale factor for values
   dScale = (double)(m_rectGraph.height - m_rectGraph.height % gridSize) / m_currMaxValue;

   // Move to first position
   pRow = data->pRows;
   for(i = 0; (i < data->dwNumRows) && ((time_t)pRow->dwTimeStamp > m_timeTo); i++)
      inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);
   if (i < data->dwNumRows)
   {
		pts[0].x = m_rectGraph.GetRight() - (int)((double)(m_timeTo - (time_t)pRow->dwTimeStamp) / m_secondsPerPixel);
		pts[0].y = m_rectGraph.GetBottom();
		pts[1].x = pts[0].x;
		pts[1].y = (int)(m_rectGraph.GetBottom() - (double)ROW_DATA(pRow, data->wDataType) * dScale - 1);
      inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);

      for(i++; (i < data->dwNumRows) && ((time_t)pRow->dwTimeStamp >= m_timeFrom); i++)
      {
         // Calculate timestamp position on graph
         pts[2].x = m_rectGraph.GetRight() - (int)((double)(m_timeTo - (time_t)pRow->dwTimeStamp) / m_secondsPerPixel);
			pts[2].y = (int)(m_rectGraph.GetBottom() - (double)ROW_DATA(pRow, data->wDataType) * dScale - 1);
			pts[3].x = pts[2].x;
			pts[3].y = m_rectGraph.GetBottom();
			dc.DrawPolygon(4, pts);
			memcpy(&pts[0], &pts[3], sizeof(POINT));
			memcpy(&pts[1], &pts[2], sizeof(POINT));
         inc_ptr(pRow, data->wRowSize, NXC_DCI_ROW);
      }
   }

   dc.SetPen(wxNullPen);
	dc.SetBrush(wxNullBrush);
}


//
// Draw entire graph on bitmap in memory
//

/* Select appropriate style for ordinate marks */
#define SELECT_ORDINATE_MARKS \
   if (m_currMaxValue > 100000000000) \
   { \
      szModifier[0] = 'G'; \
      szModifier[1] = 0; \
      nDivider = 1000000000; \
      bIntMarks = TRUE; \
   } \
   else if (m_currMaxValue > 100000000) \
   { \
      szModifier[0] = 'M'; \
      szModifier[1] = 0; \
      nDivider = 1000000; \
      bIntMarks = TRUE; \
   } \
   else if (m_currMaxValue > 100000) \
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
         if (m_data[i] != NULL) \
         { \
            if ((m_data[i]->wDataType == DCI_DT_FLOAT) || \
                (m_data[i]->wDataType == DCI_DT_STRING)) \
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
      timeStamp = m_timeFrom + (DWORD)((double)(x - iLeftMargin) * m_secondsPerPixel); \
      nMonth = MonthFromTS(timeStamp); \
		if (nMonth != -1) \
			while(1) \
			{ \
				timeStamp = m_timeFrom + (DWORD)((double)(x - iLeftMargin - 1) * m_secondsPerPixel); \
				if (MonthFromTS(timeStamp) != nMonth) \
					break; \
				x--; \
			} \
   }

wxBitmap *nxGraph::DrawGraphOnBitmap(wxSize &size)
{
   wxMemoryDC dc;           // Window dc and in-memory dc
	wxBitmap *bitmap;
   wxPen pen;
   wxFont font(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Verdana"));
   wxBrush brushBkgnd(m_rgbBkColor);
   wxSize textSize;
   DWORD i;
	time_t timeStamp;
   int iLeftMargin, iBottomMargin, iRightMargin = 5, iTopMargin = 5;
   int x, y, iTimeLen, iStep, iGraphLen, nDivider;
   int nGridSizeX, nGridSizeY, nGrids, nDataAreaHeight;
   int nColSize, nCols, nCurrCol, nTimeLabel;
   double dStep, dMark;
   TCHAR szBuffer[256], szModifier[4];
   BOOL bIntMarks;
   static double nSecPerMonth[12] = { 2678400, 2419200, 2678400, 2592000,
                                      2678400, 2592000, 2678400, 2678400,
                                      2592000, 2678400, 2592000, 2678400 };

   // Create bitmap for painting
	bitmap = new wxBitmap(size.x, size.y);

   // Initial DC setup
   dc.SelectObject(*bitmap);
	dc.SetFont(font);
   dc.SetTextForeground(m_rgbTextColor);
   dc.SetTextBackground(m_rgbBkColor);

   // Fill background
	dc.SetBrush(brushBkgnd);
	dc.DrawRectangle(-1, -1, size.x + 2, size.y + 2);

   // Calculate text size and left margin
   textSize = dc.GetTextExtent(_T("0000.000"));
   iLeftMargin = textSize.x + 10;
   if (IsLegendVisible())
   {
      wxSize size;

      for(i = 0, nColSize = 0; i < MAX_GRAPH_ITEMS; i++)
      {
         if (m_dciInfo != NULL)
            if (m_dciInfo[i] != NULL)
            {
               size = dc.GetTextExtent(m_dciInfo[i]->m_pszDescription);
               if (size.x > nColSize)
                  nColSize = size.x;
            }
      }
      nColSize += textSize.y + 20;
      nCols = (size.x - iLeftMargin - iRightMargin) / nColSize;
      if (nCols == 0)
         nCols = 1;

      iBottomMargin = textSize.y + 12;
      for(i = 0, nCurrCol = 0; i < MAX_GRAPH_ITEMS; i++)
         if (m_data[i] != NULL)
         {
            if (nCurrCol == 0)
               iBottomMargin += textSize.y + LEGEND_TEXT_SPACING;
            nCurrCol++;
            if (nCurrCol == nCols)
               nCurrCol = 0;
         }
   }
   else
   {
      iBottomMargin = textSize.y + 8;
   }

	// Draw title if needed and adjust top margin
	if (IsTitleVisible())
	{
		wxRect rcTitle;

		rcTitle.x = iLeftMargin;
		rcTitle.y = iTopMargin;
		rcTitle.width = size.x - iRightMargin;
		rcTitle.height = rcTitle.y + textSize.y;
		dc.DrawLabel(m_title, rcTitle, DT_CENTER);
		iTopMargin += textSize.y + 7;
	}

   // Calculate data rectangle
   m_rectGraph.x = iLeftMargin;
   m_rectGraph.y = iTopMargin;
   m_rectGraph.width = size.x - iRightMargin;
   m_rectGraph.height = size.y - iBottomMargin;
   iGraphLen = m_rectGraph.width + 1;   // Actual data area length in pixels
   nDataAreaHeight = size.y - iBottomMargin - textSize.y / 2 - iTopMargin - textSize.y / 2;

   // Calculate how many seconds represent each pixel
   // and select time stamp label's style
   m_secondsPerPixel = (double)(m_timeTo - m_timeFrom) / (double)iGraphLen;
   if (m_timeTo - m_timeFrom >= 10368000)   // 120 days
   {
      iTimeLen = dc.GetTextExtent(_T("MMM")).x;
      nTimeLabel = TS_MONTH;
//      nGridSizeX = (int)(2592000 / m_secondsPerPixel);
   }
   else if (m_timeTo - m_timeFrom >= 432000)   // 5 days
   {
      iTimeLen = dc.GetTextExtent(_T("MMM/00")).x;
      nTimeLabel = TS_DAY_AND_MONTH;
      nGridSizeX = (int)ceil(86400.0 / m_secondsPerPixel);
   }
   else
   {
      iTimeLen = dc.GetTextExtent(_T("00:00:00")).x;
      nTimeLabel = TS_LONG_TIME;
      nGridSizeX = 40;
   }

   // Calculate max graph value
   if (IsAutoscale())
   {
      for(i = 0, m_currMaxValue = 0; i < MAX_GRAPH_ITEMS; i++)
         if (m_data[i] != NULL)
         {
            NXC_DCI_ROW *pRow;
            double dCurrValue;
            DWORD j;

            // Skip values beyond right graph border
            pRow = m_data[i]->pRows;
            for(j = 0; (j < m_data[i]->dwNumRows) && ((time_t)pRow->dwTimeStamp > m_timeTo); j++)
               inc_ptr(pRow, m_data[i]->wRowSize, NXC_DCI_ROW);

            for(; (j < m_data[i]->dwNumRows) && ((time_t)pRow->dwTimeStamp >= m_timeFrom); j++)
            {
               dCurrValue = (double)ROW_DATA(pRow, m_data[i]->wDataType);
               if (dCurrValue > m_currMaxValue)
                  m_currMaxValue = dCurrValue;
               inc_ptr(pRow, m_data[i]->wRowSize, NXC_DCI_ROW);
            }
         }

      if (m_currMaxValue == 0)
         m_currMaxValue = 1;

      // Round max value
      for(double d = 0.00001; d < 10000000000000000000; d *= 10)
         if ((m_currMaxValue >= d) && (m_currMaxValue <= d * 10))
         {
            m_currMaxValue -= fmod(m_currMaxValue, d);
            m_currMaxValue += d;

            SELECT_ORDINATE_MARKS;
   
            // For integer values, Y axis step cannot be less than 1
            if (bIntMarks && (d < 1))
               d = 1;

            // Calculate grid size for Y axis
            nGridSizeY = (int)(nDataAreaHeight / (m_currMaxValue / d));
            if (nGridSizeY > 2)
            {
               if (bIntMarks)
               {
                  nGrids = nDataAreaHeight / (nGridSizeY / 2);
                  while((nGridSizeY >= 50) && ((INT64)m_currMaxValue % nGrids == 0))
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
      m_currMaxValue = m_maxValue;

      SELECT_ORDINATE_MARKS;
      
      nGridSizeY = 40;
   }

   // Draw grid
   if (IsGridVisible())
   {
      wxPen penGrid(m_rgbGridColor, 1);
      dc.SetPen(penGrid);
      if (nTimeLabel == TS_MONTH)
      {
         x = iLeftMargin + NextMonthOffset(m_timeFrom);
      }
      else
      {
         x = iLeftMargin + nGridSizeX;
      }
      while(x < size.x - iRightMargin)
      {
         dc.DrawLine(x, size.y - iBottomMargin, x, iTopMargin);
         if (nTimeLabel == TS_MONTH)
         {
            timeStamp = m_timeFrom + (DWORD)((double)(x - iLeftMargin) * m_secondsPerPixel);
            x += NextMonthOffset(timeStamp);
            CORRECT_MONTH_OFFSET;
         }
         else
         {
            x += nGridSizeX;
         }
      }
      for(y = size.y - iBottomMargin - nGridSizeY; y > iTopMargin; y -= nGridSizeY)
      {
         dc.DrawLine(iLeftMargin, y, size.x - iRightMargin, y);
      }
		dc.SetPen(wxNullPen);
   }

   // Draw each parameter
   dc.SetClippingRegion(m_rectGraph);
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
      if (m_data[i] != NULL)
		{
			switch(m_graphItemStyles[i].type)
			{
				case GRAPH_TYPE_LINE:
					DrawLineGraph(dc, m_data[i], m_graphItemStyles[i].rgbColor, nGridSizeY);
					break;
				case GRAPH_TYPE_AREA:
					DrawAreaGraph(dc, m_data[i], m_graphItemStyles[i].rgbColor, nGridSizeY);
					break;
				default:
					break;
			}
		}
	}
   dc.DestroyClippingRegion();

   // Paint ordinates
   wxPen penAxis(m_rgbAxisColor, 3);
   dc.SetPen(penAxis);
   dc.DrawLine(iLeftMargin, size.y - iBottomMargin, iLeftMargin, iTopMargin);
   dc.DrawLine(iLeftMargin, size.y - iBottomMargin, size.x - iRightMargin, size.y - iBottomMargin);

   // Display ordinate marks
   dStep = m_currMaxValue / ((size.y - iBottomMargin - iTopMargin) / nGridSizeY);
   for(y = size.y - iBottomMargin - textSize.y / 2, dMark = 0; y > iTopMargin; y -= nGridSizeY, dMark += dStep)
   {
      if (bIntMarks)
         _stprintf(szBuffer, INT64_FMT _T("%s"), (INT64)dMark / nDivider, szModifier);
      else
         _stprintf(szBuffer, _T("%5.3f%s"), dMark, szModifier);
      wxSize cz = dc.GetTextExtent(szBuffer);
      dc.DrawText(szBuffer, iLeftMargin - cz.x - 5, y);
   }

   // Display absciss marks
   y = size.y - iBottomMargin + 3;
   iStep = iTimeLen / nGridSizeX + 1;    // How many grid lines we should skip
   for(x = iLeftMargin; x < size.x - iRightMargin;)
   {
      timeStamp = m_timeFrom + (time_t)((double)(x - iLeftMargin) * m_secondsPerPixel);
      NXMCFormatTimeStamp(timeStamp, szBuffer, nTimeLabel);
      dc.DrawText(szBuffer, x, y);
      if (nTimeLabel == TS_MONTH)
      {
         x += NextMonthOffset(timeStamp);
         CORRECT_MONTH_OFFSET;
      }
      else
      {
         x += nGridSizeX * iStep;
      }
   }

   // Draw legend
   if (IsLegendVisible())
   {
      wxPen penLegend(m_rgbTextColor, 1);
      dc.SetPen(penLegend);
      for(i = 0, nCurrCol = 0, x = m_rectGraph.GetLeft(),
               y = m_rectGraph.GetBottom() + textSize.y + 8;
          i < MAX_GRAPH_ITEMS; i++)
         if (m_data[i] != NULL)
         {
            wxBrush brushLegend(m_graphItemStyles[i].rgbColor);
            dc.SetBrush(brushLegend);
            dc.DrawRectangle(x, y, x + textSize.y, y + textSize.y);
            dc.SetBrush(wxNullBrush);

            if (m_dciInfo != NULL)
               if (m_dciInfo[i] != NULL)
               {
                  dc.DrawText(m_dciInfo[i]->m_pszDescription, x + 14, y);
                  nCurrCol++;
                  if (nCurrCol == nCols)
                  {
                     nCurrCol = 0;
                     x = m_rectGraph.x;
                     y += textSize.y + LEGEND_TEXT_SPACING;
                  }
                  else
                  {
                     x += nColSize;
                  }
               }
         }
		dc.SetPen(wxNullPen);
   }

   // Save used grid size
   m_lastGridSizeY = nGridSizeY;

	// Cleanup
	dc.SetPen(wxNullPen);
	dc.SetBrush(wxNullBrush);
	dc.SetFont(wxNullFont);

	return bitmap;
}


//
// Calculate offset (in pixels) of next month start
//

int nxGraph::NextMonthOffset(time_t timeStamp)
{
   static double secondsPerMonth[12] = { 2678400, 2419200, 2678400, 2592000,
                                         2678400, 2592000, 2678400, 2678400,
                                         2592000, 2678400, 2592000, 2678400 };
   struct tm *plt;

   plt = localtime(&timeStamp);
   if ((plt->tm_year % 4 == 0) && (plt->tm_mon == 1))
      return (int)ceil(2505600.0 / m_secondsPerPixel) + 1;
   else
      return (int)ceil(secondsPerMonth[plt->tm_mon] / m_secondsPerPixel) + 1;
}
