// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "ObjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectView

CObjectView::CObjectView()
{
}

CObjectView::~CObjectView()
{
}


BEGIN_MESSAGE_MAP(CObjectView, CWnd)
	//{{AFX_MSG_MAP(CObjectView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectView message handlers


//
// WM_CREATE message handler
//

int CObjectView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE, rect, this, ID_TREE_CTRL);
	
	return 0;
}
