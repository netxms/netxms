// RuleHeader.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleHeader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleHeader

CRuleHeader::CRuleHeader()
{
}

CRuleHeader::~CRuleHeader()
{
}


BEGIN_MESSAGE_MAP(CRuleHeader, CHeaderCtrl)
	//{{AFX_MSG_MAP(CRuleHeader)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleHeader message handlers


//
// Overrided DrawItem()
//

void CRuleHeader::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   HDITEM hdi;
   TCHAR lpBuffer[256];
   COLORREF rgbOldColor;
   RECT rcText;

   // Get item's text
   hdi.mask = HDI_TEXT;
   hdi.pszText = lpBuffer;
   hdi.cchTextMax = 256;
   GetItem(lpDrawItemStruct->itemID, &hdi);

   // Calculate text rectangle
   memcpy(&rcText, &lpDrawItemStruct->rcItem, sizeof(RECT));
   rcText.left += 3;
   rcText.right -= 3;
   if (rcText.right < rcText.left)
      rcText.right = rcText.left;

   // Draw frame.
//   DrawFrameControl(lpDrawItemStruct->hDC, 
//      &lpDrawItemStruct->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH);
   DrawEdge(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, 
            BDR_SUNKENOUTER | BDR_SUNKENINNER, BF_RECT);

   // Draw the items text
   rgbOldColor = SetTextColor(lpDrawItemStruct->hDC, RGB(0, 0, 50));
   DrawText(lpDrawItemStruct->hDC, lpBuffer, strlen(lpBuffer), 
      &rcText, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
   SetTextColor(lpDrawItemStruct->hDC, rgbOldColor);
}
