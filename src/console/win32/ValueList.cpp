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

   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                           0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_DONTCARE, "Verdana");
   m_fontBold.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                         0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                         VARIABLE_PITCH | FF_DONTCARE, "Verdana");

   SetFont(&m_fontNormal);
	
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
   CFont *pOrigFont;

   // Setup DC
   dc.Attach(lpDrawItemStruct->hDC);

   // Get image number
   // iImage == 1 means item is modified and should be drawn in red
   item.mask = LVIF_IMAGE;
   item.iItem = lpDrawItemStruct->itemID;
   item.iSubItem = 0;
   GetItem(&item);

   dc.FillSolidRect(&lpDrawItemStruct->rcItem, GetSysColor(COLOR_WINDOW));

   memcpy(&rcItem, &lpDrawItemStruct->rcItem, sizeof(RECT));
   for(i = 0, nPos = rcItem.left; i < 4; i++)
   {
      rcItem.left = nPos + 5;
      rcItem.right = nPos + GetColumnWidth(i) - 7;
      GetItemText(lpDrawItemStruct->itemID, i, szBuffer, 1024);
      if ((i == 2) && (item.iImage == 1))
      {
         rgbOldColor = dc.SetTextColor(RGB(192, 0, 0));
         pOrigFont = dc.SelectObject(&m_fontBold);
      }
      dc.DrawText(szBuffer, _tcslen(szBuffer), &rcItem,
                  DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT | DT_END_ELLIPSIS);
      if ((i == 2) && (item.iImage == 1))
      {
         dc.SetTextColor(rgbOldColor);
         dc.SelectObject(pOrigFont);
      }
      nPos += GetColumnWidth(i);
   }

   // Restore DC to original state
   dc.Detach();
}
