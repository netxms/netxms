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
         dwMask = pObject->subnet.dwIpNetMask;
         break;
      case OBJECT_INTERFACE:
         dwMask = pObject->iface.dwIpNetMask;
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

   switch(SORT_MODE(lParamSort))
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
         dwNum1 = ((NXC_OBJECT *)lParam1)->dwIpAddr;
         dwNum2 = ((NXC_OBJECT *)lParam2)->dwIpAddr;
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

   return (SORT_DIR(lParamSort) == SORT_ASCENDING) ? iResult : -iResult;
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
   m_dwFlags = SHOW_OBJECT_PREVIEW | VIEW_OBJECTS_AS_TREE;
   m_dwSortMode = OBJECT_SORT_BY_NAME;
   m_pCurrentObject = NULL;
   m_bRestoredDesktop = FALSE;
   m_bImmediateSorting = TRUE;
   m_nTimer = 0;
}

CObjectBrowser::CObjectBrowser(TCHAR *pszParams)
{
   DWORD dwObjectId;

   m_pImageList = NULL;
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
   m_dwFlags = ExtractWindowParamULong(pszParams, _T("F"), SHOW_OBJECT_PREVIEW | VIEW_OBJECTS_AS_TREE);
   m_dwSortMode = ExtractWindowParamULong(pszParams, _T("SM"), OBJECT_SORT_BY_NAME);
   dwObjectId = ExtractWindowParamULong(pszParams, _T("O"), 0);
   if (dwObjectId != 0)
   {
      m_pCurrentObject = NXCFindObjectById(g_hSession, dwObjectId);
   }
   else
   {
      m_pCurrentObject = NULL;
   }
   m_bRestoredDesktop = TRUE;
   m_bImmediateSorting = TRUE;
   m_nTimer = 0;
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
	ON_COMMAND(ID_OBJECT_DELETE, OnObjectDelete)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_DELETE, OnUpdateObjectDelete)
	ON_COMMAND(ID_OBJECT_POLL_STATUS, OnObjectPollStatus)
	ON_COMMAND(ID_OBJECT_POLL_CONFIGURATION, OnObjectPollConfiguration)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_POLL_STATUS, OnUpdateObjectPollStatus)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_POLL_CONFIGURATION, OnUpdateObjectPollConfiguration)
	ON_WM_CLOSE()
	ON_COMMAND(ID_OBJECT_CREATE_TEMPLATE, OnObjectCreateTemplate)
	ON_COMMAND(ID_OBJECT_CREATE_TEMPLATEGROUP, OnObjectCreateTemplategroup)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_WAKEUP, OnUpdateObjectWakeup)
	ON_COMMAND(ID_OBJECT_CREATE_SERVICE, OnObjectCreateService)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_LASTDCIVALUES, OnUpdateObjectLastdcivalues)
	ON_COMMAND(ID_OBJECT_LASTDCIVALUES, OnObjectLastdcivalues)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_APPLY, OnUpdateObjectApply)
	ON_COMMAND(ID_OBJECT_APPLY, OnObjectApply)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNBIND, OnUpdateObjectUnbind)
	ON_COMMAND(ID_OBJECT_UNBIND, OnObjectUnbind)
	ON_COMMAND(ID_OBJECT_CHANGEIPADDRESS, OnObjectChangeipaddress)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_CHANGEIPADDRESS, OnUpdateObjectChangeipaddress)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_AGENTCFG, OnUpdateObjectAgentcfg)
	ON_COMMAND(ID_OBJECT_AGENTCFG, OnObjectAgentcfg)
	ON_COMMAND(ID_OBJECT_CREATE_VPNCONNECTOR, OnObjectCreateVpnconnector)
	ON_COMMAND(ID_OBJECT_MOVE, OnObjectMove)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_MOVE, OnUpdateObjectMove)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
   ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_VIEW, OnTreeViewSelChange)
   ON_NOTIFY(TVN_GETDISPINFO, IDC_TREE_VIEW, OnTreeViewGetDispInfo)
   ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE_VIEW, OnTreeViewItemExpanding)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VIEW, OnListViewItemChange)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
   ON_MESSAGE(WM_OBJECT_CHANGE, OnObjectChange)
   ON_MESSAGE(WM_FIND_OBJECT, OnFindObject)
   ON_MESSAGE(WM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_COMMAND_RANGE(OBJTOOL_MENU_FIRST_ID, OBJTOOL_MENU_LAST_ID, OnObjectTool)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser message handlers


//
// WM_DESTROY message handler
//

void CObjectBrowser::OnDestroy() 
{
   if (m_nTimer != 0)
      KillTimer(m_nTimer);
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
   BYTE *pwp;
   UINT iBytes;
   int i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Read browser configuration
   if (!m_bRestoredDesktop)
   {
      m_dwFlags = theApp.GetProfileInt(_T("ObjectBrowser"), _T("Flags"), m_dwFlags);
      m_dwSortMode = theApp.GetProfileInt(_T("ObjectBrowser"), _T("SortMode"), m_dwSortMode);
   }

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
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
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
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Mark sorting column in list control
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = m_iLastObjectImage + SORT_DIR(m_dwSortMode);
   m_wndListCtrl.SetColumn(SORT_MODE(m_dwSortMode), &lvCol);

   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      m_wndTreeCtrl.ShowWindow(SW_SHOW);
   else
      m_wndListCtrl.ShowWindow(SW_SHOW);

   // Restore window size and position if we have one
   if (theApp.GetProfileBinary(_T("ObjectBrowser"), _T("WindowPlacement"),
                               &pwp, &iBytes))
   {
      if (iBytes == sizeof(WINDOWPLACEMENT))
      {
         RestoreMDIChildPlacement(this, (WINDOWPLACEMENT *)pwp);
      }
      delete pwp;
   }

   m_nTimer = SetTimer(1, 1000, NULL);

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
   NXC_OBJECT **ppRootObjects;
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, j, dwNumObjects, dwNumRootObj;
   
   // Populate object's list and select root objects

   m_wndListCtrl.DeleteAllItems();
   NXCLockObjectIndex(g_hSession);
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
   ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
   for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
      if (!pIndex[i].pObject->bIsDeleted)
      {
         AddObjectToList(pIndex[i].pObject);

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
   m_wndListCtrl.SortItems(CompareListItems, m_dwSortMode);

   // Populate objects' tree
   m_wndTreeCtrl.DeleteAllItems();
   m_dwTreeHashSize = 0;

   for(i = 0; i < dwNumRootObj; i++)
      AddObjectToTree(ppRootObjects[i], TVI_ROOT);
   safe_free(ppRootObjects);
}


//
// Add new object to tree
//

void CObjectBrowser::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   HTREEITEM hItem;
   TVITEM tvi;
   char szBuffer[512];
   int iImage;

   // Add object record with class-dependent text
   CreateTreeItemText(pObject, szBuffer);
   iImage = GetObjectImageIndex(pObject);
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, iImage, iImage, hParent);
   m_wndTreeCtrl.SetItemData(hItem, (LPARAM)pObject);
   m_wndTreeCtrl.SetItemState(hItem, INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);

   // Add to hash
   AddObjectEntryToHash(pObject, hItem);

   // Add object's childs
   // For node objects, don't add childs if node has more than 10 childs to
   // prevent adding millions of items if node has thousands of interfaces in
   // thousands subnets. Childs will be added only if user expands node.
/*   if ((pObject->iClass != OBJECT_NODE) || (pObject->dwNumChilds <= 10))
   {
      for(i = 0; i < pObject->dwNumChilds; i++)
      {
         NXC_OBJECT *pChildObject = NXCFindObjectById(g_hSession, pObject->pdwChildList[i]);
         if (pChildObject != NULL)
            AddObjectToTree(pChildObject, hItem);
      }
   }
   else*/
   tvi.mask = TVIF_CHILDREN;
   tvi.hItem = hItem;
   tvi.cChildren = I_CHILDRENCALLBACK;
   m_wndTreeCtrl.SetItem(&tvi);
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

void CObjectBrowser::OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      m_pCurrentObject = (NXC_OBJECT *)lpnmt->itemNew.lParam;
      m_wndPreviewPane.SetCurrentObject(m_pCurrentObject);
   }
   *pResult = 0;
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
   // First, check for built-in objects
   // If they presented, they will be located in first 10 positions
   if (dwObjectId < 10)
   {
      DWORD i, dwLimit;

      dwLimit = min(m_dwTreeHashSize, dwObjectId);
      for(i = 0; i < dwLimit; i++)
         if (m_pTreeHash[i].dwObjectId == dwObjectId)
         {
            return i;
         }
      return INVALID_INDEX;
   }

   // Second, check two common cases - object is last added, i.e. it
   // will be located in last cell, and object is newly created -
   // then it's id will be greater than last cell value
   if (m_dwTreeHashSize > 0)
   {
      if (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId == dwObjectId)
         return m_dwTreeHashSize - 1;
      if (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId < dwObjectId)
         return INVALID_INDEX;
   }

   // Do binary search on a hash array as last resort
   return SearchTreeHash(m_pTreeHash, m_dwTreeHashSize, dwObjectId);
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
   DWORD i, dwIndex;
   NXC_OBJECT *pObject;

   // Delete all child items
   hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   while(hItem != NULL)
   {
      DeleteObjectTreeItem(hItem);
      hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   }

   // Find hash record for current item
   pObject = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hRootItem);
   dwIndex = FindObjectInTree(pObject->dwId);
   if (dwIndex != INVALID_INDEX)
   {
      for(i = 0; i < m_pTreeHash[dwIndex].dwNumEntries; i++)
         if (m_pTreeHash[dwIndex].phTreeItemList[i] == hRootItem)
         {
            if (m_pTreeHash[dwIndex].dwNumEntries == 1)
            {
               // Last entry, delete entire record in tree hash list
               free(m_pTreeHash[dwIndex].phTreeItemList);
               m_dwTreeHashSize--;
               if (dwIndex < m_dwTreeHashSize)
                  memmove(&m_pTreeHash[dwIndex], &m_pTreeHash[dwIndex + 1], 
                          sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize - dwIndex));
            }
            else
            {
               m_pTreeHash[dwIndex].dwNumEntries--;
               memmove(&m_pTreeHash[dwIndex].phTreeItemList[i],
                       &m_pTreeHash[dwIndex].phTreeItemList[i + 1],
                       sizeof(HTREEITEM) * (m_pTreeHash[dwIndex].dwNumEntries - i));
            }
            break;
         }
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
      if (dwIndex != INVALID_INDEX)
      {
         HTREEITEM *phItemList;
         DWORD dwNumItems;

         dwNumItems = m_pTreeHash[dwIndex].dwNumEntries;
         phItemList = (HTREEITEM *)nx_memdup(m_pTreeHash[dwIndex].phTreeItemList, sizeof(HTREEITEM) * dwNumItems);
         // Delete all tree items
         for(i = 0; i < dwNumItems; i++)
            DeleteObjectTreeItem(phItemList[i]);
         free(phItemList);
      }
   }
   else
   {
      HTREEITEM hItem;
      NXC_OBJECT *pParent;

      if (dwIndex != INVALID_INDEX)
      {
         char szBuffer[256];
         DWORD j, *pdwParentList;

         // Create a copy of object's parent list
         pdwParentList = (DWORD *)nx_memdup(pObject->pdwParentList, 
                                            sizeof(DWORD) * pObject->dwNumParents);

         CreateTreeItemText(pObject, szBuffer);
         for(i = 0; i < m_pTreeHash[dwIndex].dwNumEntries; i++)
         {
            // Check if this item's parent still in object's parents list
            hItem = m_wndTreeCtrl.GetParentItem(m_pTreeHash[dwIndex].phTreeItemList[i]);
            if (hItem != NULL)
            {
               pParent = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem);
               for(j = 0; j < pObject->dwNumParents; j++)
                  if (pObject->pdwParentList[j] == pParent->dwId)
                  {
                     pdwParentList[j] = 0;   // Mark this parent as presented
                     break;
                  }
               if (j == pObject->dwNumParents)  // Not a parent anymore
               {
                  // Delete this tree item
                  BOOL bStop = (m_pTreeHash[dwIndex].dwNumEntries == 1);
                  DeleteObjectTreeItem(m_pTreeHash[dwIndex].phTreeItemList[i]);
                  if (bStop)
                     break;
                  i--;
               }
               else  // Current tree item is still valid
               {
                  m_wndTreeCtrl.SetItemText(m_pTreeHash[dwIndex].phTreeItemList[i], szBuffer);
                  m_wndTreeCtrl.SetItemState(m_pTreeHash[dwIndex].phTreeItemList[i],
                                    INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);
                  SortTreeItems(hItem);
               }
            }
            else  // Current tree item has no parent
            {
               m_wndTreeCtrl.SetItemText(m_pTreeHash[dwIndex].phTreeItemList[i], szBuffer);
               m_wndTreeCtrl.SetItemState(m_pTreeHash[dwIndex].phTreeItemList[i],
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
                  for(j = 0; j < m_pTreeHash[dwIndex].dwNumEntries; j++)
                  {
                     if (m_wndTreeCtrl.GetItemState(m_pTreeHash[dwIndex].phTreeItemList[j], TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
                     {
                        AddObjectToTree(pObject, m_pTreeHash[dwIndex].phTreeItemList[j]);
                        dwIndex = AdjustIndex(dwIndex, pObject->pdwParentList[i]);
                        SortTreeItems(m_pTreeHash[dwIndex].phTreeItemList[j]);
                     }
                  }
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
               for(j = 0; j < m_pTreeHash[dwIndex].dwNumEntries; j++)
               {
                  if (m_wndTreeCtrl.GetItemState(m_pTreeHash[dwIndex].phTreeItemList[j], TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
                  {
                     AddObjectToTree(pObject, m_pTreeHash[dwIndex].phTreeItemList[j]);
                     dwIndex = AdjustIndex(dwIndex, pObject->pdwParentList[i]);
                     SortTreeItems(m_pTreeHash[dwIndex].phTreeItemList[j]);
                  }
               }
            }
         }
      }

      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      {
         if (m_dwFlags & FOLLOW_OBJECT_UPDATES)
         {
            dwIndex = FindObjectInTree(dwObjectId);
            if (dwIndex != INVALID_INDEX)    // Shouldn't happen
               m_wndTreeCtrl.Select(m_pTreeHash[dwIndex].phTreeItemList[0], TVGN_CARET);
         }
         else
         {
            // Check if current item has been changed
            hItem = m_wndTreeCtrl.GetSelectedItem();
            if (hItem != NULL)
            {
               if (((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId == dwObjectId)
                  m_wndPreviewPane.Refresh();
            }
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
   HTREEITEM hItem = NULL;

   m_dwFlags |= VIEW_OBJECTS_AS_TREE;
   m_wndTreeCtrl.ShowWindow(SW_SHOW);
   m_wndListCtrl.ShowWindow(SW_HIDE);

   // Display currenly selected item in preview pane
   hItem = m_wndTreeCtrl.GetSelectedItem();
   m_pCurrentObject = (hItem == NULL) ? NULL : (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem);
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
   iColumn = SORT_MODE(m_dwSortMode);
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(iColumn, &lvCol);

   // Change current sort mode and resort list
   if (iColumn == pNMHDR->iSubItem)
   {
      // Same column, change sort direction
      m_dwSortMode = iColumn | 
         (((SORT_DIR(m_dwSortMode) == SORT_ASCENDING) ? SORT_DESCENDING : SORT_ASCENDING) << 8);
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
   lvCol.iImage = (SORT_DIR(m_dwSortMode) == SORT_ASCENDING) ? m_iLastObjectImage : (m_iLastObjectImage + 1);
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

      pObject = NXCFindObjectByName(g_hSession, (char *)lParam);
      if (pObject != NULL)
      {
         // Object found, select it in the current view
         if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         {
            DWORD dwIndex;

            dwIndex = FindObjectInTree(pObject->dwId);
            if (dwIndex != INVALID_INDEX)
               m_wndTreeCtrl.Select(m_pTreeHash[dwIndex].phTreeItemList[0], TVGN_CARET);
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
         dwId = ((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId;
   }
   else
   {
      if (m_wndListCtrl.GetSelectedCount() == 1)
      {
         int iItem;

         iItem = m_wndListCtrl.GetSelectionMark();
         if (iItem != -1)
            dwId = ((NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem))->dwId;
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

   m_wndListCtrl.SetItemState(iItem, INDEXTOOVERLAYMASK(pObject->iStatus), LVIS_OVERLAYMASK);

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

void CObjectBrowser::OnUpdateObjectAgentcfg(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}

void CObjectBrowser::OnUpdateObjectDatacollection(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(TRUE));
}

void CObjectBrowser::OnUpdateObjectLastdcivalues(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}

void CObjectBrowser::OnUpdateObjectWakeup(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE) || CurrObjectIsInterface());
}

void CObjectBrowser::OnUpdateObjectUnmanage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}

void CObjectBrowser::OnUpdateObjectManage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}

void CObjectBrowser::OnUpdateObjectDelete(CCmdUI* pCmdUI) 
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

void CObjectBrowser::OnUpdateObjectUnbind(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                        (m_pCurrentObject->iClass == OBJECT_SERVICEROOT) ||
                        (m_pCurrentObject->iClass == OBJECT_TEMPLATE));
      else
         pCmdUI->Enable(((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                         (m_pCurrentObject->iClass == OBJECT_SERVICEROOT) ||
                         (m_pCurrentObject->iClass == OBJECT_TEMPLATE)) &&
                        (m_wndListCtrl.GetSelectedCount() == 1));
   }
}

void CObjectBrowser::OnUpdateObjectApply(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
         pCmdUI->Enable(m_pCurrentObject->iClass == OBJECT_TEMPLATE);
      else
         pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_TEMPLATE) &&
                        (m_wndListCtrl.GetSelectedCount() == 1));
   }
}

void CObjectBrowser::OnUpdateObjectPollStatus(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode());
}

void CObjectBrowser::OnUpdateObjectPollConfiguration(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode());
}

void CObjectBrowser::OnUpdateObjectChangeipaddress(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode());
}

void CObjectBrowser::OnUpdateObjectMove(CCmdUI* pCmdUI) 
{
   BOOL bEnable = FALSE;

   if ((m_pCurrentObject != NULL) && (m_dwFlags & VIEW_OBJECTS_AS_TREE))
   {
      HTREEITEM hItem;

      hItem = m_wndTreeCtrl.GetParentItem(m_wndTreeCtrl.GetSelectedItem());
      if (hItem != NULL)
      {
         NXC_OBJECT *pObject;

         pObject = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem);
         if (pObject != NULL)
         {
            if ((pObject->iClass == OBJECT_CONTAINER) ||
                (pObject->iClass == OBJECT_SERVICEROOT))
               bEnable = TRUE;
         }
      }
   }
   pCmdUI->Enable(bEnable);
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
   if (m_pCurrentObject != NULL)
      theApp.BindObject(m_pCurrentObject);
}


//
// WM_COMMAND::ID_OBJECT_UNBIND message handler
//

void CObjectBrowser::OnObjectUnbind() 
{
   if (m_pCurrentObject != NULL)
      theApp.UnbindObject(m_pCurrentObject);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_CONTAINER message handler
//

void CObjectBrowser::OnObjectCreateContainer() 
{
   theApp.CreateContainer((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_NODE message handler
//

void CObjectBrowser::OnObjectCreateNode() 
{
   theApp.CreateNode((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_TEMPLATE message handler
//

void CObjectBrowser::OnObjectCreateTemplate() 
{
   theApp.CreateTemplate((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_TEMPLATEGROUP message handler
//

void CObjectBrowser::OnObjectCreateTemplategroup() 
{
   theApp.CreateTemplateGroup((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_SERVICE message handler
//

void CObjectBrowser::OnObjectCreateService() 
{
   theApp.CreateNetworkService((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_CREATE_VPNCONNECTOR message handler
//

void CObjectBrowser::OnObjectCreateVpnconnector() 
{
   theApp.CreateVPNConnector((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// WM_COMMAND::ID_OBJECT_DELETE message handler
//

void CObjectBrowser::OnObjectDelete() 
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.DeleteNetXMSObject(m_pCurrentObject);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.DeleteNetXMSObject(pObject);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_POLL_STATUS message handler
//

void CObjectBrowser::OnObjectPollStatus() 
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.PollNode(m_pCurrentObject->dwId, POLL_STATUS);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.PollNode(pObject->dwId, POLL_STATUS);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_POLL_CONFIGURATION message handler
//

void CObjectBrowser::OnObjectPollConfiguration() 
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.PollNode(m_pCurrentObject->dwId, POLL_CONFIGURATION);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.PollNode(pObject->dwId, POLL_CONFIGURATION);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// Returns TRUE if currently selected object is node
//

BOOL CObjectBrowser::CurrObjectIsNode(BOOL bIncludeTemplates)
{
   if (m_pCurrentObject == NULL)
   {
      return FALSE;
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      {
         return bIncludeTemplates ? 
            ((m_pCurrentObject->iClass == OBJECT_NODE) || 
             (m_pCurrentObject->iClass == OBJECT_TEMPLATE)) :
            (m_pCurrentObject->iClass == OBJECT_NODE);
      }
      else
      {
         return ((bIncludeTemplates ? 
            ((m_pCurrentObject->iClass == OBJECT_NODE) || 
             (m_pCurrentObject->iClass == OBJECT_TEMPLATE)) :
            (m_pCurrentObject->iClass == OBJECT_NODE)) &&
                 (m_wndListCtrl.GetSelectedCount() == 1));
      }
   }
}


//
// Returns TRUE if currently selected object is interface
//

BOOL CObjectBrowser::CurrObjectIsInterface()
{
   if (m_pCurrentObject == NULL)
   {
      return FALSE;
   }
   else
   {
      if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
      {
         return m_pCurrentObject->iClass == OBJECT_INTERFACE;
      }
      else
      {
         return ((m_pCurrentObject->iClass == OBJECT_INTERFACE) &&
                 (m_wndListCtrl.GetSelectedCount() == 1));
      }
   }
}


//
// WM_CLOSE message handler
//

void CObjectBrowser::OnClose() 
{
   WINDOWPLACEMENT wp;

   theApp.WriteProfileInt(_T("ObjectBrowser"), _T("Flags"), m_dwFlags);
   theApp.WriteProfileInt(_T("ObjectBrowser"), _T("SortMode"), m_dwSortMode);
   GetWindowPlacement(&wp);
   theApp.WriteProfileBinary(_T("ObjectBrowser"), _T("WindowPlacement"), 
                             (BYTE *)&wp, sizeof(WINDOWPLACEMENT));
	
	CMDIChildWnd::OnClose();
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

void CObjectBrowser::SortTreeItems(HTREEITEM hItem)
{
   if (m_bImmediateSorting)
   {
      TVSORTCB tvs;

      tvs.hParent = hItem;
      tvs.lpfnCompare = CompareTreeItems;
      m_wndTreeCtrl.SortChildrenCB(&tvs);
      m_bImmediateSorting = FALSE;
   }
   else
   {
   }
}


//
// WM_COMMAND::ID_OBJECT_LASTDCIVALUES message handler
//

void CObjectBrowser::OnObjectLastdcivalues() 
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.ShowLastValues(m_pCurrentObject);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.ShowLastValues(pObject);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_APPLY message handler
//

void CObjectBrowser::OnObjectApply() 
{
   if (m_pCurrentObject != NULL)
      theApp.ApplyTemplate(m_pCurrentObject);
}


//
// WM_COMMAND::ID_OBJECT_CHANGEIPADDRESS message handler
//

void CObjectBrowser::OnObjectChangeipaddress() 
{
   if (m_pCurrentObject != NULL)
      theApp.ChangeNodeAddress(m_pCurrentObject->dwId);
}


//
// Get save info for desktop saving
//

LRESULT CObjectBrowser::OnGetSaveInfo(WPARAM wParam, WINDOW_SAVE_INFO *pInfo)
{
   pInfo->iWndClass = WNDC_OBJECT_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   _sntprintf(pInfo->szParameters, MAX_WND_PARAM_LEN, _T("F:%u\x7FSM:%u\x7FO:%u"),
              m_dwFlags, m_dwSortMode, 
              (m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
   return 1;
}


//
// Edit agent's configuration file
//

void CObjectBrowser::OnObjectAgentcfg() 
{
   if (m_pCurrentObject != NULL)
      theApp.EditAgentConfig(m_pCurrentObject);
}


//
// Handler for object tools
//

void CObjectBrowser::OnObjectTool(UINT nID)
{
   if (m_dwFlags & VIEW_OBJECTS_AS_TREE)
   {
      if (m_pCurrentObject != NULL)
         theApp.ExecuteObjectTool(m_pCurrentObject, nID - OBJTOOL_MENU_FIRST_ID);
   }
   else
   {
      int iItem;
      NXC_OBJECT *pObject;

      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
      while(iItem != -1)
      {
         pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(iItem);
         theApp.ExecuteObjectTool(pObject, nID - OBJTOOL_MENU_FIRST_ID);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVNI_SELECTED);
      }
   }
}


//
// WM_COMMAND::ID_OBJECT_MOVE message handler
//

void CObjectBrowser::OnObjectMove() 
{
   HTREEITEM hItem;

   if ((m_dwFlags & VIEW_OBJECTS_AS_TREE) && (m_pCurrentObject != NULL))
   {
      hItem = m_wndTreeCtrl.GetParentItem(m_wndTreeCtrl.GetSelectedItem());
      if (hItem != NULL)
      {
         theApp.MoveObject(m_pCurrentObject->dwId, 
                           ((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId);
      }
   }
}


//
// Handler for TVN_GETDISPINFO notification from tree view control
//

void CObjectBrowser::OnTreeViewGetDispInfo(LPNMTVDISPINFO lpdi, LRESULT *pResult)
{
   NXC_OBJECT *pObject;

   if (lpdi->item.mask == TVIF_CHILDREN)
   {
      pObject = (NXC_OBJECT *)lpdi->item.lParam;
      if (pObject != NULL)
      {
         lpdi->item.cChildren = (pObject->dwNumChilds > 0) ? 1 : 0;
      }
      else
      {
         lpdi->item.cChildren = 0;
      }
   }
   *pResult = 0;
}


//
// Handler for TVN_ITEMEXPANDING notification from tree view control
//

void CObjectBrowser::OnTreeViewItemExpanding(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   if ((lpnmt->action == TVE_EXPAND) ||
       (lpnmt->action == TVE_EXPANDPARTIAL) ||
       (lpnmt->action == TVE_TOGGLE))
   {
      NXC_OBJECT *pObject, *pChildObject;
      DWORD i;

      pObject = (NXC_OBJECT *)lpnmt->itemNew.lParam;
      if ((pObject != NULL) && ((m_wndTreeCtrl.GetItemState(lpnmt->itemNew.hItem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE) == 0))
      {
         for(i = 0; i < pObject->dwNumChilds; i++)
         {
            pChildObject = NXCFindObjectById(g_hSession, pObject->pdwChildList[i]);
            if (pChildObject != NULL)
               AddObjectToTree(pChildObject, lpnmt->itemNew.hItem);
         }
         SortTreeItems(lpnmt->itemNew.hItem);
      }
   }
   *pResult = 0;
}


//
// Add new object entry to hash
// Return TRUE if hash array needs to be resorted
//

void CObjectBrowser::AddObjectEntryToHash(NXC_OBJECT *pObject, HTREEITEM hItem)
{
   DWORD dwIndex, dwEntry;

   dwIndex = FindObjectInTree(pObject->dwId);
   if (dwIndex == INVALID_INDEX)
   {
      m_pTreeHash = (OBJ_TREE_HASH *)realloc(m_pTreeHash, sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize + 1));
      m_pTreeHash[m_dwTreeHashSize].dwObjectId = pObject->dwId;
      m_pTreeHash[m_dwTreeHashSize].dwNumEntries = 1;
      m_pTreeHash[m_dwTreeHashSize].phTreeItemList = (HTREEITEM *)malloc(sizeof(HTREEITEM));
      m_pTreeHash[m_dwTreeHashSize].phTreeItemList[0] = hItem;
      m_dwTreeHashSize++;
      if ((m_dwTreeHashSize > 1) && 
          (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId < m_pTreeHash[m_dwTreeHashSize - 2].dwObjectId))
      {
         qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
      }
   }
   else
   {
      // Object already presented in hash array, just add new HTREEITEM
      dwEntry = m_pTreeHash[dwIndex].dwNumEntries;
      m_pTreeHash[dwIndex].dwNumEntries++;
      m_pTreeHash[dwIndex].phTreeItemList = (HTREEITEM *)realloc(m_pTreeHash[dwIndex].phTreeItemList, sizeof(HTREEITEM)  * m_pTreeHash[dwIndex].dwNumEntries);
      m_pTreeHash[dwIndex].phTreeItemList[dwEntry] = hItem;
   }
}


//
// Adjust object index after new object insertion into tree
//

DWORD CObjectBrowser::AdjustIndex(DWORD dwIndex, DWORD dwObjectId)
{
   if (m_pTreeHash[dwIndex].dwObjectId == dwObjectId)
      return dwIndex;
   if ((dwIndex > 0) && (m_pTreeHash[dwIndex - 1].dwObjectId == dwObjectId))
      return dwIndex - 1;
   else
      return dwIndex + 1;
}


//
// WM_TIMER message handler
//

void CObjectBrowser::OnTimer(UINT nIDEvent) 
{
   m_bImmediateSorting = TRUE;
}
