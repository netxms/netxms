// CondPropsData.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CondPropsData.h"

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
   DWORD i;

	CPropertyPage::OnInitDialog();
	
   m_wndListCtrl.InsertColumn(0, _T("Pos"), LVCFMT_LEFT, 50);
   m_wndListCtrl.InsertColumn(1, _T("ID"), LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(2, _T("Node"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(3, _T("Parameter"), LVCFMT_LEFT, 100);

   m_dwNumDCI = m_pObject->cond.dwNumDCI;
   m_pDCIList = (INPUT_DCI *)nx_memdup(m_pObject->cond.pDCIList, sizeof(INPUT_DCI) * m_dwNumDCI);
   for(i = 0; i < m_dwNumDCI; i++)
      AddListItem(i, &m_pDCIList[i]);
	
	return TRUE;
}


//
// Add item to list
//

void CCondPropsData::AddListItem(int nPos, INPUT_DCI *pItem)
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
   }
}


//
// "Add..." button handler
//

void CCondPropsData::OnButtonAdd() 
{
}
