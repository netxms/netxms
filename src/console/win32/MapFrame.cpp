// MapFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapFrame

IMPLEMENT_DYNCREATE(CMapFrame, CMDIChildWnd)

CMapFrame::CMapFrame()
{
}

CMapFrame::~CMapFrame()
{
}


BEGIN_MESSAGE_MAP(CMapFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMapFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapFrame message handlers

BOOL CMapFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_NETMAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CMapFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create and initialize map view
   GetClientRect(&rect);
   m_wndMapView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
   
	return 0;
}


//
// WM_SIZE message handler
//

void CMapFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndMapView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);	
}
