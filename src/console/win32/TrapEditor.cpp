// TrapEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "TrapEditor.h"
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
}

CTrapEditor::~CTrapEditor()
{
   if (m_pTrapList != NULL)
      NXCDestroyTrapList(m_dwNumTraps, m_pTrapList);
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
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

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

void CTrapEditor::AddItem(DWORD dwIndex)
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
}

void CTrapEditor::OnUpdateTrapEdit(CCmdUI* pCmdUI) 
{
}


//
// WM_COMMAND::ID_TRAP_NEW message handler
//

void CTrapEditor::OnTrapNew() 
{
	// TODO: Add your command handler code here
	
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
