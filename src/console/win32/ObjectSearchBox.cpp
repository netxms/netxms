// ObjectSearchBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectSearchBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define X_MARGIN     4
#define Y_MARGIN     4


/////////////////////////////////////////////////////////////////////////////
// CObjectSearchBox

CObjectSearchBox::CObjectSearchBox()
{
}

CObjectSearchBox::~CObjectSearchBox()
{
}


BEGIN_MESSAGE_MAP(CObjectSearchBox, CToolBox)
	//{{AFX_MSG_MAP(CObjectSearchBox)
	ON_WM_CREATE()
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// WM_CREATE message handler
//

int CObjectSearchBox::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CToolBox::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font
   m_fontNormal.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");

   // Create static text
   GetClientRect(&rect);
   rect.left += X_MARGIN;
   rect.right -= X_MARGIN;
   rect.top += Y_MARGIN;
   rect.bottom = rect.top + 12;
   m_wndStatic.Create("Enter object name or IP address:", WS_CHILD | WS_VISIBLE, rect, this);
   m_wndStatic.SetFont(&m_fontNormal);

   // Create edit box
   rect.top = rect.bottom + Y_MARGIN;
   rect.bottom = rect.top + 18;
   m_wndEditBox.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                       rect, this, IDC_EDIT_SEARCH_STRING);
   m_wndEditBox.SetFont(&m_fontNormal);
   m_wndEditBox.SetLimitText(255);

   // Create button
   rect.top = rect.bottom + Y_MARGIN * 2;
   rect.bottom = rect.top + 18;
   rect.left = rect.right - 70;
   m_wndButton.Create("&Find", WS_CHILD | WS_VISIBLE, rect, this, ID_FIND_OBJECT);
   m_wndButton.SetFont(&m_fontNormal);

	return 0;
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CObjectSearchBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   HBRUSH hbr;

   // Static control
   if (pWnd->m_hWnd == m_wndStatic.m_hWnd)
   {
	   hbr = CreateSolidBrush(RGB(255, 255, 255));
      pDC->SetBkColor(RGB(255, 255, 255));
   }
	
	return hbr;
}
