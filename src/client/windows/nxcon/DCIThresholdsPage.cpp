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
	m_bAllThresholds = FALSE;
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
	DDX_Check(pDX, IDC_CHECK_ALL_THRESHOLDS, m_bAllThresholds);
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
   TCHAR szBuffer[256], szArgs[256];

   m_wndListCtrl.SetItemData(iItem, dwIndex);

   switch(m_pItem->pThresholdList[dwIndex].function)
   {
      case F_AVERAGE:
      case F_DEVIATION:
      case F_ERROR:
         _sntprintf_s(szArgs, 256, _TRUNCATE, _T("%d"), m_pItem->pThresholdList[dwIndex].sampleCount);
         break;
      default:
         szArgs[0] = 0;
         break;
   }

   // Threshold expression
   if (m_pItem->pThresholdList[dwIndex].function == F_ERROR)
      _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%s(%s)"),
                   g_pszThresholdFunction[m_pItem->pThresholdList[dwIndex].function], szArgs);
   else
      _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%s(%s) %s %s"),
                   g_pszThresholdFunction[m_pItem->pThresholdList[dwIndex].function], szArgs,
                   g_pszThresholdOperation[m_pItem->pThresholdList[dwIndex].operation], 
						 m_pItem->pThresholdList[dwIndex].value);
   m_wndListCtrl.SetItemText(iItem, 0, szBuffer);

   // Event
   m_wndListCtrl.SetItemText(iItem, 1, 
      NXCGetEventName(g_hSession, m_pItem->pThresholdList[dwIndex].activationEvent));
}


//
// Edit threshold
//

BOOL CDCIThresholdsPage::EditThreshold(NXC_DCI_THRESHOLD *pThreshold)
{
   CThresholdDlg dlg;
   BOOL bResult = FALSE;

   dlg.m_iFunction = pThreshold->function;
   dlg.m_iOperation = pThreshold->operation;
   dlg.m_dwArg1 = pThreshold->sampleCount;
   dlg.m_dwEventId = pThreshold->activationEvent;
   dlg.m_dwRearmEventId = pThreshold->rearmEvent;
	dlg.m_strValue = pThreshold->value;
	switch(pThreshold->repeatInterval)
	{
		case -1:
			dlg.m_nRepeat = 0;
			dlg.m_nSeconds = 3600;
			break;
		case 0:
			dlg.m_nRepeat = 1;
			dlg.m_nSeconds = 3600;
			break;
		default:
			dlg.m_nRepeat = 2;
			dlg.m_nSeconds = pThreshold->repeatInterval;
			break;
	}
   if (dlg.DoModal() == IDOK)
   {
      pThreshold->sampleCount = dlg.m_dwArg1;
      pThreshold->function = (WORD)dlg.m_iFunction;
      pThreshold->operation = (WORD)dlg.m_iOperation;
      pThreshold->activationEvent = dlg.m_dwEventId;
      pThreshold->rearmEvent = dlg.m_dwRearmEventId;
		nx_strncpy(pThreshold->value, dlg.m_strValue, MAX_STRING_VALUE);
		if (g_nCurrentDCIDataType != DCI_DT_STRING)
		{
			StrStrip(pThreshold->value);
		}
		switch(dlg.m_nRepeat)
		{
			case 0:
				pThreshold->repeatInterval = -1;
				break;
			case 1:
				pThreshold->repeatInterval = 0;
				break;
			case 2:
				pThreshold->repeatInterval = dlg.m_nSeconds;
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
   dct.sampleCount = 1;
   dct.activationEvent = EVENT_THRESHOLD_REACHED;
   dct.rearmEvent = EVENT_THRESHOLD_REARMED;
	dct.repeatInterval = -1;

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
