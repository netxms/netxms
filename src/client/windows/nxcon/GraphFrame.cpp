// GraphFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphFrame.h"
#include "GraphSettingsPage.h"
#include "GraphStylePage.h"
#include "GraphDataPage.h"
#include "DefineGraphDlg.h"

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
   m_bFullRefresh = TRUE;
	m_nPendingUpdates = 0;

	LoadSettings(_T("Default"));
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
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_GRAPH_COPYTOCLIPBOARD, OnGraphCopytoclipboard)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LAST10MINUTES, OnUpdateGraphPresetsLast10minutes)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LAST2HOURS, OnUpdateGraphPresetsLast2hours)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LAST30MINUTES, OnUpdateGraphPresetsLast30minutes)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LAST4HOURS, OnUpdateGraphPresetsLast4hours)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LASTDAY, OnUpdateGraphPresetsLastday)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LASTHOUR, OnUpdateGraphPresetsLasthour)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LASTMONTH, OnUpdateGraphPresetsLastmonth)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LASTWEEK, OnUpdateGraphPresetsLastweek)
	ON_UPDATE_COMMAND_UI(ID_GRAPH_PRESETS_LASTYEAR, OnUpdateGraphPresetsLastyear)
	ON_COMMAND(ID_GRAPH_DEFINE, OnGraphDefine)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_UPDATE_GRAPH_POINT, OnUpdateGraphPoint)
   ON_MESSAGE(NXCM_GRAPH_ZOOM_CHANGED, OnGraphZoomChange)
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_PROCESSING_REQUEST, OnProcessingRequest)
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

   m_bFullRefresh = TRUE;
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   if (m_dwFlags & GF_AUTOUPDATE)
      m_hTimer = SetTimer(1, 1000, NULL);

   m_wndStatusBar.m_dwMaxValue = m_dwRefreshInterval;
   m_wndStatusBar.SetText(m_wndGraph.m_bAutoScale ? _T("Autoscale") : _T(""), 0, 0);
   m_wndStatusBar.SetText(m_dwFlags & GF_AUTOUPDATE ? _T("Autoupdate") : _T(""), 1, 0);
//   m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
//                          (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);

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
   DWORD i, dwTimeFrom, dwTimeTo;
   BOOL bPartial;

	if (m_nPendingUpdates > 0)
		return;

	m_nPendingUpdates = m_dwNumItems;

   // Set new time frame
   if (m_iTimeFrameType == 1)
   {
      m_dwTimeTo = (DWORD)time(NULL);
      m_dwTimeTo += 60 - m_dwTimeTo % 60;   // Round to minute boundary
      m_dwTimeFrom = m_dwTimeTo - m_dwTimeFrame;
   }
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);

   for(i = 0; i < m_dwNumItems; i++)
   {
      if (!m_bFullRefresh)
      {
         bPartial = m_wndGraph.GetTimeFrameForUpdate(i, &dwTimeFrom, &dwTimeTo);
         if (!bPartial)
         {
            dwTimeFrom = m_dwTimeFrom;
            dwTimeTo = m_dwTimeTo;
         }
      }
      else
      {
         bPartial = FALSE;
         dwTimeFrom = m_dwTimeFrom;
         dwTimeTo = m_dwTimeTo;
      }
      DoAsyncRequestArg7(m_hWnd, MAKEWPARAM(i, bPartial), NXCGetDCIData, g_hSession,
		                   (void *)m_ppItems[i]->m_dwNodeId, 
                         (void *)m_ppItems[i]->m_dwItemId, 0, (void *)dwTimeFrom,
                         (void *)dwTimeTo, &m_pDCIData[i]);
   }
	if (m_bFullRefresh)
	{
		for(;i < MAX_GRAPH_ITEMS; i++)
		{
         m_wndGraph.SetData(i, NULL);
		}
	}
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
	if (EditProperties(TRUE))
	{
      /* TODO: send refresh only if time frame or DCI list was changed */
      m_bFullRefresh = TRUE;
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	}
}


//
// WM_DESTROY message handler
//

void CGraphFrame::OnDestroy() 
{
	if (g_dwOptions & UI_OPT_SAVE_GRAPH_SETTINGS)
	{
		SaveCurrentSettings(_T("Default"));
	}

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
//   m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
//                          (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);
}


//
// Get graph configuration
//

void CGraphFrame::GetConfiguration(TCHAR *config)
{
   TCHAR szBuffer[512];
   DWORD i;

   _sntprintf(config, MAX_WND_PARAM_LEN,
              _T("F:%d\x7FN:%d\x7FTS:%d\x7FTF:%d\x7F") _T("A:%d\x7F")
              _T("TFT:%d\x7FTU:%d\x7FNTU:%d\x7FS:%d\x7F")
              _T("CA:%u\x7F") _T("CB:%u\x7F") _T("CG:%u\x7F") _T("CS:%u\x7F")
              _T("CT:%u\x7F") _T("CR:%u\x7FR:%d\x7FG:%d\x7FL:%d"),
              m_dwFlags, m_dwNumItems, m_dwTimeFrom, m_dwTimeTo, m_dwRefreshInterval,
              m_iTimeFrameType, m_iTimeUnit, m_dwNumTimeUnits,
              m_wndGraph.m_bAutoScale, m_wndGraph.m_rgbAxisColor, 
              m_wndGraph.m_rgbBkColor, m_wndGraph.m_rgbGridColor,
              m_wndGraph.m_rgbSelRectColor, m_wndGraph.m_rgbTextColor,
				  m_wndGraph.m_rgbRulerColor, m_wndGraph.m_bShowRuler,
              m_wndGraph.m_bShowGrid, m_wndGraph.m_bShowLegend);

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      _sntprintf(szBuffer, 512, _T("\x7F") _T("C%d:%lu"), i, m_wndGraph.m_graphItemStyles[i].rgbColor);
      if (_tcslen(config) + _tcslen(szBuffer) < MAX_WND_PARAM_LEN)
         _tcscat(config, szBuffer);
   }
   
   for(i = 0; i < m_dwNumItems; i++)
   {
      _sntprintf(szBuffer, 512, _T("\x7FN%d:%d\x7FI%d:%d\x7FIS%d:%d\x7FIT%d:%d\x7FIN%d:%s\x7FID%d:%s"),
                 i, m_ppItems[i]->m_dwNodeId, i, m_ppItems[i]->m_dwItemId,
                 i, m_ppItems[i]->m_iSource, i, m_ppItems[i]->m_iDataType,
                 i, m_ppItems[i]->m_pszParameter, i, m_ppItems[i]->m_pszDescription);
      if (_tcslen(config) + _tcslen(szBuffer) < MAX_WND_PARAM_LEN)
         _tcscat(config, szBuffer);
   }

   _sntprintf(&config[_tcslen(config)],
              MAX_WND_PARAM_LEN - _tcslen(config),
              _T("\x7FT:%s"), m_szSubTitle);

}


//
// Get save info for desktop saving
//

LRESULT CGraphFrame::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = (WINDOW_SAVE_INFO *)lParam;
   pInfo->iWndClass = WNDC_GRAPH;
   GetWindowPlacement(&pInfo->placement);
	GetConfiguration(pInfo->szParameters);
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
   m_wndGraph.m_rgbSelRectColor = ExtractWindowParamULong(pszParams, _T("CS"), m_wndGraph.m_rgbSelRectColor);
   m_wndGraph.m_rgbTextColor = ExtractWindowParamULong(pszParams, _T("CT"), m_wndGraph.m_rgbTextColor);
   m_wndGraph.m_rgbRulerColor = ExtractWindowParamULong(pszParams, _T("CR"), m_wndGraph.m_rgbRulerColor);

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      _sntprintf(szBuffer, 32, _T("C%d"), i);
      m_wndGraph.m_graphItemStyles[i].rgbColor = 
         ExtractWindowParamULong(pszParams, szBuffer, m_wndGraph.m_graphItemStyles[i].rgbColor);
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
   Preset(TIME_UNIT_DAY, 31);
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
   m_bFullRefresh = TRUE;
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
}


//
// NXCM_UPDATE_GRAPH_POINT message handler
//

LRESULT CGraphFrame::OnUpdateGraphPoint(WPARAM wParam, LPARAM lParam)
{
	double *pdValue = (double *)lParam;

   if (pdValue != NULL)
   {
      TCHAR szTimeStamp[64], szText[256];

      FormatTimeStamp(wParam, szTimeStamp, TS_LONG_DATE_TIME);
      _sntprintf(szText, 256, _T("Time: %s  Value: %.3f"), szTimeStamp, *pdValue);
      m_wndStatusBar.SetText(szText, 3, 0);
   }
   else
   {
      m_wndStatusBar.SetText(_T(""), 3, 0);
   }
	return 0;
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

LRESULT CGraphFrame::OnGraphZoomChange(WPARAM nZoomLevel, LPARAM lParam)
{
   TCHAR szBuffer[32];

   if (nZoomLevel > 0)
   {
      _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), nZoomLevel);
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
	return 0;
}


//
// WM_COMMAND::ID_FILE_PRINT message handler
//

void CGraphFrame::OnFilePrint() 
{
   CPrintDialog dlg(FALSE);
   CDC dc;
   HDC hdc;
   DOCINFO docinfo;
   RECT rect, rcGraph;
   TCHAR szBuffer[1024];
   int nLen, xmargin, ymargin;
   double scale;
   CBitmap bmp;

   dlg.m_pd.hDevMode = CopyGlobalMem(theApp.GetDevMode());
   dlg.m_pd.hDevNames = CopyGlobalMem(theApp.GetDevNames());
   if (dlg.DoModal() == IDOK)
   {
      hdc = dlg.GetPrinterDC();
      if (hdc == NULL)
         return;

      dc.Attach(hdc);
      memset(&docinfo, 0, sizeof(DOCINFO));
      docinfo.cbSize = sizeof(DOCINFO);
      docinfo.lpszDocName = _T("NetXMS Graph");

      if (dc.StartDoc(&docinfo) >= 0)
      {
         rect.left = 0;
         rect.top = 0;
         rect.right = dc.GetDeviceCaps(HORZRES);
         rect.bottom = dc.GetDeviceCaps(VERTRES);
         xmargin = rect.right * 10 / dc.GetDeviceCaps(HORZSIZE);  // 10mm margin
         ymargin = rect.bottom * 10 / dc.GetDeviceCaps(VERTSIZE);  // 10mm margin
         InflateRect(&rect, -xmargin * 2, -ymargin * 2);
         if (dc.StartPage() >= 0)
         {
            // Print header
            _sntprintf(szBuffer, 1024, _T("NetXMS History Graph - %s"), m_szSubTitle);
            nLen = _tcslen(szBuffer);
            dc.DrawText(szBuffer, nLen, &rect,
                        DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);
            rect.top += ymargin / 2 + dc.GetTextExtent(szBuffer, nLen).cy;

            // Draw graph on in-memory bitmap and print it
            memcpy(&rcGraph, &rect, sizeof(RECT));
            OffsetRect(&rcGraph, -rcGraph.left, -rcGraph.top);
            scale = max((double)rcGraph.right / 1024.0, (double)rcGraph.bottom / 1024.0);
            if (scale > 0)
            {
               rcGraph.right = (LONG)((double)rcGraph.right / scale);
               rcGraph.bottom = (LONG)((double)rcGraph.bottom / scale);
            }
            m_wndGraph.DrawGraphOnBitmap(bmp, rcGraph);
            PrintBitmap(dc, bmp, &rect);

            // Draw black frame around graph
            InflateRect(&rect, xmargin / 4, ymargin / 4);
            ::FrameRect(dc.m_hDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

            dc.EndPage();
         }
         else
         {
            MessageBox(_T("Print error: cannot start page"), _T("Error"), MB_OK | MB_ICONSTOP);
         }
         dc.EndDoc();
      }
      else
      {
         MessageBox(_T("Print error: cannot start document"), _T("Error"), MB_OK | MB_ICONSTOP);
      }
   }

   SafeGlobalFree(dlg.m_pd.hDevMode);
   SafeGlobalFree(dlg.m_pd.hDevNames);
}


//
// Handler for "Copy to clipboard" menu
//

void CGraphFrame::OnGraphCopytoclipboard() 
{
   if (OpenClipboard())
   {
      if (EmptyClipboard())
      {
         SetClipboardData(CF_BITMAP, m_wndGraph.GetBitmap()->GetSafeHandle());
      }
      else
      {
         MessageBox(_T("Cannot empty clipboard"), _T("Error"), MB_OK | MB_ICONSTOP);
      }
      CloseClipboard();
   }
   else
   {
      MessageBox(_T("Cannot open clipboard"), _T("Error"), MB_OK | MB_ICONSTOP);
   }
}


//
// Update handlers for "Presets->..." menu items
//

void CGraphFrame::OnUpdateGraphPresetsLast10minutes(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 600));
}

void CGraphFrame::OnUpdateGraphPresetsLast2hours(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 7200));
}

void CGraphFrame::OnUpdateGraphPresetsLast30minutes(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 1800));
}

void CGraphFrame::OnUpdateGraphPresetsLast4hours(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 14400));
}

void CGraphFrame::OnUpdateGraphPresetsLastday(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 86400));
}

void CGraphFrame::OnUpdateGraphPresetsLasthour(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 3600));
}

void CGraphFrame::OnUpdateGraphPresetsLastmonth(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 2678400));
}

void CGraphFrame::OnUpdateGraphPresetsLastweek(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 604800));
}

void CGraphFrame::OnUpdateGraphPresetsLastyear(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck((m_iTimeFrameType == 1) && (m_dwTimeFrame == 31536000));
}


//
// Save graph as predefined
//

void CGraphFrame::OnGraphDefine() 
{
	DWORD dwResult;
	TCHAR szBuffer[256], *ptr;
	CDefineGraphDlg dlg;
	WINDOW_SAVE_INFO *pInfo;
	NXC_GRAPH graph;

	GetWindowText(szBuffer, 256);
	ptr = _tcschr(szBuffer, _T('['));
	if (ptr != NULL)
	{
		ptr++;
		memmove(szBuffer, ptr, (_tcslen(ptr) + 1) * sizeof(TCHAR));
		ptr = _tcsrchr(szBuffer, _T(']'));
		if (ptr != NULL)
			*ptr = 0;
	}
	dlg.m_strName = szBuffer;
	if (dlg.DoModal() == IDOK)
	{
		pInfo = (WINDOW_SAVE_INFO *)malloc(sizeof(WINDOW_SAVE_INFO));
		if (SendMessage(NXCM_GET_SAVE_INFO, 0, (LPARAM)pInfo) != 0)
		{
			graph.dwId = 0;	// New graph
			graph.pszName = (TCHAR *)((LPCTSTR)dlg.m_strName);
			graph.pszConfig = pInfo->szParameters;
			graph.dwAclSize = dlg.m_dwACLSize;
			graph.pACL = dlg.m_pACL;
			dwResult = DoRequestArg2(NXCDefineGraph, g_hSession, &graph, _T("Saving graph configuration on server..."));
			if (dwResult != RCC_SUCCESS)
			{
				theApp.ErrorBox(dwResult, _T("Cannot save graph configuration: %s"));
			}
		}
		else
		{
			MessageBox(_T("Cannot save graph - internal console error"), _T("Error"), MB_OK | MB_ICONSTOP);
		}
	}
}


//
// NXCM_REQUEST_COMPLETED message handler
//

LRESULT CGraphFrame::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
	int nIndex;

   if (lParam == RCC_SUCCESS)
   {
		nIndex = LOWORD(wParam);
      if (HIWORD(wParam))
      {
         m_wndGraph.UpdateData(nIndex, m_pDCIData[nIndex]);
      }
      else
      {
         m_wndGraph.SetData(nIndex, m_pDCIData[nIndex]);
      }
   }
   else
   {
      theApp.ErrorBox(lParam, _T("Unable to retrieve colected data: %s"));
   }

	m_nPendingUpdates--;
	if (m_nPendingUpdates == 0)
	{
		m_wndGraph.m_bUpdating = FALSE;
		m_wndGraph.ClearZoomHistory();
		m_wndGraph.Update();
		m_bFullRefresh = FALSE;
	}

	return 0;
}


//
// NXCM_PROCESSING_REQUEST message handler
//

LRESULT CGraphFrame::OnProcessingRequest(WPARAM wParam, LPARAM lParam)
{
	m_wndGraph.m_bUpdating = TRUE;
	m_wndGraph.Invalidate(FALSE);
	return 0;
}


//
// Save current graph settings
//

void CGraphFrame::SaveCurrentSettings(const TCHAR *pszName)
{
	TCHAR section[256], option[64];
	int i;

	_sntprintf(section, 256, _T("GraphCfg_%s"), pszName);
	theApp.WriteProfileInt(section, _T("Flags"), m_dwFlags);
	theApp.WriteProfileInt(section, _T("TimeTo"), m_dwTimeTo);
	theApp.WriteProfileInt(section, _T("TimeFrom"), m_dwTimeFrom);
	theApp.WriteProfileInt(section, _T("TimeFrameType"), m_iTimeFrameType);
	theApp.WriteProfileInt(section, _T("TimeUnit"), m_iTimeUnit);
	theApp.WriteProfileInt(section, _T("NumTimeUnits"), m_dwNumTimeUnits);
	theApp.WriteProfileInt(section, _T("AutoScale"), m_wndGraph.m_bAutoScale);
	theApp.WriteProfileInt(section, _T("EnableZoom"), m_wndGraph.m_bEnableZoom);
	theApp.WriteProfileInt(section, _T("ShowGrid"), m_wndGraph.m_bShowGrid);
	theApp.WriteProfileInt(section, _T("ShowHostNames"), m_wndGraph.m_bShowHostNames);
	theApp.WriteProfileInt(section, _T("ShowLegend"), m_wndGraph.m_bShowLegend);
	theApp.WriteProfileInt(section, _T("ShowRuler"), m_wndGraph.m_bShowRuler);
	theApp.WriteProfileInt(section, _T("ShowTitle"), m_wndGraph.m_bShowTitle);
	theApp.WriteProfileInt(section, _T("AxisColor"), m_wndGraph.m_rgbAxisColor);
	theApp.WriteProfileInt(section, _T("BkColor"), m_wndGraph.m_rgbBkColor);
	theApp.WriteProfileInt(section, _T("GridColor"), m_wndGraph.m_rgbGridColor);
	theApp.WriteProfileInt(section, _T("RulerColor"), m_wndGraph.m_rgbRulerColor);
	theApp.WriteProfileInt(section, _T("SelRectColor"), m_wndGraph.m_rgbSelRectColor);
	theApp.WriteProfileInt(section, _T("TextColor"), m_wndGraph.m_rgbTextColor);

	for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemColor_%d"), i);
		theApp.WriteProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].rgbColor);

		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemType_%d"), i);
		theApp.WriteProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].nType);

		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemLineWidth_%d"), i);
		theApp.WriteProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].nLineWidth);
	}
}


//
// Load graph settings
//

void CGraphFrame::LoadSettings(const TCHAR *pszName)
{
	TCHAR section[256], option[64];
	int i;

	_sntprintf(section, 256, _T("GraphCfg_%s"), pszName);
	m_dwFlags = theApp.GetProfileInt(section, _T("Flags"), m_dwFlags);
	m_dwTimeTo = theApp.GetProfileInt(section, _T("TimeTo"), m_dwTimeTo);
	m_dwTimeFrom = theApp.GetProfileInt(section, _T("TimeFrom"), m_dwTimeFrom);
	m_iTimeFrameType = theApp.GetProfileInt(section, _T("TimeFrameType"), m_iTimeFrameType);
	m_iTimeUnit = theApp.GetProfileInt(section, _T("TimeUnit"), m_iTimeUnit);
	m_dwNumTimeUnits = theApp.GetProfileInt(section, _T("NumTimeUnits"), m_dwNumTimeUnits);
	m_wndGraph.m_bAutoScale = theApp.GetProfileInt(section, _T("AutoScale"), m_wndGraph.m_bAutoScale);
	m_wndGraph.m_bEnableZoom = theApp.GetProfileInt(section, _T("EnableZoom"), m_wndGraph.m_bEnableZoom);
	m_wndGraph.m_bShowGrid = theApp.GetProfileInt(section, _T("ShowGrid"), m_wndGraph.m_bShowGrid);
	m_wndGraph.m_bShowHostNames = theApp.GetProfileInt(section, _T("ShowHostNames"), m_wndGraph.m_bShowHostNames);
	m_wndGraph.m_bShowLegend = theApp.GetProfileInt(section, _T("ShowLegend"), m_wndGraph.m_bShowLegend);
	m_wndGraph.m_bShowRuler = theApp.GetProfileInt(section, _T("ShowRuler"), m_wndGraph.m_bShowRuler);
	m_wndGraph.m_bShowTitle = theApp.GetProfileInt(section, _T("ShowTitle"), m_wndGraph.m_bShowTitle);
	m_wndGraph.m_rgbAxisColor = theApp.GetProfileInt(section, _T("AxisColor"), m_wndGraph.m_rgbAxisColor);
	m_wndGraph.m_rgbBkColor = theApp.GetProfileInt(section, _T("BkColor"), m_wndGraph.m_rgbBkColor);
	m_wndGraph.m_rgbGridColor = theApp.GetProfileInt(section, _T("GridColor"), m_wndGraph.m_rgbGridColor);
	m_wndGraph.m_rgbRulerColor = theApp.GetProfileInt(section, _T("RulerColor"), m_wndGraph.m_rgbRulerColor);
	m_wndGraph.m_rgbSelRectColor = theApp.GetProfileInt(section, _T("SelRectColor"), m_wndGraph.m_rgbSelRectColor);
	m_wndGraph.m_rgbTextColor = theApp.GetProfileInt(section, _T("TextColor"), m_wndGraph.m_rgbTextColor);

	for(i = 0; i < MAX_GRAPH_ITEMS; i++)
	{
		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemColor_%d"), i);
		m_wndGraph.m_graphItemStyles[i].rgbColor = theApp.GetProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].rgbColor);

		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemType_%d"), i);
		m_wndGraph.m_graphItemStyles[i].nType = theApp.GetProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].nType);

		_sntprintf_s(option, 64, _TRUNCATE, _T("ItemLineWidth_%d"), i);
		m_wndGraph.m_graphItemStyles[i].nLineWidth = theApp.GetProfileInt(section, option, m_wndGraph.m_graphItemStyles[i].nLineWidth);
	}

   if (m_iTimeFrameType == 0)
   {
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
}


//
// Edit graph properties. Return TRUE if properties was changed.
//

BOOL CGraphFrame::EditProperties(BOOL frameIsValid)
{
   CPropertySheet dlg(_T("Graph Properties"), theApp.GetMainWnd(), 0);
   CGraphSettingsPage pgSettings;
	CGraphStylePage pgStyle;
   CGraphDataPage pgData;
	BOOL ok = FALSE;

   // Create "Settings" page
	pgSettings.m_strTitle = m_szSubTitle;
   pgSettings.m_bAutoscale = m_wndGraph.m_bAutoScale;
   pgSettings.m_bShowGrid = m_wndGraph.m_bShowGrid;
   pgSettings.m_bRuler = m_wndGraph.m_bShowRuler;
   pgSettings.m_bShowLegend = m_wndGraph.m_bShowLegend;
   pgSettings.m_bEnableZoom = m_wndGraph.m_bEnableZoom;
   pgSettings.m_bShowHostNames = m_wndGraph.m_bShowHostNames;
   pgSettings.m_bAutoUpdate = (m_dwFlags & GF_AUTOUPDATE) ? TRUE : FALSE;
   pgSettings.m_dwRefreshInterval = m_dwRefreshInterval;
   pgSettings.m_rgbAxisLines = m_wndGraph.m_rgbAxisColor;
   pgSettings.m_rgbBackground = m_wndGraph.m_rgbBkColor;
   pgSettings.m_rgbGridLines = m_wndGraph.m_rgbGridColor;
   pgSettings.m_rgbSelection = m_wndGraph.m_rgbSelRectColor;
	pgSettings.m_rgbRuler = m_wndGraph.m_rgbRulerColor;
   pgSettings.m_rgbText = m_wndGraph.m_rgbTextColor;
   pgSettings.m_iTimeFrame = m_iTimeFrameType;
   pgSettings.m_iTimeUnit = m_iTimeUnit;
   pgSettings.m_dwNumUnits = m_dwNumTimeUnits;
   pgSettings.m_dateFrom = (time_t)m_dwTimeFrom;
   pgSettings.m_timeFrom = (time_t)m_dwTimeFrom;
   pgSettings.m_dateTo = (time_t)m_dwTimeTo;
   pgSettings.m_timeTo = (time_t)m_dwTimeTo;
   dlg.AddPage(&pgSettings);

	// Create "Style" page
	memcpy(pgStyle.m_styles, m_wndGraph.m_graphItemStyles, sizeof(GRAPH_ITEM_STYLE) * MAX_GRAPH_ITEMS);
	dlg.AddPage(&pgStyle);

   // Create "Data Sources" page
   pgData.m_dwNumItems = m_dwNumItems;
   memcpy(pgData.m_ppItems, m_ppItems, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);
   dlg.AddPage(&pgData);

   // Open property sheet
   dlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;
   if (dlg.DoModal() == IDOK)
   {
		nx_strncpy(m_szSubTitle, pgSettings.m_strTitle, MAX_DB_STRING);

		if (frameIsValid)
		{
			CString strFullString, strTitle;

			if (strFullString.LoadString(IDR_DCI_HISTORY_GRAPH))
				AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

			strTitle += " - [";
			strTitle += m_szSubTitle;
			strTitle += "]";
			SetTitle(strTitle);
			OnUpdateFrameTitle(TRUE);
		}

      m_dwNumItems = pgData.m_dwNumItems;
      memcpy(m_ppItems, pgData.m_ppItems, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);

      if (m_hTimer != 0)
         KillTimer(m_hTimer);
      m_dwRefreshInterval = pgSettings.m_dwRefreshInterval;
      m_wndStatusBar.m_dwMaxValue = m_dwRefreshInterval;
      if (pgSettings.m_bAutoUpdate)
      {
         m_dwFlags |= GF_AUTOUPDATE;
			if (frameIsValid)
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
      m_wndGraph.m_bShowHostNames = pgSettings.m_bShowHostNames;

      m_wndGraph.m_rgbAxisColor = pgSettings.m_rgbAxisLines;
      m_wndGraph.m_rgbBkColor = pgSettings.m_rgbBackground;
      m_wndGraph.m_rgbGridColor = pgSettings.m_rgbGridLines;
      m_wndGraph.m_rgbSelRectColor = pgSettings.m_rgbSelection;
      m_wndGraph.m_rgbTextColor = pgSettings.m_rgbText;
		m_wndGraph.m_rgbRulerColor = pgSettings.m_rgbRuler;

		memcpy(m_wndGraph.m_graphItemStyles, pgStyle.m_styles, sizeof(GRAPH_ITEM_STYLE) * MAX_GRAPH_ITEMS);

		if (frameIsValid)
		{
			m_wndStatusBar.SetText(m_wndGraph.m_bAutoScale ? _T("Autoscale") : _T(""), 0, 0);
			m_wndStatusBar.SetText(m_dwFlags & GF_AUTOUPDATE ? _T("Autoupdate") : _T(""), 1, 0);
		}

      m_iTimeFrameType = pgSettings.m_iTimeFrame;
      m_iTimeUnit = pgSettings.m_iTimeUnit;
      m_dwNumTimeUnits = pgSettings.m_dwNumUnits;
      if (m_iTimeFrameType == 0)
      {
         m_dwTimeFrom = (DWORD)CTime(pgSettings.m_dateFrom.GetYear(), pgSettings.m_dateFrom.GetMonth(),
                              pgSettings.m_dateFrom.GetDay(), pgSettings.m_timeFrom.GetHour(),
                              pgSettings.m_timeFrom.GetMinute(),
                              pgSettings.m_timeFrom.GetSecond(), -1).GetTime();
         m_dwTimeTo = (DWORD)CTime(pgSettings.m_dateTo.GetYear(), pgSettings.m_dateTo.GetMonth(),
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

		m_dwNumItems = pgData.m_dwNumItems;
		memcpy(m_ppItems, pgData.m_ppItems, sizeof(DCIInfo *) * MAX_GRAPH_ITEMS);
		ok = TRUE;
   }
	return ok;
}
