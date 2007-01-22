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

/////////////////////////////////////////////////////////////////////////////
// CGraphDataPage property page

IMPLEMENT_DYNCREATE(CGraphDataPage, CPropertyPage)

CGraphDataPage::CGraphDataPage() : CPropertyPage(CGraphDataPage::IDD)
{
	//{{AFX_DATA_INIT(CGraphDataPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGraphDataPage::~CGraphDataPage()
{
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
   int iItem;
   TCHAR szBuffer[256];
   NXC_OBJECT *pObject;

	CPropertyPage::OnInitDialog();

   // Setup list control
   m_wndListCtrl.InsertColumn(0, _T("Pos."), LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(1, _T("Node"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, 150);
   m_wndListCtrl.InsertColumn(4, _T("Description"), LVCFMT_LEFT, 150);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Add items to list control
   for(i = 0; i < m_dwNumItems; i++)
   {
      _stprintf(szBuffer, _T("%d"), i + 1);
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
      m_wndListCtrl.SetItemData(iItem, i);
      pObject = NXCFindObjectById(g_hSession, m_ppItems[i]->m_dwNodeId);
      if (pObject != NULL)
         m_wndListCtrl.SetItemText(iItem, 1, pObject->szName);
      else
         m_wndListCtrl.SetItemText(iItem, 1, _T("<unknown>"));
      m_wndListCtrl.SetItemText(iItem, 2, g_pszItemOrigin[m_ppItems[i]->m_iSource]);
      m_wndListCtrl.SetItemText(iItem, 3, m_ppItems[i]->m_pszParameter);
      m_wndListCtrl.SetItemText(iItem, 4, m_ppItems[i]->m_pszDescription);
   }
	
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

	if (dlg.DoModal())
	{
		m_ppItems[m_dwNumItems] = new DCIInfo;
		m_ppItems[m_dwNumItems]->m_dwNodeId = dlg.m_dwNodeId;
		m_ppItems[m_dwNumItems]->m_dwItemId = dlg.m_dwItemId;
	}
}
