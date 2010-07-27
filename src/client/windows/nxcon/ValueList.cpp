// ValueList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ValueList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CValueList

CValueList::CValueList()
{
}

CValueList::~CValueList()
{
}


BEGIN_MESSAGE_MAP(CValueList, CListCtrl)
	//{{AFX_MSG_MAP(CValueList)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValueList message handlers


//
// WM_CREATE message handler
//

int CValueList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}


//
// Draw item
//

void CValueList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   CDC dc;
   TCHAR szBuffer[1024];
   RECT rcItem;
   int i, nPos;
   COLORREF rgbOldColor;
   LVITEM item;
   //CFont *pOrigFont;

   // Setup DC
   dc.Attach(lpDrawItemStruct->hDC);

   // Get image number
   // iImage == 1 means item is modified and should be drawn in red
   item.mask = LVIF_IMAGE | LVIF_STATE;
   item.iItem = lpDrawItemStruct->itemID;
   item.iSubItem = 0;
   item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
   GetItem(&item);

   dc.FillSolidRect(&lpDrawItemStruct->rcItem, 
                    (item.state & LVIS_SELECTED) ? 
                        GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW));

   memcpy(&rcItem, &lpDrawItemStruct->rcItem, sizeof(RECT));
   for(i = 0, nPos = rcItem.left; i < 5; i++)
   {
      rcItem.left = nPos + 5;
      rcItem.right = nPos + GetColumnWidth(i) - 7;
      GetItemText(lpDrawItemStruct->itemID, i, szBuffer, 1024);
      if (i != 3)
      {
         if (item.state & LVIS_SELECTED)
         {
            rgbOldColor = dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
         }
         else
         {
            if ((i == 2) && (item.iImage == 1))
            {
               rgbOldColor = dc.SetTextColor(RGB(192, 0, 0));
            }
            else
            {
               rgbOldColor = dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
            }
         }
         dc.DrawText(szBuffer, (int)_tcslen(szBuffer), &rcItem,
                     DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT | DT_END_ELLIPSIS);
         dc.SetTextColor(rgbOldColor);
      }
      else
      {
         if (szBuffer[0] == 'X')
         {
            GetImageList(LVSIL_SMALL)->Draw(&dc, 3, CPoint(rcItem.left, rcItem.top),
                                            ILD_TRANSPARENT);
         }
      }

      nPos += GetColumnWidth(i);
   }

   if (item.state & LVIS_FOCUSED)
      dc.DrawFocusRect(&lpDrawItemStruct->rcItem);

   // Restore DC to original state
   dc.Detach();
}
