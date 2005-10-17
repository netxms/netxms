// ToolBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ToolBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define TOOLBOX_TITLE_COLOR      RGB(120, 145, 225)


//
// CToolBox implementation
//

CToolBox::CToolBox()
{
   memset(m_szTitle, 0, MAX_TOOLBOX_TITLE);
}

CToolBox::~CToolBox()
{
}


BEGIN_MESSAGE_MAP(CToolBox, CWnd)
	//{{AFX_MSG_MAP(CToolBox)
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolBox message handlers

BOOL CToolBox::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}

BOOL CToolBox::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
   nx_strncpy(m_szTitle, lpszWindowName, MAX_TOOLBOX_TITLE - 1);
	return CWnd::Create(NULL, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}


//
// WM_NCCALCSIZE message handler
//

void CToolBox::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
   if (bCalcValidRects)
   {
	   CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
   }
   else
   {
      int iBorderWidth = 0;

      if (GetWindowLong(m_hWnd, GWL_STYLE) & WS_BORDER)
         iBorderWidth++;

      lpncsp->rgrc[0].left += iBorderWidth;
      lpncsp->rgrc[0].right -= iBorderWidth;
      lpncsp->rgrc[0].top += 19 + iBorderWidth;
      lpncsp->rgrc[0].bottom -= iBorderWidth;
   }
}


//
// WM_NCPAINT message handler
//

void CToolBox::OnNcPaint() 
{
   CDC *dc;
   RECT rect;
   int iBorderWidth = 0;

   // Get device context for entire window
   dc = GetWindowDC();

   // Calculate coordinates of lower right window corner
   GetWindowRect(&rect);
   rect.bottom -= rect.top;
   rect.right -= rect.left;

   // Draw border
   if (GetWindowLong(m_hWnd, GWL_STYLE) & WS_BORDER)
   {
      dc->Draw3dRect(0, 0, rect.right, rect.bottom, TOOLBOX_TITLE_COLOR, TOOLBOX_TITLE_COLOR);
      iBorderWidth++;
   }

   // Draw title
   dc->SelectObject(&m_fontTitle);
   dc->FillSolidRect(iBorderWidth, iBorderWidth, rect.right - (iBorderWidth * 2), 
                     19, TOOLBOX_TITLE_COLOR);
   dc->SetTextColor(RGB(255, 255, 255));
   dc->SetBkColor(TOOLBOX_TITLE_COLOR);
   dc->TextOut(5, 3, m_szTitle, strlen(m_szTitle));

   // Cleanup
   ReleaseDC(dc);
}


//
// WM_CREATE message handler
//

int CToolBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   m_fontTitle.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CToolBox::OnDestroy() 
{
	CWnd::OnDestroy();
	
	m_fontTitle.DeleteObject();
}
