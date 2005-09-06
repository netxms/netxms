// WaitView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "WaitView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitView

CWaitView::CWaitView()
{
}

CWaitView::~CWaitView()
{
}


BEGIN_MESSAGE_MAP(CWaitView, CWnd)
	//{{AFX_MSG_MAP(CWaitView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWaitView message handlers

BOOL CWaitView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         ::LoadCursor(NULL, IDC_WAIT), 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         NULL);
	return CWnd::PreCreateWindow(cs);
}
