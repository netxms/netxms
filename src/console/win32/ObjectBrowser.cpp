//
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

#define PREVIEW_PANE_WIDTH    200


//
// Compare two items in object tree hash for qsort()
//

static int CompareTreeHashItems(const void *p1, const void *p2)
{
   return ((OBJ_TREE_HASH *)p1)->dwObjectId < ((OBJ_TREE_HASH *)p2)->dwObjectId ? -1 :
            (((OBJ_TREE_HASH *)p1)->dwObjectId > ((OBJ_TREE_HASH *)p2)->dwObjectId ? 1 : 0);
}


//
// Get netmask value depending on object's class
//

static DWORD GetObjectNetMask(NXC_OBJECT *pObject)
{
   DWORD dwMask;

   switch(pObject->iClass)
   {
      case OBJECT_SUBNET:
         dwMask = ntohl(pObject->subnet.dwIpNetMask);
         break;
      case OBJECT_INTERFACE:
         dwMask = ntohl(pObject->iface.dwIpNetMask);
         break;
      default:
         dwMask = 0;
         break;
   }
   return dwMask;
}


//
// Compare two list view items
//

static int CALLBACK CompareListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   CObjectBrowser *pBrowser = (CObjectBrowser *)lParamSort;
   char *pszStr1, *pszStr2;
   DWORD dwNum1, dwNum2;
   int iResult;

   switch(SortMode(lParamSort))
   {
      case OBJECT_SORT_BY_ID:
         iResult = COMPARE_NUMBERS(((NXC_OBJECT *)lParam1)->dwId, ((NXC_OBJECT *)lParam2)->dwId);
         break;
      case OBJECT_SORT_BY_NAME:
         iResult = stricmp(((NXC_OBJECT *)lParam1)->szName, ((NXC_OBJECT *)lParam2)->szName);
         break;
      case OBJECT_SORT_BY_CLASS:
         iResult = COMPARE_NUMBERS(((NXC_OBJECT *)lParam1)->iClass, ((NXC_OBJECT *)lParam2)->iClass);
         break;
      case OBJECT_SORT_BY_STATUS:
         iResult = COMPARE_NUMBERS(((NXC_OBJECT *)lParam1)->iStatus, ((NXC_OBJECT *)lParam2)->iStatus);
         break;
      case OBJECT_SORT_BY_IP:
         dwNum1 = ntohl(((NXC_OBJECT *)lParam1)->dwIpAddr);
         dwNum2 = ntohl(((NXC_OBJECT *)lParam2)->dwIpAddr);
         iResult = COMPARE_NUMBERS(dwNum1, dwNum2);
         break;
      case OBJECT_SORT_BY_MASK:
         dwNum1 = GetObjectNetMask((NXC_OBJECT *)lParam1);
         dwNum2 = GetObjectNetMask((NXC_OBJECT *)lParam2);
         iResult = COMPARE_NUMBERS(dwNum1, dwNum2);
         break;
      case OBJECT_SORT_BY_IFINDEX:
         dwNum1 = (((NXC_OBJECT *)lParam1)->iClass == OBJECT_INTERFACE) ? 
                     ((NXC_OBJECT *)lParam1)->iface.dwIfIndex : 0;
         dwNum2 = (((NXC_OBJECT *)lParam2)->iClass == OBJECT_INTERFACE) ? 
                     ((NXC_OBJECT *)lParam2)->iface.dwIfIndex : 0;
         iResult = COMPARE_NUMBERS(dwNum1, dwNum2);
         break;
      case OBJECT_SORT_BY_OID:
         pszStr1 = (((NXC_OBJECT *)lParam1)->iClass == OBJECT_NODE) ? 
                        ((NXC_OBJECT *)lParam1)->node.szObjectId : "";
         pszStr2 = (((NXC_OBJECT *)lParam2)->iClass == OBJECT_NODE) ? 
                        ((NXC_OBJECT *)lParam2)->node.szObjectId : "";
         iResult = stricmp(pszStr1, pszStr2);
         break;
      default:
         iResult = 0;
         break;
   }

   return (SortDir(lParamSort) == SORT_ASCENDING) ? iResult : -iResult;
}


//
// CObjectBrowser implementation
//

IMPLEMENT_DYNCREATE(CObjectBrowser, CMDIChildWnd)

CObjectBrowser::CObjectBrowser()
{
   m_pImageList = NULL;
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
   m_dwFlags = SHOW_OBJECT_PREVIEW;
   m_dwSortMode = OBJECT_SORT_BY_NAME;
   m_pCurrentObject = NULL;
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
	ON_COMMAND(ID_OBJECT_VIEW_VIEWASLIST, OnObjectViewViewaslist)
	ON_COMMAND(ID_OBJECT_VIEW_VIEWASTREE, OnObjectViewViewastree)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OBJECT_PROPERTIES, OnObjectProperties)
	ON_COMMAND(ID_OBJECT_VIEW_SELECTION, OnObjectViewSelection)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_VIEW_VIEWASLIST, OnUpdateObjectViewViewaslist)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_VIEW_VIEWASTREE, OnUpdateObjectViewViewastree)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_VIEW_SELECTION, OnUpdateObjectViewSelection)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_VIEW_SHOWPREVIEWPANE, OnUpdateObjectViewShowpreviewpane)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_PROPERTIES, OnUpdateObjectProperties)
	ON_COMMAND(ID_OBJECT_DATACOLLECTION, OnObjectDatacollection)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_DATACOLLECTION, OnUpdateObjectDatacollection)
	ON_COMMAND(ID_OBJECT_MANAGE, OnObjectManage)
	ON_COMMAND(ID_OBJECT_UNMANAGE, OnObjectUnmanage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNMANAGE, OnUpdateObjectUnmanage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_MANAGE, OnUpdateObjectManage)
	ON_COMMAND(ID_OBJECT_BIND, OnObjectBind)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_BIND, OnUpdateObjectBind)
	ON_COMMAND(ID_OBJECT_CREATE_CONTAINER, OnObjectCreateContainer)
	ON_COMMAND(ID_OBJECT_CREATE_NODE, OnObjectCreateNode)
	//}}AFX_MSG_MAP
   ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_VIEW, OnTreeViewSelChange)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VIEW, OnListViewItemChange)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_FIND_OBJECT, OnFindObject)
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
   LVCOLUMN lvCol;

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
   m_wndTreeCtrl.Create(WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                        rect, this, IDC_TREE_VIEW);

   // Create list view control
   m_wndListCtrl.Create(WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Setup list view columns
   m_wndListCtrl.InsertColumn(0, "ID", LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(1, "Name", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(2, "Class", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, "Status", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(4, "IP Address", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(5, "IP Netmask", LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(6, "IFIndex", LVCFMT_LEFT, 60);
   m_wndListCtrl.InsertColumn(7, "IFType", LVCFMT_LEFT, 120);
   m_wndListCtrl.InsertColumn(8, "Caps", LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(9, "Object ID", LVCFMT_LEFT, 150);

   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(g_pObjectSmallImageList);
   m_iLastObjectImage = m_pImageList->GetImageCount();
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(AfxGetApp()->LoadIcon(IDI_SORT_DOWN));
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Mark sorting column in list control
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iLastObjectImage;
   m_wndListCtrl.SetColumn(m_dwSortMode, &lvCol);

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
      m_wndListCtrl.SetWindowPos(NULL, PREVIEW_PANE_WIDTH, 0, cx - PREVIEW_PANE_WIDTH, 
                                 cy, SWP_NOZORDER);
   }
   else
   {
      m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
      m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   }
}


//
// WM_SETFOCUS message handler
//

void CObjectBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      m_wndTreeCtrl.SetFocus();	
   else
      m_wndListCtrl.SetFocus();
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

   pObject = NXCGetTopologyRootObject();
   if (pObject != NULL)
      AddObjectToTree(pObject, TVI_ROOT);

   pObject = NXCGetServiceRootObject();
   if (pObject != NULL)
      AddObjectToTree(pObject, TVI_ROOT);
   
   qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);

   // Populate object's list
   m_wndListCtrl.DeleteAllItems();
   NXCLockObjectIndex();
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(&dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
      AddObjectToList(pIndex[i].pObject);
   NXCUnlockObjectIndex();
   m_wndListCtrl.SortItems(CompareListItems, m_dwSortMode);
}


//
// Add new object to tree
//

void CObjectBrowser::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   DWORD i;
   HTREEITEM hItem;
   char szBuffer[512];
   int iImage;

   // Add object record with class-dependent text
   CreateTreeItemText(pObject, szBuffer);
   iImage = GetObjectImageIndex(pObject);
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, iImage, iImage, hParent);
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
      m_wndListCtrl.SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);
   }
   else
   {
      m_dwFlags |= SHOW_OBJECT_PREVIEW;
      m_wndTreeCtrl.SetWindowPos(NULL, PREVIEW_PANE_WIDTH, 0, rect.right - PREVIEW_PANE_WIDTH,
                                 rect.bottom, SWP_NOZORDER);
      m_wndListCtrl.SetWindowPos(NULL, PREVIEW_PANE_WIDTH, 0, rect.right - PREVIEW_PANE_WIDTH,
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
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      m_pCurrentObject = NXCFindObjectById(lpnmt->itemNew.lParam);
      m_wndPreviewPane.SetCurrentObject(m_pCurrentObject);
   }
}


//
// WM_OBJECT_CHANGE message handler
// wParam contains object's ID, and lParam pointer to corresponding NXC_OBJECT structure
//

void CObjectBrowser::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   UpdateObjectTree(wParam, (NXC_OBJECT *)lParam);
   UpdateObjectList((NXC_OBJECT *)lParam);
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
         safe_free(pdwParentList);
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

      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      {
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
}


//
// Add new object to objects' list
//

void CObjectBrowser::AddObjectToList(NXC_OBJECT *pObject)
{
   int iItem;
   char szBuffer[16];

   sprintf(szBuffer, "%ld", pObject->dwId);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, GetObjectImageIndex(pObject));
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, (LPARAM)pObject);
      UpdateObjectListEntry(iItem, pObject);
   }
}


//
// WM_COMMAND::ID_OBJECT_VIEW_VIEWASLIST message handler
//

void CObjectBrowser::OnObjectViewViewaslist() 
{
   int iItem;

   m_dwFlags &= ~VIEW_OBJECTS_AS_TREE;
   m_wndListCtrl.ShowWindow(SW_SHOW);
   m_wndTreeCtrl.ShowWindow(SW_HIDE);

   // Display currenly selected item in preview pane
   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   m_pCurrentObject = (iItem == -1) ? NULL : (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
   m_wndPreviewPane.SetCurrentObject(m_pCurrentObject);
}


//
// WM_COMMAND::ID_OBJECT_VIEW_VIEWASTREE message handler
//

void CObjectBrowser::OnObjectViewViewastree() 
{
   HTREEITEM hItem;

   m_dwFlags |= VIEW_OBJECTS_AS_TREE;
   m_wndTreeCtrl.ShowWindow(SW_SHOW);
   m_wndListCtrl.ShowWindow(SW_HIDE);

   // Display currenly selected item in preview pane
   hItem = m_wndTreeCtrl.GetSelectedItem();
   m_pCurrentObject = (hItem == NULL) ? NULL : NXCFindObjectById(m_wndTreeCtrl.GetItemData(hItem));
   m_wndPreviewPane.SetCurrentObject(m_pCurrentObject);
}


//
// WM_NOTIFY::LVN_COLUMNCLICK message handler
//

void CObjectBrowser::OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;
   int iColumn;

   // Unmark old sorting column
   iColumn = SortMode(m_dwSortMode);
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(iColumn, &lvCol);

   // Change current sort mode and resort list
   if (iColumn == pNMHDR->iSubItem)
   {
      // Same column, change sort direction
      m_dwSortMode = iColumn | 
         (((SortDir(m_dwSortMode) == SORT_ASCENDING) ? SORT_DESCENDING : SORT_ASCENDING) << 8);
   }
   else
   {
      // Another sorting column
      m_dwSortMode = (m_dwSortMode & 0xFFFFFF00) | pNMHDR->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItems, m_dwSortMode);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (SortDir(m_dwSortMode) == SORT_ASCENDING) ? m_iLastObjectImage : (m_iLastObjectImage + 1);
   m_wndListCtrl.SetColumn(pNMHDR->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// WM_NOTIFY::LVN_ITEMACTIVATE message handler
//

void CObjectBrowser::OnListViewItemChange(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   if (!(m_dwFlags & VIEW_OBJECTS_AS_TREE))
   {
      if (pNMHDR->iItem != -1)
         if (pNMHDR->uChanged & LVIF_STATE)
         {
            if (pNMHDR->uNewState & LVIS_FOCUSED)
            {
               m_pCurrentObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(pNMHDR->iItem);
            }
            else
            {
               m_pCurrentObject = NULL;
            }
            m_wndPreviewPane.SetCurrentObject(m_pCurrentObject);
         }
   }   
   *pResult = 0;
}


//
// WM_FIND_OBJECT message handler
//

void CObjectBrowser::OnFindObject(WPARAM wParam, LPARAM lParam)
{
   if (*((char *)lParam) != 0)
   {
      NXC_OBJECT *pObject;

      pObject = NXCFindObjectByName((char *)lParam);
      if (pObject != NULL)
      {
         // Object found, select it in the current view
         if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         {
            DWORD dwIndex;

            dwIndex = FindObjectInTree(pObject->dwId);
            if (dwIndex != INVALID_INDEX)
               m_wndTreeCtrl.Select(m_pTreeHash[dwIndex].hTreeItem, TVGN_CARET);
            else
               MessageBox("Object found, but it's hidden and cannot be displayed at the moment",
                          "Find", MB_OK | MB_ICONEXCLAMATION);
            m_wndTreeCtrl.SetFocus();
         }
         else
         {
            LVFINDINFO lvfi;
            int iItem, iOldItem;

            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = (LPARAM)pObject;
            iItem = m_wndListCtrl.FindItem(&lvfi);
            if (iItem != -1)
            {
               ClearListSelection();
               iOldItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
               if (iOldItem != -1)
                  m_wndListCtrl.SetItemState(iOldItem, 0, LVIS_FOCUSED);
               m_wndListCtrl.EnsureVisible(iItem, FALSE);
               m_wndListCtrl.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, 
                                          LVIS_SELECTED | LVIS_FOCUSED);
            }
            else
            {
               MessageBox("Object found, but it's hidden and cannot be displayed at the moment",
                          "Find", MB_OK | MB_ICONEXCLAMATION);
            }
            m_wndListCtrl.SetFocus();
         }
      }
      else
      {
         char szBuffer[384];

         sprintf(szBuffer, "Matching object for pattern \"%s\" not found", lParam);
         MessageBox(szBuffer, "Find", MB_OK | MB_ICONSTOP);
      }
   }
}


//
// Clear selection from list control
//

void CObjectBrowser::ClearListSelection()
{
   int iItem;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      m_wndListCtrl.SetItemState(iItem, 0, LVIS_SELECTED);
      iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
   }
}


//
// WM_CONTEXTMENU message handler
//

void CObjectBrowser::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
   CPoint pt;

   pt = point;
   pWnd->ScreenToClient(&pt);

   if (pWnd->GetDlgCtrlID() == IDC_LIST_VIEW)
   {
      int iItem;
      UINT uFlags;

      iItem = m_wndListCtrl.HitTest(pt, &uFlags);
      if ((iItem != -1) && (uFlags & LVHT_ONITEM))
      {
         pMenu = theApp.GetContextMenu(1);
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
      }
   }
   else if (pWnd->GetDlgCtrlID() == IDC_TREE_VIEW)
   {
      HTREEITEM hItem;
      UINT uFlags;

      hItem = m_wndTreeCtrl.HitTest(pt, &uFlags);
      if ((hItem != NULL) && (uFlags & TVHT_ONITEM))
      {
         m_wndTreeCtrl.Select(hItem, TVGN_CARET);

         pMenu = theApp.GetContextMenu(1);
         pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_PROPERTIES message handler
//

void CObjectBrowser::OnObjectProperties() 
{
   DWORD dwId;

   dwId = GetSelectedObject();
   if (dwId != 0)
      theApp.ObjectProperties(dwId);
}


//
// Get ID of currently selected object
// Will return 0 if there are no selection
//

DWORD CObjectBrowser::GetSelectedObject()
{
   DWORD dwId = 0;

   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      HTREEITEM hItem;

      hItem = m_wndTreeCtrl.GetSelectedItem();
      if (hItem != NULL)
         dwId = m_wndTreeCtrl.GetItemData(hItem);
   }
   else
   {
      if (m_wndListCtrl.GetSelectedCount() == 1)
      {
         int iItem;
         NXC_OBJECT *pObject;

         iItem = m_wndListCtrl.GetSelectionMark();
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         if (pObject != NULL)
            dwId = pObject->dwId;
      }
   }
   return dwId;
}


//
// Update object list with new object information
//

void CObjectBrowser::UpdateObjectList(NXC_OBJECT *pObject)
{
   LVFINDINFO lvfi;
   int iItem;

   // Find object in list
   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = (LPARAM)pObject;
   iItem = m_wndListCtrl.FindItem(&lvfi);

   if (pObject->bIsDeleted)
   {
      if (iItem != -1)
         m_wndListCtrl.DeleteItem(iItem);
   }
   else
   {
      if (iItem != -1)
      {
         UpdateObjectListEntry(iItem, pObject);
      }
      else
      {
         AddObjectToList(pObject);
      }
      m_wndListCtrl.SortItems(CompareListItems, m_dwSortMode);

      if (!(m_dwFlags & VIEW_OBJECTS_AS_TREE))
      {
         if (m_dwFlags & FOLLOW_OBJECT_UPDATES)
         {
            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = (LPARAM)pObject;
            iItem = m_wndListCtrl.FindItem(&lvfi);
            if (iItem != -1)    // Shouldn't happen
            {
               int iOldItem;

               ClearListSelection();
               iOldItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
               if (iOldItem != -1)
                  m_wndListCtrl.SetItemState(iOldItem, 0, LVIS_FOCUSED);
               m_wndListCtrl.EnsureVisible(iItem, FALSE);
               m_wndListCtrl.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, 
                                          LVIS_SELECTED | LVIS_FOCUSED);
            }
         }
         else
         {
            int iCurrItem;

            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = (LPARAM)pObject;
            iItem = m_wndListCtrl.FindItem(&lvfi);

            // Check if current item has been changed
            iCurrItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
            if (iItem == iCurrItem)
            {
               m_wndPreviewPane.Refresh();
               m_wndListCtrl.EnsureVisible(iItem, FALSE);
            }
         }
      }
   }
}


//
// Replace specific list control item with new object's data
//

void CObjectBrowser::UpdateObjectListEntry(int iItem, NXC_OBJECT *pObject)
{
   char szBuffer[64];
   int iPos;

   m_wndListCtrl.SetItemText(iItem, 1, pObject->szName);
   m_wndListCtrl.SetItemText(iItem, 2, g_szObjectClass[pObject->iClass]);
   m_wndListCtrl.SetItemText(iItem, 3, g_szStatusTextSmall[pObject->iStatus]);
   m_wndListCtrl.SetItemText(iItem, 4, IpToStr(pObject->dwIpAddr, szBuffer));

   // Class-specific fields
   switch(pObject->iClass)
   {
      case OBJECT_SUBNET:
         m_wndListCtrl.SetItemText(iItem, 5, IpToStr(pObject->subnet.dwIpNetMask, szBuffer));
         break;
      case OBJECT_INTERFACE:
         m_wndListCtrl.SetItemText(iItem, 5, IpToStr(pObject->iface.dwIpNetMask, szBuffer));
         sprintf(szBuffer, "%d", pObject->iface.dwIfIndex);
         m_wndListCtrl.SetItemText(iItem, 6, szBuffer);
         m_wndListCtrl.SetItemText(iItem, 7, pObject->iface.dwIfType > MAX_INTERFACE_TYPE ?
                                     "Unknown" : g_szInterfaceTypes[pObject->iface.dwIfType]);
         break;
      case OBJECT_NODE:
         // Create node capabilities string
         iPos = 0;
         if (pObject->node.dwFlags & NF_IS_NATIVE_AGENT)
            szBuffer[iPos++] = 'A';
         if (pObject->node.dwFlags & NF_IS_SNMP)
            szBuffer[iPos++] = 'S';
         if (pObject->node.dwFlags & NF_IS_LOCAL_MGMT)
            szBuffer[iPos++] = 'M';
         szBuffer[iPos] = 0;
         m_wndListCtrl.SetItemText(iItem, 8, szBuffer);
         m_wndListCtrl.SetItemText(iItem, 9, pObject->node.szObjectId);
         break;
      default:
         break;
   }
}


//
// Change "selection follow changes" mode
//

void CObjectBrowser::OnObjectViewSelection() 
{
   if (m_dwFlags & FOLLOW_OBJECT_UPDATES)
      m_dwFlags &= ~FOLLOW_OBJECT_UPDATES;
   else
      m_dwFlags |= FOLLOW_OBJECT_UPDATES;
}


//
// ON_COMMAND_UPDATE_UI handlers
//

void CObjectBrowser::OnUpdateObjectViewViewaslist(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(!(m_dwFlags & VIEW_OBJECTS_AS_TREE));
}

void CObjectBrowser::OnUpdateObjectViewViewastree(CCmdUI* pCmdUI) 
{
	pCmdUI->SetRadio(m_dwFlags & VIEW_OBJECTS_AS_TREE);
}

void CObjectBrowser::OnUpdateObjectViewSelection(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_dwFlags & FOLLOW_OBJECT_UPDATES);
}

void CObjectBrowser::OnUpdateObjectViewShowpreviewpane(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck(m_dwFlags & SHOW_OBJECT_PREVIEW);
}

void CObjectBrowser::OnUpdateObjectProperties(CCmdUI* pCmdUI) 
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      pCmdUI->Enable(m_pCurrentObject != NULL);
   else
      pCmdUI->Enable((m_pCurrentObject != NULL) &&
                     (m_wndListCtrl.GetSelectedCount() == 1));
}

void CObjectBrowser::OnUpdateObjectDatacollection(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         pCmdUI->Enable(m_pCurrentObject->iClass == OBJECT_NODE);
      else
         pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_NODE) &&
                        (m_wndListCtrl.GetSelectedCount() == 1));
   }
}

void CObjectBrowser::OnUpdateObjectUnmanage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}

void CObjectBrowser::OnUpdateObjectManage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}

void CObjectBrowser::OnUpdateObjectBind(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                        (m_pCurrentObject->iClass == OBJECT_SERVICEROOT));
      else
         pCmdUI->Enable(((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                         (m_pCurrentObject->iClass == OBJECT_SERVICEROOT)) &&
                        (m_wndListCtrl.GetSelectedCount() == 1));
   }
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CObjectBrowser::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_OBJECT_PROPERTIES, 0);
}


//
// WM_COMMAND::ID_OBJECT_DATACOLLECTION message handler
//

void CObjectBrowser::OnObjectDatacollection() 
{
   if (m_pCurrentObject != NULL)
      theApp.StartObjectDCEditor(m_pCurrentObject);
}


//
// WM_COMMAND::ID_OBJECT_MANAGE message handler
//

void CObjectBrowser::OnObjectManage() 
{
   ChangeMgmtStatus(TRUE);
}


//
// WM_COMMAND::ID_OBJECT_UNMANAGE message handler
//

void CObjectBrowser::OnObjectUnmanage() 
{
   ChangeMgmtStatus(FALSE);
}


//
// Change management status for currently selected object(s)
//

void CObjectBrowser::ChangeMgmtStatus(BOOL bIsManaged)
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.SetObjectMgmtStatus(m_pCurrentObject, bIsManaged);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.SetObjectMgmtStatus(pObject, bIsManaged);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_BIND message handler
//

void CObjectBrowser::OnObjectBind() 
{
   CObjectSelDlg dlg;
   DWORD i, dwResult;

   dlg.m_dwAllowedClasses = SCL_NODE | SCL_CONTAINER | SCL_SUBNET;
   if (dlg.DoModal() == IDOK)
   {
      for(i = 0; i < dlg.m_dwNumObjects; i++)
      {
         dwResult = DoRequestArg2(NXCBindObject, (void *)m_pCurrentObject->dwId,
                                  (void *)dlg.m_pdwObjectList[i], "Binding objects...");
         if (dwResult != RCC_SUCCESS)
         {
            theApp.ErrorBox(dwResult, "Cannot bind object: %s");
            break;
         }
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_CREATE_CONTAINER message handler
//

void CObjectBrowser::OnObjectCreateContainer() 
{
   theApp.CreateObject(OBJECT_CONTAINER, 
                       (m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_NODE message handler
//

void CObjectBrowser::OnObjectCreateNode() 
{
   theApp.CreateObject(OBJECT_NODE, 
                       (m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}
