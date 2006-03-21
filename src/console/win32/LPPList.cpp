// LPPList.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "LPPList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLPPList

IMPLEMENT_DYNCREATE(CLPPList, CMDIChildWnd)

CLPPList::CLPPList()
{
}

CLPPList::~CLPPList()
{
}


BEGIN_MESSAGE_MAP(CLPPList, CMDIChildWnd)
	//{{AFX_MSG_MAP(CLPPList)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLPPList message handlers

BOOL CLPPList::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_LPP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CLPPList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_LPP_EDITOR, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(2, "Version", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "Flags", LVCFMT_LEFT, 70);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_SETFOCUS message handler
//

void CLPPList::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CLPPList::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_DESTROY message handler
//

void CLPPList::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_LPP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CLPPList::OnViewRefresh() 
{
   DWORD i, dwResult;
   NXC_LPP_LIST *pList;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg2(NXCLoadLPPList, g_hSession, &pList,
                            _T("Loading list of log processing policies..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < pList->dwNumEntries; i++)
         AddListEntry(pList->pList[i].dwId, pList->pList[i].szName,
                      pList->pList[i].dwVersion, pList->pList[i].dwFlags);
      NXCDestroyLPPList(pList);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading list of log processing policies: %s"));
   }
}


//
// Add new entry to list
//

void CLPPList::AddListEntry(DWORD dwId, TCHAR *pszName, DWORD dwVersion, DWORD dwFlags)
{
   int iItem;
   TCHAR szBuffer[64];

   _stprintf(szBuffer, _T("%d"), dwId);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, 0);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, dwId);
      m_wndListCtrl.SetItemText(iItem, 1, pszName);
      _stprintf(szBuffer, _T("%d"), dwVersion);
      m_wndListCtrl.SetItemText(iItem, 2, szBuffer);
      _stprintf(szBuffer, _T("0x%08X"), dwFlags);
      m_wndListCtrl.SetItemText(iItem, 3, szBuffer);
   }
}
