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
   m_pRootObject = NULL;
   m_pImageList = NULL;
   m_dwHistoryPos = 0;
}

CMapFrame::~CMapFrame()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CMapFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CMapFrame)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_OBJECT_OPEN, OnObjectOpen)
	ON_COMMAND(ID_OBJECT_OPENPARENT, OnObjectOpenparent)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
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
   int i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create and initialize map view
   GetClientRect(&rect);
   //m_wndMapView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
   m_wndMapView.Create(WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_SHOWSELALWAYS, rect, this, IDC_LIST_VIEW);
   
   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(g_pObjectNormalImageList);
   m_iStatusImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_WARNING));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_MINOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_MAJOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_CRITICAL));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_UNKNOWN));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_UNMANAGED));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_DISABLED));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_TESTING));
   for(i = STATUS_WARNING; i <= STATUS_TESTING; i++)
      m_pImageList->SetOverlayImage(m_iStatusImageBase + i - 1, i);
   m_wndMapView.SetImageList(m_pImageList, LVSIL_NORMAL);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// WM_SIZE message handler
//

void CMapFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndMapView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);	
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
   NXC_OBJECT *pObject;
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

   ::SetWindowText(m_hWnd, strTitle);
}


//
// Add object to view
//

void CMapFrame::AddObjectToView(NXC_OBJECT *pObject)
{
   LVITEM item;

   item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
   item.iItem = 0x7FFFFFFF;
   item.iSubItem = 0;
   item.pszText = pObject->szName;
   item.iImage = GetObjectImageIndex(pObject);
   item.lParam = (LPARAM)pObject;
   item.stateMask = LVIS_OVERLAYMASK;
   item.state = INDEXTOOVERLAYMASK(pObject->iStatus);
   m_wndMapView.InsertItem(&item);
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CMapFrame::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_OBJECT_OPEN, 0);
}


//
// WM_COMMAND::ID_OBJECT_OPEN message handler
//

void CMapFrame::OnObjectOpen() 
{
   NXC_OBJECT *pObject;

   pObject = GetSelectedObject();
   if (pObject != NULL)
   {
      if (m_dwHistoryPos < OBJECT_HISTORY_SIZE)
         m_pObjectHistory[m_dwHistoryPos++] = m_pRootObject;
      m_pRootObject = pObject;
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   }
}


//
// WM_COMMAND::ID_OBJECT_OPENPARENT message handler
//

void CMapFrame::OnObjectOpenparent() 
{
   if (m_pRootObject != NULL)
   {
      if (m_dwHistoryPos > 0)
      {
         m_pRootObject = m_pObjectHistory[--m_dwHistoryPos];
      }
      else
      {
         if (m_pRootObject->dwNumParents > 0)
         {
            m_pRootObject = NXCFindObjectById(g_hSession, m_pRootObject->pdwParentList[0]);
         }
         else
         {
            m_pRootObject = NULL;
         }
      }
      PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
   }
}


//
// Get currently selected object, if any
//

NXC_OBJECT *CMapFrame::GetSelectedObject()
{
   int iItem;
   NXC_OBJECT *pObject = NULL;

   if (m_wndMapView.GetSelectedCount() == 1)
   {
      iItem = m_wndMapView.GetSelectionMark();
      pObject = (NXC_OBJECT *)m_wndMapView.GetItemData(iItem);
   }
   return pObject;
}
