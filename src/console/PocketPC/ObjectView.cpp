// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "ObjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Compare two items in object tree hash for qsort()
//

static int CompareTreeHashItems(const void *p1, const void *p2)
{
   return ((OBJ_TREE_HASH *)p1)->dwObjectId < ((OBJ_TREE_HASH *)p2)->dwObjectId ? -1 :
            (((OBJ_TREE_HASH *)p1)->dwObjectId > ((OBJ_TREE_HASH *)p2)->dwObjectId ? 1 : 0);
}


/////////////////////////////////////////////////////////////////////////////
// CObjectView

CObjectView::CObjectView()
{
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
}

CObjectView::~CObjectView()
{
   safe_free(m_pTreeHash);
}


BEGIN_MESSAGE_MAP(CObjectView, CWnd)
	//{{AFX_MSG_MAP(CObjectView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectView message handlers


//
// WM_CREATE message handler
//

int CObjectView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   GetClientRect(&rect);
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | 
                        TVS_HASBUTTONS | TVS_LINESATROOT, rect, this, ID_TREE_CTRL);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR | ILC_MASK, 12, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_UNKNOWN));    // For OBJECT_GENERIC
   m_imageList.Add(theApp.LoadIcon(IDI_SUBNET));
   m_imageList.Add(theApp.LoadIcon(IDI_NODE));
   m_imageList.Add(theApp.LoadIcon(IDI_INTERFACE));
   m_imageList.Add(theApp.LoadIcon(IDI_NETWORK));
   m_imageList.Add(theApp.LoadIcon(IDI_CONTAINER));
   m_imageList.Add(theApp.LoadIcon(IDI_UNKNOWN));     // For OBJECT_ZONE
   m_imageList.Add(theApp.LoadIcon(IDI_NETWORK));
   m_imageList.Add(theApp.LoadIcon(IDI_TEMPLATE));
   m_imageList.Add(theApp.LoadIcon(IDI_TEMPLATE_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_TEMPLATE_ROOT));
   m_imageList.Add(theApp.LoadIcon(IDI_SERVICE));
   m_wndTreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);
	
	return 0;
}


//
// WM_SIZE message handler
//

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

   m_wndTreeCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CObjectView::OnViewRefresh() 
{
   NXC_OBJECT **ppRootObjects;
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, j, dwNumObjects, dwNumRootObj;
   
   // Populate object's list and select root objects
   NXCLockObjectIndex(g_hSession);
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
   ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
   for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
      if (!pIndex[i].pObject->bIsDeleted)
      {
         // Check if some of the parents are accessible
         for(j = 0; j < pIndex[i].pObject->dwNumParents; j++)
            if (NXCFindObjectByIdNoLock(g_hSession, pIndex[i].pObject->pdwParentList[j]) != NULL)
               break;
         if (j == pIndex[i].pObject->dwNumParents)
         {
            // No accessible parents or no parents at all
            ppRootObjects[dwNumRootObj++] = pIndex[i].pObject;
         }
      }
   NXCUnlockObjectIndex(g_hSession);

   // Populate objects' tree
   m_wndTreeCtrl.DeleteAllItems();
   m_dwTreeHashSize = 0;

   for(i = 0; i < dwNumRootObj; i++)
      AddObjectToTree(ppRootObjects[i], TVI_ROOT);
   safe_free(ppRootObjects);
   
   qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
}


//
// Add new object to tree
//

void CObjectView::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   DWORD i;
   HTREEITEM hItem;
   TCHAR szBuffer[512];

   // Add object record with class-dependent text
   CreateTreeItemText(pObject, szBuffer);
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, pObject->iClass, pObject->iClass, hParent);
   m_wndTreeCtrl.SetItemData(hItem, pObject->dwId);

   // Add to hash
   m_pTreeHash = (OBJ_TREE_HASH *)realloc(m_pTreeHash, sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize + 1));
   m_pTreeHash[m_dwTreeHashSize].dwObjectId = pObject->dwId;
   m_pTreeHash[m_dwTreeHashSize].hTreeItem = hItem;
   m_dwTreeHashSize++;

   // Add object's childs
   for(i = 0; i < pObject->dwNumChilds; i++)
   {
      NXC_OBJECT *pChildObject = NXCFindObjectById(g_hSession, pObject->pdwChildList[i]);
      if (pChildObject != NULL)
         AddObjectToTree(pChildObject, hItem);
   }

   // Sort childs
   SortTreeItems(hItem);
}


//
// Create class-depemdent text for tree item
//

void CObjectView::CreateTreeItemText(NXC_OBJECT *pObject, TCHAR *pszBuffer)
{
   TCHAR szIpBuffer[32];

   switch(pObject->iClass)
   {
      case OBJECT_NODE:
      case OBJECT_SUBNET:
      case OBJECT_NETWORK:
      case OBJECT_CONTAINER:
      case OBJECT_SERVICEROOT:
      case OBJECT_NETWORKSERVICE:
         if (pObject->iStatus != STATUS_NORMAL)
            swprintf(pszBuffer, L"%s [%s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         else
            wcscpy(pszBuffer, pObject->szName);
         break;
      case OBJECT_INTERFACE:
         if (pObject->dwIpAddr != 0)
            swprintf(pszBuffer, L"%s [%s/%d %s]", pObject->szName, 
                    IpToStr(pObject->dwIpAddr, szIpBuffer), 
                    BitsInMask(pObject->iface.dwIpNetMask), g_szStatusText[pObject->iStatus]);
         else
            swprintf(pszBuffer, L"%s [%s]", pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      default:
         wcscpy(pszBuffer, pObject->szName);
         break;
   }
}


//
// Comparision function for tree items sorting
//

static int CALLBACK CompareTreeItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   TCHAR szName1[MAX_OBJECT_NAME], szName2[MAX_OBJECT_NAME];

   NXCGetComparableObjectName(g_hSession, lParam1, szName1);
   NXCGetComparableObjectName(g_hSession, lParam2, szName2);
   return _tcsicmp(szName1, szName2);
}


//
// Sort childs of given tree item
//

void CObjectView::SortTreeItems(HTREEITEM hItem)
{
   TVSORTCB tvs;

   tvs.hParent = hItem;
   tvs.lpfnCompare = CompareTreeItems;
   m_wndTreeCtrl.SortChildrenCB(&tvs);
}


//
// Delete tree item and appropriate record in hash
//

void CObjectView::DeleteObjectTreeItem(HTREEITEM hRootItem)
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

void CObjectView::UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject)
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
         TCHAR szBuffer[256];
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
                  m_wndTreeCtrl.SetItemState(m_pTreeHash[i].hTreeItem,
                                    INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);
                  SortTreeItems(hItem);
               }
            }
            else  // Current tree item has no parent
            {
               m_wndTreeCtrl.SetItemText(m_pTreeHash[i].hTreeItem, szBuffer);
               m_wndTreeCtrl.SetItemState(m_pTreeHash[i].hTreeItem,
                                       INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);
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
                  {
                     AddObjectToTree(pObject, m_pTreeHash[j].hTreeItem);
                     SortTreeItems(m_pTreeHash[j].hTreeItem);
                  }
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
               {
                  AddObjectToTree(pObject, m_pTreeHash[j].hTreeItem);
                  SortTreeItems(m_pTreeHash[j].hTreeItem);
               }
               qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
            }
         }
      }
   }
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

DWORD CObjectView::FindObjectInTree(DWORD dwObjectId)
{
   DWORD dwIndex;

   dwIndex = SearchTreeHash(m_pTreeHash, m_dwTreeHashSize, dwObjectId);
   if (dwIndex != INVALID_INDEX)
      while((dwIndex > 0) && (m_pTreeHash[dwIndex - 1].dwObjectId == dwObjectId))
         dwIndex--;
   return dwIndex;
}


//
// Handler for object changes
//

void CObjectView::OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject)
{
   UpdateObjectTree(dwObjectId, pObject);
}
