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
   m_rgbTextColor = RGB(255, 255, 255);
   m_rgbBkColor = RGB(0, 0, 127);
}

CRuleHeader::~CRuleHeader()
{
}


BEGIN_MESSAGE_MAP(CRuleHeader, CHeaderCtrl)
	//{{AFX_MSG_MAP(CRuleHeader)
	ON_WM_ERASEBKGND()
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
   COLORREF rgbOldTextColor, rgbOldBkColor;
   RECT rcText;
   CPen pen;
   CBrush brush;
   HGDIOBJ hOldPen, hOldBrush;

   // Get item's text
   hdi.mask = HDI_TEXT;
   hdi.pszText = lpBuffer;
   hdi.cchTextMax = 256;
   GetItem(lpDrawItemStruct->itemID, &hdi);

   // Calculate text rectangle
   memcpy(&rcText, &lpDrawItemStruct->rcItem, sizeof(RECT));
   rcText.left++;
   rcText.top++;
   rcText.right--;
   rcText.bottom--;
   if (rcText.right < rcText.left)
      rcText.right = rcText.left;

   // Draw frame.
   Draw3dRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem,
              RGB(128, 128, 128), RGB(255, 255, 255));

   // Draw the items text
   pen.CreatePen(PS_SOLID, 1, m_rgbBkColor);
   brush.CreateSolidBrush(m_rgbBkColor);
   hOldPen = SelectObject(lpDrawItemStruct->hDC, pen.m_hObject);
   hOldBrush = SelectObject(lpDrawItemStruct->hDC, brush.m_hObject);
   Rectangle(lpDrawItemStruct->hDC, rcText.left, rcText.top, rcText.right, rcText.bottom);
   rgbOldTextColor = SetTextColor(lpDrawItemStruct->hDC, m_rgbTextColor);
   rgbOldBkColor = SetBkColor(lpDrawItemStruct->hDC, m_rgbBkColor);
   DrawText(lpDrawItemStruct->hDC, lpBuffer, (int)_tcslen(lpBuffer), 
            &rcText, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
   SetTextColor(lpDrawItemStruct->hDC, rgbOldTextColor);
   SetBkColor(lpDrawItemStruct->hDC, rgbOldBkColor);
   SelectObject(lpDrawItemStruct->hDC, hOldPen);
   SelectObject(lpDrawItemStruct->hDC, hOldBrush);
}


//
// WM_ERASEBKGND message handler
//

BOOL CRuleHeader::OnEraseBkgnd(CDC* pDC) 
{
   return TRUE;
}


//
// Set title colors
//

void CRuleHeader::SetColors(COLORREF rgbTextColor, COLORREF rgbBkColor)
{
   m_rgbTextColor = rgbTextColor;
   m_rgbBkColor = rgbBkColor;
}
