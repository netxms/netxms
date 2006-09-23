// ObjectPreview.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPreview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CObjectPreview

CObjectPreview::CObjectPreview()
{
}

CObjectPreview::~CObjectPreview()
{
}


BEGIN_MESSAGE_MAP(CObjectPreview, CWnd)
	//{{AFX_MSG_MAP(CObjectPreview)
	ON_WM_CREATE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_FIND_OBJECT, OnFindObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectPreview message handlers

BOOL CObjectPreview::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}

BOOL CObjectPreview::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	return CWnd::Create(NULL, _T(""), dwStyle, rect, pParentWnd, nID, pContext);
}


//
// WM_CREATE message handler
//

int CObjectPreview::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   rect.left += TOOLBOX_X_MARGIN;
   rect.right -= TOOLBOX_X_MARGIN * 2;
   rect.top += TOOLBOX_Y_MARGIN;
   rect.bottom = 100;
   m_wndObjectSearch.Create(_T("Search"), WS_CHILD | WS_VISIBLE,
                             rect, this, IDC_TOOLBOX_OBJECT_DETAILS);

   rect.top = rect.bottom + TOOLBOX_X_MARGIN;
   rect.bottom = rect.top + 250;
   m_wndObjectPreview.Create(_T("Object Details"), WS_CHILD | WS_VISIBLE,
                             rect, this, IDC_TOOLBOX_OBJECT_DETAILS);

	return 0;
}


//
// Called when object selection changes
//

void CObjectPreview::SetCurrentObject(NXC_OBJECT *pObject)
{
   m_wndObjectPreview.SetCurrentObject(pObject);
}


//
// WM_PAINT message handler
//

void CObjectPreview::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;

   GetClientRect(&rect);
   dc.DrawEdge(&rect, EDGE_BUMP, BF_RIGHT);
}


//
// Refresh object information
//

void CObjectPreview::Refresh()
{
   m_wndObjectPreview.Invalidate();
   m_wndObjectPreview.UpdateWindow();
}


//
// WM_FIND_OBJECT message handler
//

void CObjectPreview::OnFindObject(WPARAM wParam, LPARAM lParam)
{
   GetParent()->SendMessage(NXCM_FIND_OBJECT, wParam, lParam);
}
