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
	//}}AFX_DATA_INIT
}

CDCIThresholdsPage::~CDCIThresholdsPage()
{
}

void CDCIThresholdsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIThresholdsPage)
	DDX_Control(pDX, IDC_BUTTON_MOVEDOWN, m_wndButtonDown);
	DDX_Control(pDX, IDC_BUTTON_MOVEUP, m_wndButtonUp);
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
   DWORD i;

	CPropertyPage::OnInitDialog();

   // Setup buttons
   m_wndButtonUp.SetBitmap(CreateMappedBitmap(theApp.m_hInstance, IDB_UP_ARROW, 0, NULL, 0));
   m_wndButtonDown.SetBitmap(CreateMappedBitmap(theApp.m_hInstance, IDB_DOWN_ARROW, 0, NULL, 0));
	
   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, "Operation", LVCFMT_LEFT, rect.right - 80 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.InsertColumn(1, "Event", LVCFMT_LEFT, 80);

   // Add items to list
   for(i = 0; i < m_pItem->dwNumThresholds; i++)
      AddListEntry(i);
	
	return TRUE;
}


//
// Add new entry to list
//

int CDCIThresholdsPage::AddListEntry(DWORD dwIndex)
{
   char szBuffer[256], szArgs[256], szValue[MAX_DCI_STRING_VALUE];
   int iItem;

   switch(m_pItem->pThresholdList[dwIndex].wFunction)
   {
      case F_LAST:
         szArgs[0] = 0;
         break;
      case F_AVERAGE:
         sprintf(szArgs, "%d", m_pItem->pThresholdList[dwIndex].dwArg1);
         break;
      case F_DEVIATION:
         szArgs[0] = 0;
         break;
   }

   switch(m_pItem->iDataType)
   {
      case DTYPE_INTEGER:
         sprintf(szValue, "%d", m_pItem->pThresholdList[dwIndex].value.dwInt32);
         break;
      case DTYPE_INT64:
         sprintf(szValue, "%64I", m_pItem->pThresholdList[dwIndex].value.qwInt64);
         break;
      case DTYPE_FLOAT:
         sprintf(szValue, "%f", m_pItem->pThresholdList[dwIndex].value.dFloat);
         break;
      case DTYPE_STRING:
         sprintf(szValue, "\"%s\"", m_pItem->pThresholdList[dwIndex].value.szString);
         break;
   }

   sprintf(szBuffer, "%s(%s) %s %s", g_pszThresholdFunction[m_pItem->pThresholdList[dwIndex].wFunction],
           szArgs, g_pszThresholdOperation[m_pItem->pThresholdList[dwIndex].wOperation],
           szValue);
   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);

   return iItem;
}
