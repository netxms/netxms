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
}

CTrapEditor::~CTrapEditor()
{
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
	
   theApp.OnViewCreate(IDR_TRAP_EDITOR, this);

   m_pImageList = CreateEventImageList();
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "SNMP Trap ID", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(2, "Event", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, "Description", LVCFMT_LEFT, 250);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_DESTROY message handler
//

void CTrapEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_TRAP_EDITOR, this);
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
   DoRequest(NXCUnlockTrapCfg, "Unlocking SNMP trap configuration database...");
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
   dwResult = DoRequestArg2(NXCLoadTrapCfg, &m_dwNumTraps, &m_pTrapList, _T("Loading SNMP trap configuration..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumTraps; i++)
         AddItem(i);
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

   _stprintf(szBuffer, _T("%d"), m_pTrapList[dwIndex].dwId);
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
   m_wndListCtrl.SetItemText(iItem, 2, NXCGetEventName(m_pTrapList[dwIndex].dwEventId));
   m_wndListCtrl.SetItemText(iItem, 3, m_pTrapList[dwIndex].szDescription);
   
   m_wndListCtrl.SetItem(iItem, 0, LVIF_IMAGE, NULL, 
                         NXCGetEventSeverity(m_pTrapList[dwIndex].dwEventId),
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

   dwResult = DoRequestArg1(NXCCreateTrap, &dwTrapId, _T("Creating trap configuration record..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_pTrapList = (NXC_TRAP_CFG_ENTRY *)realloc(m_pTrapList, sizeof(NXC_TRAP_CFG_ENTRY) * (m_dwNumTraps + 1));
      memset(&m_pTrapList[m_dwNumTraps], 0, sizeof(NXC_TRAP_CFG_ENTRY));
      m_pTrapList[m_dwNumTraps].dwId = dwTrapId;
      m_pTrapList[m_dwNumTraps].dwEventId = EVENT_SNMP_UNMATCHED_TRAP;
      m_dwNumTraps++;
      SelectListViewItem(&m_wndListCtrl, AddItem(m_dwNumTraps - 1));
      PostMessage(WM_COMMAND, ID_TRAP_EDIT);
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
   LVFINDINFO lvfi;
   int iItem;

   lvfi.flags = LVFI_PARAM;
   for(i = 0; i < dwNumTraps; i++)
   {
      dwResult = NXCDeleteTrap(pdwDeleteList[i]);
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
         pdwDeleteList[i] = m_wndListCtrl.GetItemData(iItem);
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
// WM_COMMAND::ID_TRAP_EDIT message handler
//

void CTrapEditor::OnTrapEdit() 
{
   CTrapEditDlg dlg;
   int iItem;
   DWORD i, dwIndex, dwTrapId, dwResult;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      if (iItem != -1)
      {
         dwTrapId = m_wndListCtrl.GetItemData(iItem);
         for(dwIndex = 0; dwIndex < m_dwNumTraps; dwIndex++)
            if (m_pTrapList[dwIndex].dwId == dwTrapId)
               break;
         if (dwIndex < m_dwNumTraps)
         {
            // Copy existing record to dialog object for editing
            memcpy(&dlg.m_trap, &m_pTrapList[dwIndex], sizeof(NXC_TRAP_CFG_ENTRY));
            dlg.m_trap.pdwObjectId = 
               (DWORD *)nx_memdup(m_pTrapList[dwIndex].pdwObjectId, 
                                  sizeof(DWORD) * m_pTrapList[dwIndex].dwOidLen);
            dlg.m_trap.pMaps = 
               (NXC_OID_MAP *)nx_memdup(m_pTrapList[dwIndex].pMaps, 
                                        sizeof(NXC_OID_MAP) * m_pTrapList[dwIndex].dwNumMaps);
            for(i = 0; i < m_pTrapList[dwIndex].dwNumMaps; i++)
            {
               dlg.m_trap.pMaps[i].pdwObjectId = 
                  (DWORD *)nx_memdup(m_pTrapList[dwIndex].pMaps[i].pdwObjectId, 
                                     sizeof(DWORD) * m_pTrapList[dwIndex].pMaps[i].dwOidLen);
            }

            // Run dialog and update record list on success
            if (dlg.DoModal() == IDOK)
            {
               // Update record on server
               dwResult = DoRequestArg1(NXCModifyTrap, &dlg.m_trap, _T("Updating trap configuration..."));
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
               }
               else
               {
                  theApp.ErrorBox(dwResult, _T("Error updating trap configuration:\n%s"));
               }
            }
         }
      }
   }
}
