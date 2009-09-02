// ObjToolPropColumns.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjToolPropColumns.h"
#include "ObjToolColumnEditDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns property page

IMPLEMENT_DYNCREATE(CObjToolPropColumns, CPropertyPage)

CObjToolPropColumns::CObjToolPropColumns() : CPropertyPage(CObjToolPropColumns::IDD)
{
	//{{AFX_DATA_INIT(CObjToolPropColumns)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_pColumnList = NULL;
}

CObjToolPropColumns::~CObjToolPropColumns()
{
   safe_free(m_pColumnList);
}

void CObjToolPropColumns::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjToolPropColumns)
	DDX_Control(pDX, IDC_BUTTON_MOVEUP, m_wndButtonUp);
	DDX_Control(pDX, IDC_BUTTON_MOVEDOWN, m_wndButtonDown);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_wndButtonAdd);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_wndButtonDelete);
	DDX_Control(pDX, IDC_BUTTON_MODIFY, m_wndButtonModify);
	DDX_Control(pDX, IDC_LIST_COLUMNS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjToolPropColumns, CPropertyPage)
	//{{AFX_MSG_MAP(CObjToolPropColumns)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_COLUMNS, OnItemchangedListColumns)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_COLUMNS, OnDblclkListColumns)
	ON_BN_CLICKED(IDC_BUTTON_MOVEUP, OnButtonMoveup)
	ON_BN_CLICKED(IDC_BUTTON_MOVEDOWN, OnButtonMovedown)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_MODIFY, OnButtonModify)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjToolPropColumns message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjToolPropColumns::OnInitDialog() 
{
   RECT rect;
   DWORD i;

	CPropertyPage::OnInitDialog();

   // Setup buttons
   m_wndButtonUp.SetBitmap(CreateMappedBitmap(theApp.m_hInstance, IDB_UP_ARROW, 0, NULL, 0));
   m_wndButtonDown.SetBitmap(CreateMappedBitmap(theApp.m_hInstance, IDB_DOWN_ARROW, 0, NULL, 0));
   m_wndButtonModify.EnableWindow(FALSE);
   m_wndButtonDelete.EnableWindow(FALSE);
   
   // Setup list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, _T("Format"), LVCFMT_LEFT, 80);
   if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
      m_wndListCtrl.InsertColumn(2, _T("SubStr"), LVCFMT_LEFT,
                                 rect.right - GetSystemMetrics(SM_CXVSCROLL) - 180);
   else
      m_wndListCtrl.InsertColumn(2, _T("OID"), LVCFMT_LEFT,
                                 rect.right - GetSystemMetrics(SM_CXVSCROLL) - 180);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Fill list control with data
   for(i = 0; i < m_dwNumColumns; i++)
		AddListEntry(i);
	
	return TRUE;
}


//
// Add new entry to list
//

int CObjToolPropColumns::AddListEntry(DWORD dwIndex)
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

void CObjToolPropColumns::UpdateListEntry(int iItem, DWORD dwIndex)
{
   m_wndListCtrl.SetItemData(iItem, dwIndex);

   m_wndListCtrl.SetItemText(iItem, 0, m_pColumnList[dwIndex].szName);
   m_wndListCtrl.SetItemText(iItem, 1, g_szToolColFmt[m_pColumnList[dwIndex].nFormat]);
   if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
   {
      TCHAR szBuffer[32];

      _sntprintf(szBuffer, 32, _T("%d"), m_pColumnList[dwIndex].nSubstr);
      m_wndListCtrl.SetItemText(iItem, 2, szBuffer);
   }
   else
   {
      m_wndListCtrl.SetItemText(iItem, 2, m_pColumnList[dwIndex].szOID);
   }
}


//
// Handler for LVN_ITEMCHANGED notification message
//

void CObjToolPropColumns::OnItemchangedListColumns(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CObjToolPropColumns::RecalcIndexes(DWORD dwIndex)
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
// Handler for double click in column list
//

void CObjToolPropColumns::OnDblclkListColumns(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDC_BUTTON_MODIFY, 0);
	*pResult = 0;
}


//
// Swap two columns
//

void CObjToolPropColumns::SwapColumns(DWORD index1, DWORD index2)
{
	NXC_OBJECT_TOOL_COLUMN temp;

	memcpy(&temp, &m_pColumnList[index1], sizeof(NXC_OBJECT_TOOL_COLUMN));
	memcpy(&m_pColumnList[index1], &m_pColumnList[index2], sizeof(NXC_OBJECT_TOOL_COLUMN));
	memcpy(&m_pColumnList[index2], &temp, sizeof(NXC_OBJECT_TOOL_COLUMN));
}


//
// WM_COMMAND::IDC_BUTTON_MOVEUP message handler
//

void CObjToolPropColumns::OnButtonMoveup() 
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
         SwapColumns(dwIndex1, dwIndex2);
         UpdateListEntry(iItem - 1, dwIndex2);
         UpdateListEntry(iItem, dwIndex1);
         SelectListViewItem(&m_wndListCtrl, iItem - 1);
      }
   }
}


//
// WM_COMMAND::IDC_BUTTON_MOVEDOWN message handler
//

void CObjToolPropColumns::OnButtonMovedown() 
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
         SwapColumns(dwIndex1, dwIndex2);
         UpdateListEntry(iItem + 1, dwIndex2);
         UpdateListEntry(iItem, dwIndex1);
         SelectListViewItem(&m_wndListCtrl, iItem + 1);
      }
   }
}


//
// WM_COMMAND::IDC_BUTTON_ADD message handler
//

void CObjToolPropColumns::OnButtonAdd() 
{
	NXC_OBJECT_TOOL_COLUMN temp;

	memset(&temp, 0, sizeof(NXC_OBJECT_TOOL_COLUMN));
	temp.nSubstr = 1;
	if (ModifyColumn(&temp, TRUE))
	{
		m_dwNumColumns++;
		m_pColumnList = (NXC_OBJECT_TOOL_COLUMN *)realloc(m_pColumnList, sizeof(NXC_OBJECT_TOOL_COLUMN) * m_dwNumColumns);
		memcpy(&m_pColumnList[m_dwNumColumns - 1], &temp, sizeof(NXC_OBJECT_TOOL_COLUMN));
		int item = AddListEntry(m_dwNumColumns - 1);
      SelectListViewItem(&m_wndListCtrl, item);
	}
}


//
// Modify column
//

BOOL CObjToolPropColumns::ModifyColumn(NXC_OBJECT_TOOL_COLUMN *column, BOOL isCreate)
{
	CObjToolColumnEditDlg dlg;
	BOOL ok = FALSE;

	dlg.m_isCreate = isCreate;
	dlg.m_name = column->szName;
	dlg.m_type = column->nFormat;
	if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
	{
		dlg.m_isAgentTable = TRUE;
		dlg.m_oidLabel = _T("Substring index (starting from 1)");
		dlg.m_oid.Format(_T("%d"), column->nSubstr);
	}
	else
	{
		dlg.m_oidLabel = _T("SNMP Object Identifier (OID)");
		dlg.m_oid = column->szOID;
	}
	if (dlg.DoModal() == IDOK)
	{
		nx_strncpy(column->szName, (LPCTSTR)dlg.m_name, MAX_DB_STRING);
		StrStrip(column->szName);
		column->nFormat = dlg.m_type;
		if (m_iToolType == TOOL_TYPE_TABLE_AGENT)
		{
			column->nSubstr = _tcstol((LPCTSTR)dlg.m_oid, NULL, 0);
		}
		else
		{
			nx_strncpy(column->szOID, dlg.m_oid, MAX_DB_STRING);
			StrStrip(column->szOID);
		}
		ok = TRUE;
	}
	return ok;
}


//
// WM_COMMAND::IDC_BUTTON_MODIFY message handler
//

void CObjToolPropColumns::OnButtonModify() 
{
   int iItem;
   DWORD dwIndex;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      dwIndex = m_wndListCtrl.GetItemData(iItem);
      if (ModifyColumn(&m_pColumnList[dwIndex]))
      {
         UpdateListEntry(iItem, dwIndex);
      }
   }
}


//
// WM_COMMAND::IDC_BUTTON_DELETE message handler
//

void CObjToolPropColumns::OnButtonDelete() 
{
   int iItem;
   DWORD dwIndex;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   while(iItem != -1)
   {
      dwIndex = m_wndListCtrl.GetItemData(iItem);
		m_dwNumColumns--;
		memmove(&m_pColumnList[dwIndex], &m_pColumnList[dwIndex + 1], sizeof(NXC_OBJECT_TOOL_COLUMN) * (m_dwNumColumns - dwIndex));
      m_wndListCtrl.DeleteItem(iItem);
      RecalcIndexes(dwIndex);
      iItem = m_wndListCtrl.GetNextItem(-1, LVNI_SELECTED);
   }
}
