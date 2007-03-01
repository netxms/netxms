// NodePerfView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePerfView.h"

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
#define GRAPH_Y_MARGIN		50
#define GRAPH_HEIGHT			150


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

	WorkerTask(int nTask) { m_nTask = nTask; }
	WorkerTask(int nTask, DWORD dwId) { m_nTask = nTask; m_dwId = dwId; }
	WorkerTask(int nTask, DWORD dwId, DWORD dwId2,
	           DWORD dwTimeFrom, DWORD dwTimeTo)
	{
		m_nTask = nTask;
		m_dwId = dwId;
		m_dwId2 = dwId2;
		m_dwTimeFrom = dwTimeFrom;
		m_dwTimeTo = dwTimeTo;
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
}

CNodePerfView::~CNodePerfView()
{
	WorkerTask *pTask;

	while((pTask = (WorkerTask *)m_workerQueue.Get()) != NULL)
		delete pTask;
	m_workerQueue.Put(new WorkerTask(TASK_SHUTDOWN));
	ThreadJoin(PerfViewWorkerThread);
	safe_free(m_pGraphList);
}


BEGIN_MESSAGE_MAP(CNodePerfView, CWnd)
	//{{AFX_MSG_MAP(CNodePerfView)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_TIMER()
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
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_hWorkerThread = ThreadCreateEx(PerfViewWorkerThread, 0, this);

	return 0;
}


//
// NXCM_SET_OBJECT message handler
//

void CNodePerfView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
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
   m_pObject = pObject;
	m_nState = STATE_LOADING;
	InvalidateRect(NULL);
	m_workerQueue.Put(new WorkerTask(TASK_GET_AVAIL_DCI));
}


//
// WM_PAINT message handler
//

void CNodePerfView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
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
// Create graph if appropriate parameter exists
//

void CNodePerfView::CreateGraph(NXC_SYSTEM_DCI *pItemList, DWORD dwNumItems,
										  TCHAR *pszParam, TCHAR *pszTitle, RECT &rect)
{
	DWORD i;

	for(i = 0; i < dwNumItems; i++)
	{
		if ((!_tcsicmp(pItemList[i].szName, pszParam)) &&
			 (pItemList[i].nStatus == ITEM_STATUS_ACTIVE))
		{
			m_pGraphList[m_dwNumGraphs].dwItemId = pItemList[i].dwId;
			m_pGraphList[m_dwNumGraphs].pWnd = new CGraph;
			m_pGraphList[m_dwNumGraphs].pWnd->m_bSet3DEdge = FALSE;
			m_pGraphList[m_dwNumGraphs].pWnd->m_bShowLegend = FALSE;
			m_pGraphList[m_dwNumGraphs].pWnd->SetColorScheme(GCS_LIGHT);
			m_pGraphList[m_dwNumGraphs].pWnd->Create(WS_CHILD | WS_VISIBLE, rect, this, m_dwNumGraphs);
			m_dwNumGraphs++;
			rect.top += GRAPH_HEIGHT + GRAPH_Y_MARGIN;
			rect.bottom = rect.top + GRAPH_HEIGHT;
			break;
		}
	}
}


//
// Handler for NXCM_REQUEST_COMPLETED message
//

void CNodePerfView::OnRequestCompleted(WPARAM wParam, LPARAM lParam)
{
	RECT rect;

	if (m_nState != STATE_LOADING)
		return;

	if (wParam != 0xFFFFFFFF)
	{
		GetClientRect(&rect);
		rect.left += GRAPH_X_MARGIN;
		rect.right -= GRAPH_X_MARGIN;
		rect.top += GRAPH_Y_MARGIN;
		rect.bottom = rect.top + GRAPH_HEIGHT;

		m_pGraphList = (PERF_GRAPH *)realloc(m_pGraphList, wParam * sizeof(PERF_GRAPH));
		m_dwNumGraphs = 0;

		CreateGraph((NXC_SYSTEM_DCI *)lParam, wParam, _T("@system.cpu_usage"), _T("CPU Utilization"), rect);
		CreateGraph((NXC_SYSTEM_DCI *)lParam, wParam, _T("@system.freemem"), _T("Free Physical Memory"), rect);

		safe_free(CAST_TO_POINTER(lParam, void *));
		if (m_dwNumGraphs > 0)
		{
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
}


//
// Worker thread
//

void CNodePerfView::WorkerThread()
{
	WorkerTask *pTask;
	DWORD dwResult, dwNumItems;
	NXC_SYSTEM_DCI *pItemList;
	NXC_DCI_DATA *pData;

	while(1)
	{
		pTask = (WorkerTask *)m_workerQueue.GetOrBlock();
		if (pTask->m_nTask == TASK_SHUTDOWN)
			break;

		switch(pTask->m_nTask)
		{
			case TASK_GET_AVAIL_DCI:
				dwResult = NXCGetSystemDCIList(g_hSession, m_pObject->dwId, &dwNumItems, &pItemList);
				PostMessage(NXCM_REQUEST_COMPLETED,
				            (dwResult == RCC_SUCCESS) ? dwNumItems : 0xFFFFFFFF,
				            CAST_FROM_POINTER(pItemList, LPARAM));
				break;
			case TASK_FINISH_UPDATE:
				PostMessage(NXCM_UPDATE_FINISHED, 0, pTask->m_dwId);
				break;
			case TASK_UPDATE_GRAPH:
				dwResult = NXCGetDCIData(g_hSession, pTask->m_dwId2, pTask->m_dwId, 0,
				                         pTask->m_dwTimeFrom, pTask->m_dwTimeTo, &pData);
				PostMessage(NXCM_GRAPH_DATA, pTask->m_dwId,
				            (dwResult == RCC_SUCCESS) ? CAST_FROM_POINTER(pData, LPARAM) : 0);
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

void CNodePerfView::OnTimer(UINT nIDEvent) 
{
	if (m_nState == STATE_IDLE)
		UpdateAllGraphs();
}


//
// NXCM_UPDATE_FINISHED message handler
//

void CNodePerfView::OnUpdateFinished(WPARAM wParam, LPARAM lParam)
{
	if ((m_nState != STATE_UPDATING) || (m_pObject == NULL))
		return;

	if (m_pObject->dwId == (DWORD)lParam)
		m_nState = STATE_IDLE;
}


//
// Initiate update of all graphs
//

void CNodePerfView::UpdateAllGraphs()
{
	DWORD i;

	m_nState = STATE_UPDATING;
   m_dwTimeTo = time(NULL);
   m_dwTimeTo += 60 - m_dwTimeTo % 60;   // Round to minute boundary
   m_dwTimeFrom = m_dwTimeTo - 3600;
	for(i = 0; i < m_dwNumGraphs; i++)
		m_workerQueue.Put(new WorkerTask(TASK_UPDATE_GRAPH, m_pGraphList[i].dwItemId,
		                                 m_pObject->dwId, m_dwTimeFrom, m_dwTimeTo));
	m_workerQueue.Put(new WorkerTask(TASK_FINISH_UPDATE, m_pObject->dwId));
}


//
// NXCM_GRAPH_DATA message handler
//

void CNodePerfView::OnGraphData(WPARAM wParam, LPARAM lParam)
{
	DWORD i;

	if (m_nState != STATE_UPDATING)
		return;

	for(i = 0; i < m_dwNumGraphs; i++)
	{
		if (m_pGraphList[i].dwItemId == wParam)
		{
			m_pGraphList[i].pWnd->SetData(0, CAST_TO_POINTER(lParam, NXC_DCI_DATA *));
			m_pGraphList[i].pWnd->SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
			m_pGraphList[i].pWnd->Update();
			break;
		}
	}

	if (i == m_dwNumGraphs)		// Graph for given DCI ID not found
		NXCDestroyDCIData(CAST_TO_POINTER(lParam, NXC_DCI_DATA *));
}
