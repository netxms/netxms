// AlarmBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AlarmBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Compare two list view items
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   NXC_ALARM *pAlarm1, *pAlarm2;
   NXC_OBJECT *pObject1, *pObject2;
   TCHAR szName1[MAX_OBJECT_NAME], szName2[MAX_OBJECT_NAME];
   int iResult;

   pAlarm1 = ((CAlarmBrowser *)lParamSort)->FindAlarmInList(lParam1);
   pAlarm2 = ((CAlarmBrowser *)lParamSort)->FindAlarmInList(lParam2);
   if ((pAlarm1 == NULL) || (pAlarm2 == NULL))
      return 0;

   switch(((CAlarmBrowser *)lParamSort)->SortMode())
   {
      case 0:  // Severity
         iResult = COMPARE_NUMBERS(pAlarm1->nCurrentSeverity, pAlarm2->nCurrentSeverity);
         break;
      case 1:  // State
         iResult = -COMPARE_NUMBERS(pAlarm1->nState, pAlarm2->nState);
         break;
      case 2:  // Source
         pObject1 = NXCFindObjectById(g_hSession, pAlarm1->dwSourceObject);
         pObject2 = NXCFindObjectById(g_hSession, pAlarm2->dwSourceObject);

         if (pObject1 == NULL)
            _tcscpy(szName1, _T("<unknown>"));
         else
            _tcscpy(szName1, pObject1->szName);
         
         if (pObject2 == NULL)
            _tcscpy(szName2, _T("<unknown>"));
         else
            _tcscpy(szName2, pObject2->szName);

         iResult = _tcsicmp(szName1, szName2);
         break;
      case 3:  // Message
         iResult = _tcsicmp(pAlarm1->szMessage, pAlarm2->szMessage);
         break;
      case 4:  // Repeat count
         iResult = COMPARE_NUMBERS(pAlarm1->dwRepeatCount, pAlarm2->dwRepeatCount);
         break;
      case 5:  // Creation time
         iResult = COMPARE_NUMBERS(pAlarm1->dwCreationTime, pAlarm2->dwCreationTime);
         break;
      case 6:  // Last change time
         iResult = COMPARE_NUMBERS(pAlarm1->dwLastChangeTime, pAlarm2->dwLastChangeTime);
         break;
      default:
         iResult = 0;
         break;
   }
   return (((CAlarmBrowser *)lParamSort)->SortDir() == 0) ? iResult : -iResult;
}


/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser

IMPLEMENT_DYNCREATE(CAlarmBrowser, CMDIChildWnd)

CAlarmBrowser::CAlarmBrowser()
{
   m_pImageList = NULL;
   m_pObjectImageList = NULL;
   m_bShowAllAlarms = FALSE;
   m_iSortDir = theApp.GetProfileInt(_T("AlarmBrowser"), _T("SortDir"), 1);
   m_iSortMode = theApp.GetProfileInt(_T("AlarmBrowser"), _T("SortMode"), 0);
   m_bShowNodes = theApp.GetProfileInt(_T("AlarmBrowser"), _T("ShowNodes"), FALSE);
   m_bRestoredDesktop = FALSE;
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   m_dwCurrNode = 0;
}

CAlarmBrowser::CAlarmBrowser(TCHAR *pszParams)
{
   m_pImageList = NULL;
   m_pObjectImageList = NULL;
   m_bShowAllAlarms = FALSE;
   m_bRestoredDesktop = TRUE;
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   m_dwCurrNode = 0;
   m_iSortMode = ExtractWindowParamLong(pszParams, _T("SM"), 0);
   m_iSortDir = ExtractWindowParamLong(pszParams, _T("SD"), 0);
   m_bShowNodes = ExtractWindowParamLong(pszParams, _T("SN"), 0);
}

CAlarmBrowser::~CAlarmBrowser()
{
   delete m_pImageList;
   delete m_pObjectImageList;
   safe_free(m_pAlarmList);
}


BEGIN_MESSAGE_MAP(CAlarmBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CAlarmBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_ALARM_ACKNOWLEDGE, OnAlarmAcknowledge)
	ON_UPDATE_COMMAND_UI(ID_ALARM_ACKNOWLEDGE, OnUpdateAlarmAcknowledge)
	ON_WM_CLOSE()
	ON_COMMAND(ID_ALARM_SHOWNODES, OnAlarmShownodes)
	ON_UPDATE_COMMAND_UI(ID_ALARM_SHOWNODES, OnUpdateAlarmShownodes)
	ON_COMMAND(ID_ALARM_SOUNDCONFIGURATION, OnAlarmSoundconfiguration)
	ON_COMMAND(ID_ALARM_TERMINATE, OnAlarmTerminate)
	ON_UPDATE_COMMAND_UI(ID_ALARM_TERMINATE, OnUpdateAlarmTerminate)
	ON_COMMAND(ID_ALARM_LASTDCIVALUES, OnAlarmLastdcivalues)
	ON_UPDATE_COMMAND_UI(ID_ALARM_LASTDCIVALUES, OnUpdateAlarmLastdcivalues)
	ON_COMMAND(ID_ALARM_DETAILS, OnAlarmDetails)
	ON_UPDATE_COMMAND_UI(ID_ALARM_DETAILS, OnUpdateAlarmDetails)
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_COLUMNCLICK, AFX_IDW_PANE_FIRST, OnListViewColumnClick)
	ON_NOTIFY(LVN_COLUMNCLICK, AFX_IDW_PANE_FIRST + 1, OnListViewColumnClick)
   ON_NOTIFY(TVN_SELCHANGED, AFX_IDW_PANE_FIRST, OnTreeViewSelChange)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_COMMAND_RANGE(OBJTOOL_AV_MENU_FIRST_ID, OBJTOOL_AV_MENU_LAST_ID, OnObjectTool)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser message handlers

//
// Overrided PreCreateWindow()
//

BOOL CAlarmBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, GetSysColorBrush(COLOR_WINDOW), 
                                         theApp.LoadIcon(IDI_ALARM));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CAlarmBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   BYTE *pwp;
   UINT iBytes;
   RECT rect;
   int i;
   LVCOLUMN lvCol;
   DWORD dwResult;
   static int widths[6] = { 50, 100, 150, 200, 250, -1 };
   static int icons[5] = { IDI_SEVERITY_NORMAL, IDI_SEVERITY_WARNING, IDI_SEVERITY_MINOR,
                           IDI_SEVERITY_MAJOR, IDI_SEVERITY_CRITICAL };

   if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);

   // Create status bar
	m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(6, widths);
   for(i = 0; i < 5; i++)
      m_wndStatusBar.SetIcon(i, (HICON)LoadImage(theApp.m_hInstance, 
                                                 MAKEINTRESOURCE(icons[i]),
                                                 IMAGE_ICON, 16, 16, LR_SHARED));
   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;

   // Create splitter
   m_wndSplitter.CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE, IDC_SPLITTER);
	
   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 1));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_LABELTIP | LVS_EX_SUBITEMIMAGES |
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Create image lists
   m_pImageList = CreateEventImageList();
   m_iSortImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_iStateImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_TIPS));
   m_pImageList->Add(theApp.LoadIcon(IDI_ACK));
   m_pImageList->Add(theApp.LoadIcon(IDI_NACK));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   m_pObjectImageList = new CImageList;
   m_pObjectImageList->Create(g_pObjectSmallImageList);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Severity"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(1, _T("State"), LVCFMT_LEFT, 95);
   m_wndListCtrl.InsertColumn(2, _T("Source"), LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(3, _T("Message"), LVCFMT_LEFT, 400);
   m_wndListCtrl.InsertColumn(4, _T("Count"), LVCFMT_LEFT, 45);
   m_wndListCtrl.InsertColumn(5, _T("Created"), LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(6, _T("Last Change"), LVCFMT_LEFT, 135);
   LoadListCtrlColumns(m_wndListCtrl, _T("AlarmBrowser"), _T("AlarmListV2"));
	
   // Mark sorting column in list control
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortImageBase + m_iSortDir;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Create tree view control
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 0));
   m_wndTreeCtrl.SetImageList(m_pObjectImageList, TVSIL_NORMAL);
   m_hTreeRoot = m_wndTreeCtrl.InsertItem(_T("All Nodes"), OBJECT_NETWORK, OBJECT_NETWORK);

   // Create panes in splitter
   m_wndSplitter.SetupView(0, 0, CSize(150, 100));
   m_wndSplitter.SetupView(0, 1, CSize(400, 100));
   if (!m_bShowNodes)
      m_wndSplitter.HideColumn(0);
   m_wndSplitter.InitComplete();

   // Restore window size and position if we have one
   if (!m_bRestoredDesktop)
   {
      if (theApp.GetProfileBinary(_T("AlarmBrowser"), _T("WindowPlacement"),
                                  &pwp, &iBytes))
      {
         if (iBytes == sizeof(WINDOWPLACEMENT))
         {
            RestoreMDIChildPlacement(this, (WINDOWPLACEMENT *)pwp);
         }
         delete pwp;
      }
   }

   theApp.OnViewCreate(VIEW_ALARMS, this);
   dwResult = DoRequestArg2(NXCSubscribe, g_hSession, (void *)NXC_CHANNEL_ALARMS,
                            _T("Subscribing to ALARMS channel..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot subscribe to ALARMS channel: %s"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CAlarmBrowser::OnDestroy() 
{
   SaveListCtrlColumns(m_wndListCtrl, _T("AlarmBrowser"), _T("AlarmListV2"));
   DoRequestArg2(NXCUnsubscribe, g_hSession, (void *)NXC_CHANNEL_ALARMS,
                 _T("Unsubscribing from ALARMS channel..."));
   theApp.OnViewDestroy(VIEW_ALARMS, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SETFOCUS message handler
//

void CAlarmBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CAlarmBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndSplitter.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAlarmBrowser::OnViewRefresh() 
{
   DWORD i, dwRetCode;

   m_wndListCtrl.DeleteAllItems();
   m_wndTreeCtrl.DeleteAllItems();
   m_hTreeRoot = m_wndTreeCtrl.InsertItem(_T("All Nodes"), OBJECT_NETWORK, OBJECT_NETWORK);
   safe_free(m_pAlarmList);
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   m_dwCurrNode = 0;
   dwRetCode = DoRequestArg3(NXCLoadAllAlarms, g_hSession, &m_dwNumAlarms, &m_pAlarmList, _T("Loading alarms..."));
   if (dwRetCode == RCC_SUCCESS)
   {
      memset(m_iNumAlarms, 0, sizeof(int) * 5);
      for(i = 0; i < m_dwNumAlarms; i++)
      {
         AddNodeToTree(m_pAlarmList[i].dwSourceObject);
         AddAlarm(&m_pAlarmList[i]);
         m_iNumAlarms[m_pAlarmList[i].nCurrentSeverity]++;
      }
      UpdateStatusBar();
      m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
   }
   else
   {
      theApp.ErrorBox(dwRetCode, _T("Error loading alarm list: %s"));
   }
}


//
// Add new alarm record to list
//

void CAlarmBrowser::AddAlarm(NXC_ALARM *pAlarm)
{
   int iIdx;

   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, g_szStatusTextSmall[pAlarm->nCurrentSeverity],
                                   pAlarm->nCurrentSeverity);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemData(iIdx, pAlarm->dwAlarmId);
      UpdateListItem(iIdx, pAlarm);
   }
}


//
// Update existing list entry with new data
//

void CAlarmBrowser::UpdateListItem(int nItem, NXC_ALARM *pAlarm)
{
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;
   LVITEM lvi;

   pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);

   // Set images
   lvi.mask = LVIF_IMAGE;
   lvi.iItem = nItem;

   lvi.iSubItem = 0;
   lvi.iImage = pAlarm->nCurrentSeverity;
   m_wndListCtrl.SetItem(&lvi);

   lvi.iSubItem = 1;
   lvi.iImage = m_iStateImageBase + pAlarm->nState;
   m_wndListCtrl.SetItem(&lvi);

   // Set texts
   m_wndListCtrl.SetItemText(nItem, 0, g_szStatusTextSmall[pAlarm->nCurrentSeverity]);
   m_wndListCtrl.SetItemText(nItem, 1, g_szAlarmState[pAlarm->nState]);
   m_wndListCtrl.SetItemText(nItem, 2, (pObject != NULL) ? pObject->szName : _T("<unknown>"));
   m_wndListCtrl.SetItemText(nItem, 3, pAlarm->szMessage);

   _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%d"), pAlarm->dwRepeatCount);
   m_wndListCtrl.SetItemText(nItem, 4, szBuffer);

   m_wndListCtrl.SetItemText(nItem, 5, FormatTimeStamp(pAlarm->dwCreationTime, szBuffer, TS_LONG_DATE_TIME));
   
   m_wndListCtrl.SetItemText(nItem, 6, FormatTimeStamp(pAlarm->dwLastChangeTime, szBuffer, TS_LONG_DATE_TIME));
}


//
// Process alarm updates
//

void CAlarmBrowser::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   int iItem;
   NXC_ALARM *pListItem;

   iItem = FindAlarmRecord(pAlarm->dwAlarmId);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if ((iItem == -1) && ((m_bShowAllAlarms) ||
             (pAlarm->nState != ALARM_STATE_TERMINATED)))
         {
            AddNodeToTree(pAlarm->dwSourceObject);
            if ((m_dwCurrNode == 0) || (m_dwCurrNode == pAlarm->dwSourceObject))
               AddAlarm(pAlarm);
            AddAlarmToList(pAlarm);
            UpdateStatusBar();
            m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
            PlayAlarmSound(pAlarm, TRUE, g_hSession, &g_soundCfg);
         }
         break;
      case NX_NOTIFY_ALARM_CHANGED:
         pListItem = FindAlarmInList(pAlarm->dwAlarmId);
         if (pListItem != NULL)
         {
            m_iNumAlarms[pListItem->nCurrentSeverity]--;
            m_iNumAlarms[pAlarm->nCurrentSeverity]++;
            memcpy(pListItem, pAlarm, sizeof(NXC_ALARM));
            if (iItem != -1)
            {
               UpdateListItem(iItem, pAlarm);
            }
            else
            {
               if ((m_dwCurrNode == 0) || (m_dwCurrNode == pAlarm->dwSourceObject))
                  AddAlarm(pAlarm);
            }
            m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
            UpdateStatusBar();
         }
         break;
      case NX_NOTIFY_ALARM_TERMINATED:
         if ((iItem != -1) && (!m_bShowAllAlarms))
         {
            DeleteAlarmFromList(pAlarm->dwAlarmId);
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->nCurrentSeverity]--;
            UpdateStatusBar();
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
         if (iItem != -1)
         {
            DeleteAlarmFromList(pAlarm->dwAlarmId);
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->nCurrentSeverity]--;
            UpdateStatusBar();
         }
         break;
      default:
         break;
   }
}


//
// Find alarm record in list control by alarm id
// Will return record index or -1 if no records exist
//

int CAlarmBrowser::FindAlarmRecord(DWORD dwAlarmId)
{
   LVFINDINFO lvfi;

   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = dwAlarmId;
   return m_wndListCtrl.FindItem(&lvfi);
}


//
// WM_CONTEXTMENU message handler
//

void CAlarmBrowser::OnContextMenu(CWnd *pWnd, CPoint point) 
{
   int iItem;
   UINT uFlags = 0;
   CMenu *pMenu, *pToolsMenu;
   CPoint pt;
   CWnd *pChildWnd;

   if (pWnd->GetDlgCtrlID() == IDC_SPLITTER)
   {
      pt = point;
      pWnd->ScreenToClient(&pt);
      pChildWnd = pWnd->ChildWindowFromPoint(pt, CWP_SKIPINVISIBLE);
   }
   else
   {
      pChildWnd = pWnd;
   }

   if (pChildWnd != NULL)
   {
      if (pChildWnd->GetDlgCtrlID() == m_wndSplitter.IdFromRowCol(0, m_bShowNodes ? 1 : 0))
      {
         pt = point;
         pWnd->ScreenToClient(&pt);
         iItem = m_wndListCtrl.HitTest(pt, &uFlags);
         if ((iItem != -1) && (uFlags & LVHT_ONITEM))
         {
            BOOL bMenuInserted;
            DWORD dwTemp;
            NXC_ALARM *pAlarm;
            NXC_OBJECT *pObject;

            pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(iItem));
            pMenu = theApp.GetContextMenu(6);
            if (pAlarm != NULL)
            {
               pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
               if (pObject != NULL)
               {
                  dwTemp = 0;
                  pToolsMenu = CreateToolsSubmenu(pObject, _T(""), &dwTemp, OBJTOOL_AV_MENU_FIRST_ID);
                  if (pToolsMenu->GetMenuItemCount() > 0)
                  {
                     pMenu->InsertMenu(5, MF_BYPOSITION | MF_STRING | MF_POPUP,
                                       (UINT)pToolsMenu->GetSafeHmenu(), _T("&Object tools"));
                     pToolsMenu->Detach();
                     bMenuInserted = TRUE;
                  }
                  delete pToolsMenu;
               }
            }
            pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
            if (bMenuInserted)
            {
               pMenu->DeleteMenu(5, MF_BYPOSITION);
            }
         }
      }
   }
}


//
// Alarm acknowledgement worker function
//

static DWORD AcknowledgeAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
   {
      dwResult = NXCAcknowledgeAlarm(g_hSession, pdwAlarmList[i]);
      if (dwResult == RCC_ALARM_NOT_OUTSTANDING)
         dwResult = RCC_SUCCESS;
   }
   return dwResult;
}


//
// WM_COMMAND::ID_ALARM_ACKNOWLEDGE message handler
//

void CAlarmBrowser::OnAlarmAcknowledge() 
{
   int iItem;
   DWORD i, dwNumAlarms, *pdwAlarmList, dwResult;

   dwNumAlarms = m_wndListCtrl.GetSelectedCount();
   pdwAlarmList = (DWORD *)malloc(sizeof(DWORD) * dwNumAlarms);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumAlarms); i++)
   {
      pdwAlarmList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg2(AcknowledgeAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Acknowledging alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot acknowledge alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmBrowser::OnUpdateAlarmAcknowledge(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Alarm termination worker function
//

static DWORD TerminateAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
      dwResult = NXCTerminateAlarm(g_hSession, pdwAlarmList[i]);
   return dwResult;
}


//
// WM_COMMAND::ID_ALARM_TERMINATE message handler
//

void CAlarmBrowser::OnAlarmTerminate() 
{
   int iItem;
   DWORD i, dwNumAlarms, *pdwAlarmList, dwResult;

   dwNumAlarms = m_wndListCtrl.GetSelectedCount();
   pdwAlarmList = (DWORD *)malloc(sizeof(DWORD) * dwNumAlarms);

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   for(i = 0; (iItem != -1) && (i < dwNumAlarms); i++)
   {
      pdwAlarmList[i] = m_wndListCtrl.GetItemData(iItem);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }

   dwResult = DoRequestArg2(TerminateAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Terminating alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot terminate alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmBrowser::OnUpdateAlarmTerminate(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}


//
// Update status bar information
//

void CAlarmBrowser::UpdateStatusBar(void)
{
   int i, iSum;
   TCHAR szBuffer[64];

   for(i = 0, iSum = 0; i < 5; i++)
   {
      _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%d"), m_iNumAlarms[i]);
      m_wndStatusBar.SetText(szBuffer, i, 0);
      iSum += m_iNumAlarms[i];
   }
   _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("Total: %d"), iSum);
   m_wndStatusBar.SetText(szBuffer, 5, 0);
}


//
// WM_CLOSE message handler
//

void CAlarmBrowser::OnClose() 
{
   WINDOWPLACEMENT wp;

   GetWindowPlacement(&wp);
   theApp.WriteProfileBinary(_T("AlarmBrowser"), _T("WindowPlacement"), 
                             (BYTE *)&wp, sizeof(WINDOWPLACEMENT));
   theApp.WriteProfileInt(_T("AlarmBrowser"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("AlarmBrowser"), _T("SortDir"), m_iSortDir);
   theApp.WriteProfileInt(_T("AlarmBrowser"), _T("ShowNodes"), m_bShowNodes);
	CMDIChildWnd::OnClose();
}


//
// Get save info for desktop saving
//

LRESULT CAlarmBrowser::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = (WINDOW_SAVE_INFO *)lParam;
   pInfo->iWndClass = WNDC_ALARM_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_WND_PARAM_LEN, _T("SM:%d\x7FSD:%d\x7FSN:%d"),
              m_iSortMode, m_iSortDir, m_bShowNodes);
   return 1;
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CAlarmBrowser::OnListViewColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
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
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Delete alarm from internal alarms list
//

void CAlarmBrowser::DeleteAlarmFromList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         PlayAlarmSound(&m_pAlarmList[i], FALSE, g_hSession, &g_soundCfg);
         m_dwNumAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1],
                 sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
         break;
      }
}


//
// Add new alarm to internal list
//

void CAlarmBrowser::AddAlarmToList(NXC_ALARM *pAlarm)
{
   m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * (m_dwNumAlarms + 1));
   memcpy(&m_pAlarmList[m_dwNumAlarms], pAlarm, sizeof(NXC_ALARM));
   m_dwNumAlarms++;
   m_iNumAlarms[pAlarm->nCurrentSeverity]++;
}


//
// Find alarm record in internal list
//

NXC_ALARM *CAlarmBrowser::FindAlarmInList(DWORD dwAlarmId)
{
   DWORD i;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
         return &m_pAlarmList[i];
   return NULL;
}


//
// Show/hide node tree
//

void CAlarmBrowser::OnAlarmShownodes() 
{
   if (m_bShowNodes)
   {
      m_wndSplitter.HideColumn(0);
      m_bShowNodes = FALSE;
      m_wndTreeCtrl.SelectItem(m_hTreeRoot);
      m_dwCurrNode = 0;
      RefreshAlarmList();
   }
   else
   {
      m_wndSplitter.ShowColumn(0, &m_wndTreeCtrl);
      m_bShowNodes = TRUE;
   }
}

void CAlarmBrowser::OnUpdateAlarmShownodes(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_bShowNodes);
}


//
// Add node to the tree if it doesn't exist
//

void CAlarmBrowser::AddNodeToTree(DWORD dwNodeId)
{
   NXC_OBJECT *pObject;
   HTREEITEM hItem;

   pObject = NXCFindObjectById(g_hSession, dwNodeId);
   if (pObject != NULL)
   {
      if (!IsNodeExist(dwNodeId))
      {
         hItem = m_wndTreeCtrl.InsertItem(pObject->szName, pObject->iClass, pObject->iClass, m_hTreeRoot);
         m_wndTreeCtrl.SetItemData(hItem, dwNodeId);
      }
   }
}


//
// Check if given node exists in the tree
//

BOOL CAlarmBrowser::IsNodeExist(DWORD dwNodeId)
{
   HTREEITEM hItem;
   BOOL bResult = FALSE;

   hItem = m_wndTreeCtrl.GetChildItem(m_hTreeRoot);
   while(hItem != NULL)
   {
      if (m_wndTreeCtrl.GetItemData(hItem) == dwNodeId)
      {
         bResult = TRUE;
         break;
      }
      hItem = m_wndTreeCtrl.GetNextItem(hItem, TVGN_NEXT);
   }
   return bResult;
}


//
// WM_NOTIFY::TVN_SELCHANGED message handler
//

void CAlarmBrowser::OnTreeViewSelChange(NMHDR *lpnmt, LRESULT *pResult)
{
   if (m_bShowNodes)
   {
      m_dwCurrNode = ((LPNMTREEVIEW)lpnmt)->itemNew.lParam;
      RefreshAlarmList();
   }
   *pResult = 0;
}


//
// Refresh alarm list after node change
//

void CAlarmBrowser::RefreshAlarmList()
{
   DWORD i;

   m_wndListCtrl.DeleteAllItems();
   for(i = 0; i < m_dwNumAlarms; i++)
   {
      if ((m_dwCurrNode == 0) || (m_dwCurrNode == m_pAlarmList[i].dwSourceObject))
         AddAlarm(&m_pAlarmList[i]);
   }
   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
}


//
// Configure sounds
//

void CAlarmBrowser::OnAlarmSoundconfiguration() 
{
   if (ConfigureAlarmSounds(&g_soundCfg))
   {
      SaveAlarmSoundCfg(&g_soundCfg, NXCON_ALARM_SOUND_KEY);
   }
}


//
// WM_COMMAND::ID_ALARM_LASTDCIVALUES message handler
//

void CAlarmBrowser::OnAlarmLastdcivalues() 
{
   int nItem;
   NXC_ALARM *pAlarm;
   NXC_OBJECT *pObject;

   nItem = m_wndListCtrl.GetSelectionMark();
   if (nItem != -1)
   {
      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(nItem));
      if (pAlarm != NULL)
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            theApp.ShowLastValues(pObject);
         }
      }
   }
}

void CAlarmBrowser::OnUpdateAlarmLastdcivalues(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
}


//
// Handler for object tools
//

void CAlarmBrowser::OnObjectTool(UINT nID)
{
   int nItem;
   NXC_ALARM *pAlarm;
   NXC_OBJECT *pObject;

   nItem = m_wndListCtrl.GetNextItem(-1, LVIS_FOCUSED);
   if (nItem != -1)
   {
      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(nItem));
      if (pAlarm != NULL)
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            theApp.ExecuteObjectTool(pObject, nID - OBJTOOL_AV_MENU_FIRST_ID);
         }
      }
   }
}


//
// WM_COMMAND::ID_ALARM_DETAILS message handler
//

void CAlarmBrowser::OnAlarmDetails() 
{
   int nItem;
   NXC_ALARM *pAlarm;
   NXC_OBJECT *pObject;
   Table *pTable;

   nItem = m_wndListCtrl.GetNextItem(-1, LVIS_FOCUSED);
   if (nItem != -1)
   {
      pAlarm = FindAlarmInList(m_wndListCtrl.GetItemData(nItem));
      if (pAlarm != NULL)
      {
         pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
         if (pObject != NULL)
         {
            pTable = new Table;
            pTable->addColumn(_T("Attribute"));
            pTable->addColumn(_T("Image"));
            pTable->addColumn(_T("Value"));

            // Current severity
            pTable->addRow();
            pTable->set(0, _T("Current Severity"));
            pTable->set(1, (LONG)pAlarm->nCurrentSeverity);
            pTable->set(2, g_szStatusTextSmall[pAlarm->nCurrentSeverity]);

            // Original severity
            pTable->addRow();
            pTable->set(0, _T("Original Severity"));
            pTable->set(1, (LONG)pAlarm->nOriginalSeverity);
            pTable->set(2, g_szStatusTextSmall[pAlarm->nOriginalSeverity]);

            // State
            pTable->addRow();
            pTable->set(0, _T("State"));
            pTable->set(1, (LONG)(pAlarm->nState + 5));
            pTable->set(2, g_szAlarmState[pAlarm->nState]);

            // Source
            pTable->addRow();
            pTable->set(0, _T("Source"));
            pTable->set(1, (LONG)(-1));
            pTable->set(2, pObject->szName);

            theApp.ShowDetailsWindow(VIEW_ALARM_DETAILS, m_hWnd, pTable);
         }
      }
   }
}

void CAlarmBrowser::OnUpdateAlarmDetails(CCmdUI* pCmdUI) 
{
//   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() == 1);
   pCmdUI->Enable(FALSE);
}
