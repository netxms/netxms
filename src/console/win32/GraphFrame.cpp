// GraphFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphFrame.h"
#include "GraphPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
   m_dwRefreshInterval = 30;
   m_dwFlags = 0;
   m_hTimer = 0;
   m_dwSeconds = 0;
}

CGraphFrame::~CGraphFrame()
{
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
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_GET_SAVE_INFO, OnGetSaveInfo)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphFrame message handlers


//
// WM_CREATE message handler
//

int CGraphFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static int widths[3] = { 70, 170, -1 };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);

   m_wndStatusBar.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_STATUS_BAR);
   m_wndStatusBar.SetParts(3, widths);

   m_iStatusBarHeight = GetWindowSize(&m_wndStatusBar).cy;
   rect.bottom -= m_iStatusBarHeight;
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
   m_wndGraph.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_GRAPH);

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

void CGraphFrame::AddItem(DWORD dwNodeId, DWORD dwItemId)
{
   if (m_dwNumItems < MAX_GRAPH_ITEMS)
   {
      m_pdwNodeId[m_dwNumItems] = dwNodeId;
      m_pdwItemId[m_dwNumItems] = dwItemId;
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
   i = m_dwTimeTo - m_dwTimeFrom;
   m_dwTimeTo = (time(NULL) / 60) * 60;   // Round minute boundary
   m_dwTimeFrom = m_dwTimeTo - i;
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);

   for(i = 0; i < m_dwNumItems; i++)
   {
      dwResult = DoRequestArg7(NXCGetDCIData, g_hSession, (void *)m_pdwNodeId[i], 
                               (void *)m_pdwItemId[i], 0, (void *)m_dwTimeFrom,
                               (void *)m_dwTimeTo, &pData, "Loading item data...");
      if (dwResult == RCC_SUCCESS)
      {
         m_wndGraph.SetData(i, pData);
      }
      else
      {
         theApp.ErrorBox(dwResult, "Unable to retrieve colected data: %s");
      }
   }
   m_wndGraph.Invalidate(FALSE);
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
   CGraphPropDlg dlg;
   int i;

   dlg.m_bAutoscale = m_wndGraph.m_bAutoScale;
   dlg.m_bShowGrid = m_wndGraph.m_bShowGrid;
   dlg.m_bAutoUpdate = (m_dwFlags & GF_AUTOUPDATE) ? TRUE : FALSE;
   dlg.m_dwRefreshInterval = m_dwRefreshInterval;
   dlg.m_rgbAxisLines = m_wndGraph.m_rgbAxisColor;
   dlg.m_rgbBackground = m_wndGraph.m_rgbBkColor;
   dlg.m_rgbGridLines = m_wndGraph.m_rgbGridColor;
   dlg.m_rgbLabelBkgnd = m_wndGraph.m_rgbLabelBkColor;
   dlg.m_rgbLabelText = m_wndGraph.m_rgbLabelTextColor;
   dlg.m_rgbText = m_wndGraph.m_rgbTextColor;
   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
      dlg.m_rgbItems[i] = m_wndGraph.m_rgbLineColors[i];
   if (dlg.DoModal() == IDOK)
   {
      if (m_hTimer != 0)
         KillTimer(m_hTimer);
      m_dwRefreshInterval = dlg.m_dwRefreshInterval;
      m_wndStatusBar.m_dwMaxValue = m_dwRefreshInterval;
      if (dlg.m_bAutoUpdate)
      {
         m_dwFlags |= GF_AUTOUPDATE;
         m_hTimer = SetTimer(1, 1000, NULL);
         m_dwSeconds = 0;
      }
      else
      {
         m_dwFlags &= ~GF_AUTOUPDATE;
      }

      m_wndGraph.m_bAutoScale = dlg.m_bAutoscale;
      m_wndGraph.m_bShowGrid = dlg.m_bShowGrid;

      m_wndGraph.m_rgbAxisColor = dlg.m_rgbAxisLines;
      m_wndGraph.m_rgbBkColor = dlg.m_rgbBackground;
      m_wndGraph.m_rgbGridColor = dlg.m_rgbGridLines;
      m_wndGraph.m_rgbLabelBkColor = dlg.m_rgbLabelBkgnd;
      m_wndGraph.m_rgbLabelTextColor = dlg.m_rgbLabelText;
      m_wndGraph.m_rgbTextColor = dlg.m_rgbText;
      for(i = 0; i < MAX_GRAPH_ITEMS; i++)
         m_wndGraph.m_rgbLineColors[i] = dlg.m_rgbItems[i];

      m_wndGraph.InvalidateRect(NULL, FALSE);

      m_wndStatusBar.SetText(m_wndGraph.m_bAutoScale ? _T("Autoscale") : _T(""), 0, 0);
      m_wndStatusBar.SetText((m_dwFlags & GF_AUTOUPDATE) ? (LPCTSTR)m_dwSeconds : _T(""), 1,
                             (m_dwFlags & GF_AUTOUPDATE) ? SBT_OWNERDRAW : 0);
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
   TCHAR szBuffer[32];
   DWORD i;

   pInfo->iWndClass = WNDC_GRAPH;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_WND_PARAM_LEN,
              _T("F:%ld\x7FN:%ld\x7FTS:%ld\x7FTF:%ld\x7F" "A:%ld\x7FS:%d\x7F"
                 "CA:%lu\x7F" "CB:%lu\x7F" "CG:%lu\x7F" "CK:%lu\x7F" "CL:%lu\x7F" "CT:%lu"),
              m_dwFlags, m_dwNumItems, m_dwTimeFrom, m_dwTimeTo, m_dwRefreshInterval,
              m_wndGraph.m_bAutoScale, m_wndGraph.m_rgbAxisColor, 
              m_wndGraph.m_rgbBkColor, m_wndGraph.m_rgbGridColor,
              m_wndGraph.m_rgbLabelBkColor, m_wndGraph.m_rgbLabelTextColor,
              m_wndGraph.m_rgbTextColor);

   for(i = 0; i < MAX_GRAPH_ITEMS; i++)
   {
      _sntprintf(szBuffer, 32, _T("\x7F" "C%d:%lu"), i, m_wndGraph.m_rgbLineColors[i]);
      if (_tcslen(pInfo->szParameters) + _tcslen(szBuffer) < MAX_WND_PARAM_LEN)
         _tcscat(pInfo->szParameters, szBuffer);
   }
   
   for(i = 0; i < m_dwNumItems; i++)
   {
      _sntprintf(szBuffer, 32, _T("\x7FN%d:%d\x7FI%d:%d"), i, m_pdwNodeId[i], i, m_pdwItemId[i]);
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
   TCHAR szBuffer[32];
   DWORD i;

   m_dwFlags = ExtractWindowParamULong(pszParams, _T("F"), 0);
   m_dwNumItems = ExtractWindowParamULong(pszParams, _T("N"), 0);
   m_dwTimeFrom = ExtractWindowParamULong(pszParams, _T("TS"), 0);
   m_dwTimeTo = ExtractWindowParamULong(pszParams, _T("TF"), 0);
   m_dwRefreshInterval = ExtractWindowParamULong(pszParams, _T("A"), 30);
   m_wndGraph.m_bAutoScale = (BOOL)ExtractWindowParamLong(pszParams, _T("S"), TRUE);
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
      _sntprintf(szBuffer, 32, _T("N%d"), i);
      m_pdwNodeId[i] = ExtractWindowParamULong(pszParams, szBuffer, 0);
      _sntprintf(szBuffer, 32, _T("I%d"), i);
      m_pdwItemId[i] = ExtractWindowParamULong(pszParams, szBuffer, 0);
   }

   ExtractWindowParam(pszParams, _T("T"), m_szSubTitle, 256);
}
