// GraphView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "GraphView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Static data
//

static DWORD m_dwTimeUnitSize[MAX_TIME_UNITS] = { 60, 3600, 86400 };


/////////////////////////////////////////////////////////////////////////////
// CGraphView

CGraphView::CGraphView()
{
   m_dwNumItems = 0;
   memset(m_pdwItemList, 0, sizeof(DWORD) * MAX_GRAPH_ITEMS);
   m_dwRefreshInterval = 30;
//   m_dwFlags = 0;
//   m_hTimer = 0;
   m_dwSeconds = 0;
   m_iTimeFrameType = 1;   // Back from now
   m_iTimeUnit = 1;  // Hours
   m_dwNumTimeUnits = 1;
   m_dwTimeFrame = 3600;   // By default, graph covers 3600 seconds
}

CGraphView::~CGraphView()
{
}


BEGIN_MESSAGE_MAP(CGraphView, CDynamicView)
	//{{AFX_MSG_MAP(CGraphView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST10MINUTES, OnGraphPresetsLast10minutes)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST30MINUTES, OnGraphPresetsLast30minutes)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTHOUR, OnGraphPresetsLasthour)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST2HOURS, OnGraphPresetsLast2hours)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST4HOURS, OnGraphPresetsLast4hours)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTDAY, OnGraphPresetsLastday)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTWEEK, OnGraphPresetsLastweek)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_GRAPH_FULLSCREEN, OnGraphFullscreen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Get view's fingerprint
//

QWORD CGraphView::GetFingerprint()
{
   return CREATE_VIEW_FINGERPRINT(VIEW_CLASS_GRAPH, m_dwSum1, m_dwSum2);
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Data is a pointer to object.
//

void CGraphView::InitView(void *pData)
{
   BYTE hash[MD5_DIGEST_SIZE];

   m_dwNodeId = ((GRAPH_VIEW_INIT *)pData)->dwNodeId;
   m_dwNumItems = ((GRAPH_VIEW_INIT *)pData)->dwNumItems;
   memcpy(m_pdwItemList, ((GRAPH_VIEW_INIT *)pData)->pdwItemList, sizeof(DWORD) * m_dwNumItems);
   CalculateMD5Hash((BYTE *)pData, sizeof(GRAPH_VIEW_INIT), hash);
   memcpy(&m_dwSum1, hash, sizeof(DWORD));
   memcpy(&m_dwSum2, &hash[sizeof(DWORD)], sizeof(DWORD));
}


/////////////////////////////////////////////////////////////////////////////
// CGraphView message handlers

int CGraphView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

   if (CDynamicView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
   m_wndGraph.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_GRAPH);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   //if (m_dwFlags & GF_AUTOUPDATE)
   //   m_hTimer = SetTimer(1, 1000, NULL);

	return 0;
}


//
// WM_SIZE message handler
//

void CGraphView::OnSize(UINT nType, int cx, int cy) 
{
	CDynamicView::OnSize(nType, cx, cy);
   m_wndGraph.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CGraphView::OnViewRefresh() 
{
   DWORD i, dwResult;
   NXC_DCI_DATA *pData;

   // Set new time frame
   if (m_iTimeFrameType == 1)
   {
      m_dwTimeTo = WCE_FCTN(time)(NULL);
      m_dwTimeTo -= m_dwTimeTo % 60;   // Round to minute boundary
      m_dwTimeFrom = m_dwTimeTo - m_dwTimeFrame;
   }
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);

   for(i = 0; i < m_dwNumItems; i++)
   {
      dwResult = DoRequestArg7(NXCGetDCIData, g_hSession, (void *)m_dwNodeId, 
                               (void *)m_pdwItemList[i], 0, (void *)m_dwTimeFrom,
                               (void *)m_dwTimeTo, &pData, _T("Loading item data..."));
      if (dwResult == RCC_SUCCESS)
      {
         m_wndGraph.SetData(i, pData);
      }
      else
      {
         theApp.ErrorBox(dwResult, _T("Unable to retrieve colected data: %s"));
      }
   }
   m_wndGraph.Invalidate(FALSE);
}


//
// Set time frame to given preset
//

void CGraphView::Preset(int nTimeUnit, DWORD dwNumUnits)
{
   m_iTimeFrameType = 1;   // Back from now
   m_iTimeUnit = nTimeUnit;
   m_dwNumTimeUnits = dwNumUnits;
   m_dwTimeFrame = m_dwNumTimeUnits * m_dwTimeUnitSize[m_iTimeUnit];
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// Presets handlers
//

void CGraphView::OnGraphPresetsLast10minutes() 
{
   Preset(TIME_UNIT_MINUTE, 10);
}

void CGraphView::OnGraphPresetsLast30minutes() 
{
   Preset(TIME_UNIT_MINUTE, 30);
}

void CGraphView::OnGraphPresetsLasthour() 
{
   Preset(TIME_UNIT_HOUR, 1);
}

void CGraphView::OnGraphPresetsLast2hours() 
{
   Preset(TIME_UNIT_HOUR, 2);
}

void CGraphView::OnGraphPresetsLast4hours() 
{
   Preset(TIME_UNIT_HOUR, 4);
}

void CGraphView::OnGraphPresetsLastday() 
{
   Preset(TIME_UNIT_DAY, 1);
}

void CGraphView::OnGraphPresetsLastweek() 
{
   Preset(TIME_UNIT_DAY, 7);
}


//
// WM_CONTEXTMENU message handler
//

void CGraphView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(3);

   // Disable or check some menu items
   pMenu->EnableMenuItem(ID_GRAPH_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
   pMenu->CheckMenuItem(ID_GRAPH_FULLSCREEN, 
      MF_BYCOMMAND | (((CMainFrame *)theApp.m_pMainWnd)->IsFullScreen() ? MF_CHECKED : 0));

   pMenu->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, NULL);
}


//
// WM_COMMAND::ID_GRAPH_FULLSCREEN message handler
//

void CGraphView::OnGraphFullscreen() 
{
   ((CMainFrame *)theApp.m_pMainWnd)->ToggleFullScreen();
}
