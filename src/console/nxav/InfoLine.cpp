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
}

CInfoLine::~CInfoLine()
{
}


BEGIN_MESSAGE_MAP(CInfoLine, CWnd)
	//{{AFX_MSG_MAP(CInfoLine)
	ON_WM_PAINT()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CInfoLine message handlers

BOOL CInfoLine::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.lpszClass = AfxRegisterWndClass(0, 0, CreateSolidBrush(RGB(255, 255, 255)));
	
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CInfoLine::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create fonts
   m_fontSmall.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
	
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

   // Cleanup
   dc.SelectObject(pFont);
}
