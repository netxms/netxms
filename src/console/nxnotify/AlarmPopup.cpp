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
   g_imgListSeverity.Draw(&dc, m_pAlarm->wSeverity, CPoint(5, 5), ILD_TRANSPARENT);

   // Draw source name
   pObject = NXCFindObjectById(g_hSession, m_pAlarm->dwSourceObject);
   if (pObject != NULL)
   {
      rect.left = 26;
      rect.top = 5;
      rect.right = m_rcClient.right - 5;
      rect.bottom = rect.top + 16;
      pOldFont = dc.SelectObject(&m_fontBold);
      dc.DrawText(pObject->szName, _tcslen(pObject->szName), &rect,
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
   dc.DrawText(m_pAlarm->szMessage, _tcslen(m_pAlarm->szMessage), &rect,
               DT_LEFT | DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
   dc.SelectObject(pOldFont);
}


//
// WM_LBUTTONUP message handler
//

void CAlarmPopup::OnLButtonUp(UINT nFlags, CPoint point) 
{
   DestroyWindow();
}
