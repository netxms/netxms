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
// CGraphFrame

IMPLEMENT_DYNCREATE(CGraphFrame, CMDIChildWnd)

CGraphFrame::CGraphFrame()
{
   m_dwNumItems = 0;
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphFrame message handlers


//
// WM_CREATE message handler
//

int CGraphFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndGraph.SetTimeFrame(m_dwTimeFrom, m_dwTimeTo);
   m_wndGraph.Create(WS_CHILD | WS_VISIBLE, rect, this, IDC_GRAPH);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

	return 0;
}


//
// WM_SIZE message handler
//

void CGraphFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

   m_wndGraph.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
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
      dwResult = DoRequestArg6(NXCGetDCIData, (void *)m_pdwNodeId[i], (void *)m_pdwItemId[i],
                               0, (void *)m_dwTimeFrom, (void *)m_dwTimeTo, &pData, "Loading item data...");
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

   if (dlg.DoModal() == IDOK)
   {
   }
}
