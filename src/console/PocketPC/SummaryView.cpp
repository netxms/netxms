// SummaryView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "SummaryView.h"
#include "MainFrm.h"

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
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CSummaryView, CWnd)
	//{{AFX_MSG_MAP(CSummaryView)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
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
   m_pImageList = CreateEventImageList();
	
	return 0;
}


//
// WM_PAINT message handler
//

void CSummaryView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;

	if (g_hSession != NULL)
	{
		GetClientRect(&rect);
		rect.left += X_MARGIN;
		rect.top += Y_MARGIN;

		if (IsPortraitOrientation())
		{
			rect.right -= X_MARGIN;
			PaintAlarmSummary(dc, rect);
			rect.top += 120;
			PaintServerStats(dc, rect);
		}
		else
		{
			int cx;

			cx = rect.right;
			rect.bottom -= Y_MARGIN;
			rect.right = rect.right / 2 - X_MARGIN;
			PaintAlarmSummary(dc, rect);
			rect.left = rect.right + X_MARGIN * 2;
			rect.right = cx - X_MARGIN;
			PaintServerStats(dc, rect);
		}
	}
}


//
// Paint nodes status summary
//

void CSummaryView::PaintServerStats(CDC &dc, RECT &rcView)
{
	RECT rcTitle, rcText;
   TCHAR szBuffer[256];
	int cy;

	CopyRect(&rcTitle, &rcView);
   DrawTitle(dc, L"Server Statistics", rcTitle);

	CopyRect(&rcText, &rcTitle);
	rcText.left += X_MARGIN / 2;
	rcText.right -= X_MARGIN / 2;
	cy = dc.GetTextExtent(L"Mqh|", 4).cy;
	rcText.bottom = rcText.top + cy;
	cy += 2;

   _snwprintf(szBuffer, 256, L"Version %s", m_serverStats.szServerVersion);
   dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX | DT_EXPANDTABS);
	OffsetRect(&rcText, 0, cy);

   _snwprintf(szBuffer, 256, L"Uptime %d days %2d:%02d:%02d",
	           m_serverStats.dwServerUptime / 86400,
				  (m_serverStats.dwServerUptime % 86400) / 3600,
				  (m_serverStats.dwServerUptime % 3600) / 60,
				  m_serverStats.dwServerUptime % 60);
   dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX | DT_EXPANDTABS);
	OffsetRect(&rcText, 0, cy);

   _snwprintf(szBuffer, 256, L"%d client sessions", m_serverStats.dwNumClientSessions);
   dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX | DT_EXPANDTABS);
	OffsetRect(&rcText, 0, cy);

   _snwprintf(szBuffer, 256, L"%d managed nodes", m_serverStats.dwNumNodes);
   dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX | DT_EXPANDTABS);
	OffsetRect(&rcText, 0, cy);

	if (m_serverStats.dwServerProcessVMSize != 0)
	{
		_snwprintf(szBuffer, 256, L"%dK memory used", m_serverStats.dwServerProcessVMSize);
		dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX | DT_EXPANDTABS);
		OffsetRect(&rcText, 0, cy);
	}
}


//
// Paint alarm summary
//

void CSummaryView::PaintAlarmSummary(CDC &dc, RECT &rcView)
{
   int i, y, *piAlarmStats;
   TCHAR szBuffer[256];
   RECT rcText, rcTitle;

	CopyRect(&rcTitle, &rcView);
   DrawTitle(dc, L"Active Alarms", rcTitle);
   piAlarmStats = ((CMainFrame *)theApp.m_pMainWnd)->GetAlarmStats();
   for(i = 0, y = rcTitle.top; i < 5; i++, y += 20)
   {
      m_pImageList->Draw(&dc, i, CPoint(rcView.left + X_MARGIN, y), ILD_TRANSPARENT);
      _snwprintf(szBuffer, 256, L"%-10s\t%d", g_szStatusTextSmall[i], piAlarmStats[i]);
      rcText.left = rcView.left + 16 + X_MARGIN * 2;
      rcText.right = rcView.right - X_MARGIN;
      rcText.top = y;
      rcText.bottom = y + 16;
      dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_EXPANDTABS);
   }
}


//
// Draw section title
// This method get view rectangle on input, draws header at the top of view,
// and updates view rectangle to exclude header
//

void CSummaryView::DrawTitle(CDC &dc, TCHAR *pszText, RECT &rect)
{
   CFont *pOldFont;
   RECT rcText;

   memcpy(&rcText, &rect, sizeof(RECT));
   pOldFont = dc.SelectObject(&m_fontTitle);
   rcText.left += X_MARGIN / 2;
   rcText.right -= X_MARGIN / 2;
   dc.DrawText(pszText, -1, &rcText, DT_LEFT);
   rect.top += dc.GetTextExtent(L"X", 1).cy + 2;
   dc.MoveTo(rect.left, rect.top);
   dc.LineTo(rect.right, rect.top);
   rect.top += Y_MARGIN;
   dc.SelectObject(pOldFont);
}


//
// Refresh view
//

void CSummaryView::OnViewRefresh() 
{
	DoRequestArg2(NXCGetServerStats, g_hSession, &m_serverStats, L"Loading server statistics...");
   InvalidateRect(NULL);
}


//
// Check if screen has portrait or landscape orientation
//

BOOL CSummaryView::IsPortraitOrientation()
{
	RECT rect;

	GetClientRect(&rect);
	return rect.bottom > rect.right;
}
