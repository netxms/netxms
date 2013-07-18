// TrapEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapEditor.h"
#include "TrapEditDlg.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrapEditor

IMPLEMENT_DYNCREATE(CTrapEditor, CMDIChildWnd)

CTrapEditor::CTrapEditor()
{
   m_pTrapList = NULL;
   m_dwNumTraps = 0;
   m_pImageList = NULL;

   m_iSortMode = theApp.GetProfileInt(_T("TrapEditor"), _T("SortMode"), 0);
   m_iSortDir = theApp.GetProfileInt(_T("TrapEditor"), _T("SortDir"), 1);
}

CTrapEditor::~CTrapEditor()
{
   theApp.WriteProfileInt(_T("TrapEditor"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("TrapEditor"), _T("SortDir"), m_iSortDir);

   if (m_pTrapList != NULL)
      NXCDestroyTrapList(m_dwNumTraps, m_pTrapList);
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CTrapEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CTrapEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_TRAP_DELETE, OnUpdateTrapDelete)
	ON_UPDATE_COMMAND_UI(ID_TRAP_EDIT, OnUpdateTrapEdit)
	ON_COMMAND(ID_TRAP_NEW, OnTrapNew)
	ON_COMMAND(ID_TRAP_DELETE, OnTrapDelete)
	ON_COMMAND(ID_TRAP_EDIT, OnTrapEdit)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrapEditor message handlers

BOOL CTrapEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_TRAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CTrapEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_TRAP_EDITOR, this);

   // Create image list
   m_pImageList = CreateEventImageList();
   m_iSortImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(1, _T("SNMP Trap ID"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(2, _T("Event"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, _T("Description"), LVCFMT_LEFT, 250);

	LoadListCtrlColumns(m_wndListCtrl, _T("TrapEditor"), _T("ListCtrl"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CTrapEditor::OnDestroy() 
{
	SaveListCtrlColumns(m_wndListCtrl, _T("TrapEditor"), _T("ListCtrl"));
   theApp.OnViewDestroy(VIEW_TRAP_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CTrapEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CTrapEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_CLOSE message handler
//

void CTrapEditor::OnClose() 
{
	CMDIChildWnd::OnClose();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CTrapEditor::OnViewRefresh() 
{
   DWORD i, dwResult;

   if (m_pTrapList != NULL)
      NXCDestroyTrapList(m_dwNumTraps, m_pTrapList);
   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg3(NXCLoadTrapCfg, g_hSession, &m_dwNumTraps, 
                            &m_pTrapList, _T("Loading SNMP trap configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumTraps; i++)
         AddItem(i);
      SortList();
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading SNMP trap configuration: %s"));
   }
}


//
// Add new item to list
//

int CTrapEditor::AddItem(DWORD dwIndex)
{
   TCHAR szBuffer[32];
   int iItem;

   _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), m_pTrapList[dwIndex].dwId);
   iItem = m_wndListCtrl.InsertItem(dwIndex, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, m_pTrapList[dwIndex].dwId);
      UpdateItem(iItem, dwIndex);
   }
   return iItem;
}


//
// Update item in list
//

void CTrapEditor::UpdateItem(int iItem, DWORD dwIndex)
{
   TCHAR szBuffer[1024];

   SNMPConvertOIDToText(m_pTrapList[dwIndex].dwOidLen, m_pTrapList[dwIndex].pdwObjectId,
                        szBuffer, 1024);
   m_wndListCtrl.SetItemText(iItem, 1, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 2, NXCGetEventName(g_hSession, m_pTrapList[dwIndex].dwEventCode));
   m_wndListCtrl.SetItemText(iItem, 3, m_pTrapList[dwIndex].szDescription);
   
   m_wndListCtrl.SetItem(iItem, 0, LVIF_IMAGE, NULL, 
                         NXCGetEventSeverity(g_hSession, m_pTrapList[dwIndex].dwEventCode),
                         0, 0, 0);
}


//
// WM_CONTEXTMENU message handler
//

void CTrapEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(11);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Update status for menu items
//

void CTrapEditor::OnUpdateTrapDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}

void CTrapEditor::OnUpdateTrapEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// WM_COMMAND::ID_TRAP_NEW message handler
//

void CTrapEditor::OnTrapNew() 
{
   DWORD dwResult, dwTrapId;

   dwResult = DoRequestArg2(NXCCreateTrap, g_hSession, &dwTrapId, _T("Creating trap configuration record..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_pTrapList = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapList, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
      memset(&m_pTrapList[m_dwNumTraps], 0, sizeof(NXC_TRAP_CFG_ENTRY));
      m_pTrapList[m_dwNumTraps].dwId = dwTrapId;
      m_pTrapList[m_dwNumTraps].dwEventCode = EVENT_SNMP_UNMATCHED_TRAP;
      m_dwNumTraps++;
      SelectListViewItem(&m_wndListCtrl, AddItem(m_dwNumTraps - 1));
      EditTrap(TRUE);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error creating trap configuration:\n%s"));
   }
}


//
// Worker function for trap deletion
//

static DWORD DeleteTraps(DWORD dwNumTraps, DWORD *pdwDeleteList, CListCtrl *pList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; i < dwNumTraps; i++)
   {
      dwResult = NXCDeleteTrap(g_hSession, pdwDeleteList[i]);
      if (dwResult != RCC_SUCCESS)
         break;
   }
   return dwResult;
}


//
// WM_COMMAND::ID_TRAP_DELETE message handler
//

void CTrapEditor::OnTrapDelete() 
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
         pdwDeleteList[i] = (DWORD)m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }

      dwResult = DoRequestArg3(DeleteTraps, (void *)dwNumItems, pdwDeleteList, 
                               &m_wndListCtrl, _T("Deleting trap configuration record(s)..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error deleting trap configuration record:\n%s"));
      free(pdwDeleteList);
   }
}


//
// Edit currently selected trap record
//

void CTrapEditor::EditTrap(BOOL bNewTrap)
{
   CTrapEditDlg dlg;
   int iItem;
   DWORD i, dwIndex, dwTrapId, dwResult;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      if (iItem != -1)
      {
         dwTrapId = (DWORD)m_wndListCtrl.GetItemData(iItem);
         for(dwIndex = 0; dwIndex < m_dwNumTraps; dwIndex++)
            if (m_pTrapList[dwIndex].dwId == dwTrapId)
               break;
         if (dwIndex < m_dwNumTraps)
         {
            // Copy existing record to dialog object for editing
				NXCCopyTrapCfgEntry(&dlg.m_trap, &m_pTrapList[dwIndex]);

            // Run dialog and update record list on success
            if (dlg.DoModal() == IDOK)
            {
               // Update record on server
               dwResult = DoRequestArg2(NXCModifyTrap, g_hSession,
                                        &dlg.m_trap, _T("Updating trap configuration..."));
               if (dwResult == RCC_SUCCESS)
               {
                  // Clean existing configuration record
                  for(i = 0; i < m_pTrapList[dwIndex].dwNumMaps; i++)
                     safe_free(m_pTrapList[dwIndex].pMaps[i].pdwObjectId);
                  safe_free(m_pTrapList[dwIndex].pMaps);
                  safe_free(m_pTrapList[dwIndex].pdwObjectId);

                  // Copy updated record over existing
                  memcpy(&m_pTrapList[dwIndex], &dlg.m_trap, sizeof(NXC_TRAP_CFG_ENTRY));

                  // Prevent memory blocks from freeing by dialog destructor
                  dlg.m_trap.pdwObjectId = NULL;
                  dlg.m_trap.dwNumMaps = 0;
                  dlg.m_trap.pMaps = NULL;

                  UpdateItem(iItem, dwIndex);
                  SortList();
               }
               else
               {
                  theApp.ErrorBox(dwResult, _T("Error updating trap configuration:\n%s"));
               }
            }
            else  /* user press cancel */
            {
               if (bNewTrap)
                  PostMessage(WM_COMMAND, ID_TRAP_DELETE, 0);
            }
         }
      }
   }
}


//
// WM_COMMAND::ID_TRAP_EDIT message handler
//

void CTrapEditor::OnTrapEdit() 
{
   EditTrap(FALSE);
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CTrapEditor::OnListViewDblClk(NMHDR *pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_TRAP_EDIT, 0);
}


//
// Compare two OIDs
//

static int CompareOID(UINT32 *pdwOid1, UINT32 *pdwOid2, UINT32 dwLen)
{
   DWORD i;

   for(i = 0; i < dwLen; i++)
   {
      if (pdwOid1[i] < pdwOid2[i])
         return -1;
      if (pdwOid1[i] > pdwOid2[i])
         return 1;
   }
   return 0;
}


//
// Trap comparision procedure for sorting
//

static int CALLBACK TrapCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CTrapEditor *pWnd = (CTrapEditor *)lParamSort;
   NXC_TRAP_CFG_ENTRY *pTrap1, *pTrap2;
   TCHAR szEvent1[MAX_EVENT_NAME], szEvent2[MAX_EVENT_NAME];
   int iResult;
   
   pTrap1 = pWnd->GetTrapById((DWORD)lParam1);
   pTrap2 = pWnd->GetTrapById((DWORD)lParam2);
   
   switch(pWnd->GetSortMode())
   {
      case 0:     // ID
         iResult = COMPARE_NUMBERS(pTrap1->dwId, pTrap2->dwId);
         break;
      case 1:     // Object ID
         iResult = CompareOID(pTrap1->pdwObjectId, pTrap2->pdwObjectId,
                              min(pTrap1->dwOidLen, pTrap2->dwOidLen));
         if (iResult == 0)
            iResult = COMPARE_NUMBERS(pTrap1->dwOidLen, pTrap2->dwOidLen);
         break;
      case 2:     // Event
         NXCGetEventNameEx(g_hSession, pTrap1->dwEventCode, szEvent1, MAX_EVENT_NAME);
         NXCGetEventNameEx(g_hSession, pTrap2->dwEventCode, szEvent2, MAX_EVENT_NAME);
         iResult = _tcsicmp(szEvent1, szEvent2);
         break;
      case 3:     // Description
         iResult = _tcsicmp(pTrap1->szDescription, pTrap2->szDescription);
         break;
      default:
         iResult = 0;
         break;
   }

   return iResult * pWnd->GetSortDir();
}


//
// Sort trap list
//

void CTrapEditor::SortList()
{
   LVCOLUMN lvc;

   m_wndListCtrl.SortItems(TrapCompareProc, (LPARAM)this);
   lvc.mask = LVCF_IMAGE | LVCF_FMT;
   lvc.fmt = LVCFMT_LEFT | LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT;
   lvc.iImage = (m_iSortDir == 1) ? m_iSortImageBase : m_iSortImageBase + 1;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvc);
}


//
// Get trap structure by id
//

NXC_TRAP_CFG_ENTRY *CTrapEditor::GetTrapById(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < m_dwNumTraps; i++)
      if (m_pTrapList[i].dwId == dwId)
         return &m_pTrapList[i];
   return NULL;
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CTrapEditor::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == ((LPNMLISTVIEW)pNMHDR)->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   SortList();
   
   *pResult = 0;
}


//
// Handler for trap configuration update notifications
//

void CTrapEditor::OnTrapCfgUpdate(DWORD code, NXC_TRAP_CFG_ENTRY *trap)
{
   LVFINDINFO lvfi;
   int item, index = -1;

   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = trap->dwId;
   item = m_wndListCtrl.FindItem(&lvfi);

	for(DWORD i = 0; i < m_dwNumTraps; i++)
		if (m_pTrapList[i].dwId == trap->dwId)
		{
			index = (int)i;
			break;
		}

	switch(code)
	{
		case NX_NOTIFY_TRAPCFG_CREATED:
		case NX_NOTIFY_TRAPCFG_MODIFIED:
			if (index == -1)
			{
				m_pTrapList = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapList, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
				NXCCopyTrapCfgEntry(&m_pTrapList[m_dwNumTraps], trap);
				index = (int)m_dwNumTraps;
				m_dwNumTraps++;
			}
			else
			{
				for(DWORD i = 0; i < m_pTrapList[index].dwNumMaps; i++)
					safe_free(m_pTrapList[index].pMaps[i].pdwObjectId);
				safe_free(m_pTrapList[index].pMaps);
				safe_free(m_pTrapList[index].pdwObjectId);
				NXCCopyTrapCfgEntry(&m_pTrapList[index], trap);
			}
			if (item == -1)
			{
				AddItem(index);
			}
			else
			{
				UpdateItem(item, index);
			}
		   m_wndListCtrl.SortItems(TrapCompareProc, (LPARAM)this);
			break;
		case NX_NOTIFY_TRAPCFG_DELETED:
         if (item != -1)
            m_wndListCtrl.DeleteItem(item);
			break;
		default:
			break;
	}
}
