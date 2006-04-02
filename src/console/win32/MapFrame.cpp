// MapFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapFrame

IMPLEMENT_DYNCREATE(CMapFrame, CMDIChildWnd)

CMapFrame::CMapFrame()
{
}

CMapFrame::~CMapFrame()
{
}


BEGIN_MESSAGE_MAP(CMapFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMapFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_MAP_ZOOMIN, OnMapZoomin)
	ON_UPDATE_COMMAND_UI(ID_MAP_ZOOMIN, OnUpdateMapZoomin)
	ON_COMMAND(ID_MAP_ZOOMOUT, OnMapZoomout)
	ON_UPDATE_COMMAND_UI(ID_MAP_ZOOMOUT, OnUpdateMapZoomout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapFrame message handlers

BOOL CMapFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_NETMAP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CMapFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static TBBUTTON tbButtons[] =
   {
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 1, 0, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
   };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   m_wndToolBox.Create(NULL, _T(""), WS_CHILD | WS_VISIBLE, rect, this, 0);

   //EnableDocking(CBRS_ALIGN_ANY);

   // Create toolbar
   //m_wndToolBar.CreateEx(this);
   //m_wndToolBar.LoadToolBar(IDT_MAP);
   //FloatControlBar(&m_wndToolBar, TRUE, FALSE);
/*   m_wndToolBar.Create(WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT, rect, this, ID_TOOLBAR_CTRL);
   m_wndToolBar.SetExtendedStyle(WS_EX_WINDOWEDGE);
   m_wndToolBar.SetButtonSize(CSize(20, 20));
   m_wndToolBar.LoadImages(IDB_HIST_SMALL_COLOR, HINST_COMMCTRL);
   m_wndToolBar.AddButtons(2, tbButtons);*/

   // Create and initialize map view
   m_wndMapView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
   
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_SIZE message handler
//

void CMapFrame::OnSize(UINT nType, int cx, int cy) 
{
   int nToolBarHeight = 0;
   int nToolBoxWidth;

	CMDIChildWnd::OnSize(nType, cx, cy);
//   m_wndToolBar.AutoSize();
   //nToolBarHeight = GetWindowSize(&m_wndToolBar).cy;
   nToolBoxWidth = 200;
   m_wndToolBox.SetWindowPos(NULL, 0, 0, nToolBoxWidth, cy, SWP_NOZORDER);
   m_wndMapView.SetWindowPos(NULL, nToolBoxWidth, nToolBarHeight,
                             cx - nToolBoxWidth, cy - nToolBarHeight, SWP_NOZORDER);	
}


//
// WM_SETFOCUS message handler
//

void CMapFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndMapView.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CMapFrame::OnViewRefresh() 
{
   m_wndMapView.SetMap(new nxMap(1, _T("Default"), _T("Default map")));
/*   NXC_OBJECT *pObject;
   DWORD i;
   CString strFullString, strTitle;

	if (strFullString.LoadString(IDR_MAPFRAME))
		AfxExtractSubString(strTitle, strFullString, CDocTemplate::docName);

   m_wndMapView.DeleteAllItems();
   if (m_pRootObject == NULL)
   {
      pObject = NXCGetTopologyRootObject(g_hSession);
      if (pObject != NULL)
         AddObjectToView(pObject);

      pObject = NXCGetServiceRootObject(g_hSession);
      if (pObject != NULL)
         AddObjectToView(pObject);
   
      pObject = NXCGetTemplateRootObject(g_hSession);
      if (pObject != NULL)
         AddObjectToView(pObject);

      // add object name to title
      strTitle += " - [root]";
   }
   else
   {
      for(i = 0; i < m_pRootObject->dwNumChilds; i++)
      {
         pObject = NXCFindObjectById(g_hSession, m_pRootObject->pdwChildList[i]);
         if (pObject != NULL)
            if (!pObject->bIsDeleted)
               AddObjectToView(pObject);
      }

      // add object name to title
      strTitle += " - [";
      strTitle += m_pRootObject->szName;
      strTitle += "]";
   }

   ::SetWindowText(m_hWnd, strTitle);*/
}


//
// WM_COMMAND::ID_MAP_ZOOMIN message handlers
//

void CMapFrame::OnMapZoomin() 
{
   m_wndMapView.ZoomIn();
}

void CMapFrame::OnUpdateMapZoomin(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanZoomIn());
}


//
// WM_COMMAND::ID_MAP_ZOOMIN message handlers
//

void CMapFrame::OnMapZoomout() 
{
   m_wndMapView.ZoomOut();
}

void CMapFrame::OnUpdateMapZoomout(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanZoomOut());
}
