// SummaryView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "SummaryView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define X_MARGIN  4
#define Y_MARGIN  4


/////////////////////////////////////////////////////////////////////////////
// CSummaryView

CSummaryView::CSummaryView()
{
}

CSummaryView::~CSummaryView()
{
}


BEGIN_MESSAGE_MAP(CSummaryView, CWnd)
	//{{AFX_MSG_MAP(CSummaryView)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSummaryView message handlers

BOOL CSummaryView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursorW(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CSummaryView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_fontTitle.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif");
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif");
	
	return 0;
}


//
// WM_PAINT message handler
//

void CSummaryView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   PaintNodeSummary(dc);
}


//
// Paint nodes status summary
//

void CSummaryView::PaintNodeSummary(CDC &dc)
{
/*   int i, x, y = Y_MARGIN, iTabStop, iLineHeight, iCounterWidth;
   int iChartTop, iChartBottom;
   RECT rect;
   CSize size;
   WCHAR szBuffer[256];

   GetClientRect(&rect);
   x = X_MARGIN + X_MARGIN / 2;
	
   // Draw title
   dc.SelectObject(&m_fontTitle);
//   dc.TextOut(x, Y_MARGIN, L"Node Status Summary", 19);
   y += dc.GetTextExtent(L"X", 1).cy + 2;
   dc.MoveTo(X_MARGIN, y);
   dc.LineTo(rect.right - Y_MARGIN, y);
   y += Y_MARGIN * 2;
   iChartTop = y;

   // Draw legend
   dc.SelectObject(&m_fontNormal);
   size = dc.GetTextExtent(L"0000", 4);
   iLineHeight = size.cy;
   iCounterWidth = size.cx;
   iTabStop = 100;
   for(i = 0; i < OBJECT_STATUS_COUNT; i++, y+= 20)
   {
      dc.FillSolidRect(x, y, iLineHeight, iLineHeight, g_statusColorTable[i]);
      dc.Draw3dRect(x, y, iLineHeight, iLineHeight, RGB(0, 0, 0), RGB(0, 0, 0));
      dc.SetBkColor(RGB(255, 255, 255));
      swprintf(szBuffer, L"%s:\t%d", g_szStatusTextSmall[i], m_dwNodeStats[i]);
//      dc.TabbedTextOut(x + iLineHeight + 5, y, szBuffer, wcslen(szBuffer), 1, 
//                       &iTabStop, x + iLineHeight + 5);
   }
   iChartBottom = y;

   // Total
   dc.MoveTo(X_MARGIN, y);
   dc.LineTo(x + iLineHeight + iTabStop + 30, y);
   y += 5;
   swprintf(szBuffer, L"Total:\t%d", m_dwTotalNodes);
//   dc.TabbedTextOut(x + iLineHeight + 5, y, szBuffer, wcslen(szBuffer), 1, 
//                    &iTabStop, x + iLineHeight + 5);

   // Pie chart
   rect.left = 200;
   rect.top = iChartTop;
   rect.bottom = iChartBottom;
   rect.right = rect.left + (rect.bottom - rect.top);
   DrawPieChart(dc, &rect, OBJECT_STATUS_COUNT, m_dwNodeStats, g_statusColorTable);*/
}
