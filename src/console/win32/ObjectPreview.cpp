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


//
// Constants
//

#define TOOLBOX_X_MARGIN   5
#define TOOLBOX_Y_MARGIN   5


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
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectPreview message handlers

BOOL CObjectPreview::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         CreateSolidBrush(RGB(255, 255, 255)), NULL);
	return CWnd::PreCreateWindow(cs);
}

BOOL CObjectPreview::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	return CWnd::Create(NULL, "", dwStyle, rect, pParentWnd, nID, pContext);
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
   m_wndObjectSearch.Create("Search", WS_CHILD | WS_VISIBLE,
                             rect, this, IDC_TOOLBOX_OBJECT_DETAILS);

   rect.top = rect.bottom + TOOLBOX_X_MARGIN;
   rect.bottom = rect.top + 250;
   m_wndObjectPreview.Create("Object Details", WS_CHILD | WS_VISIBLE,
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

void CObjectPreview::OnPaint() 
{
   CPaintDC dc(this); // device context for painting
   RECT rect;
   CPen pen;

   pen.CreatePen(PS_SOLID, 0, RGB(127, 127, 127));
   dc.SelectObject(&pen);

   GetClientRect(&rect);
   
   dc.MoveTo(rect.right - 1, 0);
   dc.LineTo(rect.right - 1, rect.bottom);

   pen.DeleteObject();
}
