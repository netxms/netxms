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
         iResult = (pAlarm1->wSeverity < pAlarm2->wSeverity) ? -1 :
                     ((pAlarm1->wSeverity > pAlarm2->wSeverity) ? 1 : 0);
         break;
      case 1:  // Source
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
      case 2:  // Message
         iResult = _tcsicmp(pAlarm1->szMessage, pAlarm2->szMessage);
         break;
      case 3:  // Timestamp
         iResult = (pAlarm1->dwTimeStamp < pAlarm2->dwTimeStamp) ? -1 :
                     ((pAlarm1->dwTimeStamp > pAlarm2->dwTimeStamp) ? 1 : 0);
         break;
      case 4:  // Ack
         iResult = (pAlarm1->wIsAck < pAlarm2->wIsAck) ? -1 :
                     ((pAlarm1->wIsAck > pAlarm2->wIsAck) ? 1 : 0);
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
   m_bShowAllAlarms = FALSE;
   m_iSortDir = 1;
   m_iSortMode = 0;
   m_bRestoredDesktop = FALSE;
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
}

CAlarmBrowser::CAlarmBrowser(TCHAR *pszParams)
{
   TCHAR szBuffer[32];

   m_pImageList = NULL;
   m_bShowAllAlarms = FALSE;
   m_bRestoredDesktop = TRUE;
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;

   if (ExtractWindowParam(pszParams, _T("SM"), szBuffer, 32))
   {
      m_iSortMode = _tcstol(szBuffer, NULL, 0);
      if ((m_iSortMode < 0) || (m_iSortMode > 4))
         m_iSortMode = 0;
   }
   else
   {
      m_iSortMode = 0;
   }

   if (ExtractWindowParam(pszParams, _T("SD"), szBuffer, 32))
   {
      m_iSortDir = _tcstol(szBuffer, NULL, 0);
      if ((m_iSortDir != 0) && (m_iSortDir != 1))
         m_iSortDir = 0;
   }
   else
   {
      m_iSortDir = 1;
   }
}

CAlarmBrowser::~CAlarmBrowser()
{
   delete m_pImageList;
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
	ON_COMMAND(ID_ALARM_ACKNOWLEGE, OnAlarmAcknowlege)
	ON_UPDATE_COMMAND_UI(ID_ALARM_ACKNOWLEGE, OnUpdateAlarmAcknowlege)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_COLUMNCLICK, ID_LIST_VIEW, OnListViewColumnClick)
   ON_MESSAGE(WM_GET_SAVE_INFO, OnGetSaveInfo)
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
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_ALARM));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CAlarmBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static int widths[6] = { 50, 100, 150, 200, 250, -1 };
   static int icons[5] = { IDI_SEVERITY_NORMAL, IDI_SEVERITY_WARNING, IDI_SEVERITY_MINOR,
                           IDI_SEVERITY_MAJOR, IDI_SEVERITY_CRITICAL };
   int i;
   BYTE *pwp;
   UINT iBytes;
   LVCOLUMN lvCol;

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
   rect.bottom -= m_iStatusBarHeight;

   // Create font for elements
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");

   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   //m_wndListCtrl.SetFont(&m_fontNormal, FALSE);

   // Create image list
   m_pImageList = CreateEventImageList();
   m_iSortImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Severity"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(1, _T("Source"), LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(2, _T("Message"), LVCFMT_LEFT, 400);
   m_wndListCtrl.InsertColumn(3, _T("Time Stamp"), LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(4, _T("Ack"), LVCFMT_CENTER, 30);
	
   // Mark sorting column in list control
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iSortImageBase + m_iSortDir;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

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

   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_ALARMS, this);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CAlarmBrowser::OnDestroy() 
{
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_ALARMS, this);
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
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAlarmBrowser::OnViewRefresh() 
{
   DWORD i, dwRetCode;

   m_wndListCtrl.DeleteAllItems();
   safe_free(m_pAlarmList);
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   dwRetCode = DoRequestArg4(NXCLoadAllAlarms, g_hSession, (void *)m_bShowAllAlarms, 
                             &m_dwNumAlarms, &m_pAlarmList, _T("Loading alarms..."));
   if (dwRetCode == RCC_SUCCESS)
   {
      memset(m_iNumAlarms, 0, sizeof(int) * 5);
      for(i = 0; i < m_dwNumAlarms; i++)
         AddAlarm(&m_pAlarmList[i]);
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
   struct tm *ptm;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF, g_szStatusTextSmall[pAlarm->wSeverity],
                                   pAlarm->wSeverity);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemData(iIdx, pAlarm->dwAlarmId);
      m_wndListCtrl.SetItemText(iIdx, 1, pObject->szName);
      m_wndListCtrl.SetItemText(iIdx, 2, pAlarm->szMessage);
      ptm = localtime((const time_t *)&pAlarm->dwTimeStamp);
      strftime(szBuffer, 32, "%d-%b-%Y %H:%M:%S", ptm);
      m_wndListCtrl.SetItemText(iIdx, 3, szBuffer);
      m_wndListCtrl.SetItemText(iIdx, 4, pAlarm->wIsAck ? _T("X") : _T(""));
   }
   m_iNumAlarms[pAlarm->wSeverity]++;
}


//
// Process alarm updates
//

void CAlarmBrowser::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   int iItem;

   iItem = FindAlarmRecord(pAlarm->dwAlarmId);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if ((iItem == -1) && ((m_bShowAllAlarms) || (pAlarm->wIsAck == 0)))
         {
            AddAlarm(pAlarm);
            AddAlarmToList(pAlarm);
            UpdateStatusBar();
            m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
         }
         break;
      case NX_NOTIFY_ALARM_ACKNOWLEGED:
         if ((iItem != -1) && (!m_bShowAllAlarms))
         {
            DeleteAlarmFromList(pAlarm->dwAlarmId);
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->wSeverity]--;
            UpdateStatusBar();
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
         if (iItem != -1)
         {
            DeleteAlarmFromList(pAlarm->dwAlarmId);
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->wSeverity]--;
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

void CAlarmBrowser::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   int iItem;
   UINT uFlags;
   CMenu *pMenu;
   CPoint pt;

   pt = point;
   pWnd->ScreenToClient(&pt);
   iItem = m_wndListCtrl.HitTest(pt, &uFlags);
   if ((iItem != -1) && (uFlags & LVHT_ONITEM))
   {
      pMenu = theApp.GetContextMenu(6);
      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
   }
}


//
// Alarm acknowlegement worker function
//

static DWORD AcknowlegeAlarms(DWORD dwNumAlarms, DWORD *pdwAlarmList)
{
   DWORD i, dwResult = RCC_SUCCESS;

   for(i = 0; (i < dwNumAlarms) && (dwResult == RCC_SUCCESS); i++)
      dwResult = NXCAcknowlegeAlarm(g_hSession, pdwAlarmList[i]);
   return dwResult;
}


//
// WM_COMMAND::ID_ALARM_ACKNOWLEGE message handler
//

void CAlarmBrowser::OnAlarmAcknowlege() 
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

   dwResult = DoRequestArg2(AcknowlegeAlarms, (void *)dwNumAlarms, pdwAlarmList,
                            _T("Acknowleging alarm..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot acknowlege alarm: %s"));
   free(pdwAlarmList);
}

void CAlarmBrowser::OnUpdateAlarmAcknowlege(CCmdUI* pCmdUI) 
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
      _stprintf(szBuffer, _T("%d"), m_iNumAlarms[i]);
      m_wndStatusBar.SetText(szBuffer, i, 0);
      iSum += m_iNumAlarms[i];
   }
   _stprintf(szBuffer, _T("Total: %d"), iSum);
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
	CMDIChildWnd::OnClose();
}


//
// Get save info for desktop saving
//

LRESULT CAlarmBrowser::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_ALARM_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_DB_STRING, _T("SM:%d\x7FSD:%d"),
              m_iSortMode, m_iSortDir);
   return 1;
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CAlarmBrowser::OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == pNMHDR->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = 1 - m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = pNMHDR->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir == 0)  ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(pNMHDR->iSubItem, &lvCol);
   
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
