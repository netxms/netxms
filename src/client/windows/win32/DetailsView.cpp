// DetailsView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DetailsView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDetailsView

IMPLEMENT_DYNCREATE(CDetailsView, CMDIChildWnd)

CDetailsView::CDetailsView()
{
   m_dwViewId = 0;
   m_cx = 200;
   m_cy = 150;
   m_pData = NULL;
   m_pImageList = NULL;
}

CDetailsView::CDetailsView(DWORD dwId, int cx, int cy, Table *pData, CImageList *pImageList)
{
   m_dwViewId = dwId;
   m_cx = cx;
   m_cy = cy;
   m_pData = pData;
   m_pImageList = pImageList;
}

CDetailsView::~CDetailsView()
{
   delete m_pData;
}


BEGIN_MESSAGE_MAP(CDetailsView, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDetailsView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDetailsView message handlers

BOOL CDetailsView::PreCreateWindow(CREATESTRUCT& cs) 
{
   cs.style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
   cs.cx = m_cx;
   cs.cy = m_cy;
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CDetailsView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   int nWidth;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   nWidth = (int)(rect.right * 0.3);
   m_wndListCtrl.InsertColumn(0, _T("Attribute"), LVCFMT_LEFT, nWidth);
   m_wndListCtrl.InsertColumn(1, _T("Image"), LVCFMT_CENTER, 25);
   m_wndListCtrl.InsertColumn(2, _T("Value"), LVCFMT_LEFT, rect.right - nWidth - 25);

   Update();
	
	return 0;
}


//
// WM_SIZE message handler
//

void CDetailsView::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   
   m_wndListCtrl.SetWindowPos(NULL, 0, 20, cx, cy - 40, SWP_NOZORDER);
}


//
// WM_DESTROY message handler
//

void CDetailsView::OnDestroy() 
{
	CMDIChildWnd::OnDestroy();
	
}


//
// Update table with new data
//

void CDetailsView::Update()
{
   int i, nItem;

   m_wndListCtrl.DeleteAllItems();
   if (m_pData != NULL)
   {
      for(i = 0; i < m_pData->GetNumRows(); i++)
      {
         nItem = m_wndListCtrl.InsertItem(i, m_pData->GetAsString(i, 0), -1);
         if (nItem != -1)
         {
            m_wndListCtrl.SetItemText(nItem, 2, m_pData->GetAsString(i, 2));
         }
      }
   }
}
