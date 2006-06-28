// CondPropsData.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondPropsData.h"
#include "AddDCIDlg.h"
#include "ObjectPropSheet.h"

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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCondPropsData message handlers

BOOL CCondPropsData::OnInitDialog() 
{
   DWORD i, dwResult;
   TCHAR **ppszNames;

	CPropertyPage::OnInitDialog();
	
   m_wndListCtrl.InsertColumn(0, _T("Pos"), LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(1, _T("ID"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Node"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, 100);

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

void CCondPropsData::AddListItem(int nPos, INPUT_DCI *pItem, TCHAR *pszName)
{
   int iItem;
   TCHAR szBuffer[64];
   NXC_OBJECT *pObject;

   _stprintf(szBuffer, _T("%d"), nPos + 1);
   iItem = m_wndListCtrl.InsertItem(nPos, szBuffer);
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, nPos);

      _stprintf(szBuffer, _T("%d"), pItem->dwId);
      m_wndListCtrl.SetItemText(iItem, 1, szBuffer);

      pObject = NXCFindObjectById(g_hSession, pItem->dwNodeId);
      if (pObject != NULL)
      {
         m_wndListCtrl.SetItemText(iItem, 2, pObject->szName);
      }
      else
      {
         m_wndListCtrl.SetItemText(iItem, 2, _T("<unknown>"));
      }

      m_wndListCtrl.SetItemText(iItem, 3, pszName);
   }
}


//
// "Add..." button handler
//

void CCondPropsData::OnButtonAdd() 
{
   CAddDCIDlg dlg;

   if (dlg.DoModal() == IDOK)
   {
      m_pDCIList = (INPUT_DCI *)realloc(m_pDCIList, sizeof(INPUT_DCI) * (m_dwNumDCI + 1));
      
      m_pDCIList[m_dwNumDCI].dwId = dlg.m_dwItemId;
      m_pDCIList[m_dwNumDCI].dwNodeId = dlg.m_dwNodeId;
      m_pDCIList[m_dwNumDCI].nFunction = F_LAST;
      m_pDCIList[m_dwNumDCI].nPolls = 1;

      AddListItem(m_dwNumDCI, &m_pDCIList[m_dwNumDCI], (TCHAR *)((LPCTSTR)dlg.m_strItemName));
      m_dwNumDCI++;
      SetModified();
   }
}


//
// OK handler
//

void CCondPropsData::OnOK() 
{
   NXC_OBJECT_UPDATE *pUpdate;
   	
	CPropertyPage::OnOK();
   pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
   pUpdate->dwFlags |= OBJ_UPDATE_DCI_LIST;
   pUpdate->dwNumDCI = m_dwNumDCI;
   pUpdate->pDCIList = m_pDCIList;
}
