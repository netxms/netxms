// DataCollectionEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DataCollectionEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor

IMPLEMENT_DYNCREATE(CDataCollectionEditor, CMDIChildWnd)

CDataCollectionEditor::CDataCollectionEditor()
{
   m_pItemList = NULL;
}

CDataCollectionEditor::CDataCollectionEditor(NXC_DCI_LIST *pList)
{
   m_pItemList = pList;
}

CDataCollectionEditor::~CDataCollectionEditor()
{
}


BEGIN_MESSAGE_MAP(CDataCollectionEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CDataCollectionEditor)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataCollectionEditor message handlers


//
// WM_CREATE message handler
//

int CDataCollectionEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(1, "Source", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, "Name", LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(3, "Data type", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(4, "Polling Interval", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(5, "Retention Time", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(6, "Status", LVCFMT_LEFT, 80);
	
   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_DC_EDITOR, this, m_pItemList->dwNodeId);

	return 0;
}


//
// WM_DESTROY message handler
//

void CDataCollectionEditor::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_DC_EDITOR, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CLOSE message handler
//

void CDataCollectionEditor::OnClose() 
{
   DWORD dwResult;

   dwResult = DoRequestArg1(NXCCloseNodeDCIList, m_pItemList, "Saving node's data collection information...");
   if (dwResult != RCC_SUCCESS)
   {
      char szBuffer[256];

      sprintf(szBuffer, "Unable to close data collection configuration:\n%s", 
              NXCGetErrorText(dwResult));
      MessageBox(szBuffer, "Error", MB_ICONSTOP);
   }
	
	CMDIChildWnd::OnClose();
}


//
// WM_SIZE message handler
//

void CDataCollectionEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}
