/*******************************************************************************
	Author						: Aravindan Premkumar
	Unregistered Copyright 2003	: Aravindan Premkumar
	All Rights Reserved
	
	This piece of code does not have any registered copyright and is free to be 
	used as necessary. The user is free to modify as per the requirements. As a
	fellow developer, all that I expect and request for is to be given the 
	credit for intially developing this reusable code by not removing my name as 
	the author.
*******************************************************************************/

#include "stdafx.h"
#include "nxuilib.h"
#include "InPlaceCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCombo

CInPlaceCombo* CInPlaceCombo::m_pInPlaceCombo = NULL; 

CInPlaceCombo::CInPlaceCombo()
{
	m_iRowIndex = -1;
	m_iColumnIndex = -1;
	m_bESC = FALSE;

	m_nOffset = GetSystemMetrics(SM_CXHTHUMB);
}

CInPlaceCombo::~CInPlaceCombo()
{
}

BEGIN_MESSAGE_MAP(CInPlaceCombo, CComboBox)
	//{{AFX_MSG_MAP(CInPlaceCombo)
	ON_WM_CREATE()
	ON_WM_KILLFOCUS()
	ON_WM_CHAR()
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCloseup)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInPlaceCombo message handlers

int CInPlaceCombo::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

//SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
	
	// Set the proper font
	CFont* pFont = GetParent()->GetFont();
	SetFont(pFont);
	
	SetFocus();

	ResetContent(); 
	for (POSITION Pos_ = m_DropDownList.GetHeadPosition(); Pos_ != NULL;)
	{
		AddString((LPCTSTR) (m_DropDownList.GetNext(Pos_)));
	}

	return 0;
}

BOOL CInPlaceCombo::PreTranslateMessage(MSG* pMsg) 
{
	// If the message if for "Enter" or "Esc"
	// Do not process
	if (pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
		{
			::TranslateMessage(pMsg);
			::DispatchMessage(pMsg);
			// DO NOT process further
			return TRUE;				
		}
	}
	
	return CComboBox::PreTranslateMessage(pMsg);
}

void CInPlaceCombo::OnKillFocus(CWnd* pNewWnd) 
{
	CComboBox::OnKillFocus(pNewWnd);
	
	// Get the current selection text
	CString str;
	GetWindowText(str);

	// Send Notification to parent of ListView ctrl
	LV_DISPINFO dispinfo;
	dispinfo.hdr.hwndFrom = GetParent()->m_hWnd;
	dispinfo.hdr.idFrom = GetDlgCtrlID();
	dispinfo.hdr.code = LVN_ENDLABELEDIT;

	dispinfo.item.mask = LVIF_TEXT;
	dispinfo.item.iItem = m_iRowIndex;
	dispinfo.item.iSubItem = m_iColumnIndex;
	dispinfo.item.pszText = m_bESC ? LPTSTR((LPCTSTR)m_strWindowText) : LPTSTR((LPCTSTR)str);
	dispinfo.item.cchTextMax = m_bESC ? m_strWindowText.GetLength() : str.GetLength();
	
	GetParent()->SendMessage(WM_NOTIFY, GetParent()->GetDlgCtrlID(), (LPARAM)&dispinfo);

	// Close the control
	PostMessage(WM_CLOSE);
}

void CInPlaceCombo::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// If the key is "Esc" set focus back to the list control
	if (nChar == VK_ESCAPE || nChar == VK_RETURN)
	{
		if (nChar == VK_ESCAPE)
		{
			m_bESC = TRUE;
		}

		GetParent()->SetFocus();
		return;
	}
	
	CComboBox::OnChar(nChar, nRepCnt, nFlags);
}

void CInPlaceCombo::OnCloseup() 
{
	// Set the focus to the parent list control
	GetParent()->SetFocus();
}

CInPlaceCombo* CInPlaceCombo::GetInstance()
{
	if(m_pInPlaceCombo == NULL)
	{
		m_pInPlaceCombo = new CInPlaceCombo;
	}
	return m_pInPlaceCombo;
}

void CInPlaceCombo::DeleteInstance()
{
	delete m_pInPlaceCombo;
	m_pInPlaceCombo = NULL;
}

BOOL CInPlaceCombo::ShowComboCtrl(DWORD dwStyle, const CRect &rCellRect, CWnd* pParentWnd, UINT uiResourceID,
								  int iRowIndex, int iColumnIndex, CStringList* pDropDownList, 
								  CString strCurSelecetion /*= ""*/, int iCurSel /*= -1*/)
{

	m_iRowIndex = iRowIndex;
	m_iColumnIndex = iColumnIndex;
	m_bESC = FALSE;
	
	m_DropDownList.RemoveAll(); 
	m_DropDownList.AddTail(pDropDownList);

	BOOL bRetVal = TRUE;

	if (-1 != iCurSel)
	{
		GetLBText(iCurSel, m_strWindowText);
	}
	else if (!strCurSelecetion.IsEmpty()) 
	{
		m_strWindowText = strCurSelecetion;
	}
	
	if (NULL == m_pInPlaceCombo->m_hWnd) 
	{
		bRetVal = m_pInPlaceCombo->Create(dwStyle, rCellRect, pParentWnd, uiResourceID); 
//		m_pInPlaceCombo->SetItemHeight(-1, 8);
	}

	SetCurSel(iCurSel);

	return bRetVal;
}


//
// WM_PAINT message handler
//

void CInPlaceCombo::OnPaint() 
{
	COLORREF rgbBorder, rgbWindow;
	CRect rcItem;
	CDC *pDC;

	ModifyStyleEx (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
	Default();

	rgbBorder = GetSysColor(COLOR_WINDOWFRAME);
	rgbWindow = GetSysColor(COLOR_WINDOW);
	GetClientRect(&rcItem);
	pDC = GetDC();
	
	// Cover up dark 3D shadow.
	pDC->Draw3dRect(rcItem, rgbBorder, rgbBorder);
	rcItem.DeflateRect(1, 1);
	pDC->Draw3dRect(rcItem, rgbWindow, rgbWindow);

	// Cover up dark 3D shadow on drop arrow.
	rcItem.DeflateRect(1, 1);
	rcItem.left = rcItem.right - m_nOffset;
	pDC->Draw3dRect(rcItem, rgbWindow, rgbWindow);
	
	// Cover up normal 3D shadow on drop arrow.
	rcItem.DeflateRect(1, 1);
	pDC->Draw3dRect(rcItem, rgbBorder, rgbBorder);
	
	ReleaseDC(pDC);
}
