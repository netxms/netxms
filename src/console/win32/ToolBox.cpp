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

#define TOOLBOX_TITLE_COLOR      RGB(0, 0, 50)


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
   strncpy(m_szTitle, lpszWindowName, MAX_TOOLBOX_TITLE - 1);
	return CWnd::Create(NULL, lpszWindowName, dwStyle | WS_CAPTION | WS_BORDER, rect, pParentWnd, nID, pContext);
}


//
// WM_NCCALCSIZE message handler
//

void CToolBox::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);
}


//
// WM_NCPAINT message handler
//

void CToolBox::OnNcPaint() 
{
   CDC *dc;
   CFont font;
   RECT rect;

   // Calculate coordinates of lower right window corner
   GetWindowRect(&rect);
   rect.bottom -= rect.top;
   rect.right -= rect.left;

   // Draw border
   dc = GetWindowDC();
   dc->Draw3dRect(0, 0, rect.right, rect.bottom, TOOLBOX_TITLE_COLOR, TOOLBOX_TITLE_COLOR);

   // Draw title
   dc->FillSolidRect(1, 1, rect.right - 2, 20, TOOLBOX_TITLE_COLOR);
   dc->SetTextColor(RGB(255, 255, 255));
   dc->SetBkColor(TOOLBOX_TITLE_COLOR);
   dc->TextOut(5, 3, m_szTitle, strlen(m_szTitle));

   ReleaseDC(dc);
}
