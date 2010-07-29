// MapToolbox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapToolbox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapToolbox

CMapToolbox::CMapToolbox()
{
}

CMapToolbox::~CMapToolbox()
{
}


BEGIN_MESSAGE_MAP(CMapToolbox, CWnd)
	//{{AFX_MSG_MAP(CMapToolbox)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMapToolbox message handlers

BOOL CMapToolbox::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, 
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CMapToolbox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndControlBox.Create(_T("Map Control"), WS_CHILD | WS_VISIBLE, rect, this, 0);
	
	return 0;
}


//
// WM_PAINT message handler
//

void CMapToolbox::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;

   GetClientRect(&rect);
   dc.DrawEdge(&rect, EDGE_BUMP, BF_RIGHT);
}


//
// WM_SIZE message handler
//

void CMapToolbox::OnSize(UINT nType, int cx, int cy) 
{
   CWnd::OnSize(nType, cx, cy);
   m_wndControlBox.SetWindowPos(NULL, TOOLBOX_X_MARGIN, TOOLBOX_Y_MARGIN,
                                cx - TOOLBOX_X_MARGIN * 2, min(max(cy, 50), 300), SWP_NOZORDER);
}


//
// WM_COMMAND message handler
//

BOOL CMapToolbox::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	return GetParent()->SendMessage(WM_COMMAND, wParam, lParam) ? TRUE : FALSE;
}
