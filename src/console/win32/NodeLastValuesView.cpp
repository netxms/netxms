// NodeLastValuesView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodeLastValuesView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Worker thread
//

static THREAD_RESULT THREAD_CALL PerfViewWorkerThread(void *pArg)
{
	((CNodePerfView *)pArg)->WorkerThread();
	return THREAD_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CNodeLastValuesView

CNodeLastValuesView::CNodeLastValuesView()
{
	m_nTimer = 0;
   m_iSortMode = theApp.GetProfileInt(_T("NodeLastValuesView"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("NodeLastValuesView"), _T("SortDir"), 1);
	m_hWorkerThread = INVALID_THREAD_HANDLE;
}

CNodeLastValuesView::~CNodeLastValuesView()
{
   theApp.WriteProfileInt(_T("NodeLastValuesView"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("NodeLastValuesView"), _T("SortDir"), m_iSortDir);
	m_workerQueue.Clear();
	m_workerQueue.Put((void *)INVALID_INDEX);
	ThreadJoin(m_hWorkerThread);
}


BEGIN_MESSAGE_MAP(CNodeLastValuesView, CWnd)
	//{{AFX_MSG_MAP(CNodeLastValuesView)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNodeLastValuesView message handlers


//
// WM_CREATE message handler
//

int CNodeLastValuesView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   LVCOLUMN lvCol;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 5, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ACTIVE));
   m_imageList.Add(theApp.LoadIcon(IDI_DISABLED));
   m_imageList.Add(theApp.LoadIcon(IDI_UNSUPPORTED));
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS,
                        rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Description"), LVCFMT_LEFT, 250);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT | LVCFMT_BITMAP_ON_RIGHT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Changed"), LVCFMT_LEFT, 26);
   m_wndListCtrl.InsertColumn(4, _T("Timestamp"), LVCFMT_LEFT, 124);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);
		
	return 0;
}


//
// WM_SETFOCUS message handler
//

void CNodeLastValuesView::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	m_wndListCtrl.SetFocus();
}


//
// WM_SIZE message handler
//

void CNodeLastValuesView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXCM_SET_OBJECT message handler
//

void CNodeLastValuesView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
	if (m_nTimer != 0)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}

   m_pObject = pObject;
}


//
// WM_DESTROY message handler
//

void CNodeLastValuesView::OnDestroy() 
{
	if (m_nTimer != 0)
		KillTimer(m_nTimer);
	CWnd::OnDestroy();
}
