// ControlPanel.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Control panel items
//

#define CP_ITEM_EPP     0
#define CP_ITEM_USERS   1
#define CP_ITEM_EVENTS  2


//
// CControlPanel
//

IMPLEMENT_DYNCREATE(CControlPanel, CMDIChildWnd)

CControlPanel::CControlPanel()
{
   m_pImageList = NULL;
}

CControlPanel::~CControlPanel()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CControlPanel, CMDIChildWnd)
	//{{AFX_MSG_MAP(CControlPanel)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
   ON_NOTIFY(NM_DBLCLK, ID_LIST_VIEW, OnListViewDoubleClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CControlPanel message handlers



BOOL CControlPanel::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_SETUP));
	return CMDIChildWnd::PreCreateWindow(cs);
}

int CControlPanel::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create list view
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_AUTOARRANGE | LVS_SINGLESEL, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create an image list and assign it to list control
   m_pImageList = new CImageList;
   m_pImageList->Create(32, 32, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_USERS));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_RULEMGR));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVENT));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_NORMAL);

   // Populate list with items
   m_wndListCtrl.InsertItem(CP_ITEM_EPP, "Event Processing Policy", 1);
   m_wndListCtrl.InsertItem(CP_ITEM_USERS, "Users", 0);
   m_wndListCtrl.InsertItem(CP_ITEM_EVENTS, "Events", 2);

   theApp.OnViewCreate(IDR_CTRLPANEL, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CControlPanel::OnDestroy() 
{
   theApp.OnViewDestroy(IDR_CTRLPANEL, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CControlPanel::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CControlPanel::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   m_wndListCtrl.SetFocus();
}


//
// Process double click on list control
//

void CControlPanel::OnListViewDoubleClick(NMITEMACTIVATE *pInfo, LRESULT *pResult)
{
   switch(pInfo->iItem)
   {
      case CP_ITEM_EPP:
         PostMessage(WM_COMMAND, ID_CONTROLPANEL_EVENTPOLICY, 0);
         break;
      case CP_ITEM_EVENTS:
         PostMessage(WM_COMMAND, ID_CONTROLPANEL_EVENTS, 0);
         break;
      case CP_ITEM_USERS:
         PostMessage(WM_COMMAND, ID_CONTROLPANEL_USERS, 0);
         break;
      default:
         break;
   }
}
