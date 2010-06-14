// PopupInfoWindow.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "PopupInfoWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BORDER_MARGIN	6

/////////////////////////////////////////////////////////////////////////////
// CPopupInfoWindow

CPopupInfoWindow::CPopupInfoWindow()
{
}

CPopupInfoWindow::~CPopupInfoWindow()
{
}


BEGIN_MESSAGE_MAP(CPopupInfoWindow, CWnd)
	//{{AFX_MSG_MAP(CPopupInfoWindow)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPopupInfoWindow message handlers

BOOL CPopupInfoWindow::Create(CWnd* pParentWnd, DWORD dwStyle) 
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 10;
	rect.bottom = 10;
	return CWnd::CreateEx(WS_EX_TOPMOST, NULL, _T("CPopupInfoWindow"), dwStyle, rect, pParentWnd, -1, NULL);
}


//
// Show popup window at given location
//

void CPopupInfoWindow::ShowAt(POINT pt, const TCHAR *text)
{
	CDC *dc;
	RECT rect, rcParent;
	CFont *savedFont;

	SetWindowText(text);

	// Calculate required rectangle
	dc = GetDC();
	savedFont = dc->SelectObject(&m_font);
	rect.left = 0;
	rect.top = 0;
	rect.right = 500;
	rect.bottom = 1;
	dc->DrawText(text, &rect, DT_CALCRECT | DT_NOPREFIX);
	dc->SelectObject(savedFont);
	ReleaseDC(dc);
	rect.right += BORDER_MARGIN * 2;
	rect.bottom += BORDER_MARGIN * 2;

	// Adjust position to fit into parent's client area
	GetParent()->GetClientRect(&rcParent);
	if (pt.x + rect.right > rcParent.right)
		pt.x = max(0, rcParent.right - rect.right);
	if (pt.y - rect.bottom < 0)
		pt.y = rect.bottom;

	SetWindowPos(NULL, pt.x, pt.y - rect.bottom + 1, rect.right, rect.bottom, SWP_NOZORDER);
	ShowWindow(SW_SHOW);
}

void CPopupInfoWindow::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	RECT rect;
	CString text;
	CFont *savedFont;

	GetClientRect(&rect);
	dc.Rectangle(&rect);

	GetWindowText(text);
	InflateRect(&rect, -BORDER_MARGIN, -BORDER_MARGIN);
	savedFont = dc.SelectObject(&m_font);
	dc.DrawText(text, &rect, DT_NOPREFIX);
	dc.SelectObject(savedFont);
}

BOOL CPopupInfoWindow::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW), 
													  GetSysColorBrush(COLOR_INFOBK), NULL);
	return CWnd::PreCreateWindow(cs);
}

int CPopupInfoWindow::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	HDC hdc = ::GetDC(m_hWnd);
   m_font.CreateFont(-MulDiv(7, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                     0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     VARIABLE_PITCH | FF_DONTCARE, _T("Helvetica"));
	::ReleaseDC(m_hWnd, hdc);
	
	return 0;
}

void CPopupInfoWindow::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// Hide window on mouse click
	ShowWindow(SW_HIDE);
}
