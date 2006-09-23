// ModuleManager.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ModuleManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModuleManager

IMPLEMENT_DYNCREATE(CModuleManager, CMDIChildWnd)

CModuleManager::CModuleManager()
{
}

CModuleManager::~CModuleManager()
{
}


BEGIN_MESSAGE_MAP(CModuleManager, CMDIChildWnd)
	//{{AFX_MSG_MAP(CModuleManager)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModuleManager message handlers

BOOL CModuleManager::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_MODULE));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CModuleManager::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   theApp.OnViewCreate(VIEW_MODULE_MANAGER, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(2, _T("Executable"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(3, _T("Flags"), LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(4, _T("Description"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(5, _T("License Key"), LVCFMT_LEFT, 150);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	
	return 0;
}


//
// WM_DESTROY message handler
//

void CModuleManager::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_MODULE_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CModuleManager::OnViewRefresh() 
{
   DWORD i, dwResult;
   NXC_SERVER_MODULE_LIST *pList;
   TCHAR szBuffer[64];
   int iItem;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg2(NXCGetServerModuleList, g_hSession, &pList,
                            _T("Loading module list..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < pList->dwNumModules; i++)
      {
         _stprintf(szBuffer, _T("%d"), pList->pModules[i].dwModuleId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, pList->pModules[i].dwModuleId);
            m_wndListCtrl.SetItemText(iItem, 1, pList->pModules[i].pszName);
            m_wndListCtrl.SetItemText(iItem, 2, pList->pModules[i].pszExecutable);
            
            // Flags
            _tcscpy(szBuffer, _T("--"));
            if (pList->pModules[i].dwFlags & MODFLAG_DISABLED)
               szBuffer[0] = _T('D');
            if (pList->pModules[i].dwFlags & MODFLAG_ACCEPT_ALL_COMMANDS)
               szBuffer[1] = _T('A');
            m_wndListCtrl.SetItemText(iItem, 3, szBuffer);

            m_wndListCtrl.SetItemText(iItem, 4, pList->pModules[i].pszDescription);
            m_wndListCtrl.SetItemText(iItem, 5, pList->pModules[i].pszLicenseKey);
         }
      }
      NXCDestroyModuleList(pList);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load module list: %s"));
   }
}


//
// WM_SETFOCUS message handler
//

void CModuleManager::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CModuleManager::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}
