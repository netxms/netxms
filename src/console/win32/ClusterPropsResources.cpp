// ClusterPropsResources.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterPropsResources.h"

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

	m_nSortMode = theApp.GetProfileInt(_T("ClusterResources"), _T("SortMode"), 0);
	m_nSortDir = theApp.GetProfileInt(_T("ClusterResources"), _T("SortDir"), 1);
}

CClusterPropsResources::~CClusterPropsResources()
{
	theApp.WriteProfileInt(_T("ClusterResources"), _T("SortMode"), m_nSortMode);
	theApp.WriteProfileInt(_T("ClusterResources"), _T("SortDir"), m_nSortDir);
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

	CPropertyPage::OnInitDialog();

	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 150);
	m_wndListCtrl.InsertColumn(1, _T("IP Address"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(2, _T("Owner"), LVCFMT_LEFT, rect.right - 250 - GetSystemMetrics(SM_CXVSCROLL));

	// Create a copy of resource list
	m_dwNumResources = m_pObject->cluster.dwNumResources;
	m_pResourceList = (CLUSTER_RESOURCE *)nx_memdup(m_pObject->cluster.pResourceList, m_dwNumResources * sizeof(CLUSTER_RESOURCE));

	// Fill list control
	for(i = 0; i < m_dwNumResources; i++)
		AddListItem(&m_pResourceList[i]);
	m_wndListCtrl.SortItems(SortListItemsCB, (LPARAM)this);

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
