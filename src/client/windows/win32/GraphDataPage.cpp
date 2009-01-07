// GraphDataPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphDataPage.h"
#include "AddDCIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Callback for list sorting function
//

static int CALLBACK CBSortListItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	return ((CGraphDataPage *)lParamSort)->CompareListItems(lParam1, lParam2);
}


/////////////////////////////////////////////////////////////////////////////
// CGraphDataPage property page

IMPLEMENT_DYNCREATE(CGraphDataPage, CPropertyPage)

CGraphDataPage::CGraphDataPage() : CPropertyPage(CGraphDataPage::IDD)
{
	//{{AFX_DATA_INIT(CGraphDataPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_nSortMode = theApp.GetProfileInt(_T("GraphDataPage"), _T("SortMode"), 0);
	m_nSortDir = theApp.GetProfileInt(_T("GraphDataPage"), _T("SortDir"), 1);
}

CGraphDataPage::~CGraphDataPage()
{
	theApp.WriteProfileInt(_T("GraphDataPage"), _T("SortMode"), m_nSortMode);
	theApp.WriteProfileInt(_T("GraphDataPage"), _T("SortDir"), m_nSortDir);
}

void CGraphDataPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGraphDataPage)
	DDX_Control(pDX, IDC_LIST_DCI, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGraphDataPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGraphDataPage)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_DCI, OnColumnclickListDci)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DCI, OnItemchangedListDci)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGraphDataPage message handlers

//
// WM_INITDIALOG message handler
//

BOOL CGraphDataPage::OnInitDialog() 
{
   DWORD i;
   LVCOLUMN lvCol;

	CPropertyPage::OnInitDialog();

   // Setup list control
   m_wndListCtrl.InsertColumn(0, _T("Pos."), LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, _T("Node"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(4, _T("Description"), LVCFMT_LEFT, 150);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   m_imageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 2, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_imageList.Add(theApp.LoadIcon(IDI_SORT_UP));
	m_wndListCtrl.GetHeaderCtrl()->SetImageList(&m_imageList);

   // Add items to list control
   for(i = 0; i < m_dwNumItems; i++)
		AddListItem(i);
	m_wndListCtrl.SortItems(CBSortListItems, (LPARAM)this);
	
   // Mark sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_nSortDir == -1) ? 0 : 1;
   m_wndListCtrl.SetColumn(m_nSortMode, &lvCol);

	EnableDlgItem(this, IDC_BUTTON_DELETE, FALSE);
	EnableDlgItem(this, IDC_BUTTON_UP, FALSE);
	EnableDlgItem(this, IDC_BUTTON_DOWN, FALSE);

	return TRUE;
}


//
// "Delete" button handler
//

void CGraphDataPage::OnButtonDelete() 
{
	int i, nItem, nPos, nData;
	TCHAR szBuffer[32];

	if (m_wndListCtrl.GetSelectedCount() == 0)
		return;

	while((nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
	{
		nPos = m_wndListCtrl.GetItemData(nItem);
		for(i = 0; i < m_wndListCtrl.GetItemCount(); i++)
		{
			nData = m_wndListCtrl.GetItemData(i);
			if (nData > nPos)
			{
				m_wndListCtrl.SetItemData(i, nData - 1);
				_stprintf(szBuffer, _T("%d"), nData);
				m_wndListCtrl.SetItemText(i, 0, szBuffer);
			}
		}
		delete m_ppItems[nPos];
		memmove(&m_ppItems[nPos], &m_ppItems[nPos + 1], sizeof(DCIInfo *) * (MAX_GRAPH_ITEMS - nPos - 1));
		m_ppItems[MAX_GRAPH_ITEMS - 1] = NULL;
		m_dwNumItems--;
		m_wndListCtrl.DeleteItem(nItem);
	}
}


//
// "Add" button handler
//

void CGraphDataPage::OnButtonAdd() 
{
	CAddDCIDlg dlg;
	DWORD dwResult;
	NXC_DCI dci;

	if (m_dwNumItems == MAX_GRAPH_ITEMS)
	{
		MessageBox(_T("You cannot add more than 16 items to one graph"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if (dlg.DoModal() == IDOK)
	{
		dwResult = DoRequestArg4(NXCGetDCIInfo, g_hSession, (void *)dlg.m_dwNodeId,
		                         (void *)dlg.m_dwItemId, &dci, _T("Retrieving DCI information..."));
		if (dwResult == RCC_SUCCESS)
		{
			m_ppItems[m_dwNumItems] = new DCIInfo(dlg.m_dwNodeId, &dci);
			AddListItem(m_dwNumItems);
			m_dwNumItems++;
		   m_wndListCtrl.SortItems(CBSortListItems, (DWORD)this);
		}
		else
		{
			theApp.ErrorBox(dwResult, _T("Cannot get detailed DCI information: %s"));
		}
	}
}


//
// Add item to list control
//

void CGraphDataPage::AddListItem(DWORD dwIndex)
{
   int iItem;
   TCHAR szBuffer[256];
   NXC_OBJECT *pObject;

   _stprintf(szBuffer, _T("%d"), dwIndex + 1);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
   m_wndListCtrl.SetItemData(iItem, dwIndex);
   pObject = NXCFindObjectById(g_hSession, m_ppItems[dwIndex]->m_dwNodeId);
   if (pObject != NULL)
      m_wndListCtrl.SetItemText(iItem, 1, pObject->szName);
   else
      m_wndListCtrl.SetItemText(iItem, 1, _T("<unknown>"));
   m_wndListCtrl.SetItemText(iItem, 2, g_pszItemOrigin[m_ppItems[dwIndex]->m_iSource]);
   m_wndListCtrl.SetItemText(iItem, 3, m_ppItems[dwIndex]->m_pszParameter);
   m_wndListCtrl.SetItemText(iItem, 4, m_ppItems[dwIndex]->m_pszDescription);
}


//
// Compare two list items
//

int CGraphDataPage::CompareListItems(int nItem1, int nItem2)
{
	int nResult;
	NXC_OBJECT *pNode1, *pNode2;

	switch(m_nSortMode)
	{
		case 0:		// Position
			nResult = COMPARE_NUMBERS(nItem1, nItem2);
			break;
		case 1:		// Node
			pNode1 = NXCFindObjectById(g_hSession, m_ppItems[nItem1]->m_dwNodeId);
			pNode2 = NXCFindObjectById(g_hSession, m_ppItems[nItem2]->m_dwNodeId);
			nResult = _tcscmp((pNode1 != NULL) ? pNode1->szName : _T("<unknown>"),
			                  (pNode2 != NULL) ? pNode2->szName : _T("<unknown>"));
			break;
		case 2:		// Type
			nResult = _tcscmp(g_pszItemOrigin[m_ppItems[nItem1]->m_iSource], g_pszItemOrigin[m_ppItems[nItem2]->m_iSource]);
			break;
		case 3:		// Parameter
			nResult = _tcsicmp(m_ppItems[nItem1]->m_pszParameter, m_ppItems[nItem2]->m_pszParameter);
			break;
		case 4:		// Description
			nResult = _tcsicmp(m_ppItems[nItem1]->m_pszDescription, m_ppItems[nItem2]->m_pszDescription);
			break;
		default:
			nResult = 0;
			break;
	}

	return nResult * m_nSortDir;
}


//
// Handler for list control column click
//

void CGraphDataPage::OnColumnclickListDci(LPNMHDR pNMHDR, LRESULT *pResult)
{
   LVCOLUMN lvCol;

   // Unmark old sorting column
   lvCol.mask = LVCF_FMT;
   lvCol.fmt = LVCFMT_LEFT;
   m_wndListCtrl.SetColumn(m_nSortMode, &lvCol);

   // Change current sort mode and resort list
   if (m_nSortMode == ((LPNMLISTVIEW)pNMHDR)->iSubItem)
   {
      // Same column, change sort direction
      m_nSortDir = -m_nSortDir;
   }
   else
   {
      // Another sorting column
      m_nSortMode = ((LPNMLISTVIEW)pNMHDR)->iSubItem;
   }
   m_wndListCtrl.SortItems(CBSortListItems, (DWORD)this);

   // Mark new sorting column
   lvCol.mask = LVCF_IMAGE | LVCF_FMT;
   lvCol.fmt = LVCFMT_BITMAP_ON_RIGHT | LVCFMT_IMAGE | LVCFMT_LEFT;
   lvCol.iImage = (m_nSortDir == -1) ? 0 : 1;
   m_wndListCtrl.SetColumn(((LPNMLISTVIEW)pNMHDR)->iSubItem, &lvCol);
   
   *pResult = 0;
}


//
// Handler for list control item changes
//

void CGraphDataPage::OnItemchangedListDci(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	EnableDlgItem(this, IDC_BUTTON_DELETE, m_wndListCtrl.GetSelectedCount() > 0);
	EnableDlgItem(this, IDC_BUTTON_UP, m_wndListCtrl.GetSelectedCount() == 1);
	EnableDlgItem(this, IDC_BUTTON_DOWN, m_wndListCtrl.GetSelectedCount() == 1);
	
	*pResult = 0;
}


//
// Handler for "Move down" button
//

void CGraphDataPage::OnButtonDown() 
{
	int nItem, nItem2;
	DWORD dwIndex;
	LVFINDINFO lvfi;
	DCIInfo *pTemp;
	TCHAR szBuffer[32];

	nItem = m_wndListCtrl.GetSelectionMark();
	if ((nItem == -1) || (m_dwNumItems < 2))
		return;

	dwIndex = m_wndListCtrl.GetItemData(nItem);
	if (dwIndex < m_dwNumItems - 1)
	{
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = dwIndex + 1;
		nItem2 = m_wndListCtrl.FindItem(&lvfi);
		if (nItem2 != -1)
		{
			_stprintf(szBuffer, _T("%d"), dwIndex + 1);
			m_wndListCtrl.SetItemText(nItem2, 0, szBuffer);
			m_wndListCtrl.SetItemData(nItem2, dwIndex);
			_stprintf(szBuffer, _T("%d"), dwIndex + 2);
			m_wndListCtrl.SetItemText(nItem, 0, szBuffer);
			m_wndListCtrl.SetItemData(nItem, dwIndex + 1);
			pTemp = m_ppItems[dwIndex];
			m_ppItems[dwIndex] = m_ppItems[dwIndex + 1];
			m_ppItems[dwIndex + 1] = pTemp;
			if (m_nSortMode == 0)   // Resort items if current sorting mode is "by position"
			   m_wndListCtrl.SortItems(CBSortListItems, (DWORD)this);
		}
	}
}


//
// Handler for "Move up" button
//

void CGraphDataPage::OnButtonUp() 
{
	int nItem, nItem2;
	DWORD dwIndex;
	LVFINDINFO lvfi;
	DCIInfo *pTemp;
	TCHAR szBuffer[32];

	nItem = m_wndListCtrl.GetSelectionMark();
	if (nItem == -1)
		return;

	dwIndex = m_wndListCtrl.GetItemData(nItem);
	if (dwIndex > 0)
	{
		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = dwIndex - 1;
		nItem2 = m_wndListCtrl.FindItem(&lvfi);
		if (nItem2 != -1)
		{
			_stprintf(szBuffer, _T("%d"), dwIndex + 1);
			m_wndListCtrl.SetItemText(nItem2, 0, szBuffer);
			m_wndListCtrl.SetItemData(nItem2, dwIndex);
			_stprintf(szBuffer, _T("%d"), dwIndex);
			m_wndListCtrl.SetItemText(nItem, 0, szBuffer);
			m_wndListCtrl.SetItemData(nItem, dwIndex - 1);
			pTemp = m_ppItems[dwIndex];
			m_ppItems[dwIndex] = m_ppItems[dwIndex - 1];
			m_ppItems[dwIndex - 1] = pTemp;
			if (m_nSortMode == 0)   // Resort items if current sorting mode is "by position"
			   m_wndListCtrl.SortItems(CBSortListItems, (DWORD)this);
		}
	}
}
