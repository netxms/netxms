// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxnotify.h"
#include "MainFrm.h"
#include "AlarmPopup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_TASKBAR_OPEN, OnTaskbarOpen)
	ON_WM_CLOSE()
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXNM_TASKBAR_CALLBACK, OnTaskbarCallback)
   ON_MESSAGE(NXNM_ALARM_UPDATE, OnAlarmUpdate)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
   m_iSortDir = appNotify.GetProfileInt(_T("AlarmList"), _T("SortDir"), 1);
   m_iSortMode = appNotify.GetProfileInt(_T("AlarmList"), _T("SortMode"), 0);
   m_bExit = FALSE;
}

CMainFrame::~CMainFrame()
{
   appNotify.WriteProfileInt(_T("AlarmList"), _T("SortMode"), m_iSortMode);
   appNotify.WriteProfileInt(_T("AlarmList"), _T("SortDir"), m_iSortDir);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics
//

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


//
// WM_CREATE message handler
//

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create image list
   m_imageList.Create(&g_imgListSeverity);
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(appNotify.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(appNotify.LoadIcon(IDI_SORT_DOWN));

   // Create list control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, ID_LIST_CTRL);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

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

	return 0;
}


//
// WM_SETFOCUS message handler
//

void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
   CFrameWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// NXNM_TASKBAR_CALLBACK message handler
//

void CMainFrame::OnTaskbarCallback(WPARAM wParam, LPARAM lParam)
{
   NOTIFYICONDATA nid;
   POINT pt;
   CMenu *pMenu;

   switch(lParam)
   {
      case WM_LBUTTONDBLCLK:
         nid.cbSize = sizeof(NOTIFYICONDATA);
         nid.hWnd = m_hWnd;
         nid.uID = 0;
         Shell_NotifyIcon(NIM_SETFOCUS, &nid);
         if (IsIconic())
         {
            ShowWindow(SW_RESTORE);
         }
         else
         {
            ShowWindow(SW_SHOW);
         }
         SetForegroundWindow();
         break;
      case WM_LBUTTONDOWN:
         break;
      case WM_CONTEXTMENU:
         GetCursorPos(&pt);
         pMenu = appNotify.GetContextMenu(0);
         pMenu->TrackPopupMenu(TPM_RIGHTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, this);
         break;
      default:
         break;
   }
}


//
// WM_DESTROY message handler
//

void CMainFrame::OnDestroy() 
{
   NOTIFYICONDATA nid;

   nid.cbSize = sizeof(NOTIFYICONDATA);
   nid.hWnd = m_hWnd;
   nid.uID = 0;
   Shell_NotifyIcon(NIM_DELETE, &nid);
	CFrameWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
   if (nType == SIZE_MINIMIZED)
      ShowWindow(SW_HIDE);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXNM_ALARM_UPDATE mesasge handler
//

void CMainFrame::OnAlarmUpdate(WPARAM wParam, LPARAM lParam)
{
   NXC_ALARM *pAlarm = (NXC_ALARM *)lParam;
   CAlarmPopup *pAlarmPopup = NULL;
   int iItem;

   iItem = FindAlarmRecord(pAlarm->dwAlarmId);
   switch(wParam)
   {
      case NX_NOTIFY_NEW_ALARM:
         if ((iItem == -1) && (pAlarm->wIsAck == 0))
         {
            AddAlarm(pAlarm);
            //UpdateStatusBar();
            //m_wndListCtrl.SortItems(CompareListItems, (LPARAM)this);
            
            pAlarmPopup = new CAlarmPopup;
            pAlarmPopup->m_pAlarm = pAlarm;
            pAlarmPopup->SetAttr(g_dwPopupTimeout);
            pAlarmPopup->Create(220, 120, this);

            PlayAlarmSound(pAlarm, TRUE, g_hSession, &appNotify.m_soundCfg);
         }
         break;
      case NX_NOTIFY_ALARM_ACKNOWLEGED:
         if (iItem != -1)
         {
            m_wndListCtrl.DeleteItem(iItem);
            //m_iNumAlarms[pAlarm->wSeverity]--;
            //UpdateStatusBar();
            PlayAlarmSound(pAlarm, FALSE, g_hSession, &appNotify.m_soundCfg);
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
         if (iItem != -1)
         {
            m_wndListCtrl.DeleteItem(iItem);
            //m_iNumAlarms[pAlarm->wSeverity]--;
            //UpdateStatusBar();
            PlayAlarmSound(pAlarm, FALSE, g_hSession, &appNotify.m_soundCfg);
         }
         break;
      default:
         break;
   }

   // Destroy alarm record if it was not passed to popup window
   if (pAlarmPopup == NULL)
      free(pAlarm);
}


//
// Find alarm record in list control by alarm id
// Will return record index or -1 if no records exist
//

int CMainFrame::FindAlarmRecord(DWORD dwAlarmId)
{
   LVFINDINFO lvfi;

   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = dwAlarmId;
   return m_wndListCtrl.FindItem(&lvfi);
}


//
// Add new alarm record to list
//

void CMainFrame::AddAlarm(NXC_ALARM *pAlarm)
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
}


//
// WM_COMMAND::ID_TASKBAR_OPEN message handler
//

void CMainFrame::OnTaskbarOpen() 
{
   if (IsIconic())
   {
      ShowWindow(SW_RESTORE);
   }
   else
   {
      ShowWindow(SW_SHOW);
      SetForegroundWindow();
   }
}


//
// WM_CLOSE message handler
//

void CMainFrame::OnClose() 
{
   if (!m_bExit)
      CloseWindow();
   else
      CFrameWnd::OnClose();
}


//
// WM_COMMAND::ID_FILE_EXIT message handler
//

void CMainFrame::OnFileExit() 
{
   m_bExit = TRUE;
   DestroyWindow();
}
