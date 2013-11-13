// CondPropsData.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondPropsData.h"
#include "AddDCIDlg.h"
#include "ObjectPropSheet.h"
#include "CondDCIPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCondPropsData property page

IMPLEMENT_DYNCREATE(CCondPropsData, CPropertyPage)

CCondPropsData::CCondPropsData() : CPropertyPage(CCondPropsData::IDD)
{
	//{{AFX_DATA_INIT(CCondPropsData)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_dwNumDCI = 0;
   m_pDCIList = NULL;
}

CCondPropsData::~CCondPropsData()
{
   safe_free(m_pDCIList);
}

void CCondPropsData::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCondPropsData)
	DDX_Control(pDX, IDC_LIST_DCI, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCondPropsData, CPropertyPage)
	//{{AFX_MSG_MAP(CCondPropsData)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DCI, OnItemchangedListDci)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DCI, OnDblclkListDci)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCondPropsData message handlers

BOOL CCondPropsData::OnInitDialog() 
{
   DWORD i, dwResult;
   TCHAR **ppszNames;
   RECT rect;

	CPropertyPage::OnInitDialog();

   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   EnableDlgItem(this, IDC_BUTTON_EDIT, FALSE);
   EnableDlgItem(this, IDC_BUTTON_DELETE, FALSE);
   EnableDlgItem(this, IDC_BUTTON_UP, FALSE);
   EnableDlgItem(this, IDC_BUTTON_DOWN, FALSE);

   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Pos"), LVCFMT_LEFT, 35);
   m_wndListCtrl.InsertColumn(1, _T("ID"), LVCFMT_LEFT, 40);
   m_wndListCtrl.InsertColumn(2, _T("Node"), LVCFMT_LEFT, 90);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, rect.right - 235 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.InsertColumn(4, _T("Function"), LVCFMT_LEFT, 70);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);

   m_dwNumDCI = m_pObject->cond.dwNumDCI;
   m_pDCIList = (INPUT_DCI *)nx_memdup(m_pObject->cond.pDCIList, sizeof(INPUT_DCI) * m_dwNumDCI);
   
   dwResult = DoRequestArg4(NXCResolveDCINames, g_hSession, (void *)m_dwNumDCI,
                            m_pDCIList, &ppszNames, _T("Resolving DCI names..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < m_dwNumDCI; i++)
      {
         AddListItem(i, &m_pDCIList[i], CHECK_NULL(ppszNames[i]));
         safe_free(ppszNames[i]);
      }
      safe_free(ppszNames);
   }
   else
   {
      for(i = 0; i < m_dwNumDCI; i++)
         AddListItem(i, &m_pDCIList[i], _T("<unresolved>"));
   }
   
	return TRUE;
}


//
// Add item to list
//

int CCondPropsData::AddListItem(int nPos, INPUT_DCI *pItem, TCHAR *pszName)
{
   int iItem;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;

   _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%d"), nPos + 1);
   iItem = m_wndListCtrl.InsertItem(nPos, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, nPos);

      _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%d"), pItem->id);
      m_wndListCtrl.SetItemText(iItem, 1, szBuffer);

      pObject = NXCFindObjectById(g_hSession, pItem->nodeId);
      if (pObject != NULL)
      {
         m_wndListCtrl.SetItemText(iItem, 2, pObject->szName);
      }
      else
      {
         m_wndListCtrl.SetItemText(iItem, 2, _T("<unknown>"));
      }

      m_wndListCtrl.SetItemText(iItem, 3, pszName);

      if ((pItem->function == F_AVERAGE) ||
          (pItem->function == F_DEVIATION) ||
          (pItem->function == F_ERROR))
      {
         _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%s(%d)"),
                      g_pszThresholdFunction[pItem->function], pItem->polls);
         m_wndListCtrl.SetItemText(iItem, 4, szBuffer);
      }
      else
      {
         m_wndListCtrl.SetItemText(iItem, 4, g_pszThresholdFunction[pItem->function]);
      }
   }
   return iItem;
}


//
// "Add..." button handler
//

void CCondPropsData::OnButtonAdd() 
{
   CAddDCIDlg dlg;
   int iItem;

   if (dlg.DoModal() == IDOK)
   {
      m_pDCIList = (INPUT_DCI *)realloc(m_pDCIList, sizeof(INPUT_DCI) * (m_dwNumDCI + 1));
      
      m_pDCIList[m_dwNumDCI].id = dlg.m_dwItemId;
      m_pDCIList[m_dwNumDCI].nodeId = dlg.m_dwNodeId;
      m_pDCIList[m_dwNumDCI].function = F_LAST;
      m_pDCIList[m_dwNumDCI].polls = 1;

      iItem = AddListItem(m_dwNumDCI, &m_pDCIList[m_dwNumDCI], (TCHAR *)((LPCTSTR)dlg.m_strItemName));
      m_dwNumDCI++;
      Modify();

      // Open "DCI properties" dialog
      SelectListViewItem(&m_wndListCtrl, iItem);
      PostMessage(WM_COMMAND, IDC_BUTTON_EDIT, 0);
   }
}


//
// OK handler
//

void CCondPropsData::OnOK() 
{
	CPropertyPage::OnOK();
   m_pUpdate->dwNumDCI = m_dwNumDCI;
   m_pUpdate->pDCIList = m_pDCIList;
}


//
// "Delete" button handler
//

void CCondPropsData::OnButtonDelete() 
{
   int i, iItem, nIndex;
   TCHAR szBuffer[32];

   while((iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
   {
      nIndex = (int)m_wndListCtrl.GetItemData(iItem);
      m_dwNumDCI--;
      memmove(&m_pDCIList[nIndex], &m_pDCIList[nIndex + 1], sizeof(INPUT_DCI) * (m_dwNumDCI - nIndex));
      m_wndListCtrl.DeleteItem(iItem);
      for(i = iItem; m_wndListCtrl.SetItemData(i, i); i++)
      {
         _sntprintf_s(szBuffer, 32, _TRUNCATE, _T("%d"), i + 1);
         m_wndListCtrl.SetItemText(i, 0, szBuffer);
      }
   }
   Modify();
}


//
// Handler for changes in DCI list
//

void CCondPropsData::OnItemchangedListDci(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
   UINT nCount;

   nCount = m_wndListCtrl.GetSelectedCount();
   EnableDlgItem(this, IDC_BUTTON_EDIT, nCount == 1);
   EnableDlgItem(this, IDC_BUTTON_DELETE, nCount > 0);
   EnableDlgItem(this, IDC_BUTTON_UP, nCount == 1);
   EnableDlgItem(this, IDC_BUTTON_DOWN, nCount == 1);
	
	*pResult = 0;
}


//
// "Edit" button handler
//

void CCondPropsData::OnButtonEdit() 
{
   CCondDCIPropDlg dlg;
   int iItem, nIndex;

   iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   if (iItem != -1)
   {
      nIndex = (int)m_wndListCtrl.GetItemData(iItem);
      dlg.m_nFunction = m_pDCIList[nIndex].function;
      dlg.m_nPolls = m_pDCIList[nIndex].polls;
      dlg.m_strNode = m_wndListCtrl.GetItemText(iItem, 2);
      dlg.m_strItem = m_wndListCtrl.GetItemText(iItem, 3);
      if (dlg.DoModal() == IDOK)
      {
         m_pDCIList[nIndex].function = dlg.m_nFunction;
         m_pDCIList[nIndex].polls = dlg.m_nPolls;

         // Update text in list control
         if ((m_pDCIList[nIndex].function == F_AVERAGE) ||
             (m_pDCIList[nIndex].function == F_DEVIATION) ||
             (m_pDCIList[nIndex].function == F_ERROR))
         {
            TCHAR szBuffer[64];

            _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("%s(%d)"),
                         g_pszThresholdFunction[m_pDCIList[nIndex].function],
                         m_pDCIList[nIndex].polls);
            m_wndListCtrl.SetItemText(iItem, 4, szBuffer);
         }
         else
         {
            m_wndListCtrl.SetItemText(iItem, 4, g_pszThresholdFunction[m_pDCIList[nIndex].function]);
         }

         Modify();
      }
   }
}


//
// Handler for double-click in list view
//

void CCondPropsData::OnDblclkListDci(NMHDR* pNMHDR, LRESULT* pResult) 
{
   PostMessage(WM_COMMAND, IDC_BUTTON_EDIT, 0);
	*pResult = 0;
}
