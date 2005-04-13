// GraphDataPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "GraphDataPage.h"

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
   m_wndListCtrl.InsertColumn(3, _T("DCI"), LVCFMT_LEFT, 150);

   // Add items to list control
   for(i = 0; i < m_dwNumItems; i++)
   {
      _stprintf(szBuffer, _T("%d"), i + 1);
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
      m_wndListCtrl.SetItemData(iItem, i);
      pObject = NXCFindObjectById(g_hSession, m_pdwNodeId[i]);
      if (pObject != NULL)
         m_wndListCtrl.SetItemText(iItem, 1, pObject->szName);
   }
	
	return TRUE;
}
