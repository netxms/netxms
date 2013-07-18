// AddDCIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AddDCIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddDCIDlg dialog


CAddDCIDlg::CAddDCIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddDCIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddDCIDlg)
	//}}AFX_DATA_INIT
}


void CAddDCIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDCIDlg)
	DDX_Control(pDX, IDC_LIST_DCI, m_wndListDCI);
	DDX_Control(pDX, IDC_LIST_NODES, m_wndListNodes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDCIDlg, CDialog)
	//{{AFX_MSG_MAP(CAddDCIDlg)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_NODES, OnItemchangedListNodes)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DCI, OnDblclkListDci)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddDCIDlg message handlers

BOOL CAddDCIDlg::OnInitDialog() 
{
   NXC_OBJECT_INDEX *pIndex;
   UINT32 i, dwNumObjects;
   RECT rect;
   int iItem;

	CDialog::OnInitDialog();
	
   // Prepare image list
   m_imageList.Create(g_pObjectSmallImageList);
	
   // Setup node list
   m_wndListNodes.SetImageList(&m_imageList, LVSIL_SMALL);
   m_wndListNodes.GetClientRect(&rect);
   m_wndListNodes.InsertColumn(0, _T("Name"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListNodes.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

   // Fill in node list
   NXCLockObjectIndex(g_hSession);
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
   for(i = 0; i < dwNumObjects; i++)
      if ((pIndex[i].pObject->iClass == OBJECT_NODE) &&
          (!pIndex[i].pObject->bIsDeleted))
      {
         iItem = m_wndListNodes.InsertItem(0x7FFFFFFF, pIndex[i].pObject->szName, pIndex[i].pObject->iClass);
         m_wndListNodes.SetItemData(iItem, pIndex[i].pObject->dwId);
      }
   NXCUnlockObjectIndex(g_hSession);

   // Setup DCI list
   m_wndListDCI.GetClientRect(&rect);
   m_wndListDCI.InsertColumn(0, _T("Parameter"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListDCI.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
	
	return TRUE;
}


//
// Handler for node list item change
//

void CAddDCIDlg::OnItemchangedListNodes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
   DWORD i, dwNodeId, dwNumItems, dwResult;
   NXC_DCI_VALUE *pValueList;
   int iItem;

   if ((pNMListView->uChanged == LVIF_STATE) &&
       (pNMListView->uNewState & LVIS_FOCUSED) &&
       (pNMListView->uNewState & LVIS_SELECTED))
   {
      m_wndListDCI.DeleteAllItems();
      dwNodeId = m_wndListNodes.GetItemData(pNMListView->iItem);
      dwResult = DoRequestArg4(NXCGetLastValues, g_hSession, (void *)dwNodeId, &dwNumItems,
                               &pValueList, _T("Requesting DCI list for selected node..."));
      if (dwResult == RCC_SUCCESS)
      {
         for(i = 0; i < dwNumItems; i++)
         {
            iItem = m_wndListDCI.InsertItem(i, pValueList[i].szDescription);
            if (iItem != -1)
            {
               m_wndListDCI.SetItemData(iItem, pValueList[i].dwId);
            }
         }
         safe_free(pValueList);
      }
   }
	
	*pResult = 0;
}


//
// OK button handler
//

void CAddDCIDlg::OnOK() 
{
   int iItem;

   iItem = m_wndListNodes.GetNextItem(-1, LVNI_SELECTED);
   if (iItem != -1)
   {
      m_dwNodeId = m_wndListNodes.GetItemData(iItem);
      iItem = m_wndListDCI.GetNextItem(-1, LVNI_SELECTED);
      if (iItem != -1)
      {
         m_dwItemId = m_wndListDCI.GetItemData(iItem);
         m_strItemName = m_wndListDCI.GetItemText(iItem, 0);
      	CDialog::OnOK();
         return;
      }
   }
   MessageBox(_T("You should select data collection item before pressing OK"),
              _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
}


//
// Handler for double click in DCI list
//

void CAddDCIDlg::OnDblclkListDci(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if (m_wndListDCI.GetSelectedCount() > 0)
		PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
