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
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	
	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

void CClusterView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
	InvalidateRect(NULL);
}


//
// WM_PAINT message handler
//

void CClusterView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	NXC_OBJECT *pNode;
	DWORD i;

	if (m_pObject == NULL)
		return;

	for(i = 0; i < m_pObject->dwNumChilds; i++)
	{
		pNode = NXCFindObjectById(g_hSession, m_pObject->pdwChildList[i]);
		if (pNode->iClass == OBJECT_NODE)
		{
		}
	}
}
