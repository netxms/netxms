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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
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
   m_pwndAlarmView = (CHtmlView *)RUNTIME_CLASS(CHtmlView)->CreateObject();
   m_pwndAlarmView->Create(NULL, _T("Alarm View"), WS_VISIBLE | WS_CHILD, 
                           rect, this, IDC_ALARM_LIST);

	return 0;
}


//
// Overloaded PreCreateWindow()
//

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

   cs.style = WS_POPUP | WS_MAXIMIZE;
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
   DWORD i, dwRetCode, dwNumAlarms;
   NXC_ALARM *pAlarmList;
   TCHAR szFileName[MAX_PATH];

   // Create HTML header
   _sntprintf(szFileName, MAX_PATH, _T("%s\\alarms.html"), g_szWorkDir);
   m_pHtmlFile = fopen(szFileName, "w");
   if (m_pHtmlFile != NULL)
   {
      fprintf(m_pHtmlFile, "<html><head><title>NetXMS Active Alarm List</title></head><body>\n"
                           "<table width=\"98%%\" align=center cellspacing=0 cellpadding=2 border=1>\n"
                           "<tr bgcolor=#9AAABA><td><b>Severity</b></td>"
                           "<td><b>Source</b></td><td><b>Message</b></td>"
                           "<td><b>Timestamp</b></td><td width=40 align=center><b>Ack</b></td></tr>");
      dwRetCode = DoRequestArg3(NXCLoadAllAlarms, (void *)FALSE, &dwNumAlarms, 
                                &pAlarmList, _T("Loading alarms..."));
      if (dwRetCode == RCC_SUCCESS)
      {
   //      memset(m_iNumAlarms, 0, sizeof(int) * 5);
         for(i = 0; i < dwNumAlarms; i++)
            AddAlarm(&pAlarmList[i], i & 1);
         safe_free(pAlarmList);
      }
      else
      {
         theApp.ErrorBox(dwRetCode, _T("Error loading alarm list: %s"));
      }
      fprintf(m_pHtmlFile, "</table></body></html>\n");
      fclose(m_pHtmlFile);

      _sntprintf(szFileName, MAX_PATH, _T("file://%s\\alarms.html"), g_szWorkDir);
      m_pwndAlarmView->Navigate(szFileName);
   }
}


//
// Add new alarm to list
//

void CMainFrame::AddAlarm(NXC_ALARM *pAlarm, BOOL bColoredLine)
{
   struct tm *ptm;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(pAlarm->dwSourceObject);
   ptm = localtime((const time_t *)&pAlarm->dwTimeStamp);
   strftime(szBuffer, 64, "%d-%b-%Y %H:%M:%S", ptm);
   fprintf(m_pHtmlFile, "<tr bgcolor=%s><td align=center>%s</td><td>%s</td><td>%s</td><td>%s</td><td align=center>&nbsp</td></tr>\n",
           bColoredLine ? "#EFEFEF" : "#FFFFFF",
           g_szStatusTextSmall[pAlarm->wSeverity], pObject->szName,
           pAlarm->szMessage, szBuffer);
   //m_iNumAlarms[pAlarm->wSeverity]++;
}
