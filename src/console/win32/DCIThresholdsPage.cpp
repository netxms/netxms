// DCIThresholdsPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIThresholdsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCIThresholdsPage property page

IMPLEMENT_DYNCREATE(CDCIThresholdsPage, CPropertyPage)

CDCIThresholdsPage::CDCIThresholdsPage() : CPropertyPage(CDCIThresholdsPage::IDD)
{
	//{{AFX_DATA_INIT(CDCIThresholdsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDCIThresholdsPage::~CDCIThresholdsPage()
{
}

void CDCIThresholdsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIThresholdsPage)
	DDX_Control(pDX, IDC_LIST_THRESHOLDS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCIThresholdsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCIThresholdsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCIThresholdsPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDCIThresholdsPage::OnInitDialog() 
{
   RECT rect;
   int iItem;
   DWORD i;

	CPropertyPage::OnInitDialog();
	
   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, "Op", LVCFMT_LEFT, 30);
   m_wndListCtrl.InsertColumn(1, "Value", LVCFMT_LEFT, rect.right - 110 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.InsertColumn(2, "Event", LVCFMT_LEFT, 80);

   // Add items to list
   for(i = 0; i < m_pItem->dwNumThresholds; i++)
   {
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, g_szOperationSymbols[m_pItem->pThresholdList[i].wOperation]);
   }
	
	return TRUE;
}
