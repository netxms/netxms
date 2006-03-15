// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "nxav.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define INFO_LINE_HEIGHT      80

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_CMD_EXIT, OnCmdExit)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_ALARM_UPDATE, OnAlarmUpdate)
   ON_MESSAGE(WM_DISABLE_ALARM_SOUND, OnDisableAlarmSound)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
   m_dwNumAlarms = 0;
   m_pAlarmList = NULL;
   memset(m_iNumAlarms, 0, sizeof(int) * 5);
}

CMainFrame::~CMainFrame()
{
   safe_free(m_pAlarmList);
}


//
// WM_CREATE message handler
//

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   RECT rect;

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create info line
   GetClientRect(&rect);
   m_wndInfoLine.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, this, IDC_INFO_LINE);

   // Create embedded HTML control
   m_pwndAlarmView = (CAlarmBrowser *)RUNTIME_CLASS(CAlarmBrowser)->CreateObject();
   m_pwndAlarmView->Create(NULL, _T("Alarm View"), WS_VISIBLE | WS_CHILD, 
                           rect, this, IDC_ALARM_LIST);

   m_nTimer = SetTimer(1, 1000, NULL);

	return 0;
}


//
// Overloaded PreCreateWindow()
//

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

   cs.style = WS_POPUP;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

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

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


//
// WM_SETFOCUS message handler
//

void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
   m_pwndAlarmView->SetFocus();
}


//
// WM_SIZE message handler
//

void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);

   m_wndInfoLine.SetWindowPos(NULL, 0, 0, cx, INFO_LINE_HEIGHT, SWP_NOZORDER);
   m_pwndAlarmView->SetWindowPos(NULL, 0, INFO_LINE_HEIGHT, cx, cy - INFO_LINE_HEIGHT, SWP_NOZORDER);
}


//
// Refresh alarm list
//

void CMainFrame::OnViewRefresh() 
{
   DWORD i, dwRetCode;
   CString strHTML;

   safe_free(m_pAlarmList);
   m_pAlarmList = NULL;
   m_dwNumAlarms = 0;
   memset(m_iNumAlarms, 0, sizeof(int) * 5);
   dwRetCode = DoRequestArg4(NXCLoadAllAlarms, g_hSession, (void *)FALSE, &m_dwNumAlarms, 
                             &m_pAlarmList, _T("Loading alarms..."));
   if (dwRetCode == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumAlarms; i++)
         m_iNumAlarms[m_pAlarmList[i].wSeverity]++;
      SortAlarms();
   }
   else
   {
      theApp.ErrorBox(dwRetCode, _T("Error loading alarm list: %s"));
   }

   GenerateHtml(strHTML);
//   _sntprintf(szFileName, MAX_PATH, _T("file://%s\\alarms.html"), g_szWorkDir);
//   m_pwndAlarmView->Navigate(szFileName);
   m_pwndAlarmView->Navigate(_T("about:blank"));
   m_pwndAlarmView->SetHTML(strHTML);
}


//
// Add new alarm to list
//

void CMainFrame::AddAlarm(NXC_ALARM *pAlarm, CString &strHTML, BOOL bColoredLine)
{
   struct tm *ptm;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;
   CString strBuf;

   pObject = NXCFindObjectById(g_hSession, pAlarm->dwSourceObject);
   ptm = localtime((const time_t *)&pAlarm->dwTimeStamp);
   strftime(szBuffer, 64, "%d-%b-%Y<br>%H:%M:%S", ptm);
   strBuf.Format("<tr bgcolor=%s><td align=left><table cellpadding=2 border=0><tr>"
                 "<td><img src=\"file:%s/%s.ico\" border=0/></td>"
                 "<td><b>%s</b></td></tr></table></td>"
                 "<td><b>%s</b></td>"
                 "<td><font size=-1>%s</font></td><td><font size=-1>%s</font></td>"
                 "<td align=center><a href=\"nxav:S?%d\"><img src=\"file:%s/%ssound.png\" alt=\"NOSOUND\" border=0/></a> "
                 "<a href=\"nxav:A?%d\"><img src=\"file:%s/ack.png\" alt=\"ACK\" border=0/></a></td></tr>\n",
                 bColoredLine ? "#EFEFEF" : "#FFFFFF", 
                 g_szWorkDir, g_szStatusTextSmall[pAlarm->wSeverity],
                 g_szStatusTextSmall[pAlarm->wSeverity], pObject->szName,
                 pAlarm->szMessage, szBuffer, pAlarm->dwAlarmId, g_szWorkDir,
                 pAlarm->pUserData != 0 ? "" : "no", pAlarm->dwAlarmId, g_szWorkDir);
   strHTML += strBuf;
}


//
// Process "Exit" button
//

void CMainFrame::OnCmdExit() 
{
   DestroyWindow();
}


//
// Generate HTML file
//

void CMainFrame::GenerateHtml(CString &strHTML)
{
   DWORD i;

   strHTML.Empty();

   strHTML.Format("<html><head><title>NetXMS Active Alarm List</title></head>\n"
                  "<body background=\"file:%s/background.jpg\">\n"
                  "<font face=verdana,helvetica size=+1>\n"
                  "<table width=\"99%%\" align=center cellspacing=0 cellpadding=2 border=1>\n"
                  "<tr bgcolor=#9AAABA><td><b>Severity</b></td>"
                  "<td><b>Source</b></td><td><b>Message</b></td>"
                  "<td><b>Timestamp</b></td><td width=40 align=center><b>Ack</b></td></tr>",
                  g_szWorkDir);
   for(i = 0; i < m_dwNumAlarms; i++)
      AddAlarm(&m_pAlarmList[i], strHTML, i & 1);
   strHTML += _T("</table></font></body></html>\n");
}


//
// WM_ALARM_UPDATE message handler
//

void CMainFrame::OnAlarmUpdate(WPARAM wParam, LPARAM lParam)
{
   NXC_ALARM *pAlarm = (NXC_ALARM *)lParam;
   DWORD i;
   CString strHTML;

   switch(wParam)
   {
      case NX_NOTIFY_NEW_ALARM:
         if (!pAlarm->wIsAck)
         {
            m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * (m_dwNumAlarms + 1));
            memcpy(&m_pAlarmList[m_dwNumAlarms], pAlarm, sizeof(NXC_ALARM));
            m_pAlarmList[m_dwNumAlarms].pUserData = (void *)time(NULL);
            m_dwNumAlarms++;
            SortAlarms();
            PlayAlarmSound(pAlarm, TRUE, g_hSession, &theApp.m_soundCfg);
         }
         break;
      case NX_NOTIFY_ALARM_DELETED:
      case NX_NOTIFY_ALARM_ACKNOWLEGED:
         for(i = 0; i < m_dwNumAlarms; i++)
            if (m_pAlarmList[i].dwAlarmId == pAlarm->dwAlarmId)
            {
               PlayAlarmSound(&m_pAlarmList[i], FALSE, g_hSession, &theApp.m_soundCfg);
               m_iNumAlarms[m_pAlarmList[i].wSeverity]--;
               m_dwNumAlarms--;
               if (m_dwNumAlarms > 0)
                  memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_dwNumAlarms - i));
               break;
            }
         break;
      default:
         break;
   }
   GenerateHtml(strHTML);
   //m_pwndAlarmView->Refresh();
   m_pwndAlarmView->SetHTML(strHTML);
   free(pAlarm);
}


//
// Alarm comparision function for qsort()
//

static int CompareAlarms(const NXC_ALARM *p1, const NXC_ALARM *p2)
{
   return (p1->wSeverity > p2->wSeverity) ? -1 : ((p1->wSeverity < p2->wSeverity) ? 1 : 0);
}


//
// Sort alarms in list
//

void CMainFrame::SortAlarms()
{
   qsort(m_pAlarmList, m_dwNumAlarms, sizeof(NXC_ALARM), (int (*)(const void *,const void *))CompareAlarms);
}


//
// WM_DISABLE_ALARM_SOUND message handler
//

void CMainFrame::OnDisableAlarmSound(WPARAM wParam, LPARAM lParam)
{
   DWORD i;
   CString strHTML;

   for(i = 0; i < m_dwNumAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == wParam)
      {
         m_pAlarmList[i].pUserData = 0;
         GenerateHtml(strHTML);
         m_pwndAlarmView->SetHTML(strHTML);
         break;
      }
}


//
// WM_DESTROY message handler
//

void CMainFrame::OnDestroy() 
{
   KillTimer(m_nTimer);
	CFrameWnd::OnDestroy();	
}


//
// WM_TIMER message handler
//

void CMainFrame::OnTimer(UINT nIDEvent) 
{
   DWORD i;
   time_t now;

   if (g_bRepeatSound)
   {
      now = time(NULL);
      for(i = 0; i < m_dwNumAlarms; i++)
         if ((m_pAlarmList[i].pUserData != 0) &&
             (now - (time_t)m_pAlarmList[i].pUserData > 60))
         {
            PlayAlarmSound(&m_pAlarmList[i], TRUE, g_hSession, &theApp.m_soundCfg);
            m_pAlarmList[i].pUserData = (void *)now;
         }
   }
}
