// MapView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapView

CMapView::CMapView()
{
}

CMapView::~CMapView()
{
}


BEGIN_MESSAGE_MAP(CMapView, CListCtrl)
	//{{AFX_MSG_MAP(CMapView)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMapView message handlers



void CMapView::OnPaint() 
{
   int i, iNumItems;
   POINT pt;
   RECT rect;
   LVITEM item;
   TCHAR szBuffer[256];
   CFont *pFont;

   // Setup DC
	CPaintDC dc(this); // device context for painting
   pFont = dc.SelectObject(GetFont());

   iNumItems = GetItemCount();
   for(i = 0; i < iNumItems; i++)
   {
      GetItemPosition(i, &pt);
      
      item.iItem = i;
      item.iSubItem = 0;
      item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
      item.cchTextMax = 256;
      item.pszText = szBuffer;
      GetItem(&item);

      // Draw icon
      GetImageList(LVSIL_NORMAL)->Draw(&dc, item.iImage, pt, 
             item.state & LVIS_SELECTED ? ILD_SELECTED : ILD_TRANSPARENT);
      
      // Draw text under icon
      rect.left = pt.x - 16;
      rect.right = pt.x + 48;
      rect.top = pt.y + 32;
      rect.bottom = rect.top + 32;
      dc.DrawText(item.pszText, _tcslen(item.pszText), &rect, DT_CENTER | DT_WORDBREAK);
   }

   // Cleanup DC
   dc.SelectObject(pFont);
}
