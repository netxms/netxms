// NodePoller.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePoller.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodePoller

IMPLEMENT_DYNCREATE(CNodePoller, CMDIChildWnd)

CNodePoller::CNodePoller()
{
   m_bPollingStopped = TRUE;
}

CNodePoller::~CNodePoller()
{
}


BEGIN_MESSAGE_MAP(CNodePoller, CMDIChildWnd)
	//{{AFX_MSG_MAP(CNodePoller)
	ON_WM_CREATE()
	ON_COMMAND(ID_POLL_RESTART, OnPollRestart)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(WM_POLLER_MESSAGE, OnPollerMessage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePoller message handlers


//
// WM_CREATE message handler
//

int CNodePoller::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create and setup message area
   GetClientRect(&rect);
   m_wndMsgArea.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | 
                       WS_VSCROLL | ES_AUTOVSCROLL | ES_AUTOHSCROLL, rect, this, -1);
   m_font.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                     0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     FIXED_PITCH | FF_DONTCARE, "Courier");
   m_wndMsgArea.SetFont(&m_font);

   return 0;
}


//
// (Re)start poll
//

void CNodePoller::OnPollRestart() 
{
   DWORD dwThreadId;

   if (m_bPollingStopped)
   {
      m_wndMsgArea.SetWindowText(_T(""));
      PrintMsg(_T("Sending poll request to server...\r\n"));
      m_bPollingStopped = FALSE;
      m_data.pArg1 = (void *)m_dwObjectId;
      m_data.pArg2 = (void *)m_iPollType;
      m_data.hWnd = m_hWnd;
      m_hThread = CreateThread(NULL, 0, PollerThread, &m_data, 0, &dwThreadId);
      CloseHandle(m_hThread);
   }
}


//
// WM_REQUEST_COMPLETED message handler
//

void CNodePoller::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
   m_dwResult = lParam;
   m_bPollingStopped = TRUE;
   if (m_dwResult == RCC_SUCCESS)
   {
      PrintMsg(_T("Poll completed successfully\r\n"));
   }
   else
   {
      TCHAR szBuffer[1024];

      _sntprintf(szBuffer, 1024, _T("Poll failed (%s)\r\n"), NXCGetErrorText(m_dwResult));
      PrintMsg(szBuffer);
   }
}


//
// WM_POLLER_MESSAGE message handler
//

void CNodePoller::OnPollerMessage(WPARAM wParam, LPARAM lParam)
{
   if (lParam != 0)
   {
      PrintMsg((TCHAR *)lParam);
      free((void *)lParam);
   }
}


//
// Print message in message area
//

void CNodePoller::PrintMsg(TCHAR *pszMsg)
{
   m_wndMsgArea.SetSel(0, -1);
   m_wndMsgArea.SetSel(-1, -1);
   m_wndMsgArea.ReplaceSel(pszMsg);
}


//
// WM_SIZE message handler
//

void CNodePoller::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndMsgArea.MoveWindow(0, 0, cx, cy);
}


//
// WM_SETFOCUS message handler
//

void CNodePoller::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndMsgArea.SetFocus();
}
