// NetSummaryFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NetSummaryFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetSummaryFrame

IMPLEMENT_DYNCREATE(CNetSummaryFrame, CMDIChildWnd)

CNetSummaryFrame::CNetSummaryFrame()
{
}

CNetSummaryFrame::~CNetSummaryFrame()
{
}


BEGIN_MESSAGE_MAP(CNetSummaryFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CNetSummaryFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetSummaryFrame message handlers


//
// PreCreateWindow()
//

BOOL CNetSummaryFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_NETMAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CNetSummaryFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndNodeSummary.Create(WS_CHILD | WS_VISIBLE, rect, this, 0, NULL);

   theApp.OnViewCreate(IDR_NETWORK_SUMMARY, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CNetSummaryFrame::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_NETWORK_SUMMARY, this);
	CMDIChildWnd::OnDestroy();
}
