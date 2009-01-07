// ClusterView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CLV_TOP_MARGIN		20
#define CLV_LEFT_MARGIN		10
#define CLV_ICON_SPACING	48
#define CLV_TEXT_SPACING	8


/////////////////////////////////////////////////////////////////////////////
// CClusterView

CClusterView::CClusterView()
{
	m_pObject = NULL;
}

CClusterView::~CClusterView()
{
}


BEGIN_MESSAGE_MAP(CClusterView, CWnd)
	//{{AFX_MSG_MAP(CClusterView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CClusterView message handlers

BOOL CClusterView::PreCreateWindow(CREATESTRUCT& cs) 
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

int CClusterView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_imageList.Create(32, 32, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imageList.Add(theApp.LoadIcon(IDI_SERVER));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_WARNING));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_MINOR));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_MAJOR));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_CRITICAL));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_UNKNOWN));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_UNMANAGED));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_DISABLED));
	m_imageList.Add(theApp.LoadIcon(IDI_OVL_STATUS_TESTING));

   m_fontNormal.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
   m_fontHeading.CreateFont(-MulDiv(10, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                            0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            VARIABLE_PITCH | FF_DONTCARE, _T("MS Sans Serif"));
	
   GetClientRect(&rect);
   rect.top += 50;
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 180);
   m_wndListCtrl.InsertColumn(1, _T("Virtual IP"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, _T("Current Owner"), LVCFMT_LEFT, 100);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

void CClusterView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
	Refresh();
}


//
// WM_PAINT message handler
//

void CClusterView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	NXC_OBJECT *pNode;
	RECT rect;
	DWORD i;
	int x;
	CFont *pOldFont;

	if (m_pObject == NULL)
		return;

	dc.SetBkColor(GetSysColor(COLOR_WINDOW));
	dc.SetTextAlign(GetSysColor(COLOR_WINDOWTEXT));

	// Draw nodes
	pOldFont = dc.SelectObject(&m_fontNormal);
	for(i = 0, x = CLV_LEFT_MARGIN + CLV_ICON_SPACING / 2; i < m_pObject->dwNumChilds; i++)
	{
		pNode = NXCFindObjectById(g_hSession, m_pObject->pdwChildList[i]);
		if (pNode->iClass == OBJECT_NODE)
		{
			m_imageList.Draw(&dc, 0, CPoint(x, CLV_TOP_MARGIN), ILD_TRANSPARENT);
			if (pNode->iStatus != STATUS_NORMAL)
				m_imageList.Draw(&dc, pNode->iStatus, CPoint(x, CLV_TOP_MARGIN), ILD_TRANSPARENT);
			rect.left = x - CLV_ICON_SPACING / 2;
			rect.top = CLV_TOP_MARGIN + 32 + CLV_TEXT_SPACING;
			rect.right = rect.left + 32 + CLV_ICON_SPACING;
			rect.bottom = rect.top + 40;
			dc.DrawText(pNode->szName, -1, &rect, DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
			x += CLV_ICON_SPACING + 32;
		}
	}

   GetClientRect(&rect);
   rect.left = CLV_LEFT_MARGIN;
   rect.top = CLV_TOP_MARGIN + CLV_TEXT_SPACING + 72;
   rect.bottom = rect.top + 20;
	rect.right -= CLV_LEFT_MARGIN;
   DrawHeading(dc, _T("Resource Groups"), &m_fontHeading, &rect, RGB(0, 0, 255), RGB(128, 128, 255));

	// Cleanup
	dc.SelectObject(pOldFont);
}


//
// WM_SIZE message handler
//

void CClusterView::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;

	CWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, CLV_LEFT_MARGIN, CLV_TOP_MARGIN + CLV_TEXT_SPACING + 97,
	                           cx - CLV_LEFT_MARGIN * 2, max(50, cy - CLV_TOP_MARGIN - CLV_TEXT_SPACING - 92), SWP_NOZORDER);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.SetColumnWidth(2, max(rect.right - 280, 100));
}


//
// Refresh view
//

void CClusterView::Refresh()
{
	DWORD i;
	int nItem;
	NXC_OBJECT *pObject;
	TCHAR szIpAddr[32];

	m_wndListCtrl.DeleteAllItems();
	if (m_pObject != NULL)
	{
		for(i = 0; i < m_pObject->cluster.dwNumResources; i++)
		{
			nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, m_pObject->cluster.pResourceList[i].szName);
			if (nItem != -1)
			{
				m_wndListCtrl.SetItemData(nItem, m_pObject->cluster.pResourceList[i].dwId);
				m_wndListCtrl.SetItemText(nItem, 1, IpToStr(m_pObject->cluster.pResourceList[i].dwIpAddr, szIpAddr));
				if (m_pObject->cluster.pResourceList[i].dwCurrOwner != 0)
				{
					pObject = NXCFindObjectById(g_hSession, m_pObject->cluster.pResourceList[i].dwCurrOwner);
					m_wndListCtrl.SetItemText(nItem, 2, (pObject != NULL) ? pObject->szName : _T("<unknown>"));
				}
				else
				{
					m_wndListCtrl.SetItemText(nItem, 2, _T("<none>"));
				}
			}
		}
	}
	InvalidateRect(NULL);
}
