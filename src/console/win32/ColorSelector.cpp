// ColorSelector.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ColorSelector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColorSelector

CColorSelector::CColorSelector()
{
   m_rgbColor = RGB(0, 0, 0);
   m_bMouseDown = FALSE;
}

CColorSelector::~CColorSelector()
{
}


BEGIN_MESSAGE_MAP(CColorSelector, CWnd)
	//{{AFX_MSG_MAP(CColorSelector)
	ON_WM_SETCURSOR()
	ON_WM_PAINT()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CColorSelector message handlers


//
// WM_SETCURSOR message handler
//

BOOL CColorSelector::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
   SetCursor(LoadCursor(NULL, IDC_HAND));
   return TRUE;
}


//
// WM_PAINT message handler
//

void CColorSelector::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   CBrush brush;
   RECT rect;

   GetClientRect(&rect);
   brush.CreateSolidBrush(m_rgbColor);
   dc.FillRect(&rect, &brush);
}


//
// WM_LBUTTONUP message handler
//

void CColorSelector::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CWnd::OnLButtonUp(nFlags, point);
   if (m_bMouseDown)
   {
      CColorDialog dlg;

      if (dlg.DoModal() == IDOK)
      {
         m_rgbColor = dlg.GetColor();
         InvalidateRect(NULL, FALSE);
      }
   }
   m_bMouseDown = FALSE;
}


//
// WM_LBUTTONDOWN message handler
//

void CColorSelector::OnLButtonDown(UINT nFlags, CPoint point) 
{
   m_bMouseDown = TRUE;
	CWnd::OnLButtonDown(nFlags, point);
}
