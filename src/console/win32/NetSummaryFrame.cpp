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
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
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
   BYTE *pwp;
   UINT iBytes;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndNodeSummary.Create(WS_CHILD | WS_VISIBLE, rect, this, 0, NULL);

   // Restore window size and position if we have one
   if (theApp.GetProfileBinary(_T("NetSummaryFrame"), _T("WindowPlacement"),
                               &pwp, &iBytes))
   {
      if (iBytes == sizeof(WINDOWPLACEMENT))
      {
         RestoreMDIChildPlacement(this, (WINDOWPLACEMENT *)pwp);
      }
      delete pwp;
   }
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


//
// WM_SIZE message handler
//

void CNetSummaryFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndNodeSummary.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_GETMINMAXINFO message handler
//

void CNetSummaryFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
   lpMMI->ptMinTrackSize.x = 405;
   lpMMI->ptMinTrackSize.y = 270;
}


//
// WM_OBJECT_CHANGE message handler
//

void CNetSummaryFrame::OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject)
{
   if (pObject->iClass == OBJECT_NODE)
      m_wndNodeSummary.Refresh();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CNetSummaryFrame::OnViewRefresh() 
{
   m_wndNodeSummary.Refresh();
}


//
// WM_CLOSE message handler
//

void CNetSummaryFrame::OnClose() 
{
   WINDOWPLACEMENT wp;

   GetWindowPlacement(&wp);
   theApp.WriteProfileBinary(_T("NetSummaryFrame"), _T("WindowPlacement"), 
                             (BYTE *)&wp, sizeof(WINDOWPLACEMENT));
   CMDIChildWnd::OnClose();
}
