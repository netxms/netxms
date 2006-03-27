// ViewEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ViewEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewEditor

IMPLEMENT_DYNCREATE(CViewEditor, CMDIChildWnd)

CViewEditor::CViewEditor()
{
}

CViewEditor::~CViewEditor()
{
}


BEGIN_MESSAGE_MAP(CViewEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CViewEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewEditor message handlers

BOOL CViewEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_ALARM));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CViewEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_BUILDER, this);
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CViewEditor::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_BUILDER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_PAINT message handler
//

void CViewEditor::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;

   GetClientRect(&rect);

ViewElement e;
e.m_pos.x = 0;
e.m_pos.y = 0;
e.m_size.cx = 80;
e.m_size.cy = 80;
DrawElement(dc, rect, &e);
}


//
// Draw single view element
//

void CViewEditor::DrawElement(CDC &dc, RECT &clRect, ViewElement *pElement)
{
   CBrush brush, *pBrush;
   RECT rect;

   rect.left = clRect.right * pElement->m_pos.x / 100;
   rect.top = clRect.bottom * pElement->m_pos.y / 100;
   rect.right = clRect.right * (pElement->m_pos.x + pElement->m_size.cx) / 100;
   rect.bottom = clRect.bottom * (pElement->m_pos.y + pElement->m_size.cy) / 100;

   brush.CreateSolidBrush(RGB(0, 0, 200));
   pBrush = dc.SelectObject(&brush);
   dc.Rectangle(&rect);
   dc.SelectObject(pBrush);
}
