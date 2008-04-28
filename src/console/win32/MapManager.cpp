// MapManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapManager

IMPLEMENT_DYNCREATE(CMapManager, CMDIChildWnd)

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
}


BEGIN_MESSAGE_MAP(CMapManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMapManager)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapManager message handlers


//
// WM_CREATE message handler
//

int CMapManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	GetClientRect(&rect);
	m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
	m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
	m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(2, _T("Root object"), LVCFMT_LEFT, 150);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	LoadListCtrlColumns(m_wndListCtrl, _T("MapManager"), _T("ListCtrl"));

	PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_SIZE message handler
//

void CMapManager::OnSize(UINT nType, int cx, int cy) 
{
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CMapManager::OnSetFocus(CWnd* pOldWnd) 
{
	m_wndListCtrl.SetFocus();
}


//
// WM_DESTROY message handler
//

void CMapManager::OnDestroy() 
{
	SaveListCtrlColumns(m_wndListCtrl, _T("MapManager"), _T("ListCtrl"));
	CMDIChildWnd::OnDestroy();
}
