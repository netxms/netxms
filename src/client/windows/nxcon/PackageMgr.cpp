// PackageMgr.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "PackageMgr.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPackageMgr

IMPLEMENT_DYNCREATE(CPackageMgr, CMDIChildWnd)

CPackageMgr::CPackageMgr()
{
   m_pPackageList = NULL;
   m_dwNumPackages = 0;
}

CPackageMgr::~CPackageMgr()
{
   safe_free(m_pPackageList);
}


BEGIN_MESSAGE_MAP(CPackageMgr, CMDIChildWnd)
	//{{AFX_MSG_MAP(CPackageMgr)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_COMMAND(ID_PACKAGE_INSTALL, OnPackageInstall)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_PACKAGE_REMOVE, OnUpdatePackageRemove)
	ON_COMMAND(ID_PACKAGE_REMOVE, OnPackageRemove)
	ON_COMMAND(ID_PACKAGE_DEPLOY, OnPackageDeploy)
	ON_UPDATE_COMMAND_UI(ID_PACKAGE_DEPLOY, OnUpdatePackageDeploy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPackageMgr message handlers

BOOL CPackageMgr::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_DATABASE));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CPackageMgr::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_PACKAGE_MANAGER, this);
	
   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_PACKAGE));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(2, _T("Version"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Platform"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(4, _T("File"), LVCFMT_LEFT, 200);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_CLOSE message handler
//

void CPackageMgr::OnClose() 
{
   DoRequestArg1(NXCUnlockPackageDB, g_hSession, _T("Unlocking package database..."));
	CMDIChildWnd::OnClose();
}


//
// WM_DESTROY message handler
//

void CPackageMgr::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_PACKAGE_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CPackageMgr::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CPackageMgr::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndListCtrl.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CPackageMgr::OnViewRefresh() 
{
   DWORD i, dwResult;

   m_wndListCtrl.DeleteAllItems();
   safe_free(m_pPackageList);
   dwResult = DoRequestArg3(NXCGetPackageList, g_hSession, &m_dwNumPackages, 
                            &m_pPackageList, _T("Loading package database..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumPackages; i++)
      {
         AddListItem(&m_pPackageList[i], FALSE);
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading package list from server: %s"));
   }
}


//
// Add new item to package list
//

void CPackageMgr::AddListItem(NXC_PACKAGE_INFO *pInfo, BOOL bSelect)
{
   TCHAR szBuffer[64];
   int iItem;

   _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%d"), pInfo->dwId);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, 0);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, pInfo->dwId);
      m_wndListCtrl.SetItemText(iItem, 1, pInfo->szName);
      m_wndListCtrl.SetItemText(iItem, 2, pInfo->szVersion);
      m_wndListCtrl.SetItemText(iItem, 3, pInfo->szPlatform);
      m_wndListCtrl.SetItemText(iItem, 4, pInfo->szFileName);

      if (bSelect)
         SelectListViewItem(&m_wndListCtrl, iItem);
   }
}


//
// Package installation worker function
//

static DWORD InstallPackage(TCHAR *pszPkgFile, NXC_PACKAGE_INFO *pInfo)
{
   DWORD dwResult;
   TCHAR *ptr, szDataFile[MAX_PATH];

   // First, parse package information file
   dwResult = NXCParseNPIFile(pszPkgFile, pInfo);
   if (dwResult == RCC_SUCCESS)
   {
      nx_strncpy(szDataFile, pszPkgFile, MAX_PATH);
      ptr = _tcsrchr(szDataFile, _T('\\'));
      ASSERT(ptr != NULL);
      if (ptr == NULL)  // Normally we should receive full path, so it's just a paranoid check
         ptr = szDataFile;
      else
         ptr++;
      _tcscpy(ptr, pInfo->szFileName);

      dwResult = NXCInstallPackage(g_hSession, pInfo, szDataFile);
   }

   return dwResult;
}


//
// WM_COMMAND::ID_PACKAGE_INSTALL message handler
//

void CPackageMgr::OnPackageInstall() 
{
   CFileDialog dlg(TRUE, _T(".npi"), NULL, OFN_FILEMUSTEXIST, 
                   _T("NetXMS Package Info Files (*.npi)|*.npi|All Files (*.*)|*.*||"), this);
   DWORD dwResult;
   NXC_PACKAGE_INFO info;

   if (dlg.DoModal() == IDOK)
   {
      dwResult = DoRequestArg2(InstallPackage, dlg.m_ofn.lpstrFile, &info, _T("Installing package..."));
      if (dwResult == RCC_SUCCESS)
      {
         AddListItem(&info, TRUE);
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Unable to install package: %s"));
      }
   }
}


//
// WM_CONTEXTMENU message handler
//

void CPackageMgr::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(13);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// OnUpdate... handlers
//

void CPackageMgr::OnUpdatePackageRemove(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CPackageMgr::OnUpdatePackageDeploy(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// Worker function for package deletion
//

static DWORD RemovePackages(DWORD dwNumPackages, DWORD *pdwDeleteList, CListCtrl *pList)
{
   DWORD i, dwResult = RCC_SUCCESS;
   LVFINDINFO lvfi;
   int iItem;

   lvfi.flags = LVFI_PARAM;
   for(i = 0; i < dwNumPackages; i++)
   {
      dwResult = NXCRemovePackage(g_hSession, pdwDeleteList[i]);
      if (dwResult == RCC_SUCCESS)
      {
         lvfi.lParam = pdwDeleteList[i];
         iItem = pList->FindItem(&lvfi);
         if (iItem != -1)
            pList->DeleteItem(iItem);
      }
      else
      {
         break;
      }
   }
   return dwResult;
}


//
// WM_COMMAND::ID_PACKAGE_REMOVE message handler
//

void CPackageMgr::OnPackageRemove() 
{
   int iItem;
   DWORD i, dwNumItems, *pdwDeleteList, dwResult;

   dwNumItems = m_wndListCtrl.GetSelectedCount();
   if (dwNumItems > 0)
   {
      pdwDeleteList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; (i < dwNumItems) && (iItem != -1); i++)
      {
         pdwDeleteList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }

      dwResult = DoRequestArg3(RemovePackages, (void *)dwNumItems, pdwDeleteList, 
                               &m_wndListCtrl, _T("Deleting package(s)..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error removing package:\n%s"));
      free(pdwDeleteList);
   }
}


//
// WM_COMMAND::ID_PACKAGE_DEPLOY message handler
//

void CPackageMgr::OnPackageDeploy() 
{
   int iItem;

   iItem = m_wndListCtrl.GetSelectionMark();
   if (iItem != -1)
   {
      CObjectSelDlg dlg;

      dlg.m_bAllowEmptySelection = FALSE;
      dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET | SCL_NETWORK | SCL_SERVICEROOT;
      if (dlg.DoModal() == IDOK)
      {
         theApp.DeployPackage(m_wndListCtrl.GetItemData(iItem), dlg.m_dwNumObjects,
                              dlg.m_pdwObjectList);
      }
   }
}
