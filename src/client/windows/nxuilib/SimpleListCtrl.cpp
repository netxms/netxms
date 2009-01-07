// SimpleListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "SimpleListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSimpleListCtrl

CSimpleListCtrl::CSimpleListCtrl()
{
}

CSimpleListCtrl::~CSimpleListCtrl()
{
}


BEGIN_MESSAGE_MAP(CSimpleListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CSimpleListCtrl)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimpleListCtrl message handlers


//
// WM_CREATE message handler
//

int CSimpleListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

   SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CSimpleListCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CListCtrl::OnSize(nType, cx, cy);
   SetColumnWidth(0, cx);	
}
