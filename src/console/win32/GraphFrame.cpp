// GraphFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphFrame.h"
#include "GraphSettingsPage.h"
#include "GraphDataPage.h"

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
// CGraphStatusBar

void CGraphStatusBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   CDC *pdc;
   CBrush brush;
   RECT rect;

   brush.CreateSolidBrush(RGB(0, 0, 128));
   pdc = CDC::FromHandle(lpDrawItemStruct->hDC);
   memcpy(&rect, &lpDrawItemStruct->rcItem, sizeof(RECT));
   rect.left++;
   rect.top++;
   rect.bottom--;
   rect.right = rect.left + 
      (lpDrawItemStruct->rcItem.right - lpDrawItemStruct->rcItem.left - 1) * 
         lpDrawItemStruct->itemData / m_dwMaxValue;
   pdc->FillRect(&rect, &brush);
}


/////////////////////////////////////////////////////////////////////////////
// CGraphFrame

IMPLEMENT_DYNCREATE(CGraphFrame, CMDIChildWnd)

CGraphFrame::CGraphFrame()
{
   m_dwNumItems = 0;
   memset(m_ppItems, 0, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);
   m_dwRefreshInterval = 30;
   m_dwFlags = 0;
   m_hTimer = 0;
   m_dwSeconds = 0;
   m_iTimeFrameType = 1;   // Back from now
   m_iTimeUnit = 1;  // Hours
   m_dwNumTimeUnits = 1;
   m_dwTimeFrame = 3600;   // By default, graph covers 3600 seconds
}

CGraphFrame::~CGraphFrame()
{
   DWORD i;

   for(i = 0; i < m_dwNumItems; i++)
      delete m_ppItems[i];
}


BEGIN_MESSAGE_MAP(CGraphFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CGraphFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_GRAPH_PROPERTIES, OnGraphProperties)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_GRAPH_PRESETS_LASTHOUR, OnGraphPresetsLasthour)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST2HOURS, OnGraphPresetsLast2hours)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST4HOURS, OnGraphPresetsLast4hours)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTDAY, OnGraphPresetsLastday)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTWEEK, OnGraphPresetsLastweek)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST10MINUTES, OnGraphPresetsLast10minutes)
	ON_COMMAND(ID_GRAPH_PRESETS_LAST30MINUTES, OnGraphPresetsLast30minutes)
	ON_COMMAND(ID_GRAPH_RULER, OnGraphRuler)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_RULER, OnUpdateGraphRuler)
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_GRAPH_LEGEND, OnGraphLegend)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_LEGEND, OnUpdateGraphLegend)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTMONTH, OnGraphPresetsLastmonth)
	ON_COMMAND(ID_GRAPH_PRESETS_LASTYEAR, OnGraphPresetsLastyear)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_ZOOMOUT, OnUpdateGraphZoomout)
	ON_COMMAND(ID_GRAPH_ZOOMOUT, OnGraphZoomout)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_UPDATE_GRAPH_POINT, OnUpdateGraphPoint)
   ON_MESSAGE(NXCM_GRAPH_ZOOM_CHANGED, OnGraphZoomChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphFrame message handlers


//
// WM_CREATE message handler
//

int CGraphFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static int widths[4] = { 70, 170, 220, -1 };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);

   m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(4, widths);

   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;
   rect.bottom -= m_iStatusBarHeight;
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
   m_wndGraph.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_GRAPH);
   m_wndGraph.SetDCIInfo(m_ppItems);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   if (m_dwFlags & GF_AUTOUPDATE)
      m_hTimer = SetTimer(1, 1000, NULL);

   m_wndStatusBar.m_dwMaxValue = m_dwRefreshInterval;
   m_wndStatusBar.SetText(m_wndGraph.m_bAutoScale ? _T("Autoscale") : _T(""), 0, 0);
   m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
                          (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);

	return 0;
}


//
// WM_SIZE message handler
//

void CGraphFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndGraph.SetWindowPos(NULL, 0, 0, cx, cy - m_iStatusBarHeight, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CGraphFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);

   m_wndGraph.SetFocus();
}


//
// Add new item to display
//

void CGraphFrame::AddItem(DWORD dwNodeId, NXC_DCI *pItem)
{
   if (m_dwNumItems < MAX_GRAPH_ITEMS)
   {
      m_ppItems[m_dwNumItems] = new DCIInfo(dwNodeId, pItem);
      m_dwNumItems++;
   }
}


//
// Set time frame for graph
//

void CGraphFrame::SetTimeFrame(DWORD dwTimeFrom, DWORD dwTimeTo)
{
   m_dwTimeFrom = dwTimeFrom;
   m_dwTimeTo = dwTimeTo;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CGraphFrame::OnViewRefresh() 
{
   DWORD i, dwResult;
   NXC_DCI_DATA *pData;

   // Set new time frame
   if (m_iTimeFrameType == 1)
   {
      m_dwTimeTo = time(NULL);
      m_dwTimeTo -= m_dwTimeTo % 60;   // Round to minute boundary
      m_dwTimeFrom = m_dwTimeTo - m_dwTimeFrame;
   }
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);

   for(i = 0; i < m_dwNumItems; i++)
   {
      dwResult = DoRequestArg7(NXCGetDCIData, g_hSession, (void *)m_ppItems[i]->m_dwNodeId, 
                               (void *)m_ppItems[i]->m_dwItemId, 0, (void *)m_dwTimeFrom,
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
   m_wndGraph.ClearZoomHistory();
   m_wndGraph.Update();
}


//
// Redefined PreCreateWindow()
//

BOOL CGraphFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_GRAPH));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_COMMAND::ID_GRAPH_PROPERTIES
//

void CGraphFrame::OnGraphProperties() 
{
   CPropertySheet dlg(_T("Graph Properties"), theApp.GetMainWnd(), 0);
   CGraphSettingsPage pgSettings;
   CGraphDataPage pgData;
   int i;

   // Create "Settings" page
   pgSettings.m_bAutoscale = m_wndGraph.m_bAutoScale;
   pgSettings.m_bShowGrid = m_wndGraph.m_bShowGrid;
   pgSettings.m_bRuler = m_wndGraph.m_bShowRuler;
   pgSettings.m_bShowLegend = m_wndGraph.m_bShowLegend;
   pgSettings.m_bEnableZoom = m_wndGraph.m_bEnableZoom;
   pgSettings.m_bAutoUpdate = (m_dwFlags & GF_AUTOUPDATE) ? TRUE : FALSE;
   pgSettings.m_dwRefreshInterval = m_dwRefreshInterval;
   pgSettings.m_rgbAxisLines = m_wndGraph.m_rgbAxisColor;
   pgSettings.m_rgbBackground = m_wndGraph.m_rgbBkColor;
   pgSettings.m_rgbGridLines = m_wndGraph.m_rgbGridColor;
   pgSettings.m_rgbLabelBkgnd = m_wndGraph.m_rgbLabelBkColor;
   pgSettings.m_rgbLabelText = m_wndGraph.m_rgbLabelTextColor;
   pgSettings.m_rgbText = m_wndGraph.m_rgbTextColor;
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      pgSettings.m_rgbItems[i] = m_wndGraph.m_rgbLineColors[i];
   pgSettings.m_iTimeFrame = m_iTimeFrameType;
   pgSettings.m_iTimeUnit = m_iTimeUnit;
   pgSettings.m_dwNumUnits = m_dwNumTimeUnits;
   pgSettings.m_dateFrom = (time_t)m_dwTimeFrom;
   pgSettings.m_timeFrom = (time_t)m_dwTimeFrom;
   pgSettings.m_dateTo = (time_t)m_dwTimeTo;
   pgSettings.m_timeTo = (time_t)m_dwTimeTo;
   dlg.AddPage(&pgSettings);

   // Create "Data Sources" page
   pgData.m_dwNumItems = m_dwNumItems;
   memcpy(pgData.m_ppItems, m_ppItems, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);
   dlg.AddPage(&pgData);

   // Open property sheet
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   if (dlg.DoModal() == IDOK)
   {
      m_dwNumItems = pgData.m_dwNumItems;
      memcpy(m_ppItems, pgData.m_ppItems, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);

      if (m_hTimer != 0)
         KillTimer(m_hTimer);
      m_dwRefreshInterval = pgSettings.m_dwRefreshInterval;
      m_wndStatusBar.m_dwMaxValue = m_dwRefreshInterval;
      if (pgSettings.m_bAutoUpdate)
      {
         m_dwFlags |= GF_AUTOUPDATE;
         m_hTimer = SetTimer(1, 1000, NULL);
         m_dwSeconds = 0;
      }
      else
      {
         m_dwFlags &= ~GF_AUTOUPDATE;
      }

      m_wndGraph.m_bAutoScale = pgSettings.m_bAutoscale;
      m_wndGraph.m_bShowGrid = pgSettings.m_bShowGrid;
      m_wndGraph.m_bShowLegend = pgSettings.m_bShowLegend;
      m_wndGraph.m_bShowRuler = pgSettings.m_bRuler;
      m_wndGraph.m_bEnableZoom = pgSettings.m_bEnableZoom;

      m_wndGraph.m_rgbAxisColor = pgSettings.m_rgbAxisLines;
      m_wndGraph.m_rgbBkColor = pgSettings.m_rgbBackground;
      m_wndGraph.m_rgbGridColor = pgSettings.m_rgbGridLines;
      m_wndGraph.m_rgbLabelBkColor = pgSettings.m_rgbLabelBkgnd;
      m_wndGraph.m_rgbLabelTextColor = pgSettings.m_rgbLabelText;
      m_wndGraph.m_rgbTextColor = pgSettings.m_rgbText;
      for(i = 0; i < MAX_GRAPH_ITEMS; i++)
         m_wndGraph.m_rgbLineColors[i] = pgSettings.m_rgbItems[i];

      m_wndStatusBar.SetText(m_wndGraph.m_bAutoScale ? _T("Autoscale") : _T(""), 0, 0);
      m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
                             (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);

      m_iTimeFrameType = pgSettings.m_iTimeFrame;
      m_iTimeUnit = pgSettings.m_iTimeUnit;
      m_dwNumTimeUnits = pgSettings.m_dwNumUnits;
      if (m_iTimeFrameType == 0)
      {
         m_dwTimeFrom = CTime(pgSettings.m_dateFrom.GetYear(), pgSettings.m_dateFrom.GetMonth(),
                              pgSettings.m_dateFrom.GetDay(), pgSettings.m_timeFrom.GetHour(),
                              pgSettings.m_timeFrom.GetMinute(),
                              pgSettings.m_timeFrom.GetSecond(), -1).GetTime();
         m_dwTimeTo = CTime(pgSettings.m_dateTo.GetYear(), pgSettings.m_dateTo.GetMonth(),
                            pgSettings.m_dateTo.GetDay(), pgSettings.m_timeTo.GetHour(),
                            pgSettings.m_timeTo.GetMinute(),
                            pgSettings.m_timeTo.GetSecond(), -1).GetTime();
         if (m_dwTimeTo < m_dwTimeFrom)
         {
            m_dwTimeTo = m_dwTimeFrom;
         }
         m_dwTimeFrame = m_dwTimeTo - m_dwTimeFrom;
      }
      else
      {
         // Back from now
         m_dwTimeFrame = m_dwNumTimeUnits * m_dwTimeUnitSize[m_iTimeUnit];
      }

      /* TODO: send refresh only if time frame was changed */
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   }
}


//
// WM_DESTROY message handler
//

void CGraphFrame::OnDestroy() 
{
   if (m_hTimer != 0)
      KillTimer(m_hTimer);

	CMDIChildWnd::OnDestroy();
}


//
// WM_TIMER message handler
//

void CGraphFrame::OnTimer(UINT nIDEvent) 
{
   m_dwSeconds++;
   if (m_dwSeconds == m_dwRefreshInterval)
   {
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
      m_dwSeconds = 0;
   }
   m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
                          (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);
}


//
// Get save info for desktop saving
//

LRESULT CGraphFrame::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   TCHAR szBuffer[512];
   DWORD i;

   pInfo->iWndClass = WNDC_GRAPH;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_WND_PARAM_LEN,
              _T("F:%d\x7FN:%d\x7FTS:%d\x7FTF:%d\x7F") _T("A:%d\x7F")
              _T("TFT:%d\x7FTU:%d\x7FNTU:%d\x7FS:%d\x7F")
              _T("CA:%u\x7F") _T("CB:%u\x7F") _T("CG:%u\x7F") _T("CK:%u\x7F") _T("CL:%u\x7F")
              _T("CT:%u\x7FR:%d\x7FG:%d\x7FL:%d"),
              m_dwFlags, m_dwNumItems, m_dwTimeFrom, m_dwTimeTo, m_dwRefreshInterval,
              m_iTimeFrameType, m_iTimeUnit, m_dwNumTimeUnits,
              m_wndGraph.m_bAutoScale, m_wndGraph.m_rgbAxisColor, 
              m_wndGraph.m_rgbBkColor, m_wndGraph.m_rgbGridColor,
              m_wndGraph.m_rgbLabelBkColor, m_wndGraph.m_rgbLabelTextColor,
              m_wndGraph.m_rgbTextColor, m_wndGraph.m_bShowRuler,
              m_wndGraph.m_bShowGrid, m_wndGraph.m_bShowLegend);

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      _sntprintf(szBuffer, 512, _T("\x7F") _T("C%d:%lu"), i, m_wndGraph.m_rgbLineColors[i]);
      if (_tcslen(pInfo->szParameters) + _tcslen(szBuffer) < MAX_WND_PARAM_LEN)
         _tcscat(pInfo->szParameters, szBuffer);
   }
   
   for(i = 0; i < m_dwNumItems; i++)
   {
      _sntprintf(szBuffer, 512, _T("\x7FN%d:%d\x7FI%d:%d\x7FIS%d:%d\x7FIT%d:%d\x7FIN%d:%s\x7FID%d:%s"),
                 i, m_ppItems[i]->m_dwNodeId, i, m_ppItems[i]->m_dwItemId,
                 i, m_ppItems[i]->m_iSource, i, m_ppItems[i]->m_iDataType,
                 i, m_ppItems[i]->m_pszParameter, i, m_ppItems[i]->m_pszDescription);
      if (_tcslen(pInfo->szParameters) + _tcslen(szBuffer) < MAX_WND_PARAM_LEN)
         _tcscat(pInfo->szParameters, szBuffer);
   }

   _sntprintf(&pInfo->szParameters[_tcslen(pInfo->szParameters)],
              MAX_WND_PARAM_LEN - _tcslen(pInfo->szParameters),
              _T("\x7FT:%s"), m_szSubTitle);
   return 1;
}


//
// Restore graph window from server
//

void CGraphFrame::RestoreFromServer(TCHAR *pszParams)
{
   TCHAR szBuffer[32], szValue[256];
   DWORD i;

   m_dwFlags = ExtractWindowParamULong(pszParams, _T("F"), 0);
   m_dwNumItems = ExtractWindowParamULong(pszParams, _T("N"), 0);
   m_iTimeFrameType = ExtractWindowParamLong(pszParams, _T("TFT"), 0);
   m_iTimeUnit = ExtractWindowParamLong(pszParams, _T("TU"), 0);
   m_dwNumTimeUnits = ExtractWindowParamULong(pszParams, _T("NTU"), 0);
   m_dwTimeFrom = ExtractWindowParamULong(pszParams, _T("TS"), 0);
   m_dwTimeTo = ExtractWindowParamULong(pszParams, _T("TF"), 0);
   m_dwRefreshInterval = ExtractWindowParamULong(pszParams, _T("A"), 30);
   m_wndGraph.m_bAutoScale = (BOOL)ExtractWindowParamLong(pszParams, _T("S"), TRUE);
   m_wndGraph.m_bShowGrid = (BOOL)ExtractWindowParamLong(pszParams, _T("G"), TRUE);
   m_wndGraph.m_bShowLegend = (BOOL)ExtractWindowParamLong(pszParams, _T("L"), TRUE);
   m_wndGraph.m_bShowRuler = (BOOL)ExtractWindowParamLong(pszParams, _T("R"), TRUE);
   m_wndGraph.m_rgbAxisColor = ExtractWindowParamULong(pszParams, _T("CA"), m_wndGraph.m_rgbAxisColor);
   m_wndGraph.m_rgbBkColor = ExtractWindowParamULong(pszParams, _T("CB"), m_wndGraph.m_rgbBkColor);
   m_wndGraph.m_rgbGridColor = ExtractWindowParamULong(pszParams, _T("CG"), m_wndGraph.m_rgbGridColor);
   m_wndGraph.m_rgbLabelBkColor = ExtractWindowParamULong(pszParams, _T("CK"), m_wndGraph.m_rgbLabelBkColor);
   m_wndGraph.m_rgbLabelTextColor = ExtractWindowParamULong(pszParams, _T("CL"), m_wndGraph.m_rgbLabelTextColor);
   m_wndGraph.m_rgbTextColor = ExtractWindowParamULong(pszParams, _T("CT"), m_wndGraph.m_rgbTextColor);

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      _sntprintf(szBuffer, 32, _T("C%d"), i);
      m_wndGraph.m_rgbLineColors[i] = 
         ExtractWindowParamULong(pszParams, szBuffer, m_wndGraph.m_rgbLineColors[i]);
   }

   for(i = 0; i < m_dwNumItems; i++)
   {
      m_ppItems[i] = new DCIInfo;
      
      _sntprintf(szBuffer, 32, _T("N%d"), i);
      m_ppItems[i]->m_dwNodeId = ExtractWindowParamULong(pszParams, szBuffer, 0);
      
      _sntprintf(szBuffer, 32, _T("I%d"), i);
      m_ppItems[i]->m_dwItemId = ExtractWindowParamULong(pszParams, szBuffer, 0);
      
      _sntprintf(szBuffer, 32, _T("IS%d"), i);
      m_ppItems[i]->m_iSource = ExtractWindowParamLong(pszParams, szBuffer, 0);
      
      _sntprintf(szBuffer, 32, _T("IT%d"), i);
      m_ppItems[i]->m_iDataType = ExtractWindowParamLong(pszParams, szBuffer, 0);
      
      _sntprintf(szBuffer, 32, _T("IN%d"), i);
      if (ExtractWindowParam(pszParams, szBuffer, szValue, 256))
         m_ppItems[i]->m_pszParameter = _tcsdup(szValue);
      else
         m_ppItems[i]->m_pszParameter = _tcsdup(_T("<unknown>"));

      _sntprintf(szBuffer, 32, _T("ID%d"), i);
      if (ExtractWindowParam(pszParams, szBuffer, szValue, 256))
         m_ppItems[i]->m_pszDescription = _tcsdup(szValue);
      else
         m_ppItems[i]->m_pszDescription = _tcsdup(_T("<unknown>"));
   }

   ExtractWindowParam(pszParams, _T("T"), m_szSubTitle, 256);

   if (m_iTimeFrameType == 1)
   {
      m_dwTimeFrame = m_dwNumTimeUnits * m_dwTimeUnitSize[m_iTimeUnit];
   }
   else
   {
      m_dwTimeFrame = m_dwTimeTo - m_dwTimeFrom;
   }
}


//
// WM_CONTEXTMENU message handler
//

void CGraphFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(12);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Preset selection handlers
//

void CGraphFrame::OnGraphPresetsLast10minutes() 
{
   Preset(TIME_UNIT_MINUTE, 10);
}

void CGraphFrame::OnGraphPresetsLast30minutes() 
{
   Preset(TIME_UNIT_MINUTE, 30);
}

void CGraphFrame::OnGraphPresetsLasthour() 
{
   Preset(TIME_UNIT_HOUR, 1);
}

void CGraphFrame::OnGraphPresetsLast2hours() 
{
   Preset(TIME_UNIT_HOUR, 2);
}

void CGraphFrame::OnGraphPresetsLast4hours() 
{
   Preset(TIME_UNIT_HOUR, 4);
}

void CGraphFrame::OnGraphPresetsLastday() 
{
   Preset(TIME_UNIT_DAY, 1);
}

void CGraphFrame::OnGraphPresetsLastweek() 
{
   Preset(TIME_UNIT_DAY, 7);
}

void CGraphFrame::OnGraphPresetsLastmonth() 
{
   Preset(TIME_UNIT_DAY, 30);
}

void CGraphFrame::OnGraphPresetsLastyear() 
{
   Preset(TIME_UNIT_DAY, 365);
}


//
// Set time frame to given preset
//

void CGraphFrame::Preset(int nTimeUnit, DWORD dwNumUnits)
{
   m_iTimeFrameType = 1;   // Back from now
   m_iTimeUnit = nTimeUnit;
   m_dwNumTimeUnits = dwNumUnits;
   m_dwTimeFrame = m_dwNumTimeUnits * m_dwTimeUnitSize[m_iTimeUnit];
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// NXCM_UPDATE_GRAPH_POINT message handler
//

void CGraphFrame::OnUpdateGraphPoint(DWORD dwTimeStamp, double *pdValue)
{
   if (pdValue != NULL)
   {
      TCHAR szTimeStamp[64], szText[256];

      FormatTimeStamp(dwTimeStamp, szTimeStamp, TS_LONG_DATE_TIME);
      _sntprintf(szText, 256, _T("Time: %s  Value: %.3f"), szTimeStamp, *pdValue);
      m_wndStatusBar.SetText(szText, 3, 0);
   }
   else
   {
      m_wndStatusBar.SetText(_T(""), 3, 0);
   }
}


//
// WM_COMMAND::ID_GRAPH_RULER message handlers
//

void CGraphFrame::OnGraphRuler() 
{
   m_wndGraph.m_bShowRuler = !m_wndGraph.m_bShowRuler;
   m_wndGraph.Invalidate(FALSE);
}

void CGraphFrame::OnUpdateGraphRuler(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_wndGraph.m_bShowRuler);
}


//
// WM_GETMINMAXINFO message handler
//

void CGraphFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
   lpMMI->ptMinTrackSize.x = 300;
   lpMMI->ptMinTrackSize.y = 200;
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
}


//
// WM_COMMAND::ID_GRAPH_LEGEND message handlers
//

void CGraphFrame::OnGraphLegend() 
{
   m_wndGraph.m_bShowLegend = !m_wndGraph.m_bShowLegend;
   m_wndGraph.Update();
}

void CGraphFrame::OnUpdateGraphLegend(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_wndGraph.m_bShowLegend);
}


//
// WM_COMMAND::ID_GRAPH_ZOOMOUT message handlers
//

void CGraphFrame::OnGraphZoomout() 
{
   m_wndGraph.ZoomOut();
}

void CGraphFrame::OnUpdateGraphZoomout(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndGraph.CanZoomOut());
}


//
// NXCM_GRAPH_ZOOM_CHANGED message handler
//

void CGraphFrame::OnGraphZoomChange(WPARAM nZoomLevel, LPARAM lParam)
{
   TCHAR szBuffer[32];

   if (nZoomLevel > 0)
   {
      _stprintf(szBuffer, _T("%d"), nZoomLevel);
      m_wndStatusBar.SetText(szBuffer, 2, 0);
      m_wndStatusBar.SetIcon(2,
         (HICON)LoadImage(theApp.m_hInstance, MAKEINTRESOURCE(IDI_ZOOMIN),
                          IMAGE_ICON, 16, 16, LR_SHARED));
   }
   else
   {
      m_wndStatusBar.SetText(_T(""), 2, 0);
      m_wndStatusBar.SetIcon(2, NULL);
   }
}
