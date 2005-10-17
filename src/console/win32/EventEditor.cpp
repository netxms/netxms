// EventEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EventEditor.h"
#include "EditEventDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEventEditor

IMPLEMENT_DYNCREATE(CEventEditor, CMDIChildWnd)

CEventEditor::CEventEditor()
{
   m_pImageList = NULL;
}

CEventEditor::~CEventEditor()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CEventEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CEventEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_EVENT_EDIT, OnUpdateEventEdit)
	ON_COMMAND(ID_EVENT_EDIT, OnEventEdit)
	ON_UPDATE_COMMAND_UI(ID_EVENT_DELETE, OnUpdateEventDelete)
	ON_COMMAND(ID_EVENT_NEW, OnEventNew)
	ON_COMMAND(ID_EVENT_DELETE, OnEventDelete)
	//}}AFX_MSG_MAP
   ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDoubleClick)
END_MESSAGE_MAP()

//
// CEventEditor message handlers
//

int CEventEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   TCHAR szBuffer[32];
   DWORD i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   theApp.OnViewCreate(IDR_EVENT_EDITOR, this);

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 190);
   m_wndListCtrl.InsertColumn(2, _T("Severity"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, _T("Flags"), LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(4, _T("Message"), LVCFMT_LEFT, 300);
   m_wndListCtrl.InsertColumn(5, _T("Description"), LVCFMT_LEFT, 300);
	
   // Create image list
   m_pImageList = CreateEventImageList();
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Load event templates
   NXCGetEventDB(g_hSession, &m_ppEventTemplates, &m_dwNumTemplates);
   for(i = 0; i < m_dwNumTemplates; i++)
   {
      _stprintf(szBuffer, _T("%ld"), m_ppEventTemplates[i]->dwCode);
      m_wndListCtrl.InsertItem(i, szBuffer, m_ppEventTemplates[i]->dwSeverity);
      m_wndListCtrl.SetItemData(i, m_ppEventTemplates[i]->dwCode);
      UpdateItem(i, m_ppEventTemplates[i]);
   }

   return 0;
}


//
// Update list view item
//

void CEventEditor::UpdateItem(int iItem, NXC_EVENT_TEMPLATE *pData)
{
   TCHAR szBuffer[32];

   m_wndListCtrl.SetItem(iItem, 0, LVIF_IMAGE, NULL, pData->dwSeverity, 0, 0, 0);
   m_wndListCtrl.SetItemText(iItem, 1, pData->szName);
   m_wndListCtrl.SetItemText(iItem, 2, g_szStatusTextSmall[pData->dwSeverity]);
   _stprintf(szBuffer, _T("%ld"), pData->dwFlags);
   m_wndListCtrl.SetItemText(iItem, 3, szBuffer);
   m_wndListCtrl.SetItemText(iItem, 4, pData->pszMessage);
   m_wndListCtrl.SetItemText(iItem, 5, pData->pszDescription);
}


//
// WM_DESTROY message handler
//

void CEventEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_EVENT_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CEventEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Handler for double-clicks in list view
//

void CEventEditor::OnListViewDoubleClick(NMITEMACTIVATE *pInfo, LRESULT *pResult)
{
   if (pInfo->iItem != -1)
      EditEvent(pInfo->iItem);
}


//
// Edit currently selected event
//

BOOL CEventEditor::EditEvent(int iItem)
{
   DWORD dwIndex, dwResult, dwId;
   CEditEventDlg dlgEditEvent;
   BOOL bResult = FALSE;

   dwId = m_wndListCtrl.GetItemData(iItem);
   for(dwIndex = 0; dwIndex < m_dwNumTemplates; dwIndex++)
      if (m_ppEventTemplates[dwIndex]->dwCode == dwId)
         break;

   if (dwIndex < m_dwNumTemplates)
   {
      dlgEditEvent.m_bWriteLog = m_ppEventTemplates[dwIndex]->dwFlags & EF_LOG;
      dlgEditEvent.m_dwEventId = m_ppEventTemplates[dwIndex]->dwCode;
      dlgEditEvent.m_strName = m_ppEventTemplates[dwIndex]->szName;
      dlgEditEvent.m_strMessage = m_ppEventTemplates[dwIndex]->pszMessage;
      dlgEditEvent.m_strDescription = m_ppEventTemplates[dwIndex]->pszDescription;
      dlgEditEvent.m_dwSeverity = m_ppEventTemplates[dwIndex]->dwSeverity;

      if (dlgEditEvent.DoModal() == IDOK)
      {
         NXC_EVENT_TEMPLATE evt;

         evt.dwCode = m_ppEventTemplates[dwIndex]->dwCode;
         evt.dwFlags = dlgEditEvent.m_bWriteLog ? EF_LOG : 0;
         evt.dwSeverity = dlgEditEvent.m_dwSeverity;
         evt.pszDescription = _tcsdup((LPCTSTR)dlgEditEvent.m_strDescription);
         evt.pszMessage = _tcsdup((LPCTSTR)dlgEditEvent.m_strMessage);
         nx_strncpy(evt.szName, (LPCTSTR)dlgEditEvent.m_strName, MAX_EVENT_NAME);

         dwResult = DoRequestArg2(NXCSetEventInfo, g_hSession, 
                                  &evt, _T("Updating event configuration database..."));
         if (dwResult == RCC_SUCCESS)
         {
            // Record was successfully updated on server, update local copy
            safe_free(m_ppEventTemplates[dwIndex]->pszDescription);
            safe_free(m_ppEventTemplates[dwIndex]->pszMessage);
            memcpy(m_ppEventTemplates[dwIndex], &evt, sizeof(NXC_EVENT_TEMPLATE));
            UpdateItem(iItem, m_ppEventTemplates[dwIndex]);
            bResult = TRUE;
         }
         else
         {
            theApp.ErrorBox(dwResult, _T("Unable to update event configuration record: %s"));
            safe_free(evt.pszDescription);
            safe_free(evt.pszMessage);
         }
      }
   }
   return bResult;
}


//
// WM_SETFOCUS message handler
//

void CEventEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_CLOSE message handler
//

void CEventEditor::OnClose() 
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCUnlockEventDB, g_hSession, _T("Unlocking event configuration database..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Unable to unlock event configuration database: %s"));
	CMDIChildWnd::OnClose();
}


//
// PreCreateWindow()
//

BOOL CEventEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_LOG));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CONTEXTMENU message handler
//

void CEventEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(10);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// OnUpdate...() handlers
//

void CEventEditor::OnUpdateEventEdit(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}

void CEventEditor::OnUpdateEventDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// WM_COMMAND::ID_EVENT_EDIT message handler
//

void CEventEditor::OnEventEdit() 
{
   int iItem;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_FOCUSED);
      if (iItem != -1)
         EditEvent(iItem);
   }
}


//
// WM_COMMAND::ID_EVENT_NEW message handler
//

void CEventEditor::OnEventNew() 
{
   DWORD dwNewCode, dwResult;
   NXC_EVENT_TEMPLATE *pData;
   TCHAR szBuffer[32];
   int iItem;

   dwResult = DoRequestArg2(NXCGenerateEventCode, g_hSession, &dwNewCode,
                            _T("Generating code for new event..."));
   if (dwResult == RCC_SUCCESS)
   {
      // Create new item in list view
      _stprintf(szBuffer, _T("%ld"), dwNewCode);
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, 0);
      m_wndListCtrl.SetItemData(iItem, dwNewCode);

      // Create empty record in template list
      pData = (NXC_EVENT_TEMPLATE *)malloc(sizeof(NXC_EVENT_TEMPLATE));
      memset(pData, 0, sizeof(NXC_EVENT_TEMPLATE));
      pData->dwCode = dwNewCode;
      NXCAddEventTemplate(g_hSession, pData);

      // Pointers inside client library can change after adding new template, so we reget it
      NXCGetEventDB(g_hSession, &m_ppEventTemplates, &m_dwNumTemplates);

      // Edit new event
      if (!EditEvent(iItem))
      {
         m_wndListCtrl.DeleteItem(iItem);
         NXCDeleteEDBRecord(g_hSession, dwNewCode);
         NXCGetEventDB(g_hSession, &m_ppEventTemplates, &m_dwNumTemplates);
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Unable to generate code for new event: %s"));
   }
}


//
// Worker function for deleting event templates
//

DWORD CEventEditor::DeleteEvents(DWORD dwNumEvents, DWORD *pdwEventList)
{
   DWORD i, dwResult = RCC_SUCCESS;
   int iItem;
   LVFINDINFO lvfi;

   for(i = 0; i < dwNumEvents; i++)
   {
      dwResult = NXCDeleteEventTemplate(g_hSession, pdwEventList[i]);
      if (dwResult != RCC_SUCCESS)
         break;
      NXCGetEventDB(g_hSession, &m_ppEventTemplates, &m_dwNumTemplates);
      lvfi.flags = LVFI_PARAM;
      lvfi.lParam = pdwEventList[i];
      iItem = m_wndListCtrl.FindItem(&lvfi);
      if (iItem != -1)
         m_wndListCtrl.DeleteItem(iItem);
   }
   return dwResult;
}


//
// Wrapper for DeleteEvents()
//

static DWORD StartDeleteEvents(CEventEditor *pWnd, DWORD dwNumEvents, DWORD *pdwEventList)
{
   return pWnd->DeleteEvents(dwNumEvents, pdwEventList);
}


//
// WM_COMMAND::ID_EVENT_DELETE message handler
//

void CEventEditor::OnEventDelete() 
{
   int iItem;
   DWORD i, dwNumEvents, *pdwEventList, dwResult;

   dwNumEvents = m_wndListCtrl.GetSelectedCount();
   pdwEventList = (DWORD *)malloc(sizeof(DWORD) * dwNumEvents);
   memset(pdwEventList, 0, sizeof(DWORD) * dwNumEvents);
   for(i = 0, iItem = -1; i < dwNumEvents; i++)
   {
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      if (iItem == -1)
         break;   // Shouldn't be
      pdwEventList[i] = m_wndListCtrl.GetItemData(iItem);
   }

   dwResult = DoRequestArg3(StartDeleteEvents, this, (void *)dwNumEvents,
                            pdwEventList, _T("Deleting event templates..."));
   if (dwResult != RCC_SUCCESS)
   {
      theApp.ErrorBox(dwResult, _T("Cannot delete event template: %s"));
   }
   safe_free(pdwEventList);
}
