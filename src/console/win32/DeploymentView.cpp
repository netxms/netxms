// DeploymentView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DeploymentView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeploymentView

IMPLEMENT_DYNCREATE(CDeploymentView, CMDIChildWnd)

CDeploymentView::CDeploymentView()
{
}

CDeploymentView::~CDeploymentView()
{
}


BEGIN_MESSAGE_MAP(CDeploymentView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDeploymentView)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(WM_START_DEPLOYMENT, OnStartDeployment)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeploymentView message handlers

BOOL CDeploymentView::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CDeploymentView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view inside window
   GetClientRect(&rect);
   rect.top += 20;
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.InsertColumn(0, _T("Node"), LVCFMT_LEFT, 120);
   m_wndListCtrl.InsertColumn(1, _T("Status"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, _T("Message"), LVCFMT_LEFT, 250);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES);
	
	return 0;
}


//
// Deployment worker thread
//

static void DeploymentThread(DEPLOYMENT_JOB *pJob)
{
   DWORD dwResult;

   dwResult = NXCDeployPackage(g_hSession, pJob->dwPkgId, pJob->dwNumObjects, pJob->pdwObjectList);
   safe_free(pJob->pdwObjectList);
   free(pJob);
}


//
// WM_START_DEPLOYMENT message handler
//

void CDeploymentView::OnStartDeployment(WPARAM wParam, DEPLOYMENT_JOB *pJob)
{
   pJob->hWnd = m_hWnd;
   CreateThread(NULL, 0, DeploymentThread, pJob, 0, &dwThreadId);
}
