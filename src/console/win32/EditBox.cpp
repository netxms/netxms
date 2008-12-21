// EditBox.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditBox

CEditBox::CEditBox()
{
}

CEditBox::~CEditBox()
{
}


BEGIN_MESSAGE_MAP(CEditBox, CEdit)
	//{{AFX_MSG_MAP(CEditBox)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditBox message handlers

void CEditBox::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
   if ((nChar == VK_RETURN) && (nRepCnt == 1))
      GetParent()->PostMessage(NXCM_EDITBOX_EVENT, GetDlgCtrlID(), EDITBOX_ENTER_PRESSED);
   else
	   CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}
