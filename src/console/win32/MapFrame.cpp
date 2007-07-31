// MapFrame.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "MapFrame.h"
#include "SubmapBkgndDlg.h"
#include "MapLinkPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAP_SECTION     _T("MapFrame")

#define STATUS_PANE_LAYOUT    1
#define STATUS_PANE_SCALE     2


/////////////////////////////////////////////////////////////////////////////
// CMapFrame

IMPLEMENT_DYNCREATE(CMapFrame, CMDIChildWnd)

CMapFrame::CMapFrame()
{
   m_bShowToolBar = theApp.GetProfileInt(MAP_SECTION, _T("ShowToolBar"), TRUE);
   m_bShowToolBox = theApp.GetProfileInt(MAP_SECTION, _T("ShowToolBox"), FALSE);
   m_bShowStatusBar = theApp.GetProfileInt(MAP_SECTION, _T("ShowStatusBar"), TRUE);
}

CMapFrame::~CMapFrame()
{
   theApp.WriteProfileInt(MAP_SECTION, _T("ShowToolBar"), m_bShowToolBar);
   theApp.WriteProfileInt(MAP_SECTION, _T("ShowToolBox"), m_bShowToolBox);
   theApp.WriteProfileInt(MAP_SECTION, _T("ShowStatusBar"), m_bShowStatusBar);
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
	ON_COMMAND(ID_OBJECT_OPENPARENT, OnObjectOpenparent)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_OPENPARENT, OnUpdateObjectOpenparent)
	ON_COMMAND(ID_MAP_BACK, OnMapBack)
	ON_UPDATE_COMMAND_UI(ID_MAP_BACK, OnUpdateMapBack)
	ON_COMMAND(ID_MAP_FORWARD, OnMapForward)
	ON_UPDATE_COMMAND_UI(ID_MAP_FORWARD, OnUpdateMapForward)
	ON_COMMAND(ID_MAP_HOME, OnMapHome)
	ON_COMMAND(ID_MAP_SAVE, OnMapSave)
	ON_COMMAND(ID_MAP_REDOLAYOUT, OnMapRedolayout)
	ON_WM_PAINT()
	ON_COMMAND(ID_MAP_SHOW_STATUSBAR, OnMapShowStatusbar)
	ON_UPDATE_COMMAND_UI(ID_MAP_SHOW_STATUSBAR, OnUpdateMapShowStatusbar)
	ON_COMMAND(ID_MAP_SHOW_TOOLBAR, OnMapShowToolbar)
	ON_UPDATE_COMMAND_UI(ID_MAP_SHOW_TOOLBAR, OnUpdateMapShowToolbar)
	ON_COMMAND(ID_MAP_SHOW_TOOLBOX, OnMapShowToolbox)
	ON_UPDATE_COMMAND_UI(ID_MAP_SHOW_TOOLBOX, OnUpdateMapShowToolbox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OBJECT_PROPERTIES, OnObjectProperties)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_PROPERTIES, OnUpdateObjectProperties)
	ON_COMMAND(ID_OBJECT_LASTDCIVALUES, OnObjectLastdcivalues)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_LASTDCIVALUES, OnUpdateObjectLastdcivalues)
	ON_COMMAND(ID_MAP_SETBACKGROUND, OnMapSetbackground)
	ON_COMMAND(ID_OBJECT_DATACOLLECTION, OnObjectDatacollection)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_DATACOLLECTION, OnUpdateObjectDatacollection)
	ON_COMMAND(ID_OBJECT_MANAGE, OnObjectManage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_MANAGE, OnUpdateObjectManage)
	ON_COMMAND(ID_OBJECT_UNBIND, OnObjectUnbind)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNBIND, OnUpdateObjectUnbind)
	ON_COMMAND(ID_OBJECT_UNMANAGE, OnObjectUnmanage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNMANAGE, OnUpdateObjectUnmanage)
	ON_COMMAND(ID_OBJECT_BIND, OnObjectBind)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_BIND, OnUpdateObjectBind)
	ON_UPDATE_COMMAND_UI(ID_MAP_SAVE, OnUpdateMapSave)
	ON_COMMAND(ID_MAP_LINK, OnMapLink)
	ON_UPDATE_COMMAND_UI(ID_MAP_LINK, OnUpdateMapLink)
	ON_COMMAND(ID_MAP_AUTOLAYOUT, OnMapAutolayout)
	ON_UPDATE_COMMAND_UI(ID_MAP_AUTOLAYOUT, OnUpdateMapAutolayout)
	ON_WM_CLOSE()
	ON_COMMAND(ID_MAP_UNLINK, OnMapUnlink)
	ON_UPDATE_COMMAND_UI(ID_MAP_UNLINK, OnUpdateMapUnlink)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(NXCM_SUBMAP_CHANGE, OnSubmapChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapFrame message handlers

BOOL CMapFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   BOOL bRet;

   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_NETMAP));
	bRet = CMDIChildWnd::PreCreateWindow(cs);
   cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
   return bRet;
}


//
// WM_CREATE message handler
//

int CMapFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   static TBBUTTON tbButtons[] =
   {
      { 5, ID_MAP_BACK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 6, ID_MAP_FORWARD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 7, ID_MAP_PARENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 8, ID_MAP_HOME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
      { 0, ID_MAP_ZOOMIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 1, ID_MAP_ZOOMOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
      { 2, ID_MAP_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 4, ID_FILE_PRINT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 },
      { 3, ID_VIEW_REFRESH, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
      { 9, ID_MAP_REDOLAYOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
      //{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0, 0 }
   };

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);

   m_wndToolBox.Create(NULL, _T(""),
                       m_bShowToolBox ? (WS_CHILD | WS_VISIBLE) : WS_CHILD,
                       rect, this, 0);

   // Create image list for toolbar
   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 8, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_ZOOMIN));
   m_imageList.Add(theApp.LoadIcon(IDI_ZOOMOUT));
   m_imageList.Add(theApp.LoadIcon(IDI_SAVE));
   m_imageList.Add(theApp.LoadIcon(IDI_REFRESH));
   m_imageList.Add(theApp.LoadIcon(IDI_PRINT));
   m_imageList.Add(theApp.LoadIcon(IDI_BACK));
   m_imageList.Add(theApp.LoadIcon(IDI_FORWARD));
   m_imageList.Add(theApp.LoadIcon(IDI_GO_PARENT));
   m_imageList.Add(theApp.LoadIcon(IDI_HOME));
   m_imageList.Add(theApp.LoadIcon(IDI_REDO_LAYOUT));

   // Create toolbar
   m_wndToolBar.CreateEx(this, CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT,
                         WS_CHILD | (m_bShowToolBar ? WS_VISIBLE : 0) | CBRS_ALIGN_TOP,
                         CRect(0, 0, 0, 0), ID_TOOLBAR_CTRL);
   m_wndToolBar.GetToolBarCtrl().SetImageList(&m_imageList);
   m_wndToolBar.GetToolBarCtrl().AddButtons(sizeof(tbButtons) / sizeof(TBBUTTON), tbButtons);

   // Create status bar
   m_wndStatusBar.Create(WS_CHILD | (m_bShowStatusBar ? WS_VISIBLE : 0) | CCS_BOTTOM | SBARS_SIZEGRIP, rect, this, -1);

   // Create and initialize map view
   m_wndMapView.CreateEx(WS_EX_CLIENTEDGE, NULL, _T("MapView"), WS_CHILD | WS_VISIBLE, rect, this, 0);
	m_wndMapView.m_bShowConnectorNames = TRUE;
   
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// Redo map frame layout after resize or configuration change
//

void CMapFrame::RedoLayout()
{
   int nToolBarHeight, nStatusBarHeight, nToolBoxWidth, nShift;
   RECT rect;

   m_wndStatusBar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER);
   m_wndStatusBar.GetClientRect(&rect);
   nShift = GetSystemMetrics(SM_CXVSCROLL);
   int widths[3] = { rect.right - 110 - nShift, 
                     rect.right - 30 - nShift, rect.right -  nShift };
   m_wndStatusBar.SetParts(3, widths);
   nStatusBarHeight = m_bShowStatusBar ? GetWindowSize(&m_wndStatusBar).cy : 0;

   GetClientRect(&rect);

   nToolBarHeight = m_bShowToolBar ? (GetWindowSize(&m_wndToolBar).cy + 1) : 0;
   nToolBoxWidth = m_bShowToolBox ? 150 : 0;

   m_wndToolBox.SetWindowPos(NULL, 0, nToolBarHeight, nToolBoxWidth,
                             rect.bottom - nToolBarHeight - nStatusBarHeight, SWP_NOZORDER);
   m_wndMapView.SetWindowPos(NULL, nToolBoxWidth, nToolBarHeight,
                             rect.right - nToolBoxWidth,
                             rect.bottom - nToolBarHeight - nStatusBarHeight, SWP_NOZORDER);
}


//
// WM_SIZE message handler
//

void CMapFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   RedoLayout();
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
// Map (re)loading function
//

static DWORD LoadMap(TCHAR *pszName, nxMap **ppMap)
{
   DWORD dwResult, dwMapId;

   dwResult = NXCResolveMapName(g_hSession, pszName, &dwMapId);
   if (dwResult == RCC_SUCCESS)
   {
      dwResult = NXCLoadMap(g_hSession, dwMapId, (void **)ppMap);
   }
   return dwResult;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CMapFrame::OnViewRefresh() 
{
   DWORD dwResult;
   nxMap *pMap;

   dwResult = DoRequestArg2(LoadMap, _T("Default"), &pMap, _T("Loading map..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndMapView.SetMap(pMap);
      m_wndStatusBar.SetText(m_wndMapView.GetScaleText(), STATUS_PANE_SCALE, 0);
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot load map from server: %s"));
      m_wndStatusBar.SetText(_T(""), STATUS_PANE_SCALE, 0);
   }
   //m_wndMapView.SetMap(new nxMap(1, _T("Default"), _T("Default map")));
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
   m_wndStatusBar.SetText(m_wndMapView.GetScaleText(), STATUS_PANE_SCALE, 0);
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
   m_wndStatusBar.SetText(m_wndMapView.GetScaleText(), STATUS_PANE_SCALE, 0);
}

void CMapFrame::OnUpdateMapZoomout(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanZoomOut());
}


//
// WM_COMMAND::ID_OBJECT_OPENPARENT message handlers
//

void CMapFrame::OnObjectOpenparent() 
{
   m_wndMapView.GoToParentObject();
}

void CMapFrame::OnUpdateObjectOpenparent(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanGoToParent());
}


//
// WM_COMMAND::ID_MAP_BACK message handlers
//

void CMapFrame::OnMapBack() 
{
   m_wndMapView.GoBack();
}

void CMapFrame::OnUpdateMapBack(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanGoBack());
}


//
// WM_COMMAND::ID_MAP_FORWARD message handlers
//

void CMapFrame::OnMapForward() 
{
   m_wndMapView.GoForward();
}

void CMapFrame::OnUpdateMapForward(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.CanGoForward());
}


//
// WM_COMMAND::ID_MAP_HOME message handlers
//

void CMapFrame::OnMapHome()
{
   m_wndMapView.OpenRoot();
}


//
// NXCM_OBJECT_CHANGE message handler
// wParam contains object's ID, and lParam pointer to corresponding NXC_OBJECT structure
//

void CMapFrame::OnObjectChange(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_wndMapView.OnObjectChange(wParam, pObject);
}


//
// WM_COMMAND::ID_MAP_SAVE message handlers
//

void CMapFrame::OnMapSave() 
{
   DWORD dwResult;

   dwResult = DoRequestArg2(NXCSaveMap, g_hSession, m_wndMapView.GetMap(), _T("Saving map..."));
   if (dwResult == RCC_SUCCESS)
   {
      m_wndMapView.m_bIsModified = FALSE;
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Cannot save map on server: %s"));
   }
}

void CMapFrame::OnUpdateMapSave(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.m_bIsModified);
}


//
// WM_COMMAND::ID_MAP_REDOLAYOUT message handlers
//

void CMapFrame::OnMapRedolayout() 
{
   m_wndMapView.DoSubmapLayout();
   m_wndMapView.Update();
}


//
// WM_PAINT message handler
//

void CMapFrame::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
   RECT rect;

   if (m_bShowToolBar)
   {
      GetClientRect(&rect);
      rect.top = GetWindowSize(&m_wndToolBar).cy;
      dc.DrawEdge(&rect, EDGE_ETCHED, BF_TOP);
   }
}


//
// WM_COMMAND::ID_MAP_SHOW_STATUSBAR message handlers
//

void CMapFrame::OnMapShowStatusbar() 
{
	// TODO: Add your command handler code here
	
}

void CMapFrame::OnUpdateMapShowStatusbar(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_bShowStatusBar);
}


//
// WM_COMMAND::ID_MAP_SHOW_TOOLBAR message handlers
//

void CMapFrame::OnMapShowToolbar() 
{
   m_bShowToolBar = !m_bShowToolBar;
   m_wndToolBar.ShowWindow(m_bShowToolBar ? SW_SHOWNOACTIVATE : SW_HIDE);
   RedoLayout();
}

void CMapFrame::OnUpdateMapShowToolbar(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_bShowToolBar);
}


//
// WM_COMMAND::ID_MAP_SHOW_TOOLBOX message handlers
//

void CMapFrame::OnMapShowToolbox() 
{
   m_bShowToolBox = !m_bShowToolBox;
   m_wndToolBox.ShowWindow(m_bShowToolBox ? SW_SHOWNOACTIVATE : SW_HIDE);
   RedoLayout();
}

void CMapFrame::OnUpdateMapShowToolbox(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_bShowToolBox);
}


//
// WM_CONTEXTMENU message handler
//

void CMapFrame::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(9);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// WM_COMMAND::ID_OBJECT_PROPERTIES message handlers
//

void CMapFrame::OnObjectProperties() 
{
   DWORD dwCount, *pdwList;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   if (dwCount == 1)
      theApp.ObjectProperties(*pdwList);
   safe_free(pdwList);
}

void CMapFrame::OnUpdateObjectProperties(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.GetSelectionCount() == 1);
}


//
// WM_COMMAND::ID_OBJECT_LASTDCIVALUES message handlers
//

void CMapFrame::OnObjectLastdcivalues() 
{
   DWORD i, dwCount, *pdwList;
   NXC_OBJECT *pObject;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   for(i = 0; i < dwCount; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pdwList[i]);
      if ((pObject != NULL) && (pObject->iClass == OBJECT_NODE))
         theApp.ShowLastValues(pObject);
   }
   safe_free(pdwList);
}

void CMapFrame::OnUpdateObjectLastdcivalues(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode());
}


//
// Determine if currenly selected object on map is node
//

BOOL CMapFrame::CurrObjectIsNode()
{
   DWORD i, dwCount, *pdwList;
   NXC_OBJECT *pObject;
   BOOL bRet = FALSE;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   for(i = 0; i < dwCount; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pdwList[i]);
      if (pObject != NULL)
      {
         bRet = (pObject->iClass == OBJECT_NODE);
         if (!bRet)
            break;
      }
   }
   safe_free(pdwList);
   return bRet;
}


//
// WM_COMMAND::ID_MAP_SETBACKGROUND message handlers
//

void CMapFrame::OnMapSetbackground() 
{
   CSubmapBkgndDlg dlg;
   DWORD dwResult;
   HBITMAP hBitmap;

   dlg.m_nBkType = 0;
   dlg.m_nScaleMethod = 0;
   if (dlg.DoModal() == IDOK)
   {
      if (dlg.m_nBkType == 0)
      {
         m_wndMapView.SetBkImage(NULL);
      }
      else
      {
         hBitmap = LoadPicture((TCHAR *)((LPCTSTR)dlg.m_strFileName), m_wndMapView.GetScaleFactor());
         if (hBitmap != NULL)
         {
            dwResult = DoRequestArg4(NXCUploadSubmapBkImage, g_hSession,
                                     (void *)m_wndMapView.GetMap()->MapId(),
                                     (void *)m_wndMapView.GetSubmap()->Id(),
                                     (void *)((LPCTSTR)dlg.m_strFileName),
                                     _T("Uploading new background image to server..."));
            if (dwResult == RCC_SUCCESS)
            {
               m_wndMapView.SetBkImage(hBitmap);
            }
            else
            {
               theApp.ErrorBox(dwResult, _T("Cannot upload background image to server: %s"));
               DeleteObject(hBitmap);
            }
         }
         else
         {
            MessageBox(_T("Cannot load image file (possibly unsupported file format)"),
                       _T("Error"), MB_OK | MB_ICONSTOP);
         }
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_DATACOLLECTION message handlers
//

void CMapFrame::OnObjectDatacollection() 
{
   DWORD i, dwCount, *pdwList;
   NXC_OBJECT *pObject;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   for(i = 0; i < dwCount; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pdwList[i]);
      if ((pObject != NULL) && (pObject->iClass == OBJECT_NODE))
         theApp.StartObjectDCEditor(pObject);
   }
   safe_free(pdwList);
}

void CMapFrame::OnUpdateObjectDatacollection(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode());
}


//
// WM_COMMAND::ID_OBJECT_MANAGE message handlers
//

void CMapFrame::OnObjectManage() 
{
   DWORD i, dwCount, *pdwList;
   NXC_OBJECT *pObject;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   for(i = 0; i < dwCount; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pdwList[i]);
      if (pObject != NULL)
         theApp.SetObjectMgmtStatus(pObject, TRUE);
   }
   safe_free(pdwList);
}

void CMapFrame::OnUpdateObjectManage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.GetSelectionCount() > 0);
}


//
// WM_COMMAND::ID_OBJECT_UNMANAGE message handlers
//

void CMapFrame::OnObjectUnmanage() 
{
   DWORD i, dwCount, *pdwList;
   NXC_OBJECT *pObject;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   for(i = 0; i < dwCount; i++)
   {
      pObject = NXCFindObjectById(g_hSession, pdwList[i]);
      if (pObject != NULL)
         theApp.SetObjectMgmtStatus(pObject, FALSE);
   }
   safe_free(pdwList);
}

void CMapFrame::OnUpdateObjectUnmanage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_wndMapView.GetSelectionCount() > 0);
}


//
// WM_COMMAND::ID_OBJECT_BIND message handlers
//

void CMapFrame::OnObjectBind() 
{
   NXC_OBJECT *pObject;

   pObject = GetFirstSelectedObject();
   if (pObject != NULL)
      theApp.BindObject(pObject);
}

void CMapFrame::OnUpdateObjectBind(CCmdUI* pCmdUI) 
{
   if (m_wndMapView.GetSelectionCount() != 1)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      NXC_OBJECT *pObject;

      pObject = GetFirstSelectedObject();
      pCmdUI->Enable((pObject->iClass == OBJECT_CONTAINER) ||
                     (pObject->iClass == OBJECT_SERVICEROOT));
   }
}


//
// WM_COMMAND::ID_OBJECT_UNBIND message handlers
//

void CMapFrame::OnObjectUnbind() 
{
   NXC_OBJECT *pObject;

   pObject = GetFirstSelectedObject();
   if (pObject != NULL)
      theApp.UnbindObject(pObject);
}

void CMapFrame::OnUpdateObjectUnbind(CCmdUI* pCmdUI) 
{
   if (m_wndMapView.GetSelectionCount() != 1)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      NXC_OBJECT *pObject;

      pObject = GetFirstSelectedObject();
      pCmdUI->Enable((pObject->iClass == OBJECT_CONTAINER) ||
                     (pObject->iClass == OBJECT_SERVICEROOT));
   }
}


//
// Get first selected object
//

NXC_OBJECT * CMapFrame::GetFirstSelectedObject()
{
   DWORD dwCount, *pdwList;
   NXC_OBJECT *pObject = NULL;

   pdwList = m_wndMapView.GetSelectedObjects(&dwCount);
   if (dwCount > 0)
   {
      pObject = NXCFindObjectById(g_hSession, *pdwList);
   }
   safe_free(pdwList);
   return pObject;
}


//
// NXCM_SUBMAP_CHANGE message handler
//

void CMapFrame::OnSubmapChange(WPARAM wParam, nxSubmap *pSubmap)
{
   if (pSubmap != NULL)
   {
      m_wndStatusBar.SetText(pSubmap->GetAutoLayoutFlag() ? _T("Automatic") : _T("Manual"),
                             STATUS_PANE_LAYOUT, 0);
   }
   else
   {
      m_wndStatusBar.SetText(_T(""), STATUS_PANE_LAYOUT, 0);
   }
}


//
// Handler for "Create link" menu
//

void CMapFrame::OnMapLink() 
{
	nxSubmap *submap;
	DWORD *selection, count;
	NXC_OBJECT *object;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		object = NXCFindObjectById(g_hSession, submap->Id());
		if (object != NULL)
		{
			CMapLinkPropDlg dlg;

			dlg.m_dwParentObject = submap->Id();
			selection = m_wndMapView.GetSelectedObjects(&count);
			if (count > 0)
			{
				dlg.m_dwObject1 = selection[0];
				if (count > 1)
				{
					dlg.m_dwObject2 = selection[1];
				}
			}
			safe_free(selection);
			if (dlg.DoModal() == IDOK)
			{
				submap->LinkObjects(dlg.m_dwObject1, dlg.m_strPort1, dlg.m_dwObject2, dlg.m_strPort2, LINK_TYPE_NORMAL);
				m_wndMapView.m_bIsModified = TRUE;
				m_wndMapView.Update();
			}
		}
	}
}

void CMapFrame::OnUpdateMapLink(CCmdUI* pCmdUI) 
{
	nxSubmap *submap;
	NXC_OBJECT *object;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		object = NXCFindObjectById(g_hSession, submap->Id());
		if (object != NULL)
		{
			pCmdUI->Enable(object->iClass == OBJECT_CONTAINER);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}


//
// Handler for "Enable automatic layout" menu
//

void CMapFrame::OnMapAutolayout() 
{
	nxSubmap *submap;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		submap->SetAutoLayoutFlag(!submap->GetAutoLayoutFlag());
		m_wndMapView.m_bIsModified = TRUE;
      m_wndStatusBar.SetText(submap->GetAutoLayoutFlag() ? _T("Automatic") : _T("Manual"),
                             STATUS_PANE_LAYOUT, 0);
	}
}

void CMapFrame::OnUpdateMapAutolayout(CCmdUI* pCmdUI) 
{
	nxSubmap *submap;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(submap->GetAutoLayoutFlag());
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}


//
// WM_CLOSE message handler
//

void CMapFrame::OnClose() 
{
	int ret = IDNO;

	if (m_wndMapView.m_bIsModified)
	{
		ret = MessageBox(_T("Map is modified. Do you wish to save changes?"), _T("Warning"), MB_YESNOCANCEL | MB_ICONQUESTION);
		if (ret == IDYES)
			OnMapSave();
	}
	
	if (ret != IDCANCEL)
		CMDIChildWnd::OnClose();
}


//
// Handler for "Remove link between objects" menu
//

void CMapFrame::OnMapUnlink() 
{
	nxSubmap *submap;
	DWORD *selection, count;
	NXC_OBJECT *object;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		object = NXCFindObjectById(g_hSession, submap->Id());
		if (object != NULL)
		{
			CMapLinkPropDlg dlg;

			dlg.m_dwParentObject = submap->Id();
			selection = m_wndMapView.GetSelectedObjects(&count);
			if (count > 0)
			{
				dlg.m_dwObject1 = selection[0];
				if (count > 1)
				{
					dlg.m_dwObject2 = selection[1];
				}
			}
			safe_free(selection);
			if (dlg.DoModal() == IDOK)
			{
				submap->UnlinkObjects(dlg.m_dwObject1, dlg.m_dwObject2);
				m_wndMapView.m_bIsModified = TRUE;
				m_wndMapView.Update();
			}
		}
	}
}

void CMapFrame::OnUpdateMapUnlink(CCmdUI* pCmdUI) 
{
	nxSubmap *submap;
	NXC_OBJECT *object;

	submap = m_wndMapView.GetSubmap();
	if (submap != NULL)
	{
		object = NXCFindObjectById(g_hSession, submap->Id());
		if (object != NULL)
		{
			pCmdUI->Enable(object->iClass == OBJECT_CONTAINER);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}
