// WaitView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "WaitView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_PROGRESS_BAR    100
#define ID_STATIC_TEXT     101


/////////////////////////////////////////////////////////////////////////////
// CWaitView

CWaitView::CWaitView()
{
   _tcscpy(m_szText, _T("Loading..."));
}

CWaitView::~CWaitView()
{
}


BEGIN_MESSAGE_MAP(CWaitView, CWnd)
	//{{AFX_MSG_MAP(CWaitView)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWaitView message handlers

BOOL CWaitView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         ::LoadCursor(NULL, IDC_WAIT), 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CWaitView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CDC *pdc;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   pdc = GetDC();
   m_font.CreateFont(-MulDiv(8, GetDeviceCaps(pdc->m_hDC, LOGPIXELSY), 72),
                     0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_nTextHeight = pdc->GetTextExtent(_T("TEXT"), 4).cy;
   ReleaseDC(pdc);

   rect.left = 0;
   rect.top = 0;
   rect.right = 100;
   rect.bottom = 20;
   m_wndText.Create(m_szText, WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX, rect, this, ID_STATIC_TEXT);
   m_wndText.SetFont(&m_font);
   m_wndProgressBar.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | PBS_SMOOTH, rect, this, ID_PROGRESS_BAR);
   m_wndProgressBar.SendMessage(PBM_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_WINDOW));
   m_wndProgressBar.ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, 0);
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CWaitView::OnDestroy() 
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);
	CWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CWaitView::OnSize(UINT nType, int cx, int cy) 
{
   int x, y;

	CWnd::OnSize(nType, cx, cy);
   x = cx / 2 - 150;
   y = cy / 2;
   m_wndText.SetWindowPos(NULL, x, y - m_nTextHeight - 3, 300, m_nTextHeight + 2, SWP_NOZORDER);
   m_wndProgressBar.SetWindowPos(NULL, x, y + 1, 300, 12, SWP_NOZORDER);
}


//
// WM_TIMER message handler
//

void CWaitView::OnTimer(UINT nIDEvent) 
{
   m_nCurrPos++;
   m_wndProgressBar.SetPos(m_nCurrPos);
   if (m_nCurrPos > m_nThreshold)
   {
      m_nMaxPos *= 2;
      m_nCurrPos *= 2;
      m_wndProgressBar.SetRange32(0, m_nMaxPos);
      m_wndProgressBar.SetPos(m_nCurrPos);
      m_nThreshold = m_nCurrPos + (m_nMaxPos - m_nCurrPos) / 2;
   }
}


//
// Start wait picture
//

void CWaitView::Start()
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);

   m_nCurrPos = 0;
   m_nMaxPos = 100;
   m_nThreshold = 50;
   m_wndProgressBar.SetRange32(0, m_nMaxPos);
   m_wndProgressBar.SetPos(0);
   m_nTimer = SetTimer(1, 1000, NULL);
}


//
// Stop wait picture
//

void CWaitView::Stop()
{
   if (m_nTimer != 0)
   {
      KillTimer(m_nTimer);
      m_nTimer = 0;
   }
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CWaitView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   HBRUSH hbr;

   switch(pWnd->GetDlgCtrlID())
   {
      case ID_STATIC_TEXT:
         pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
         pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
         hbr = GetSysColorBrush(COLOR_WINDOW);
         break;
      default:
      	hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
         break;
   }
	return hbr;
}
