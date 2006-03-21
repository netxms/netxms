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
	
	// TODO: Add your specialized creation code here
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CViewEditor::OnDestroy() 
{
	CMDIChildWnd::OnDestroy();
	
	// TODO: Add your message handler code here
	
}


//
// WM_PAINT message handler
//

void CViewEditor::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

   dc.SetMapMode(MM_ISOTROPIC);
   dc.SetWindowExt(100, 100);
   dc.Rectangle(0, 0, 20, 20);
}
