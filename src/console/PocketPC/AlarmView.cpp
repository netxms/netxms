// AlarmView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "AlarmView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlarmView

CAlarmView::CAlarmView()
{
   m_pImageList = NULL;
}

CAlarmView::~CAlarmView()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CAlarmView, CWnd)
	//{{AFX_MSG_MAP(CAlarmView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAlarmView message handlers


//
// WM_CREATE message handler
//

int CAlarmView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create font for alarm list
   m_fontSmall.CreateFont(-MulDiv(7, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, L"System");
   m_fontLarge.CreateFont(-MulDiv(14, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, L"System");

   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | 
                        LVS_NOCOLUMNHEADER | LVS_OWNERDRAWFIXED, rect, this, ID_LIST_CTRL);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListCtrl.SetFont(&m_fontLarge, FALSE);
	
   // Create image list
   m_pImageList = CreateEventImageList();
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Severity"), LVCFMT_LEFT, 1);
   m_wndListCtrl.InsertColumn(1, _T("Source"), LVCFMT_LEFT, 1);
   m_wndListCtrl.InsertColumn(2, _T("Message"), LVCFMT_LEFT, 1);
   m_wndListCtrl.InsertColumn(3, _T("Time Stamp"), LVCFMT_LEFT, 1);
   m_wndListCtrl.InsertColumn(4, _T("Ack"), LVCFMT_CENTER, 700);

	return 0;
}


//
// WM_SIZE message handler
//

void CAlarmView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_DRAWITEM message handler
//

void CAlarmView::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
   if (nIDCtl == ID_LIST_CTRL)
   {
      CDC *pdc;

      pdc = CDC::FromHandle(lpDrawItemStruct->hDC);
      DrawListItem(*pdc, lpDrawItemStruct->rcItem, lpDrawItemStruct->itemID,
                   lpDrawItemStruct->itemData);
//      pdc->Detach();
//      delete pdc;
   }
   else
   {
	   CWnd::OnDrawItem(nIDCtl, lpDrawItemStruct);
   }
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CAlarmView::OnViewRefresh() 
{
   DWORD i, dwRetCode, dwNumAlarms;
   NXC_ALARM *pAlarmList;

   m_wndListCtrl.DeleteAllItems();
   dwRetCode = DoRequestArg4(NXCLoadAllAlarms, g_hSession, (void *)FALSE, 
                             &dwNumAlarms, &pAlarmList, _T("Loading alarms..."));
   if (dwRetCode == RCC_SUCCESS)
   {
      //memset(m_iNumAlarms, 0, sizeof(int) * 5);
      for(i = 0; i < dwNumAlarms; i++)
         AddAlarm(&pAlarmList[i]);
      safe_free(pAlarmList);
      //UpdateStatusBar();
   }
   else
   {
      theApp.ErrorBox(dwRetCode, _T("Error loading alarm list: %s"));
   }
}


//
// Add new alarm to list
//

void CAlarmView::AddAlarm(NXC_ALARM *pAlarm)
{
   int iIdx;
//   struct tm *ptm;
//   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
   iIdx = m_wndListCtrl.InsertItem(0x7FFFFFFF,
                                   g_szStatusTextSmall[pAlarm->wSeverity],
                                   pAlarm->wSeverity);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemData(iIdx, pAlarm->dwAlarmId);
      m_wndListCtrl.SetItemText(iIdx, 1, pObject->szName);
      m_wndListCtrl.SetItemText(iIdx, 2, pAlarm->szMessage);
//      ptm = localtime((const time_t *)&pAlarm->dwTimeStamp);
//      strftime(szBuffer, 32, "%d-%b-%Y %H:%M:%S", ptm);
//      m_wndListCtrl.SetItemText(iIdx, 3, szBuffer);
      m_wndListCtrl.SetItemText(iIdx, 4, pAlarm->wIsAck ? _T("X") : _T(""));
   }
//   m_iNumAlarms[pAlarm->wSeverity]++;
}


//
// Draw alarm list item
//

void CAlarmView::DrawListItem(CDC &dc, RECT &rcItem, int iItem, UINT nData)
{
   CFont *pOldFont;
   int iHeight;
   TCHAR szBuffer[1024];
   COLORREF rgbOldColor;
   LVITEM item;
   RECT rcText;

   // Get item data
   item.mask = LVIF_IMAGE | LVIF_STATE;
   item.iItem = iItem;
   item.iSubItem = 0;
   item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
   m_wndListCtrl.GetItem(&item);

   // Draw background
   if (item.state & LVIS_SELECTED)
   {
      dc.FillSolidRect(&rcItem, GetSysColor(COLOR_HIGHLIGHT));
      rgbOldColor = dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
   }
   else
   {
      dc.FillSolidRect(&rcItem, GetSysColor(COLOR_WINDOW));
      rgbOldColor = dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
   }

   // Draw severity icon
   iHeight = rcItem.bottom - rcItem.top;
   m_pImageList->Draw(&dc, item.iImage, 
                      CPoint(rcItem.left + 2, rcItem.top + 2), ILD_TRANSPARENT);

   memcpy(&rcText, &rcItem, sizeof(RECT));
   rcText.left += 24;
   pOldFont = dc.SelectObject(&m_fontSmall);
   m_wndListCtrl.GetItemText(iItem, 1, szBuffer, 256);
   wcscat(szBuffer, L" [");
   m_wndListCtrl.GetItemText(iItem, 0, &szBuffer[wcslen(szBuffer)], 256);
   wcscat(szBuffer, L"]\r\n");
   m_wndListCtrl.GetItemText(iItem, 2, &szBuffer[wcslen(szBuffer)], 256);
   dc.DrawText(szBuffer, -1, &rcText, DT_LEFT | DT_NOPREFIX);

   if (item.state & LVIS_FOCUSED)
      dc.DrawFocusRect(&rcItem);

   // Restore DC to original state
   dc.SelectObject(pOldFont);
   dc.SetTextColor(rgbOldColor);
}


//
// Process alarm updates
//

void CAlarmView::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
/*   int iItem;

   iItem = FindAlarmRecord(pAlarm->dwAlarmId);
   switch(dwCode)
   {
      case NX_NOTIFY_NEW_ALARM:
         if ((iItem == -1) && ((m_bShowAllAlarms) || (pAlarm->wIsAck == 0)))
         {
            AddAlarm(pAlarm);
            UpdateStatusBar();
         }
         break;
      case NX_NOTIFY_ALARM_ACKNOWLEGED:
         if ((iItem != -1) && (!m_bShowAllAlarms))
         {
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->wSeverity]--;
            UpdateStatusBar();
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
         if (iItem != -1)
         {
            m_wndListCtrl.DeleteItem(iItem);
            m_iNumAlarms[pAlarm->wSeverity]--;
            UpdateStatusBar();
         }
         break;
      default:
         break;
   }*/
}
