// ObjectSubordinateView.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectSubordinateView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectSubordinateView

CObjectSubordinateView::CObjectSubordinateView()
{
	m_pObject = NULL;
   m_iSortMode = theApp.GetProfileInt(_T("ObjectSubordinateView"), _T("SortMode"), 1);
   m_iSortDir = theApp.GetProfileInt(_T("ObjectSubordinateView"), _T("SortDir"), 1);
}

CObjectSubordinateView::~CObjectSubordinateView()
{
   theApp.WriteProfileInt(_T("ObjectSubordinateView"), _T("SortMode"), m_iSortMode);
   theApp.WriteProfileInt(_T("ObjectSubordinateView"), _T("SortDir"), m_iSortDir);
}


BEGIN_MESSAGE_MAP(CObjectSubordinateView, CWnd)
	//{{AFX_MSG_MAP(CObjectSubordinateView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_SUBORDINATE_DELETE, OnSubordinateDelete)
	ON_COMMAND(ID_SUBORDINATE_MANAGE, OnSubordinateManage)
	ON_COMMAND(ID_SUBORDINATE_UNMANAGE, OnSubordinateUnmanage)
	ON_COMMAND(ID_SUBORDINATE_UNBIND, OnSubordinateUnbind)
	ON_COMMAND(ID_SUBORDINATE_PROPERTIES, OnSubordinateProperties)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_SET_OBJECT, OnSetObject)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_VIEW, OnListViewColumnClick)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CObjectSubordinateView message handlers

int CObjectSubordinateView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT rect;
   LVCOLUMN lvCol;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create image list
   m_imageList.Create(g_pObjectSmallImageList);
   m_iSortImageBase = m_imageList.GetImageCount();
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));

   // Create list view control
	GetClientRect(&rect);
	m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHAREIMAGELISTS, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
	
   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 55);
   m_wndListCtrl.InsertColumn(1, _T("Name"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(2, _T("Class"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Status"), LVCFMT_LEFT, 100);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

	return 0;
}


//
// WM_SIZE message handler
//

void CObjectSubordinateView::OnSize(UINT nType, int cx, int cy) 
{
	m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// NXCM_SET_OBJECT message handler
//

void CObjectSubordinateView::OnSetObject(WPARAM wParam, NXC_OBJECT *pObject)
{
   m_pObject = pObject;
   Refresh();
}


//
// Refresh view
//

void CObjectSubordinateView::Refresh()
{
	DWORD i;
	NXC_OBJECT *pObject;
	int nItem;
	TCHAR szBuffer[256];

	m_wndListCtrl.DeleteAllItems();
	if (m_pObject == NULL)
		return;

	for(i = 0; i < m_pObject->dwNumChilds; i++)
	{
		pObject = NXCFindObjectById(g_hSession, m_pObject->pdwChildList[i]);
		if (pObject != NULL)
		{
			_stprintf(szBuffer, _T("%d"), pObject->dwId);
			nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer, GetObjectImageIndex(pObject));
			if (nItem != -1)
			{
				m_wndListCtrl.SetItemText(nItem, 1, pObject->szName);
				m_wndListCtrl.SetItemText(nItem, 2, g_szObjectClass[pObject->iClass]);
				m_wndListCtrl.SetItemText(nItem, 3, g_szStatusTextSmall[pObject->iStatus]);
				m_wndListCtrl.SetItemData(nItem, (UINT_PTR)pObject);
			}
		}
	}
}


//
// WM_CONTEXTMENU message handler
//

void CObjectSubordinateView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;
	UINT nCount;

   pMenu = theApp.GetContextMenu(27);
	nCount = m_wndListCtrl.GetSelectedCount();
	pMenu->EnableMenuItem(ID_SUBORDINATE_MANAGE, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_SUBORDINATE_UNMANAGE, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_SUBORDINATE_UNBIND, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_SUBORDINATE_DELETE, MF_BYCOMMAND | ((nCount > 0) ? MF_ENABLED : MF_GRAYED));
	pMenu->EnableMenuItem(ID_SUBORDINATE_PROPERTIES, MF_BYCOMMAND | ((nCount == 1) ? MF_ENABLED : MF_GRAYED));
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// Worker function for actions
//

static DWORD DoAction(int nAction, DWORD dwCount, DWORD *pdwList, NXC_OBJECT *pParent)
{
	DWORD i, dwResult = RCC_SUCCESS;

	for(i = 0; (i < dwCount) && (dwResult == RCC_SUCCESS); i++)
	{
		switch(nAction)
		{
			case 0:	// Manage
				dwResult = NXCSetObjectMgmtStatus(g_hSession, pdwList[i], TRUE);
				break;
			case 1:	// Unmanage
				dwResult = NXCSetObjectMgmtStatus(g_hSession, pdwList[i], FALSE);
				break;
			case 2:	// Unbind
				dwResult = NXCUnbindObject(g_hSession, pParent->dwId, pdwList[i]);
				break;
			case 3:	// Delete
				dwResult = NXCDeleteObject(g_hSession, pdwList[i]);
				break;
			default:
				dwResult = RCC_INTERNAL_ERROR;
				break;
		}
	}
	return dwResult;
}


//
// Do requested action on child objects
//

void CObjectSubordinateView::DoActionOnSubordinates(int nAction)
{
	DWORD i, dwCount, *pdwList, dwResult;
	NXC_OBJECT *pObject;
	int nItem;
	static TCHAR *infoText[] =
	{
		_T("Changing object(s) management status..."),
		_T("Changing object(s) management status..."),
		_T("Changing objects binding..."),
		_T("Deleting object(s)..."),
	};
	static TCHAR *errorText[] =
	{
		_T("Cannot set object status to managed: %s"),
		_T("Cannot set object status to unmanaged: %s"),
		_T("Cannot unbind object: %s"),
		_T("Cannot delete object: %s"),
	};

	dwCount = m_wndListCtrl.GetSelectedCount();
	if (dwCount == 0)
		return;

	pdwList = (DWORD *)malloc(sizeof(DWORD) * dwCount);
	for(i = 0, nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED); (i < dwCount) && (nItem != -1); i++)
	{
		pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(nItem);
		pdwList[i] = (pObject != NULL) ? pObject->dwId : 0;
		nItem = m_wndListCtrl.GetNextItem(nItem, LVIS_SELECTED);
	}

	dwResult = DoRequestArg4(DoAction, (void *)nAction, (void *)dwCount, pdwList, m_pObject, infoText[nAction]);
	if (dwResult != RCC_SUCCESS)
	{
		theApp.ErrorBox(dwResult, errorText[nAction]);
	}

	free(pdwList);
}


//
// Delete subordinate object(s)
//

void CObjectSubordinateView::OnSubordinateDelete() 
{
   if (g_dwOptions & UI_OPT_CONFIRM_OBJ_DELETE)
	{
		TCHAR szBuffer[1024];
		NXC_OBJECT *pObject;

		if (m_wndListCtrl.GetSelectedCount() == 1)
		{
			pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
			_stprintf(szBuffer, _T("Do you really want to delete object %s?"), (pObject != NULL) ? pObject->szName : _T("<unknown>"));
		}
		else
		{
			_tcscpy(szBuffer, _T("Do you really want to delete selected objects?"));
		}
		if (MessageBox(szBuffer, _T("Confirmation"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			DoActionOnSubordinates(3);
		}
	}
	else
	{
		DoActionOnSubordinates(3);
	}
}


//
// Set status to managed for subordinate object(s)
//

void CObjectSubordinateView::OnSubordinateManage() 
{
	DoActionOnSubordinates(0);
	Refresh();
}


//
// Set status to unmanaged for subordinate object(s)
//

void CObjectSubordinateView::OnSubordinateUnmanage() 
{
	DoActionOnSubordinates(1);
	Refresh();
}


//
// Unbind subordinate object(s)
//

void CObjectSubordinateView::OnSubordinateUnbind() 
{
	DoActionOnSubordinates(2);
}


//
// Show properties of subordinate object
//

void CObjectSubordinateView::OnSubordinateProperties() 
{
	NXC_OBJECT *pObject;

	pObject = (NXC_OBJECT *)m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
	if (pObject != NULL)
		theApp.ObjectProperties(pObject->dwId);
}


//
// Callback for item sorting
//

static int CALLBACK CompareListItemsCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return ((CObjectSubordinateView *)lParamSort)->CompareListItems((NXC_OBJECT *)lParam1, (NXC_OBJECT *)lParam2);
}


//
// Compare two list items
//

int CObjectSubordinateView::CompareListItems(NXC_OBJECT *obj1, NXC_OBJECT *obj2)
{
	int nResult;

	switch(m_iSortMode)
	{
		case 0:	// ID
			nResult = COMPARE_NUMBERS(obj1->dwId, obj2->dwId);
			break;
		case 1:	// Name
			nResult = _tcsicmp(obj1->szName, obj2->szName);
			break;
		case 2:	// Class
			nResult = _tcsicmp(g_szObjectClass[obj1->iClass], g_szObjectClass[obj2->iClass]);
			break;
		case 3:	// Status
			nResult = COMPARE_NUMBERS(obj1->iStatus, obj2->iStatus);
			break;
		default:
			nResult = 0;
			break;
	}
	return nResult * m_iSortDir;
}


//
// Handler for list view column click
//

void CObjectSubordinateView::OnListViewColumnClick(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_iSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_iSortMode == pNMHDR->iSubItem)
   {
      // Same column, change sort direction
      m_iSortDir = -m_iSortDir;
   }
   else
   {
      // Another sorting column
      m_iSortMode = pNMHDR->iSubItem;
   }
   m_wndListCtrl.SortItems(CompareListItemsCB, (UINT_PTR)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_iSortDir > 0) ? m_iSortImageBase : (m_iSortImageBase + 1);
   m_wndListCtrl.SetColumn(pNMHDR->iSubItem, &lvCol);
   
   *pResult = 0;
}
