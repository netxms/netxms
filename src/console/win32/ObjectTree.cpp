// ObjectTree.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define STATUS_BAR_WIDTH		20


/////////////////////////////////////////////////////////////////////////////
// CObjectTree

CObjectTree::CObjectTree()
{
	m_pwndTreeCtrl = NULL;
	m_bUseIcons = FALSE;
	m_bHideNormal = FALSE;
	m_bHideUnknown = TRUE;
	m_bHideUnmanaged = TRUE;
	m_bHideNonOperational = TRUE;
}

CObjectTree::~CObjectTree()
{
}


BEGIN_MESSAGE_MAP(CObjectTree, CWnd)
	//{{AFX_MSG_MAP(CObjectTree)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectTree message handlers

BOOL CObjectTree::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectTree::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 5, 1);
	m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_NORMAL));
	m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_WARNING));
	m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_MINOR));
	m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_MAJOR));
	m_imageList.Add(theApp.LoadIcon(IDI_SEVERITY_CRITICAL));
	
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectTree::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	if (m_pwndTreeCtrl != NULL)
		m_pwndTreeCtrl->SetWindowPos(NULL, STATUS_BAR_WIDTH, 0, cx - STATUS_BAR_WIDTH, cy, SWP_NOZORDER);
}


//
// WM_NOTIFY message handler
//

BOOL CObjectTree::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if (m_pwndTreeCtrl != NULL)
	{
		if (wParam == (WPARAM)m_pwndTreeCtrl->GetDlgCtrlID())
		{
			NMTREEVIEW *pn = (LPNMTREEVIEW)lParam;

			if (pn->hdr.code == TVN_ITEMEXPANDED)
				InvalidateRect(NULL);
		}
	}
	*pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
	return TRUE;
}


//
// Check if mark for given status should be shown
//

BOOL CObjectTree::IsStatusVisible(int nStatus)
{
	switch(nStatus)
	{
		case STATUS_NORMAL:
			return !m_bHideNormal;
		case STATUS_UNKNOWN:
			return !m_bHideUnknown;
		case STATUS_UNMANAGED:
			return !m_bHideUnmanaged;
		case STATUS_DISABLED:
		case STATUS_TESTING:
			return !m_bHideNonOperational;
	}
	return TRUE;
}


//
// WM_PAINT message handler
//

void CObjectTree::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	RECT rcClient, rcItem;
	CPen pen, *pOldPen;
	CBrush brush, *pOldBrush;
	HTREEITEM hItem;
	int nStatus, nSize;

	GetClientRect(&rcClient);

	pen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	pOldPen = dc.SelectObject(&pen);
	dc.MoveTo(STATUS_BAR_WIDTH - 1, 0);
	dc.LineTo(STATUS_BAR_WIDTH - 1, rcClient.bottom);
	dc.SelectObject(pOldPen);

	hItem = m_pwndTreeCtrl->GetNextItem(NULL, TVGN_FIRSTVISIBLE);
	while(hItem != NULL)
	{
		nStatus = m_pwndTreeCtrl->GetItemState(hItem, TVIS_OVERLAYMASK) >> 8;
		m_pwndTreeCtrl->GetItemRect(hItem, &rcItem, FALSE);
		if (IsStatusVisible(nStatus))
		{
			if (m_bUseIcons)
			{
				m_imageList.Draw(&dc, nStatus, CPoint(2, rcItem.top), ILD_TRANSPARENT);
			}
			else
			{
				nSize = min(rcItem.bottom - rcItem.top, STATUS_BAR_WIDTH - 5);
				if (nSize > 10)
					nSize = 10;
				rcItem.left = (STATUS_BAR_WIDTH - nSize) / 2;
				rcItem.right = rcItem.left + nSize;
				rcItem.top = rcItem.top + (rcItem.bottom - rcItem.top - nSize) / 2;
				rcItem.bottom = rcItem.top + nSize;

				brush.CreateSolidBrush(g_statusColorTable[nStatus]);
				pOldBrush = dc.SelectObject(&brush);
				dc.Ellipse(&rcItem);
				//dc.FillSolidRect(&rcItem, g_statusColorTable[nStatus]);
				dc.SelectObject(pOldBrush);
				brush.DeleteObject();
			}
		}
		hItem = m_pwndTreeCtrl->GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}
}


//
// WM_CONTEXTMENU message handler
//

void CObjectTree::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CPoint pt = point;

	ScreenToClient(&pt);
	if ((pt.x >= 0) && (pt.x < STATUS_BAR_WIDTH))
	{
		CMenu *pMenu;

		pMenu = theApp.GetContextMenu(25);
		pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
	}
	else
	{
		// Pass to parent
		GetParent()->SendMessage(WM_CONTEXTMENU, (WPARAM)pWnd->m_hWnd, MAKELPARAM(point.x, point.y));
	}
}
