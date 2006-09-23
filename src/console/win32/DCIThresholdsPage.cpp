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

extern int g_nCurrentDCIDataType;


/////////////////////////////////////////////////////////////////////////////
// CDCIThresholdsPage property page

IMPLEMENT_DYNCREATE(CDCIThresholdsPage, CPropertyPage)

CDCIThresholdsPage::CDCIThresholdsPage() : CPropertyPage(CDCIThresholdsPage::IDD)
{
	//{{AFX_DATA_INIT(CDCIThresholdsPage)
	m_strInstance = _T("");
	//}}AFX_DATA_INIT
}

CDCIThresholdsPage::~CDCIThresholdsPage()
{
}

void CDCIThresholdsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIThresholdsPage)
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_wndButtonDelete);
	DDX_Control(pDX, IDC_BUTTON_MODIFY, m_wndButtonModify);
	DDX_Control(pDX, IDC_BUTTON_MOVEDOWN, m_wndButtonDown);
	DDX_Control(pDX, IDC_BUTTON_MOVEUP, m_wndButtonUp);
	DDX_Control(pDX, IDC_LIST_THRESHOLDS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_INSTANCE, m_strInstance);
	DDV_MaxChars(pDX, m_strInstance, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCIThresholdsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCIThresholdsPage)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY, OnButtonModify)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_MOVEUP, OnButtonMoveup)
	ON_BN_CLICKED(IDC_BUTTON_MOVEDOWN, OnButtonMovedown)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_THRESHOLDS, OnItemchangedListThresholds)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_THRESHOLDS, OnDblclkListThresholds)
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
   m_wndButtonModify.EnableWindow(FALSE);
   m_wndButtonDelete.EnableWindow(FALSE);
   
   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Operation"), LVCFMT_LEFT, rect.right - 150 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.InsertColumn(1, _T("Event"), LVCFMT_LEFT, 150);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

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
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, _T(""));
   if (iItem != -1)
      UpdateListEntry(iItem, dwIndex);
   return iItem;
}


//
// Update list entry
//

void CDCIThresholdsPage::UpdateListEntry(int iItem, DWORD dwIndex)
{
   TCHAR szBuffer[256], szArgs[256], szValue[MAX_DCI_STRING_VALUE];

   m_wndListCtrl.SetItemData(iItem, dwIndex);

   switch(m_pItem->pThresholdList[dwIndex].wFunction)
   {
      case F_AVERAGE:
         _stprintf(szArgs, _T("%d"), m_pItem->pThresholdList[dwIndex].dwArg1);
         break;
      default:
         szArgs[0] = 0;
         break;
   }

   switch(g_nCurrentDCIDataType)
   {
      case DCI_DT_INT:
         _stprintf(szValue, _T("%d"), m_pItem->pThresholdList[dwIndex].value.dwInt32);
         break;
      case DCI_DT_UINT:
         _stprintf(szValue, _T("%u"), m_pItem->pThresholdList[dwIndex].value.dwInt32);
         break;
      case DCI_DT_INT64:
         _stprintf(szValue, _T("%I64d"), m_pItem->pThresholdList[dwIndex].value.qwInt64);
         break;
      case DCI_DT_UINT64:
         _stprintf(szValue, _T("%I64u"), m_pItem->pThresholdList[dwIndex].value.qwInt64);
         break;
      case DCI_DT_FLOAT:
         _stprintf(szValue, _T("%f"), m_pItem->pThresholdList[dwIndex].value.dFloat);
         break;
      case DCI_DT_STRING:
         _stprintf(szValue, _T("\"%s\""), m_pItem->pThresholdList[dwIndex].value.szString);
         break;
   }

   // Threshold expression
   _stprintf(szBuffer, _T("%s(%s) %s %s"), g_pszThresholdFunction[m_pItem->pThresholdList[dwIndex].wFunction],
             szArgs, g_pszThresholdOperation[m_pItem->pThresholdList[dwIndex].wOperation],
             szValue);
   m_wndListCtrl.SetItemText(iItem, 0, szBuffer);

   // Event
   m_wndListCtrl.SetItemText(iItem, 1, 
      NXCGetEventName(g_hSession, m_pItem->pThresholdList[dwIndex].dwEvent));
}


//
// Edit threshold
//

BOOL CDCIThresholdsPage::EditThreshold(NXC_DCI_THRESHOLD *pThreshold)
{
   CThresholdDlg dlg;
   BOOL bResult = FALSE;
   TCHAR szBuffer[32];

   dlg.m_iFunction = pThreshold->wFunction;
   dlg.m_iOperation = pThreshold->wOperation;
   dlg.m_dwArg1 = pThreshold->dwArg1;
   dlg.m_dwEventId = pThreshold->dwEvent;
   switch(g_nCurrentDCIDataType)
   {
      case DCI_DT_INT:
         _stprintf(szBuffer, _T("%d"), pThreshold->value.dwInt32);
         dlg.m_strValue = szBuffer;
         break;
      case DCI_DT_UINT:
         _stprintf(szBuffer, _T("%u"), pThreshold->value.dwInt32);
         dlg.m_strValue = szBuffer;
         break;
      case DCI_DT_INT64:
         _stprintf(szBuffer, _T("%I64d"), pThreshold->value.qwInt64);
         dlg.m_strValue = szBuffer;
         break;
      case DCI_DT_UINT64:
         _stprintf(szBuffer, _T("%I64u"), pThreshold->value.qwInt64);
         dlg.m_strValue = szBuffer;
         break;
      case DCI_DT_FLOAT:
         _stprintf(szBuffer, _T("%f"), pThreshold->value.dFloat);
         dlg.m_strValue = szBuffer;
         break;
      case DCI_DT_STRING:
         dlg.m_strValue = pThreshold->value.szString;
         break;
   }
   if (dlg.DoModal() == IDOK)
   {
      pThreshold->dwArg1 = dlg.m_dwArg1;
      pThreshold->wFunction = (WORD)dlg.m_iFunction;
      pThreshold->wOperation = (WORD)dlg.m_iOperation;
      pThreshold->dwEvent = dlg.m_dwEventId;
      switch(g_nCurrentDCIDataType)
      {
         case DCI_DT_INT:
            pThreshold->value.dwInt32 = _tcstol((LPCTSTR)dlg.m_strValue, NULL, 0);
            break;
         case DCI_DT_UINT:
            pThreshold->value.dwInt32 = _tcstoul((LPCTSTR)dlg.m_strValue, NULL, 0);
            break;
         case DCI_DT_INT64:
            pThreshold->value.qwInt64 = _tcstoll((LPCTSTR)dlg.m_strValue, NULL, 0);
            break;
         case DCI_DT_UINT64:
            pThreshold->value.qwInt64 = _tcstoull((LPCTSTR)dlg.m_strValue, NULL, 0);
            break;
         case DCI_DT_FLOAT:
            pThreshold->value.dFloat = _tcstod((LPCTSTR)dlg.m_strValue, NULL);
            break;
         case DCI_DT_STRING:
            nx_strncpy(pThreshold->value.szString, (LPCTSTR)dlg.m_strValue, MAX_STRING_VALUE);
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


//
// WM_COMMAND::IDC_BUTTON_MODIFY message handler
//

void CDCIThresholdsPage::OnButtonModify() 
{
   int iItem;
   DWORD dwIndex;
   NXC_DCI_THRESHOLD dct;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      dwIndex = m_wndListCtrl.GetItemData(iItem);
      memcpy(&dct, &m_pItem->pThresholdList[dwIndex], sizeof(NXC_DCI_THRESHOLD));
      if (EditThreshold(&dct))
      {
         memcpy(&m_pItem->pThresholdList[dwIndex], &dct, sizeof(NXC_DCI_THRESHOLD));
         UpdateListEntry(iItem, dwIndex);
      }
   }
}


//
// WM_COMMAND::IDC_BUTTON_DELETE message handler
//

void CDCIThresholdsPage::OnButtonDelete() 
{
   int iItem;
   DWORD dwIndex;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwIndex = m_wndListCtrl.GetItemData(iItem);
      if (NXCDeleteThresholdFromItem(m_pItem, dwIndex))
      {
         m_wndListCtrl.DeleteItem(iItem);
         RecalcIndexes(dwIndex);
      }
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   }
}


//
// WM_COMMAND::IDC_BUTTON_MOVEUP message handler
//

void CDCIThresholdsPage::OnButtonMoveup() 
{
   int iItem;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
      if (iItem > 0)
      {
         DWORD dwIndex1, dwIndex2;

         dwIndex1 = m_wndListCtrl.GetItemData(iItem);
         dwIndex2 = m_wndListCtrl.GetItemData(iItem - 1);
         if (NXCSwapThresholds(m_pItem, dwIndex1, dwIndex2))
         {
            UpdateListEntry(iItem - 1, dwIndex2);
            UpdateListEntry(iItem, dwIndex1);
            SelectListViewItem(&m_wndListCtrl, iItem - 1);
         }
      }
   }
}


//
// WM_COMMAND::IDC_BUTTON_MOVEDOWN message handler
//

void CDCIThresholdsPage::OnButtonMovedown() 
{
   int iItem;

   if (m_wndListCtrl.GetSelectedCount() == 1)
   {
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
      if (iItem < (m_wndListCtrl.GetItemCount() - 1))
      {
         DWORD dwIndex1, dwIndex2;

         dwIndex1 = m_wndListCtrl.GetItemData(iItem);
         dwIndex2 = m_wndListCtrl.GetItemData(iItem + 1);
         if (NXCSwapThresholds(m_pItem, dwIndex1, dwIndex2))
         {
            UpdateListEntry(iItem + 1, dwIndex2);
            UpdateListEntry(iItem, dwIndex1);
            SelectListViewItem(&m_wndListCtrl, iItem + 1);
         }
      }
   }
}


//
// Handler for LVN_ITEMCHANGED notification message
//

void CDCIThresholdsPage::OnItemchangedListThresholds(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
   BOOL bEnable;

   // Disable some buttons if multiple items selected
   bEnable = (m_wndListCtrl.GetSelectedCount() == 1);
   m_wndButtonModify.EnableWindow(bEnable);
   //m_wndButtonUp.EnableWindow(bEnable);
   //m_wndButtonDown.EnableWindow(bEnable);
   m_wndButtonDelete.EnableWindow(m_wndListCtrl.GetSelectedCount() > 0);

	*pResult = 0;
}


//
// Recalculate item indexes after item deletion
//

void CDCIThresholdsPage::RecalcIndexes(DWORD dwIndex)
{
   int iItem, iMaxItem;
   DWORD dwData;

   iMaxItem = m_wndListCtrl.GetItemCount();
   for(iItem = 0; iItem < iMaxItem; iItem++)
   {
      dwData = m_wndListCtrl.GetItemData(iItem);
      if (dwData > dwIndex)
      {
         dwData--;
         m_wndListCtrl.SetItemData(iItem, dwData);
      }
   }
}


//
// Handler for double click in thresholds list
//

void CDCIThresholdsPage::OnDblclkListThresholds(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDC_BUTTON_MODIFY, 0);
	*pResult = 0;
}
