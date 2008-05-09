// ObjectTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectTreeCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectTreeCtrl

CObjectTreeCtrl::CObjectTreeCtrl()
{
}

CObjectTreeCtrl::~CObjectTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CObjectTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CObjectTreeCtrl)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectTreeCtrl message handlers

void CObjectTreeCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	GetParent()->PostMessage(NXCM_CHILD_VSCROLL, GetDlgCtrlID(), 0);
}

BOOL CObjectTreeCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	GetParent()->PostMessage(NXCM_CHILD_VSCROLL, GetDlgCtrlID(), 0);
	return CTreeCtrl::OnMouseWheel(nFlags, zDelta, pt);
}
