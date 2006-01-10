// InfoLine.cpp : implementation file
//

#include "stdafx.h"
#include "nxav.h"
#include "InfoLine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoLine

CInfoLine::CInfoLine()
{
   m_nTimer = 0;
}

CInfoLine::~CInfoLine()
{
}


BEGIN_MESSAGE_MAP(CInfoLine, CWnd)
	//{{AFX_MSG_MAP(CInfoLine)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CInfoLine message handlers

BOOL CInfoLine::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.lpszClass = AfxRegisterWndClass(0, LoadCursor(NULL, IDC_ARROW),
                                      CreateSolidBrush(g_rgbInfoLineBackground));
	
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CInfoLine::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create fonts
   m_fontSmall.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
	
   // Create buttons
   GetClientRect(&rect);
   m_wndButtonSettings.Create(NULL, _T("Settings..."), WS_CHILD | WS_VISIBLE,
                           rect, this, ID_CMD_SETTINGS);
   m_wndButtonClose.Create(NULL, _T("Exit"), WS_CHILD | WS_VISIBLE,
                           rect, this, ID_CMD_EXIT);
   m_wndTimer.Create(_T(""), WS_CHILD | WS_VISIBLE, rect, this, IDC_STATIC_TIMER);
   m_wndTimer.SetFont(&m_fontSmall);

   m_nTimer = SetTimer(1, 1000, NULL);
	return 0;
}


//
// WM_PAINT message handler
//

void CInfoLine::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   CDC dcBitmap;
   RECT rect;
   CBitmap bmLogo;
   BITMAP bitmapInfo;
   CFont *pFont;
   static char szVersion[] = _T("Alarm Viewer  Version ") NETXMS_VERSION_STRING;
   
   GetClientRect(&rect);

   // Draw NetXMS logo in upper left corner
   bmLogo.LoadBitmap(IDB_NETXMS_LOGO);
   dcBitmap.CreateCompatibleDC(&dc);
   dcBitmap.SelectObject(&bmLogo);
   bmLogo.GetBitmap(&bitmapInfo);
   dc.BitBlt(5, 5, bitmapInfo.bmWidth, bitmapInfo.bmHeight, &dcBitmap, 0, 0, SRCCOPY);
   dcBitmap.DeleteDC();

   // Draw program name and version
   pFont = dc.SelectObject(&m_fontSmall);
   dc.TextOut(8, bitmapInfo.bmHeight + 7, szVersion, sizeof(szVersion) - 1);

   // Draw divider at the bottom
   dc.DrawEdge(&rect, EDGE_BUMP, BF_BOTTOM);

   // Cleanup
   dc.SelectObject(pFont);
}


//
// WM_SIZE message handler
//

void CInfoLine::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	
   m_wndButtonSettings.MoveWindow(cx - 100, 10, 90, 24);
   m_wndButtonClose.MoveWindow(cx - 100, 44, 90, 24);

   m_wndTimer.MoveWindow(cx - 260, 20, 150, cy - 40);
}


//
// Override OnCmdMsg() to pass all commands to parent first
//

BOOL CInfoLine::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   if (GetParent()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
      return TRUE;
	
	return CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


//
// WM_DESTROY message handler
//

void CInfoLine::OnDestroy() 
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);
	CWnd::OnDestroy();
}


//
// WM_TIMER message handler
//

void CInfoLine::OnTimer(UINT nIDEvent) 
{
   time_t now;
   struct tm *ptm;
   TCHAR szBuffer[256];

   now = time(NULL);
   ptm = localtime(&now);
   _tcsftime(szBuffer, 256, _T("%A\n%d %B %Y\n%H:%M:%S"), ptm);
   m_wndTimer.SetWindowText(szBuffer);
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CInfoLine::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   HBRUSH hbr;

   if (pWnd->GetDlgCtrlID() == IDC_STATIC_TIMER)
   {
      hbr = CreateSolidBrush(g_rgbInfoLineBackground);
      pDC->SetBkColor(g_rgbInfoLineBackground);
      pDC->SetTextColor(g_rgbInfoLineTimer);
   }
   else
   {
   	hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
   }
	return hbr;
}
