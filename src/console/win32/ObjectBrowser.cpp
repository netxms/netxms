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
   m_dwFlags = SHOW_OBJECT_PREVIEW | FOLLOW_OBJECT_UPDATES;
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
   m_wndPreviewPane.Create(WS_CHILD | ((m_dwFlags & SHOW_OBJECT_PREVIEW) ? WS_VISIBLE : 0),
                           rect, this, IDC_PREVIEW_PANE);

   // Create tree view control
   GetClientRect(&rect);
   if (m_dwFlags & SHOW_OBJECT_PREVIEW)
   {
      rect.left = PREVIEW_PANE_WIDTH;
      rect.right -= PREVIEW_PANE_WIDTH;
   }
   m_wndTreeCtrl.Create(WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                        rect, this, IDC_TREE_VIEW);

   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup list view columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, "Class", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "IP Address", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(4, "IP Netmask", LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(5, "IFIndex", LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(6, "IFType", LVCFMT_LEFT, 70);

   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NETMAP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_INTERFACE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_NODE));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SUBNET));
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      m_wndTreeCtrl.ShowWindow(SW_SHOW);
   else
      m_wndListCtrl.ShowWindow(SW_SHOW);

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
	
   if (m_dwFlags & SHOW_OBJECT_PREVIEW)
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
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, dwNumObjects;
   
   // Populate objects' tree
   m_wndTreeCtrl.DeleteAllItems();
   m_dwTreeHashSize = 0;
   pObject = NXCGetRootObject();
   if (pObject != NULL)
   {
      AddObjectToTree(pObject, TVI_ROOT);
      qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
   }

   // Populate object's list
   NXCLockObjectIndex();
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(&dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
      AddObjectToList(pIndex[i].pObject);
   NXCUnlockObjectIndex();
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

   if (m_dwFlags & SHOW_OBJECT_PREVIEW)
   {
      m_dwFlags &= ~SHOW_OBJECT_PREVIEW;
      m_wndPreviewPane.ShowWindow(SW_HIDE);
      m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);
   }
   else
   {
      m_dwFlags |= SHOW_OBJECT_PREVIEW;
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
   UpdateObjectTree(wParam, (NXC_OBJECT *)lParam);
}


//
// Perform binary search on tree hash
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pHash == NULL will not be passed
//

static DWORD SearchTreeHash(OBJ_TREE_HASH *pBase, DWORD dwHashSize, DWORD dwKey)
{
   DWORD dwFirst, dwLast, dwMid;

   dwFirst = 0;
   dwLast = dwHashSize - 1;

   if ((dwKey < pBase[0].dwObjectId) || (dwKey > pBase[dwLast].dwObjectId))
      return INVALID_INDEX;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwKey == pBase[dwMid].dwObjectId)
         return dwMid;
      if (dwKey < pBase[dwMid].dwObjectId)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwKey == pBase[dwLast].dwObjectId)
      return dwLast;

   return INVALID_INDEX;
}


//
// Find object's tree item for give object's id
//

DWORD CObjectBrowser::FindObjectInTree(DWORD dwObjectId)
{
   DWORD dwIndex;

   dwIndex = SearchTreeHash(m_pTreeHash, m_dwTreeHashSize, dwObjectId);
   while((dwIndex > 0) && (m_pTreeHash[dwIndex - 1].dwObjectId == dwObjectId))
      dwIndex--;
   return dwIndex;
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


//
// Delete tree item and appropriate record in hash
//

void CObjectBrowser::DeleteObjectTreeItem(HTREEITEM hRootItem)
{
   HTREEITEM hItem;
   DWORD dwIndex, dwId;

   // Delete all child items
   hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   while(hItem != NULL)
   {
      DeleteObjectTreeItem(hItem);
      hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   }

   // Find hash record for current item
   dwId = m_wndTreeCtrl.GetItemData(hRootItem);
   dwIndex = FindObjectInTree(dwId);
   if (dwIndex != INVALID_INDEX)
   {
      while((dwIndex < m_dwTreeHashSize) && (m_pTreeHash[dwIndex].hTreeItem != hRootItem))
         dwIndex++;
      // Delete appropriate record in tree hash list
      if (dwIndex < m_dwTreeHashSize - 1)
         memmove(&m_pTreeHash[dwIndex], &m_pTreeHash[dwIndex + 1], 
                 sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize - dwIndex - 1));
      m_dwTreeHashSize--;
   }

   // Delete item from tree control
   m_wndTreeCtrl.DeleteItem(hRootItem);
}


//
// Update objects' tree when we receive WM_OBJECT_CHANGE message
//

void CObjectBrowser::UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject)
{
   DWORD i, j, dwIndex;

   // Find object in tree
   dwIndex = FindObjectInTree(dwObjectId);

   if (pObject->bIsDeleted)
   {
      // Delete all tree items
      while(dwIndex != INVALID_INDEX)
      {
         DeleteObjectTreeItem(m_pTreeHash[dwIndex].hTreeItem);
         dwIndex = FindObjectInTree(dwObjectId);
      }
   }
   else
   {
      HTREEITEM hItem;

      if (dwIndex != INVALID_INDEX)
      {
         char szBuffer[256];
         DWORD j, dwId, *pdwParentList;

         // Create a copy of object's parent list
         pdwParentList = (DWORD *)nx_memdup(pObject->pdwParentList, 
                                            sizeof(DWORD) * pObject->dwNumParents);

         CreateTreeItemText(pObject, szBuffer);
restart_parent_check:;
         for(i = dwIndex; (i < m_dwTreeHashSize) && (m_pTreeHash[i].dwObjectId == dwObjectId); i++)
         {
            // Check if this item's parent still in object's parents list
            hItem = m_wndTreeCtrl.GetParentItem(m_pTreeHash[i].hTreeItem);
            if (hItem != NULL)
            {
               dwId = m_wndTreeCtrl.GetItemData(hItem);
               for(j = 0; j < pObject->dwNumParents; j++)
                  if (pObject->pdwParentList[j] == dwId)
                  {
                     pdwParentList[j] = 0;   // Mark this parent as presented
                     break;
                  }
               if (j == pObject->dwNumParents)  // Not a parent anymore
               {
                  // Delete this tree item
                  DeleteObjectTreeItem(m_pTreeHash[i].hTreeItem);
                  goto restart_parent_check;   // Restart loop, because m_dwTreeHashSize was changed
               }
               else  // Current tree item is still valid
               {
                  m_wndTreeCtrl.SetItemText(m_pTreeHash[i].hTreeItem, szBuffer);
               }
            }
            else  // Current tree item has no parent
            {
               m_wndTreeCtrl.SetItemText(m_pTreeHash[i].hTreeItem, szBuffer);
            }
         }

         // Now walk through all object's parents which hasn't corresponding
         // items in tree view
         for(i = 0; i < pObject->dwNumParents; i++)
            if (pdwParentList[i] != 0)
            {
               dwIndex = FindObjectInTree(pdwParentList[i]);
               if (dwIndex != INVALID_INDEX)
               {
                  // Walk through all occurences of current parent object
                  for(j = dwIndex; (j < m_dwTreeHashSize) && 
                                   (m_pTreeHash[j].dwObjectId == pdwParentList[i]); j++)
                     AddObjectToTree(pObject, m_pTreeHash[j].hTreeItem);
                  qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
               }
            }

         // Destroy copy of object's parents list
         MemFree(pdwParentList);
      }
      else
      {
         // New object, link to all parents
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            dwIndex = FindObjectInTree(pObject->pdwParentList[i]);
            if (dwIndex != INVALID_INDEX)
            {
               // Walk through all occurences of current parent object
               for(j = dwIndex; (j < m_dwTreeHashSize) && 
                                (m_pTreeHash[j].dwObjectId == pObject->pdwParentList[i]); j++)
                  AddObjectToTree(pObject, m_pTreeHash[j].hTreeItem);
               qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
            }
         }
      }

      if (m_dwFlags & FOLLOW_OBJECT_UPDATES)
      {
         dwIndex = FindObjectInTree(dwObjectId);
         if (dwIndex != INVALID_INDEX)    // Shouldn't happen
            m_wndTreeCtrl.Select(m_pTreeHash[dwIndex].hTreeItem, TVGN_CARET);
      }
      else
      {
         // Check if current item has been changed
         hItem = m_wndTreeCtrl.GetSelectedItem();
         if (m_wndTreeCtrl.GetItemData(hItem) == dwObjectId)
            m_wndPreviewPane.Refresh();
      }
   }
}


//
// Add new object to objects' list
//

void CObjectBrowser::AddObjectToList(NXC_OBJECT *pObject)
{

}
