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
	ON_COMMAND(ID_POLLER_COPYTOCLIPBOARD, OnPollerCopytoclipboard)
	ON_UPDATE_COMMAND_UI(ID_POLLER_COPYTOCLIPBOARD, OnUpdatePollerCopytoclipboard)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_POLLER_MESSAGE, OnPollerMessage)
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
   m_wndMsgArea.Create(_T("Edit"), WS_CHILD | WS_VISIBLE, rect, this, ID_EDIT_CTRL);
   m_wndMsgArea.LoadLexer("nxlexer.dll");
   m_wndMsgArea.SetLexer("nxpoll");
	//m_wndMsgArea.SetReadOnly(TRUE);

   return 0;
}


//
// (Re)start poll
//

void CNodePoller::OnPollRestart() 
{
   if (m_bPollingStopped)
   {
      //m_wndMsgArea.DeleteAllItems();
		m_wndMsgArea.Clear();
      PrintMsg(_T("Sending poll request to server...\r\n"));
      m_bPollingStopped = FALSE;
      m_data.pArg1 = (void *)m_dwObjectId;
      m_data.pArg2 = (void *)m_iPollType;
      m_data.hWnd = m_hWnd;
      ThreadCreate(PollerThread, 0, &m_data);
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

      _sntprintf(szBuffer, 1024, _T("\x7F") _T("ePoll failed (%s)\r\n"), NXCGetErrorText(m_dwResult));
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
	TCHAR buffer[256];

	buffer[0] = _T('[');
	FormatTimeStamp(time(NULL), &buffer[1], TS_LONG_DATE_TIME);
	_tcscat(buffer, _T("] "));
	m_wndMsgArea.AppendText(buffer, FALSE);
	m_wndMsgArea.AppendText(pszMsg, TRUE);
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


//
// Copy text to clipboard
//

void CNodePoller::OnPollerCopytoclipboard() 
{
	m_wndMsgArea.Copy();
}

void CNodePoller::OnUpdatePollerCopytoclipboard(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}


//
// Handler for "Select all"
//

void CNodePoller::OnEditSelectAll() 
{
	m_wndMsgArea.SelectAll();
}
