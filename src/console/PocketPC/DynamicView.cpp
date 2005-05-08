// DynamicView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "DynamicView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDynamicView

CDynamicView::CDynamicView()
{
}

CDynamicView::~CDynamicView()
{
}


BEGIN_MESSAGE_MAP(CDynamicView, CWnd)
	//{{AFX_MSG_MAP(CDynamicView)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CLOSE, OnViewClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDynamicView message handlers

//
// Get view's fingerprint
// Should be overriden in child classes.
//

QWORD CDynamicView::GetFingerprint()
{
   return 0;
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Should be overriden in child classes.
//

void CDynamicView::InitView(void *pData)
{
}


//
// WM_DESTROY message handler
//

void CDynamicView::OnDestroy() 
{
	CWnd::OnDestroy();

   ((CMainFrame *)theApp.m_pMainWnd)->UnregisterView(this);
}


//
// WM_CREATE message handler
//

int CDynamicView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   if (!((CMainFrame *)theApp.m_pMainWnd)->RegisterView(this))
      return -1;
	
	return 0;
}


//
// WM_COMMAND::ID_VIEW_CLOSE message handler
//

void CDynamicView::OnViewClose() 
{
   DestroyWindow();
}


//
// Destroy view object
//

void CDynamicView::PostNcDestroy() 
{
	CWnd::PostNcDestroy();
   delete this;
}
