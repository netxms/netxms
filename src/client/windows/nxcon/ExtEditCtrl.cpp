// ExtEditCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ExtEditCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExtEditCtrl

CExtEditCtrl::CExtEditCtrl()
{
	m_nReturnCommand = -1;
	m_nEscapeCommand = -1;
}

CExtEditCtrl::~CExtEditCtrl()
{
}


BEGIN_MESSAGE_MAP(CExtEditCtrl, CEdit)
	//{{AFX_MSG_MAP(CExtEditCtrl)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExtEditCtrl message handlers

void CExtEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch(nChar)
	{
		case VK_RETURN:
			if (m_nReturnCommand != -1)
				GetParent()->PostMessage(WM_COMMAND, m_nReturnCommand, 0);
			break;
		case VK_ESCAPE:
			if (m_nEscapeCommand != -1)
				GetParent()->PostMessage(WM_COMMAND, m_nEscapeCommand, 0);
			break;
		default:
			CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
			break;
	}
}

void CExtEditCtrl::SetReturnCommand(int nCmd)
{
	m_nReturnCommand = nCmd;
}

void CExtEditCtrl::SetEscapeCommand(int nCmd)
{
	m_nEscapeCommand = nCmd;
}
