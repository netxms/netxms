// FlatButton.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "FlatButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFlatButton

CFlatButton::CFlatButton()
{
   m_bMouseHover = FALSE;
   m_bPressed = FALSE;
}

CFlatButton::~CFlatButton()
{
}


BEGIN_MESSAGE_MAP(CFlatButton, CWnd)
	//{{AFX_MSG_MAP(CFlatButton)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
   ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFlatButton message handlers

BOOL CFlatButton::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.lpszClass = AfxRegisterWndClass(0, LoadCursor(NULL, IDC_HAND),
                                      CreateSolidBrush(g_rgbFlatButtonBackground));
	
	return CWnd::PreCreateWindow(cs);
}


//
// WM_PAINT message handler
//

void CFlatButton::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   CBrush brLines, brBkgnd;
   CFont *pFont;
   TCHAR szText[256];
   RECT rect;
	
   // Setup DC
   brLines.CreateSolidBrush(g_rgbFlatButtonColor);
   brBkgnd.CreateSolidBrush(m_bPressed ? g_rgbFlatButtonColor : g_rgbFlatButtonBackground);
   dc.SetTextColor(m_bPressed ? g_rgbFlatButtonBackground : g_rgbFlatButtonColor);
   dc.SetBkColor(m_bPressed ? g_rgbFlatButtonColor : g_rgbFlatButtonBackground);
   pFont = dc.SelectObject(m_bMouseHover ? &m_fontFocused : &m_fontNormal);

   // Draw frame
   GetClientRect(&rect);
   if (m_bPressed)
   {
      CBrush *pBrush;

      pBrush = dc.SelectObject(&brBkgnd);
      dc.Rectangle(&rect);
      dc.SelectObject(pBrush);
   }
   else
   {
      dc.FrameRect(&rect, &brLines);
   }

   // Draw text
   rect.top += 2;
   rect.bottom -= 2;
   rect.left += 4;
   rect.right -= 4;
   GetWindowText(szText, 256);
   dc.DrawText(szText, -1, &rect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

   // Cleanup
   dc.SelectObject(pFont);
}


//
// WM_CREATE message handler
//

int CFlatButton::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create fonts
   m_fontNormal.CreateFont(-MulDiv(9, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                           0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
   m_fontFocused.CreateFont(-MulDiv(9, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_BOLD, FALSE, TRUE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
	
	return 0;
}


//
// Set mouse tracking parameters
//

void CFlatButton::SetMouseTracking()
{
   TRACKMOUSEEVENT tme;

   tme.cbSize = sizeof(TRACKMOUSEEVENT);
   tme.dwFlags = TME_HOVER | TME_LEAVE;
   tme.hwndTrack = m_hWnd;
   tme.dwHoverTime = 100;
   _TrackMouseEvent(&tme);
}


//
// WM_MOUSEHOVER message handler
//

int CFlatButton::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
   if (!m_bMouseHover)
   {
      m_bMouseHover = TRUE;
      InvalidateRect(NULL, FALSE);
   }
   return 0;
}


//
// WM_MOUSELEAVE message handler
//

int CFlatButton::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
   if (m_bMouseHover || m_bPressed)
   {
      m_bMouseHover = FALSE;
      m_bPressed = FALSE;
      InvalidateRect(NULL);
   }
   return 0;
}


//
// WM_MOUSEMOVE message handler
//

void CFlatButton::OnMouseMove(UINT nFlags, CPoint point) 
{
	CWnd::OnMouseMove(nFlags, point);
   SetMouseTracking();
}


//
// WM_LBUTTONDOWN message handler
//

void CFlatButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
   m_bPressed = TRUE;
   InvalidateRect(NULL, FALSE);
}


//
// WM_LBUTTONUP message handler
//

void CFlatButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
   if (m_bPressed)
   {
      m_bPressed = FALSE;
      InvalidateRect(NULL);
      GetParent()->PostMessage(WM_COMMAND, GetDlgCtrlID(), 0);
   }
}
