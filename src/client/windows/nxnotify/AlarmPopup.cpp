// AlarmPopup.cpp : implementation file
//

#include "stdafx.h"
#include "nxnotify.h"
#include "AlarmPopup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlarmPopup

CAlarmPopup::CAlarmPopup()
            :CTaskBarPopupWnd()
{
   HDC hDC;

   m_pAlarm = NULL;
   m_bDblClk = FALSE;

   // Create fonts
   hDC = ::GetDC(appNotify.m_pMainWnd->m_hWnd);
   m_fontNormal.CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                           0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           VARIABLE_PITCH | FF_SWISS, _T("MS Sans Serif"));
   m_fontBold.CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                         0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                         VARIABLE_PITCH | FF_SWISS, _T("MS Sans Serif"));
   ::ReleaseDC(appNotify.m_pMainWnd->m_hWnd, hDC);
}

CAlarmPopup::~CAlarmPopup()
{
   safe_free(m_pAlarm);
}


BEGIN_MESSAGE_MAP(CAlarmPopup, CTaskBarPopupWnd)
	//{{AFX_MSG_MAP(CAlarmPopup)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Draw popup window content
//

void CAlarmPopup::DrawContent(CDC &dc)
{
   RECT rect;
   NXC_OBJECT *pObject;
   CFont *pOldFont;

   if (m_pAlarm == NULL)   // Sanity check
      return;

   GetClientRect(&rect);
   dc.FillSolidRect(&m_rcClient, RGB(255, 255, 255));

   // Draw severity icon
   g_imgListSeverity.Draw(&dc, m_pAlarm->nCurrentSeverity, CPoint(5, 5), ILD_TRANSPARENT);

   // Draw source name
   pObject = NXCFindObjectById(g_hSession, m_pAlarm->dwSourceObject);
   if (pObject != NULL)
   {
      rect.left = 26;
      rect.top = 5;
      rect.right = m_rcClient.right - 5;
      rect.bottom = rect.top + 16;
      pOldFont = dc.SelectObject(&m_fontBold);
      dc.DrawText(pObject->szName, (int)_tcslen(pObject->szName), &rect,
                  DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
      dc.SelectObject(pOldFont);
   }

   // Draw separator
   dc.MoveTo(3, 26);
   dc.LineTo(m_rcClient.right - 3, 26);

   // Draw message text
   rect.left = 5;
   rect.right = m_rcClient.right - 5;
   rect.top = 31;
   rect.bottom = m_rcClient.bottom - 5;
   pOldFont = dc.SelectObject(&m_fontNormal);
   dc.DrawText(m_pAlarm->szMessage, (int)_tcslen(m_pAlarm->szMessage), &rect,
               DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
   dc.SelectObject(pOldFont);
}


//
// WM_LBUTTONUP message handler
//

void CAlarmPopup::OnLButtonUp(UINT nFlags, CPoint point) 
{
   SetTimer(1, GetDoubleClickTime() + 10, NULL);
}


//
// WM_LBUTTONDBLCLK message handler
//

void CAlarmPopup::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   m_bDblClk = TRUE;
   PopupAction(g_nActionDblClk);
}


//
// WM_RBUTTONUP message handler
//

void CAlarmPopup::OnRButtonUp(UINT nFlags, CPoint point) 
{
   PopupAction(g_nActionRight);
}


//
// Execute popup action
//

void CAlarmPopup::PopupAction(int nAction)
{
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   switch(nAction)
   {
      case NXNOTIFY_ACTION_DISMISS:
         DestroyWindow();
         break;
      case NXNOTIFY_ACTION_OPEN_LIST:
         appNotify.m_pMainWnd->PostMessage(WM_COMMAND, ID_TASKBAR_OPEN, 0);
         break;
      case NXNOTIFY_ACTION_OPEN_CONSOLE:
         memset(&si, 0, sizeof(STARTUPINFO));
         si.cb = sizeof(STARTUPINFO);
         if (CreateProcess(_T("nxcon.exe"), NULL, NULL, NULL, FALSE, 0, NULL,
                           NULL, &si, &pi))
         {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
         }
         else
         {
            appNotify.m_pMainWnd->MessageBox(_T("Failed to create process"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
         break;
      default:
         break;
   }
}


//
// WM_TIMER message handler
//

void CAlarmPopup::OnTimer(UINT_PTR nIDEvent) 
{
   if (nIDEvent == 1)
   {
      KillTimer(1);

      // Execute left-click action if double click was not detected
      if (!m_bDblClk)
         PopupAction(g_nActionLeft);
      m_bDblClk = FALSE;   // Reset double click flag
   }
   else
   {
	   CTaskBarPopupWnd::OnTimer(nIDEvent);
   }
}
