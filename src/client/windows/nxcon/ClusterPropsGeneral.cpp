// ClusterPropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterPropsGeneral.h"
#include "ObjectPropSheet.h"
#include "EditSubnetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsGeneral property page

IMPLEMENT_DYNCREATE(CClusterPropsGeneral, CPropertyPage)

CClusterPropsGeneral::CClusterPropsGeneral() : CPropertyPage(CClusterPropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CClusterPropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	//}}AFX_DATA_INIT

	m_pNetList = NULL;
}

CClusterPropsGeneral::~CClusterPropsGeneral()
{
	safe_free(m_pNetList);
}

void CClusterPropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClusterPropsGeneral)
	DDX_Control(pDX, IDC_LIST_NETWORKS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClusterPropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CClusterPropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClusterPropsGeneral message handlers


//
// Callback for list item sorting
//

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return COMPARE_NUMBERS(lParam2, lParam1);
}


//
// WM_INITDIALOG message handler
//

BOOL CClusterPropsGeneral::OnInitDialog() 
{
	DWORD i;
	int nItem;
	TCHAR szBuffer[32];
	RECT rect;

	CPropertyPage::OnInitDialog();

   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

	// Setup sync net list
	m_wndListCtrl.GetClientRect(&rect);
	rect.right -= GetSystemMetrics(SM_CXVSCROLL);
	rect.right /= 2;
	m_wndListCtrl.InsertColumn(0, _T("Subnet"), LVCFMT_LEFT, rect.right);
	m_wndListCtrl.InsertColumn(1, _T("Mask"), LVCFMT_LEFT, rect.right);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	for(i = 0; i < m_pObject->cluster.dwNumSyncNets; i++)
	{
		nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, IpToStr(m_pObject->cluster.pSyncNetList[i].dwAddr, szBuffer));
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemText(nItem, 1, IpToStr(m_pObject->cluster.pSyncNetList[i].dwMask, szBuffer));
			m_wndListCtrl.SetItemData(nItem, m_pObject->cluster.pSyncNetList[i].dwAddr);
		}
	}
	m_wndListCtrl.SortItems(CompareItems, 0);
	
	return TRUE;
}


//
// Handler for changed object name
//

void CClusterPropsGeneral::OnChangeEditName() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}


//
// PSN_OK handler
//

void CClusterPropsGeneral::OnOK() 
{
	DWORD i;
	TCHAR szBuffer[32];

	CPropertyPage::OnOK();
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
	if (m_pUpdate->qwFlags & OBJ_UPDATE_SYNC_NETS)
	{
		m_pUpdate->dwNumSyncNets = m_wndListCtrl.GetItemCount();
		if (m_pUpdate->dwNumSyncNets > 0)
		{
			m_pNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * m_pUpdate->dwNumSyncNets);
			for(i = 0; i < m_pUpdate->dwNumSyncNets; i++)
			{
				m_pNetList[i].dwAddr = (DWORD)m_wndListCtrl.GetItemData(i);
				m_wndListCtrl.GetItemText(i, 1, szBuffer, 32);
				m_pNetList[i].dwMask = ntohl(_t_inet_addr(szBuffer));
			}
			m_pUpdate->pSyncNetList = m_pNetList;
		}
	}
}


//
// "Add" button handler
//

void CClusterPropsGeneral::OnButtonAdd() 
{
   CEditSubnetDlg dlg;
	TCHAR szBuffer[32];
	int nItem;

   if (dlg.DoModal() == IDOK)
   {
		nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, IpToStr(dlg.m_subnet.dwAddr, szBuffer));
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemText(nItem, 1, IpToStr(dlg.m_subnet.dwMask, szBuffer));
			m_wndListCtrl.SetItemData(nItem, dlg.m_subnet.dwAddr);
		}
		m_wndListCtrl.SortItems(CompareItems, 0);

      m_pUpdate->qwFlags |= OBJ_UPDATE_SYNC_NETS;
      SetModified();
   }
}


//
// "Delete" button handler
//

void CClusterPropsGeneral::OnButtonDelete() 
{
	int nItem;

	if (m_wndListCtrl.GetSelectedCount() > 0)
	{
		while((nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
		{
			m_wndListCtrl.DeleteItem(nItem);
		}
		m_pUpdate->qwFlags |= OBJ_UPDATE_SYNC_NETS;
		SetModified();
	}
}
