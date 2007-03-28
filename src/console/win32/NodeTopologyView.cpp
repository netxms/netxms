// NodeTopologyView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodeTopologyView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STATE_IDLE		0
#define STATE_LOADING	1
#define STATE_NO_DATA	2


//
// Topology query thread
//

static THREAD_RESULT THREAD_CALL QueryThread(void *pArg)
{
	HWND hWnd = ((CNodeTopologyView *)pArg)->m_hWnd;
	DWORD dwResult, dwNodeId = ((CNodeTopologyView *)pArg)->GetObjectId();
	nxObjList *pTopo;

	dwResult = NXCQueryL2Topology(g_hSession, dwNodeId, (void **)&pTopo);
	if (!PostMessage(hWnd, NXCM_REQUEST_COMPLETED, dwNodeId, (LPARAM)(dwResult == RCC_SUCCESS ? pTopo : 0)))
	{
		delete pTopo;	// Destroy list if PostMessage() fails
	}
	return THREAD_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CNodeTopologyView

CNodeTopologyView::CNodeTopologyView()
{
	m_pObject = NULL;
	m_nState = STATE_NO_DATA;
}

CNodeTopologyView::~CNodeTopologyView()
{
}


BEGIN_MESSAGE_MAP(CNodeTopologyView, CWnd)
	//{{AFX_MSG_MAP(CNodeTopologyView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNodeTopologyView message handlers

BOOL CNodeTopologyView::PreCreateWindow(CREATESTRUCT& cs) 
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

int CNodeTopologyView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndMap.CreateEx(0, NULL, _T("Map"), WS_CHILD | WS_VISIBLE, rect, this, 0);
   m_wndMap.m_rgbBkColor = GetSysColor(COLOR_WINDOW);
	m_wndMap.m_rgbTextColor = GetSysColor(COLOR_WINDOWTEXT);
	m_wndMap.m_bCanOpenObjects = FALSE;
	m_wndMap.m_bShowConnectorNames = TRUE;
	
	return 0;
}


//
// WM_SIZE message handler
//

void CNodeTopologyView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
   m_wndMap.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXCM_SET_OBJECT message handler
//

void CNodeTopologyView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
	m_wndMap.ShowWindow(SW_HIDE);
   m_pObject = pObject;
	if (m_pObject->node.dwFlags & NF_IS_BRIDGE)
	{
		ThreadCreate(QueryThread, 0, this);
		m_nState = STATE_LOADING;
	}
	else
	{
		m_nState = STATE_NO_DATA;
	}
	InvalidateRect(NULL);
}


//
// NXCM_REQUEST_COMPLETED message handler
//

void CNodeTopologyView::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
	nxObjList *pList = (nxObjList *)lParam;
   nxMap *pMap;
   nxSubmap *pSubmap;
	RECT rect;

	if ((m_pObject != NULL) && (wParam == m_pObject->dwId))
	{
		if (pList != NULL)
		{
		   pMap = new nxMap(0, 0, m_pObject->szName, _T("Layer 2 Topology"));
			pSubmap = new nxSubmap((DWORD)0);
			GetClientRect(&rect);
			pSubmap->DoLayout(pList->GetNumObjects(), pList->GetObjects(),
									pList->GetNumLinks(), pList->GetLinks(), rect.right, rect.bottom,
									SUBMAP_LAYOUT_REINGOLD_TILFORD);
			pMap->AddSubmap(pSubmap);
			m_wndMap.SetMap(pMap);
			m_wndMap.ShowWindow(SW_SHOW);
		}
		else
		{
			m_nState = STATE_NO_DATA;
			InvalidateRect(NULL);
		}
	}
	delete pList;
}


//
// WM_PAINT message handler
//

void CNodeTopologyView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.SetBkColor(GetSysColor(COLOR_WINDOW));
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	switch(m_nState)
	{
		case STATE_LOADING:
			dc.TextOut(10, 10, _T("Querying device for topology information..."), 43);
			break;
		case STATE_NO_DATA:
			dc.TextOut(10, 10, _T("No topology information available"), 33);
			break;
		default:
			break;
	}
}
