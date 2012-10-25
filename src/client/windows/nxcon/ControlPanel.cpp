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
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
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


//
// Item comparision function
//

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CListCtrl *pListCtrl = (CListCtrl *)lParamSort;
   CString strItem1 = pListCtrl->GetItemText((int)lParam1, 0);
   CString strItem2 = pListCtrl->GetItemText((int)lParam2, 0);
   return _tcscmp(strItem2, strItem1);
}


//
// WM_CREATE message handler
//

int CControlPanel::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   BYTE *pwp;
   UINT iBytes;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create list view
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_ICON | 
                        LVS_AUTOARRANGE | LVS_SORTASCENDING | LVS_SINGLESEL,
                        rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create an image list and assign it to list control
   m_pImageList = new CImageList;
   m_pImageList->Create(32, 32, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_RULEMGR));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_LOG));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_TRAP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_DATABASE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SETUP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_LPP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_OBJTOOLS));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SCRIPT_LIBRARY));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_CONFIGS));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_DISCOVERY));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_CERTMGR));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVENT_CORRELATION));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NETMAP));
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_NORMAL);

   // Populate list with items
   AddItem(_T("Event Processing Policy"), 0, ID_CONTROLPANEL_EVENTPOLICY);
   AddItem(_T("Users"), 1, ID_CONTROLPANEL_USERS);
   AddItem(_T("Events"), 2, ID_CONTROLPANEL_EVENTS);
   AddItem(_T("Actions"), 3, ID_CONTROLPANEL_ACTIONS);
   AddItem(_T("SNMP Traps"), 4, ID_CONTROLPANEL_SNMPTRAPS);
   AddItem(_T("Agent Packages"), 5, ID_CONTROLPANEL_AGENTPKG);
   AddItem(_T("Server Configuration"), 6, ID_CONTROLPANEL_SERVERCFG);
   AddItem(_T("Syslog Parser"), 7, ID_CONTROLPANEL_SYSLOGPARSER);
   AddItem(_T("Object Tools"), 8, ID_CONTROLPANEL_OBJECTTOOLS);
   AddItem(_T("Script Library"), 9, ID_CONTROLPANEL_SCRIPTLIBRARY);
   AddItem(_T("Agent Configurations"), 10, ID_CONTROLPANEL_AGENTCONFIGS);
   AddItem(_T("Network Discovery"), 11, ID_CONTROLPANEL_NETWORKDISCOVERY);
   AddItem(_T("Certificates"), 12, ID_CONTROLPANEL_CERTIFICATES);
   //AddItem(_T("Event Correlation"), 13, ID_CONTROLPANEL_EVENTCORRELATION);

   m_wndListCtrl.SortItems(CompareItems, (DWORD)&m_wndListCtrl);

   // Restore window size and position if we have one saved
   if (theApp.GetProfileBinary(_T("ControlPanel"), _T("WindowPlacement"),
                               &pwp, &iBytes))
   {
      if (iBytes == sizeof(WINDOWPLACEMENT))
      {
         RestoreMDIChildPlacement(this, (WINDOWPLACEMENT *)pwp);
      }
      delete pwp;
   }

   theApp.OnViewCreate(VIEW_CTRLPANEL, this);
	return 0;
}


//
// WM_DESTROY message handler
//

void CControlPanel::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_CTRLPANEL, this);
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

void CControlPanel::OnListViewDoubleClick(NMHDR *pNMHDR, LRESULT *pResult)
{
   if (((NMITEMACTIVATE *)pNMHDR)->iItem != -1)
   {
      WPARAM wParam;

      wParam = m_wndListCtrl.GetItemData(((NMITEMACTIVATE *)pNMHDR)->iItem);
      if (wParam != 0)
         PostMessage(WM_COMMAND, wParam, 0);
   }
}


//
// Add new item to list
//

void CControlPanel::AddItem(TCHAR *pszName, int iImage, WPARAM wParam)
{
   int iIndex;

   iIndex = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszName, iImage);
   m_wndListCtrl.SetItemData(iIndex, wParam);
}


//
// Get save info for desktop saving
//

LRESULT CControlPanel::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = (WINDOW_SAVE_INFO *)lParam;
   pInfo->iWndClass = WNDC_CONTROL_PANEL;
   GetWindowPlacement(&pInfo->placement);
   pInfo->szParameters[0] = 0;
   return 1;
}


//
// WM_CLOSE message handler
//

void CControlPanel::OnClose() 
{
   WINDOWPLACEMENT wp;

   GetWindowPlacement(&wp);
   theApp.WriteProfileBinary(_T("ControlPanel"), _T("WindowPlacement"), 
                             (BYTE *)&wp, sizeof(WINDOWPLACEMENT));
	
	CMDIChildWnd::OnClose();
}
