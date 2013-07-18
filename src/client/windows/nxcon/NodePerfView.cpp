// NodePerfView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePerfView.h"
#include <nxconfig.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define STATE_IDLE		0
#define STATE_LOADING	1
#define STATE_NO_DATA	2
#define STATE_UPDATING	3

#define TASK_SHUTDOWN			0
#define TASK_GET_AVAIL_DCI		1
#define TASK_UPDATE_GRAPH		2
#define TASK_FINISH_UPDATE		3

#define GRAPH_X_MARGIN		30
#define GRAPH_Y_MARGIN		30
#define GRAPH_HEIGHT			230
#define SPLIT_LIMIT			800


//
// Worker task
//

class WorkerTask
{
public:
	int m_nTask;
	DWORD m_dwId;
	DWORD m_dwId2;
	DWORD m_dwTimeFrom;
	DWORD m_dwTimeTo;
	HWND m_hWnd;

	WorkerTask(HWND hWnd, int nTask) { m_hWnd = hWnd; m_nTask = nTask; }
	WorkerTask(HWND hWnd, int nTask, DWORD dwId) { m_hWnd = hWnd; m_nTask = nTask; m_dwId = dwId; }
	WorkerTask(HWND hWnd, int nTask, DWORD dwId, DWORD dwId2,
	           DWORD dwTimeFrom, DWORD dwTimeTo)
	{
		m_hWnd = hWnd;
		m_nTask = nTask;
		m_dwId = dwId;
		m_dwId2 = dwId2;
		m_dwTimeFrom = dwTimeFrom;
		m_dwTimeTo = dwTimeTo;
	}
};


//
// Graph data
//

class GraphData
{
public:
	NXC_DCI_DATA *m_data;
	DWORD m_numThresholds;
	NXC_DCI_THRESHOLD *m_thresholds;

	GraphData(NXC_DCI_DATA *data, DWORD numThr, NXC_DCI_THRESHOLD *thr)
	{
		m_data = data;
		m_numThresholds = numThr;
		m_thresholds = thr;
	}
};


//
// Worker thread
//

static THREAD_RESULT THREAD_CALL PerfViewWorkerThread(void *pArg)
{
	((CNodePerfView *)pArg)->WorkerThread();
	return THREAD_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CNodePerfView

CNodePerfView::CNodePerfView()
{
	m_dwNumGraphs = 0;
	m_pGraphList = NULL;
	m_hWorkerThread = INVALID_THREAD_HANDLE;
	m_nTimer = 0;
	m_nOrigin = 0;
}

CNodePerfView::~CNodePerfView()
{
	WorkerTask *pTask;

	while((pTask = (WorkerTask *)m_workerQueue.Get()) != NULL)
		delete pTask;
	m_workerQueue.Put(new WorkerTask(NULL, TASK_SHUTDOWN));
	ThreadJoin(m_hWorkerThread);
	safe_free(m_pGraphList);
}


BEGIN_MESSAGE_MAP(CNodePerfView, CWnd)
	//{{AFX_MSG_MAP(CNodePerfView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEACTIVATE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
   ON_MESSAGE(NXCM_REQUEST_COMPLETED, OnRequestCompleted)
	ON_MESSAGE(NXCM_GRAPH_DATA, OnGraphData)
	ON_MESSAGE(NXCM_UPDATE_FINISHED, OnUpdateFinished)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNodePerfView message handlers

BOOL CNodePerfView::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         LoadCursor(NULL, IDC_ARROW),
                                         (HBRUSH)(COLOR_WINDOW + 1), NULL);
	return CWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CNodePerfView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	CDC *pDC;
	CFont *pOldFont;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	pDC = GetDC();
   m_fontTitle.CreateFont(-MulDiv(8, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72),
                          0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                          VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
	pOldFont = pDC->SelectObject(&m_fontTitle);
	m_nTitleHeight = pDC->GetTextExtent(_T("MXh|qg"), 6).cy;
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);
	m_hWorkerThread = ThreadCreateEx(PerfViewWorkerThread, 0, this);

	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

LRESULT CNodePerfView::OnSetObject(WPARAM wParam, LPARAM lParam)
{
	DWORD i;

	if (m_nTimer != 0)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}

	for(i = 0; i < m_dwNumGraphs; i++)
	{
		m_pGraphList[i].pWnd->DestroyWindow();
		delete m_pGraphList[i].pWnd;
	}
	m_dwNumGraphs = 0;
   m_pObject = (NXC_OBJECT *)lParam;
	m_nState = STATE_LOADING;
	InvalidateRect(NULL);
	AdjustView();
	m_workerQueue.Put(new WorkerTask(m_hWnd, TASK_GET_AVAIL_DCI));
	return 0;
}


//
// WM_PAINT message handler
//

void CNodePerfView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.SetBkColor(GetSysColor(COLOR_WINDOW));
	dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	switch(m_nState)
	{
		case STATE_LOADING:
			dc.TextOut(10, 10, _T("Loading performance data from server..."), 39);
			break;
		case STATE_NO_DATA:
			dc.TextOut(10, 10, _T("No performance data available"), 29);
			break;
		default:
			break;
	}
}


//
// Find DCI by name
//

DWORD CNodePerfView::FindItemByName(NXC_PERFTAB_DCI *pItemList, DWORD dwNumItems, TCHAR *pszName)
{
	DWORD i;

	for(i = 0; i < dwNumItems; i++)
		if ((!_tcsicmp(pItemList[i].szName, pszName)) &&
			 (pItemList[i].nStatus == ITEM_STATUS_ACTIVE))
			return pItemList[i].dwId;
	return 0;
}


//
// Create graph if appropriate parameter exists
//

BOOL CNodePerfView::CreateGraph(NXC_PERFTAB_DCI *pItemList, DWORD dwNumItems,
										  TCHAR *pszParam, TCHAR *pszTitle, RECT &rect,
										  BOOL bArea)
{
	DWORD i;

	for(i = 0; i < dwNumItems; i++)
	{
		if ((!_tcsicmp(pItemList[i].szName, pszParam)) &&
			 (pItemList[i].nStatus == ITEM_STATUS_ACTIVE))
		{
			memset(m_pGraphList[m_dwNumGraphs].dwItemId, 0, sizeof(DWORD) * MAX_GRAPH_ITEMS);
			m_pGraphList[m_dwNumGraphs].dwItemId[0] = pItemList[i].dwId;
			m_pGraphList[m_dwNumGraphs].pWnd = new CGraph;
			m_pGraphList[m_dwNumGraphs].pWnd->m_bSet3DEdge = FALSE;
			m_pGraphList[m_dwNumGraphs].pWnd->m_bShowLegend = FALSE;
			m_pGraphList[m_dwNumGraphs].pWnd->m_bShowTitle = TRUE;
			m_pGraphList[m_dwNumGraphs].pWnd->SetTitle(pszTitle);
			m_pGraphList[m_dwNumGraphs].pWnd->SetColorScheme(GCS_LIGHT);
			m_pGraphList[m_dwNumGraphs].pWnd->m_rgbBkColor = GetSysColor(COLOR_WINDOW);
			m_pGraphList[m_dwNumGraphs].pWnd->m_rgbTextColor = GetSysColor(COLOR_WINDOWTEXT);
			m_pGraphList[m_dwNumGraphs].pWnd->m_graphItemStyles[0].nType = bArea ? GRAPH_TYPE_AREA : GRAPH_TYPE_LINE;
			m_pGraphList[m_dwNumGraphs].pWnd->Create(WS_CHILD | WS_VISIBLE, rect, this, m_dwNumGraphs);
			m_dwNumGraphs++;
			rect.top += GRAPH_HEIGHT + GRAPH_Y_MARGIN;
			rect.bottom = rect.top + GRAPH_HEIGHT;
			return TRUE;
		}
	}
	return FALSE;
}


//
// Create custom graph
//

void CNodePerfView::CreateCustomGraph(NXC_PERFTAB_DCI *dci, RECT &rect)
{
	if (dci->pszSettings[0] == 0)
		return;

	Config config;
	char *xml;

#ifdef UNICODE
	xml = UTF8StringFromWideString(dci->pszSettings);
#else
	xml = strdup(dci->pszSettings);
#endif
	if (!config.loadXmlConfigFromMemory(xml, (int)strlen(xml)))
	{
		free(xml);
		return;
	}
	free(xml);

	if (!config.getValueBoolean(_T("/enabled"), false))
		return;


	memset(m_pGraphList[m_dwNumGraphs].dwItemId, 0, sizeof(DWORD) * MAX_GRAPH_ITEMS);
	m_pGraphList[m_dwNumGraphs].dwItemId[0] = dci->dwId;
	m_pGraphList[m_dwNumGraphs].pWnd = new CGraph;
	m_pGraphList[m_dwNumGraphs].pWnd->m_bSet3DEdge = FALSE;
	m_pGraphList[m_dwNumGraphs].pWnd->m_bShowLegend = FALSE;
	m_pGraphList[m_dwNumGraphs].pWnd->m_bShowTitle = TRUE;
	const TCHAR *title = config.getValue(_T("/title"), dci->szName);
	m_pGraphList[m_dwNumGraphs].pWnd->SetTitle((*title != 0) ? title : dci->szName);
	m_pGraphList[m_dwNumGraphs].pWnd->SetColorScheme(GCS_LIGHT);
	m_pGraphList[m_dwNumGraphs].pWnd->m_rgbBkColor = GetSysColor(COLOR_WINDOW);
	m_pGraphList[m_dwNumGraphs].pWnd->m_rgbTextColor = GetSysColor(COLOR_WINDOWTEXT);
	m_pGraphList[m_dwNumGraphs].pWnd->m_graphItemStyles[0].nType = config.getValueInt(_T("/type"), GRAPH_TYPE_LINE);
	m_pGraphList[m_dwNumGraphs].pWnd->m_graphItemStyles[0].rgbColor = config.getValueUInt(_T("/color"), 0x00C000);
	m_pGraphList[m_dwNumGraphs].pWnd->m_graphItemStyles[0].bShowThresholds = config.getValueBoolean(_T("/showThresholds"), false);
	m_pGraphList[m_dwNumGraphs].pWnd->Create(WS_CHILD | WS_VISIBLE, rect, this, m_dwNumGraphs);
	m_dwNumGraphs++;
	rect.top += GRAPH_HEIGHT + GRAPH_Y_MARGIN;
	rect.bottom = rect.top + GRAPH_HEIGHT;
}


//
// Handler for NXCM_REQUEST_COMPLETED message
//

LRESULT CNodePerfView::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	int i;

	if (m_nState != STATE_LOADING)
		return 0;

	if (wParam != 0xFFFFFFFF)
	{
		GetClientRect(&rect);
		rect.left += GRAPH_X_MARGIN;
		rect.right -= GRAPH_X_MARGIN;
		rect.top += GRAPH_Y_MARGIN;
		rect.bottom = rect.top + GRAPH_HEIGHT;

		m_pGraphList = (PERF_GRAPH *)realloc(m_pGraphList, wParam * sizeof(PERF_GRAPH));
		m_dwNumGraphs = 0;

		// CPU utilization
		static TCHAR *cpuUsageDciNames[] = 
		{ 
			_T("@system.cpu_usage"),
			_T("@system.cpu_usage.cisco.0"),
			_T("@system.cpu_usage.cisco.1"),
			_T("@system.cpu_usage.passport"),
			_T("@system.cpu_usage.netscreen"),
			_T("@system.cpu_usage.ipso"),
			NULL
		};
		for(i = 0; cpuUsageDciNames[i] != NULL; i++)
			if (CreateGraph((NXC_PERFTAB_DCI *)lParam, (DWORD)wParam, cpuUsageDciNames[i], _T("CPU Utilization"), rect, FALSE))
				break;

		// CPU load average
		if (CreateGraph((NXC_PERFTAB_DCI *)lParam, (DWORD)wParam, _T("@system.load_avg"), _T("CPU Load Average"), rect, FALSE))
		{
/*			m_pGraphList[m_dwNumGraphs - 1].dwItemId[1] = FindItemByName((NXC_SYSTEM_DCI *)lParam, wParam, _T("@system.load_avg5"));
			m_pGraphList[m_dwNumGraphs - 1].dwItemId[2] = FindItemByName((NXC_SYSTEM_DCI *)lParam, wParam, _T("@system.load_avg15"));
			m_pGraphList[m_dwNumGraphs - 1].pWnd->m_graphItemStyles[1].rgbColor = RGB(255, 104, 32);
			m_pGraphList[m_dwNumGraphs - 1].pWnd->m_graphItemStyles[2].rgbColor = RGB(192, 0, 0);*/
		}

		// Physical memory
		if (CreateGraph((NXC_PERFTAB_DCI *)lParam, (DWORD)wParam, _T("@system.usedmem"), _T("Physical Memory"), rect, TRUE))
		{
			m_pGraphList[m_dwNumGraphs - 1].dwItemId[1] = FindItemByName((NXC_PERFTAB_DCI *)lParam, (DWORD)wParam, _T("@system.totalmem"));
			m_pGraphList[m_dwNumGraphs - 1].pWnd->m_graphItemStyles[0].rgbColor = RGB(210, 180, 140);
			m_pGraphList[m_dwNumGraphs - 1].pWnd->m_graphItemStyles[1].rgbColor = RGB(192, 0, 0);
		}

		// Disk queue
		if (CreateGraph((NXC_PERFTAB_DCI *)lParam, (DWORD)wParam, _T("@system.disk_queue"), _T("Disk Queue"), rect, FALSE))
		{
			m_pGraphList[m_dwNumGraphs - 1].pWnd->m_graphItemStyles[0].rgbColor = RGB(0, 0, 192);
		}

		// Custom graphs
		for(i = 0; i < (int)wParam; i++)
		{
			if (((NXC_PERFTAB_DCI *)lParam)[i].pszSettings != NULL)
			{
				CreateCustomGraph(&(((NXC_PERFTAB_DCI *)lParam)[i]), rect);
				free(((NXC_PERFTAB_DCI *)lParam)[i].pszSettings);
			}
		}
		safe_free(CAST_TO_POINTER(lParam, void *));

		if (m_dwNumGraphs > 0)
		{
			AdjustView();
			UpdateAllGraphs();
			m_nTimer = SetTimer(1, 30000, NULL);
		}
		else
		{
			m_nState = STATE_NO_DATA;
		}
	}
	else
	{
		m_nState = STATE_NO_DATA;
	}
	InvalidateRect(NULL);
	return 0;
}


//
// Worker thread
//

void CNodePerfView::WorkerThread()
{
	WorkerTask *pTask;
	UINT32 dwResult, dwNumItems, numThresholds;
	GraphData *data;
	NXC_PERFTAB_DCI *pItemList;
	NXC_DCI_DATA *dciData;
	NXC_DCI_THRESHOLD *thresholds;

	while(1)
	{
		pTask = (WorkerTask *)m_workerQueue.GetOrBlock();
		if (pTask->m_nTask == TASK_SHUTDOWN)
			break;

		switch(pTask->m_nTask)
		{
			case TASK_GET_AVAIL_DCI:
				dwResult = NXCGetPerfTabDCIList(g_hSession, m_pObject->dwId, &dwNumItems, &pItemList);
				::PostMessage(pTask->m_hWnd, NXCM_REQUEST_COMPLETED,
				              (dwResult == RCC_SUCCESS) ? dwNumItems : 0xFFFFFFFF,
				              CAST_FROM_POINTER(pItemList, LPARAM));
				break;
			case TASK_FINISH_UPDATE:
				::PostMessage(pTask->m_hWnd, NXCM_UPDATE_FINISHED, 0, pTask->m_dwId);
				break;
			case TASK_UPDATE_GRAPH:
				dwResult = NXCGetDCIDataEx(g_hSession, pTask->m_dwId2, pTask->m_dwId, 0,
				                           pTask->m_dwTimeFrom, pTask->m_dwTimeTo, &dciData,
													&thresholds, &numThresholds);
				if (dwResult == RCC_SUCCESS)
				{
					data = new GraphData(dciData, numThresholds, thresholds);
					::PostMessage(pTask->m_hWnd, NXCM_GRAPH_DATA, pTask->m_dwId, CAST_FROM_POINTER(data, LPARAM));
				}
				break;
			default:
				break;
		}

		delete pTask;
	}
	delete pTask;
}


//
// WM_DESTROY message handler
//

void CNodePerfView::OnDestroy() 
{
	if (m_nTimer != 0)
		KillTimer(m_nTimer);
	CWnd::OnDestroy();
}


//
// WM_TIMER message handler
//

void CNodePerfView::OnTimer(UINT_PTR nIDEvent) 
{
	if (m_nState == STATE_IDLE)
		UpdateAllGraphs();
}


//
// NXCM_UPDATE_FINISHED message handler
//

LRESULT CNodePerfView::OnUpdateFinished(WPARAM wParam, LPARAM lParam)
{
	if ((m_nState != STATE_UPDATING) || (m_pObject == NULL))
		return 0;

	if (m_pObject->dwId == (DWORD)lParam)
		m_nState = STATE_IDLE;
	return 0;
}


//
// Initiate update of all graphs
//

void CNodePerfView::UpdateAllGraphs()
{
	DWORD i, j;

	m_nState = STATE_UPDATING;
   m_dwTimeTo = (DWORD)time(NULL);
   m_dwTimeTo += 60 - m_dwTimeTo % 60;   // Round to minute boundary
   m_dwTimeFrom = m_dwTimeTo - 3600;
	for(i = 0; i < m_dwNumGraphs; i++)
		for(j = 0; j < MAX_GRAPH_ITEMS; j++)
			if (m_pGraphList[i].dwItemId[j] != 0)
				m_workerQueue.Put(new WorkerTask(m_hWnd, TASK_UPDATE_GRAPH, m_pGraphList[i].dwItemId[j],
															m_pObject->dwId, m_dwTimeFrom, m_dwTimeTo));
	m_workerQueue.Put(new WorkerTask(m_hWnd, TASK_FINISH_UPDATE, m_pObject->dwId));
}


//
// NXCM_GRAPH_DATA message handler
//

LRESULT CNodePerfView::OnGraphData(WPARAM wParam, LPARAM lParam)
{
	DWORD i, j;
	GraphData *data = CAST_TO_POINTER(lParam, GraphData *);

	if (m_nState != STATE_UPDATING)
	{
		NXCDestroyDCIData(data->m_data);
		safe_free(data->m_thresholds);
		delete data;
		return 0;
	}

	for(i = 0; i < m_dwNumGraphs; i++)
	{
		for(j = 0; j < MAX_GRAPH_ITEMS; j++)
		{
			if (m_pGraphList[i].dwItemId[j] == wParam)
			{
				m_pGraphList[i].pWnd->SetData(j, data->m_data);
				m_pGraphList[i].pWnd->SetThresholds(j, data->m_numThresholds, data->m_thresholds);
				m_pGraphList[i].pWnd->SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
				m_pGraphList[i].pWnd->Update();
				goto stop_search;
			}
		}
	}

stop_search:
	if (i == m_dwNumGraphs)		// Graph for given DCI ID not found
	{
		NXCDestroyDCIData(data->m_data);
		safe_free(data->m_thresholds);
	}
	delete data;
	return 0;
}


//
// WM_SIZE message handler
//

void CNodePerfView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	AdjustView();	
}


//
// Adjust view
//

void CNodePerfView::AdjustView()
{
	DWORD i;
	RECT rect;
	int y, nCol, nWidth, nColumns;
	SCROLLINFO si;

	GetClientRect(&rect);
	if ((rect.right > SPLIT_LIMIT) && (m_dwNumGraphs > 1))
	{
		nColumns = 2;
	}
	else
	{
		nColumns = 1;
	}

	// Setup scroll bar
	m_nTotalHeight = (m_dwNumGraphs + nColumns - 1) / nColumns * (GRAPH_Y_MARGIN + GRAPH_HEIGHT);
	m_nViewHeight = rect.bottom;
	if (m_nTotalHeight > m_nViewHeight)
	{
		if (m_nTotalHeight - m_nViewHeight < m_nOrigin)
			m_nOrigin = m_nTotalHeight - m_nViewHeight;

		ShowScrollBar(SB_VERT, TRUE);
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = m_nTotalHeight;
		si.nPage = m_nViewHeight;
		si.nPos = m_nOrigin;
		SetScrollInfo(SB_VERT, &si);
	}
	else
	{
		m_nOrigin = 0;
		ShowScrollBar(SB_VERT, FALSE);
	}

	GetClientRect(&rect);
	nWidth = (rect.right - GRAPH_X_MARGIN * (nColumns + 1)) / nColumns;
	y = GRAPH_Y_MARGIN - m_nOrigin;

	for(i = 0, nCol = 0; i < m_dwNumGraphs; i++)
	{
		m_pGraphList[i].pWnd->SetWindowPos(NULL, GRAPH_X_MARGIN + nCol * (nWidth + GRAPH_X_MARGIN), y, nWidth, GRAPH_HEIGHT, SWP_NOZORDER);
		nCol++;
		if (nCol == nColumns)
		{
			nCol = 0;
			y += GRAPH_HEIGHT + GRAPH_Y_MARGIN;
		}
	}
}


//
// WM_VSCROLL message handler
//

void CNodePerfView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	int nPrevOrigin = m_nOrigin;
	BOOL bUpdate = FALSE;

	switch(nSBCode)
	{
		case SB_TOP:
			if (m_nOrigin > 0)
			{
				m_nOrigin = 0;
				bUpdate = TRUE;
			}
			break;
		case SB_BOTTOM:
			if (m_nOrigin < m_nTotalHeight - m_nViewHeight)
			{
				m_nOrigin = m_nTotalHeight - m_nViewHeight;
				bUpdate = TRUE;
			}
			break;
		case SB_LINEUP:
			if (m_nOrigin > 0)
			{
				m_nOrigin -= min(m_nOrigin, 25);
				bUpdate = TRUE;
			}
			break;
		case SB_LINEDOWN:
			if (m_nOrigin < m_nTotalHeight - m_nViewHeight)
			{
				m_nOrigin += min(m_nTotalHeight - m_nViewHeight - m_nOrigin, 25);
				bUpdate = TRUE;
			}
			break;
		case SB_PAGEUP:
			if (m_nOrigin > 0)
			{
				m_nOrigin -= min(m_nOrigin, m_nViewHeight);
				bUpdate = TRUE;
			}
			break;
		case SB_PAGEDOWN:
			if (m_nOrigin < m_nTotalHeight - m_nViewHeight)
			{
				m_nOrigin += min(m_nTotalHeight - m_nViewHeight - m_nOrigin, m_nViewHeight);
				bUpdate = TRUE;
			}
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_nOrigin = nPos;
			bUpdate = TRUE;
			break;
		default:
			break;
	}

	if (bUpdate)
	{
		ScrollWindow(0, nPrevOrigin - m_nOrigin);
		AdjustView();
	}
}


//
// WM_MOUSEWHEEL message handler
//

BOOL CNodePerfView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	int lines = zDelta / WHEEL_DELTA;
	for(int i = 0; i < abs(lines); i++)
		OnVScroll(lines > 0 ? SB_LINEUP : SB_LINEDOWN, 0, NULL);
	return TRUE;
}


//
// WM_MOUSEACTIVATE message handler
//

int CNodePerfView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) 
{
	SetFocus();
	return CWnd::OnMouseActivate(pDesktopWnd, nHitTest, message);
}
