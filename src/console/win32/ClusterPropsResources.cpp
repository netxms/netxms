// ClusterPropsResources.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterPropsResources.h"
#include "ObjectPropSheet.h"
#include "ClusterResDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// List item sorting callback
//

static int CALLBACK SortListItemsCB(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return ((CClusterPropsResources *)lParamSort)->CompareListItems(lParam1, lParam2);
}


/////////////////////////////////////////////////////////////////////////////
// CClusterPropsResources property page

IMPLEMENT_DYNCREATE(CClusterPropsResources, CPropertyPage)

CClusterPropsResources::CClusterPropsResources() : CPropertyPage(CClusterPropsResources::IDD)
{
	//{{AFX_DATA_INIT(CClusterPropsResources)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_dwNumResources = 0;
	m_pResourceList = NULL;

	m_nSortMode = theApp.GetProfileInt(_T("ClusterPropsResources"), _T("SortMode"), 0);
	m_nSortDir = theApp.GetProfileInt(_T("ClusterPropsResources"), _T("SortDir"), 1);
}

CClusterPropsResources::~CClusterPropsResources()
{
	theApp.WriteProfileInt(_T("ClusterPropsResources"), _T("SortMode"), m_nSortMode);
	theApp.WriteProfileInt(_T("ClusterPropsResources"), _T("SortDir"), m_nSortDir);
	safe_free(m_pResourceList);
}

void CClusterPropsResources::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClusterPropsResources)
	DDX_Control(pDX, IDC_LIST_RESOURCES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClusterPropsResources, CPropertyPage)
	//{{AFX_MSG_MAP(CClusterPropsResources)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_RESOURCES, OnColumnclickListResources)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RESOURCES, OnItemchangedListResources)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_RESOURCES, OnDblclkListResources)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsResources message handlers


//
// WM_INITDIALOG message handler
//

BOOL CClusterPropsResources::OnInitDialog() 
{
	RECT rect;
	DWORD i;
	LVCOLUMN lvCol;

	CPropertyPage::OnInitDialog();

   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

	m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 2, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
	m_wndListCtrl.GetHeaderCtrl()->SetImageList(&m_imageList);

	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(1, _T("IP Address"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(2, _T("Owner"), LVCFMT_LEFT, rect.right - 250 - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// Create a copy of resource list
	m_dwNumResources = m_pObject->cluster.dwNumResources;
	m_pResourceList = (CLUSTER_RESOURCE *)nx_memdup(m_pObject->cluster.pResourceList, m_dwNumResources * sizeof(CLUSTER_RESOURCE));

	// Fill list control
	for(i = 0; i < m_dwNumResources; i++)
		AddListItem(&m_pResourceList[i]);
	m_wndListCtrl.SortItems(SortListItemsCB, (LPARAM)this);

   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_nSortDir == 1)  ? 1 : 0;
   m_wndListCtrl.SetColumn(m_nSortMode, &lvCol);

	EnableDlgItem(this, IDC_BUTTON_EDIT, FALSE);	
	EnableDlgItem(this, IDC_BUTTON_DELETE, FALSE);	

	return TRUE;
}


//
// Add new list item
//

void CClusterPropsResources::AddListItem(CLUSTER_RESOURCE *pResource)
{
	int nItem;
	TCHAR szIpAddr[32];
	NXC_OBJECT *pObject;

	nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pResource->szName);
	if (nItem != -1)
	{
		m_wndListCtrl.SetItemData(nItem, pResource->dwId);
		m_wndListCtrl.SetItemText(nItem, 1, IpToStr(pResource->dwIpAddr, szIpAddr));
		pObject = NXCFindObjectById(g_hSession, pResource->dwCurrOwner);
		m_wndListCtrl.SetItemText(nItem, 2, (pObject != NULL) ? pObject->szName : _T("<unknown>"));
	}
}


//
// Compare two list items
//

int CClusterPropsResources::CompareListItems(DWORD dwId1, DWORD dwId2)
{
	DWORD dwIndex1, dwIndex2;
	TCHAR *pszName1, *pszName2;
	NXC_OBJECT *pObject;
	int nRet;

	dwIndex1 = FindItemById(dwId1);
	dwIndex2 = FindItemById(dwId2);
	if ((dwIndex1 == INVALID_INDEX) || (dwIndex2 == INVALID_INDEX))	// Sanity check
		return 0;

	switch(m_nSortMode)
	{
		case 0:
			nRet = _tcsicmp(m_pResourceList[dwIndex1].szName, m_pResourceList[dwIndex2].szName);
			break;
		case 1:
			nRet = COMPARE_NUMBERS(m_pResourceList[dwIndex1].dwIpAddr, m_pResourceList[dwIndex2].dwIpAddr);
			break;
		case 2:
			pObject = NXCFindObjectById(g_hSession, m_pResourceList[dwIndex1].dwCurrOwner);
			pszName1 = (pObject != NULL) ? pObject->szName : _T("<unknown>");
			pObject = NXCFindObjectById(g_hSession, m_pResourceList[dwIndex2].dwCurrOwner);
			pszName2 = (pObject != NULL) ? pObject->szName : _T("<unknown>");
			nRet = _tcsicmp(pszName1, pszName2);
			break;
		default:
			break;
	}

	return nRet * m_nSortDir;
}


//
// Find list item by ID
//

DWORD CClusterPropsResources::FindItemById(DWORD dwId)
{
	DWORD i;

	for(i = 0; i , m_dwNumResources; i++)
		if (m_pResourceList[i].dwId == dwId)
			return i;
	return INVALID_INDEX;
}


//
// Handler for column clicks in list box
//

void CClusterPropsResources::OnColumnclickListResources(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW*)pNMHDR;
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_nSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_nSortMode == pNMListView->iSubItem)
   {
      // Same column, change sort direction
      m_nSortDir = -m_nSortDir;
   }
   else
   {
      // Another sorting column
      m_nSortMode = pNMListView->iSubItem;
   }
   m_wndListCtrl.SortItems(SortListItemsCB, (LPARAM)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_nSortDir == 1)  ? 1 : 0;
   m_wndListCtrl.SetColumn(pNMListView->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// "Add" button handler
//

void CClusterPropsResources::OnButtonAdd() 
{
	CClusterResDlg dlg;
	DWORD i, dwId;

	if (dlg.DoModal() == IDOK)
	{
		// Find free ID
		for(i = 0, dwId = 1; i < m_dwNumResources; i++)
			if (dwId <= m_pResourceList[i].dwId)
				dwId = m_pResourceList[i].dwId + 1;

		// Add new resource record
		m_dwNumResources++;
		m_pResourceList = (CLUSTER_RESOURCE *)realloc(m_pResourceList, m_dwNumResources * sizeof(CLUSTER_RESOURCE));
		m_pResourceList[i].dwId = dwId;
		m_pResourceList[i].dwIpAddr = dlg.m_dwIpAddr;
		nx_strncpy(m_pResourceList[i].szName, (LPCTSTR)dlg.m_strName, MAX_DB_STRING);
		m_pResourceList[i].dwCurrOwner = 0;
		
		AddListItem(&m_pResourceList[i]);
	   m_wndListCtrl.SortItems(SortListItemsCB, (LPARAM)this);

		m_pUpdate->qwFlags |= OBJ_UPDATE_RESOURCES;
		SetModified();
	}
}


//
// "Edit" button handler
//

void CClusterPropsResources::OnButtonEdit() 
{
	int nItem;
	DWORD dwIndex;
	TCHAR szIpAddr[32];
	CClusterResDlg dlg;

	if (m_wndListCtrl.GetSelectedCount() != 1)
		return;

	nItem = m_wndListCtrl.GetSelectionMark();
	dwIndex = FindItemById(m_wndListCtrl.GetItemData(nItem));
	if (dwIndex == INVALID_INDEX)
		return;

	dlg.m_dwIpAddr = m_pResourceList[dwIndex].dwIpAddr;
	dlg.m_strName = m_pResourceList[dwIndex].szName;
	if (dlg.DoModal() == IDOK)
	{
		m_pResourceList[dwIndex].dwIpAddr = dlg.m_dwIpAddr;
		nx_strncpy(m_pResourceList[dwIndex].szName, (LPCTSTR)dlg.m_strName, MAX_DB_STRING);

		m_wndListCtrl.SetItemText(nItem, 0, m_pResourceList[dwIndex].szName);
		m_wndListCtrl.SetItemText(nItem, 1, IpToStr(m_pResourceList[dwIndex].dwIpAddr, szIpAddr));
	   m_wndListCtrl.SortItems(SortListItemsCB, (LPARAM)this);

		m_pUpdate->qwFlags |= OBJ_UPDATE_RESOURCES;
		SetModified();
	}
}


//
// Delete button handler
//

void CClusterPropsResources::OnButtonDelete() 
{
	int nItem;
	DWORD dwIndex;

	if (m_wndListCtrl.GetSelectedCount() == 0)
		return;

	while((nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
	{
		dwIndex = FindItemById(m_wndListCtrl.GetItemData(nItem));
		if (dwIndex != INVALID_INDEX)
		{
			m_dwNumResources--;
			if (m_dwNumResources == 0)
			{
				safe_free_and_null(m_pResourceList);
			}
			else
			{
				memmove(&m_pResourceList[dwIndex], &m_pResourceList[dwIndex + 1],
				        sizeof(CLUSTER_RESOURCE) * (m_dwNumResources - dwIndex));
			}
		}
		m_wndListCtrl.DeleteItem(nItem);
	}

	m_pUpdate->qwFlags |= OBJ_UPDATE_RESOURCES;
	SetModified();
}


//
// List view item change handler
//

void CClusterPropsResources::OnItemchangedListResources(NMHDR* pNMHDR, LRESULT* pResult) 
{
	EnableDlgItem(this, IDC_BUTTON_EDIT, m_wndListCtrl.GetSelectedCount() == 1);	
	EnableDlgItem(this, IDC_BUTTON_DELETE, m_wndListCtrl.GetSelectedCount() > 0);	
	*pResult = 0;
}


//
// OK handler
//

void CClusterPropsResources::OnOK() 
{
	CPropertyPage::OnOK();
	m_pUpdate->dwNumResources = m_dwNumResources;
	m_pUpdate->pResourceList = m_pResourceList;
}


//
// Handler for double click in list control
//

void CClusterPropsResources::OnDblclkListResources(NMHDR* pNMHDR, LRESULT* pResult) 
{
	PostMessage(WM_COMMAND, IDC_BUTTON_EDIT, 0);
	*pResult = 0;
}
