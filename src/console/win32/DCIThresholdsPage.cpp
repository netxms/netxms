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
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
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


//
// Edit threshold
//

BOOL CDCIThresholdsPage::EditThreshold(NXC_DCI_THRESHOLD *pThreshold)
{
   CThresholdDlg dlg;
   BOOL bResult = FALSE;
   char szBuffer[32];

   dlg.m_iFunction = pThreshold->wFunction;
   dlg.m_iOperation = pThreshold->wOperation;
   dlg.m_dwArg1 = pThreshold->dwArg1;
   switch(m_pItem->iDataType)
   {
      case DTYPE_INTEGER:
         sprintf(szBuffer, "%d", pThreshold->value.dwInt32);
         dlg.m_strValue = szBuffer;
         break;
      case DTYPE_INT64:
         sprintf(szBuffer, "%64I", pThreshold->value.qwInt64);
         dlg.m_strValue = szBuffer;
         break;
      case DTYPE_FLOAT:
         sprintf(szBuffer, "%f", pThreshold->value.dFloat);
         dlg.m_strValue = szBuffer;
         break;
      case DTYPE_STRING:
         dlg.m_strValue = pThreshold->value.szString;
         break;
   }
   if (dlg.DoModal() == IDOK)
   {
      pThreshold->dwArg1 = dlg.m_dwArg1;
      pThreshold->wFunction = (WORD)dlg.m_iFunction;
      pThreshold->wOperation = (WORD)dlg.m_iOperation;
      switch(m_pItem->iDataType)
      {
         case DTYPE_INTEGER:
            pThreshold->value.dwInt32 = strtol((LPCTSTR)dlg.m_strValue, NULL, 0);
            break;
         case DTYPE_INT64:
            /* TODO: add conversion for 64 bit integers */
            break;
         case DTYPE_FLOAT:
            pThreshold->value.dFloat = strtod((LPCTSTR)dlg.m_strValue, NULL);
            break;
         case DTYPE_STRING:
            strcpy(pThreshold->value.szString, (LPCTSTR)dlg.m_strValue);
            break;
      }
      bResult = TRUE;
   }

   return bResult;
}


//
// WM_COMMAND::IDC_BUTTON_ADD message handler
//

void CDCIThresholdsPage::OnButtonAdd() 
{
   NXC_DCI_THRESHOLD dct;

   // Create default threshold structure
   memset(&dct, 0, sizeof(NXC_DCI_THRESHOLD));
   dct.dwArg1 = 1;
   dct.dwEvent = EVENT_THRESHOLD_REACHED;

   // Call threshold configuration dialog
   if (EditThreshold(&dct))
   {
      DWORD dwIndex;

      dwIndex = NXCAddThresholdToItem(m_pItem, &dct);
      AddListEntry(dwIndex);
   }
}
