// ObjectBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Constants
//

#define PREVIEW_PANE_WIDTH    230


//
// CObjectBrowser implementation
//

IMPLEMENT_DYNCREATE(CObjectBrowser, CMDIChildWnd)

CObjectBrowser::CObjectBrowser()
{
   m_pImageList = NULL;
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
}

CObjectBrowser::~CObjectBrowser()
{
   delete m_pImageList;
   safe_free(m_pTreeHash);
}


BEGIN_MESSAGE_MAP(CObjectBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CObjectBrowser)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_OBJECT_VIEW_SHOWPREVIEWPANE, OnObjectViewShowpreviewpane)
	//}}AFX_MSG_MAP
	ON_NOTIFY(NM_RCLICK, IDC_TREE_VIEW, OnRclickTreeView)
   ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_VIEW, OnTreeViewSelChange)
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser message handlers

void CObjectBrowser::OnDestroy() 
{
   ((CConsoleApp *)AfxGetApp())->OnViewDestroy(IDR_OBJECTS, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CREATE message handler
//

int CObjectBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create preview pane
   GetClientRect(&rect);
   rect.right = PREVIEW_PANE_WIDTH;
   m_wndPreviewPane.Create(WS_CHILD | ((g_dwFlags & AF_SHOW_OBJECT_PREVIEW) ? WS_VISIBLE : 0),
                           rect, this, IDC_PREVIEW_PANE);

   // Create list view control
   GetClientRect(&rect);
   if (g_dwFlags & AF_SHOW_OBJECT_PREVIEW)
   {
      rect.left = PREVIEW_PANE_WIDTH;
      rect.right -= PREVIEW_PANE_WIDTH;
   }
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                        rect, this, IDC_TREE_VIEW);

   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NETMAP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_INTERFACE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NODE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SUBNET));
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);

   ((CConsoleApp *)AfxGetApp())->OnViewCreate(IDR_OBJECTS, this);
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   if (g_dwFlags & AF_SHOW_OBJECT_PREVIEW)
   {
      m_wndPreviewPane.SetWindowPos(NULL, 0, 0, PREVIEW_PANE_WIDTH, cy, SWP_NOZORDER);
      m_wndTreeCtrl.SetWindowPos(NULL, PREVIEW_PANE_WIDTH, 0, cx - PREVIEW_PANE_WIDTH, 
                                 cy, SWP_NOZORDER);
   }
   else
   {
      m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   }
}


//
// WM_SETFOCUS message handler
//

void CObjectBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   m_wndTreeCtrl.SetFocus();	
}


//
// Compare two items in object tree hash for qsort()
//

static int CompareTreeHashItems(const void *p1, const void *p2)
{
   return ((OBJ_TREE_HASH *)p1)->dwObjectId < ((OBJ_TREE_HASH *)p2)->dwObjectId ? -1 :
            (((OBJ_TREE_HASH *)p1)->dwObjectId > ((OBJ_TREE_HASH *)p2)->dwObjectId ? 1 : 0);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CObjectBrowser::OnViewRefresh() 
{
   NXC_OBJECT *pObject;
   
   m_wndTreeCtrl.DeleteAllItems();
   m_dwTreeHashSize = 0;
   pObject = NXCGetRootObject();
   if (pObject != NULL)
   {
      AddObjectToTree(pObject, TVI_ROOT);
      qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
   }
}


//
// Add new object to tree
//

void CObjectBrowser::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   DWORD i;
   HTREEITEM hItem;
   char szBuffer[512];
   static int iImageCode[] = { -1, 3, 2, 1, 0 };

   // Add object record with class-dependent text
   CreateTreeItemText(pObject, szBuffer);
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, iImageCode[pObject->iClass], iImageCode[pObject->iClass], hParent);
   m_wndTreeCtrl.SetItemData(hItem, pObject->dwId);

   // Add to hash
   m_pTreeHash = (OBJ_TREE_HASH *)realloc(m_pTreeHash, sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize + 1));
   m_pTreeHash[m_dwTreeHashSize].dwObjectId = pObject->dwId;
   m_pTreeHash[m_dwTreeHashSize].hTreeItem = hItem;
   m_dwTreeHashSize++;

   // Add object's childs
   for(i = 0; i < pObject->dwNumChilds; i++)
   {
      NXC_OBJECT *pChildObject = NXCFindObjectById(pObject->pdwChildList[i]);
      if (pChildObject != NULL)
         AddObjectToTree(pChildObject, hItem);
   }

   // Sort childs
   m_wndTreeCtrl.SortChildren(hItem);
}


//
// Overloaded PreCreateWindow() method
//

BOOL CObjectBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_TREE));
	return CMDIChildWnd::PreCreateWindow(cs);
}

void CObjectBrowser::OnRclickTreeView(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
}


//
// WM_GETMINMAXINFO message handler
//

void CObjectBrowser::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	CMDIChildWnd::OnGetMinMaxInfo(lpMMI);
   lpMMI->ptMinTrackSize.x = 350;
   lpMMI->ptMinTrackSize.y = 250;
}


//
// WM_COMMAND::ID_OBJECT_VIEW_SHOWPREVIEWPANE message handler
//

void CObjectBrowser::OnObjectViewShowpreviewpane() 
{
   RECT rect;

   GetClientRect(&rect);

   if (g_dwFlags & AF_SHOW_OBJECT_PREVIEW)
   {
      g_dwFlags &= ~AF_SHOW_OBJECT_PREVIEW;
      m_wndPreviewPane.ShowWindow(SW_HIDE);
      m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);
   }
   else
   {
      g_dwFlags |= AF_SHOW_OBJECT_PREVIEW;
      m_wndTreeCtrl.SetWindowPos(NULL, PREVIEW_PANE_WIDTH, 0, rect.right - PREVIEW_PANE_WIDTH,
                                 rect.bottom, SWP_NOZORDER);
      m_wndPreviewPane.SetWindowPos(NULL, 0, 0, PREVIEW_PANE_WIDTH, rect.bottom, SWP_NOZORDER);
      m_wndPreviewPane.ShowWindow(SW_SHOW);
   }
}


//
// WM_NOTIFY::TVN_SELCHANGED message handler
//

void CObjectBrowser::OnTreeViewSelChange(LPNMTREEVIEW lpnmt)
{
   NXC_OBJECT *pObject;

   pObject = NXCFindObjectById(lpnmt->itemNew.lParam);
   m_wndPreviewPane.SetCurrentObject(pObject);
}


//
// WM_OBJECT_CHANGE message handler
// wParam contains object's ID, and lParam pointer to corresponding NXC_OBJECT structure
//

void CObjectBrowser::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   NXC_OBJECT *pObject = (NXC_OBJECT *)lParam;
   HTREEITEM hItem;

   // Find object in tree
   hItem = FindObjectInTree(wParam);

   if (pObject->bIsDeleted)
   {
      if (hItem != NULL)
         m_wndTreeCtrl.DeleteItem(hItem);
   }
   else
   {
      if (hItem != NULL)
      {
         char szBuffer[256];

         if (g_dwFlags & AF_FOLLOW_OBJECT_UPDATES)
            m_wndTreeCtrl.Select(hItem, TVGN_CARET);
         CreateTreeItemText(pObject, szBuffer);
         m_wndTreeCtrl.SetItemText(hItem, szBuffer);

         // Update preview pane
         m_wndPreviewPane.Invalidate();
         m_wndPreviewPane.UpdateWindow();
      }
      else
      {
         DWORD i;

         // New object, link to all parents
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            hItem = FindObjectInTree(pObject->pdwParentList[i]);
            if (hItem != NULL)
            {
               AddObjectToTree(pObject, hItem);
               qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
            }
         }
      }
   }
}


//
// Find object's tree item for give object's id
//

HTREEITEM CObjectBrowser::FindObjectInTree(DWORD dwObjectId)
{
   OBJ_TREE_HASH *pHashElement;

   pHashElement = (OBJ_TREE_HASH *)bsearch(&dwObjectId, m_pTreeHash, m_dwTreeHashSize,
                                           sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
   return (pHashElement == NULL) ? NULL : pHashElement->hTreeItem;
}


//
// Create class-depemdent text for tree item
//

void CObjectBrowser::CreateTreeItemText(NXC_OBJECT *pObject, char *pszBuffer)
{
   char szIpBuffer[32];

   switch(pObject->iClass)
   {
      case OBJECT_SUBNET:
         sprintf(pszBuffer, "%s [Status: %s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      case OBJECT_INTERFACE:
         if (pObject->dwIpAddr != 0)
            sprintf(pszBuffer, "%s [IP: %s/%d Status: %s]", pObject->szName, 
                    IpToStr(pObject->dwIpAddr, szIpBuffer), 
                    BitsInMask(pObject->iface.dwIpNetMask), g_szStatusText[pObject->iStatus]);
         else
            sprintf(pszBuffer, "%s [Status: %s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      default:
         strcpy(pszBuffer, pObject->szName);
         break;
   }
}
