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

/////////////////////////////////////////////////////////////////////////////
// CAlarmBrowser

IMPLEMENT_DYNCREATE(CAlarmBrowser, CMDIChildWnd)

CAlarmBrowser::CAlarmBrowser()
{
   m_pImageList = NULL;
   m_bShowAllAlarms = FALSE;
}

CAlarmBrowser::~CAlarmBrowser()
{
   delete m_pImageList;
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
	//}}AFX_MSG_MAP
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

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font for elements
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, "MS Sans Serif");

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   //m_wndListCtrl.SetFont(&m_fontNormal, FALSE);

   // Create image list
   m_pImageList = CreateEventImageList();
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "Severity", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(1, "Source", LVCFMT_LEFT, 140);
   m_wndListCtrl.InsertColumn(2, "Message", LVCFMT_LEFT, 400);
   m_wndListCtrl.InsertColumn(3, "Time Stamp", LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(4, "Ack", LVCFMT_CENTER, 30);
	
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
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAlarmBrowser::OnViewRefresh() 
{
   DWORD i, dwRetCode, dwNumAlarms;
   NXC_ALARM *pAlarmList;

   m_wndListCtrl.DeleteAllItems();
   dwRetCode = DoRequestArg3(NXCLoadAllAlarms, (void *)m_bShowAllAlarms, &dwNumAlarms, 
                             &pAlarmList, "Loading alarms...");
   if (dwRetCode == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumAlarms; i++)
         AddAlarm(&pAlarmList[i]);
      safe_free(pAlarmList);
   }
   else
   {
      theApp.ErrorBox(dwRetCode, "Error loading alarm list: %s");
   }
}


//
// Add new alarm record to list
//

void CAlarmBrowser::AddAlarm(NXC_ALARM *pAlarm)
{
   int iIdx;
   struct tm *ptm;
   char szBuffer[64];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(pAlarm->dwSourceObject);
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
      m_wndListCtrl.SetItemText(iIdx, 4, pAlarm->wIsAck ? "X" : "");
   }
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
            AddAlarm(pAlarm);
         break;
      case NX_NOTIFY_ALARM_ACKNOWLEGED:
         if ((iItem != -1) && (!m_bShowAllAlarms))
            m_wndListCtrl.DeleteItem(iItem);
         break;
      case NX_NOTIFY_ALARM_DELETED:
         if (iItem != -1)
            m_wndListCtrl.DeleteItem(iItem);
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
// WM_COMMAND::ID_ALARM_ACKNOWLEGE message handler
//

void CAlarmBrowser::OnAlarmAcknowlege() 
{
   int iItem;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      DoRequestArg1(NXCAcknowlegeAlarm, (void *)m_wndListCtrl.GetItemData(iItem), "Acknowleging alarm...");
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }
}

void CAlarmBrowser::OnUpdateAlarmAcknowlege(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndListCtrl.GetSelectedCount() > 0);
}
