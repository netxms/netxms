// ObjectPropsCustomAttrs.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsCustomAttrs.h"
#include "ObjectPropSheet.h"
#include "EditVariableDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsCustomAttrs property page

IMPLEMENT_DYNCREATE(CObjectPropsCustomAttrs, CPropertyPage)

CObjectPropsCustomAttrs::CObjectPropsCustomAttrs() : CPropertyPage(CObjectPropsCustomAttrs::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsCustomAttrs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CObjectPropsCustomAttrs::~CObjectPropsCustomAttrs()
{
}

void CObjectPropsCustomAttrs::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsCustomAttrs)
	DDX_Control(pDX, IDC_LIST_ATTRIBUTES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsCustomAttrs, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsCustomAttrs)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ATTRIBUTES, OnItemchangedListAttributes)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ATTRIBUTES, OnDblclkListAttributes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsCustomAttrs message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsCustomAttrs::OnInitDialog() 
{
	int item;
	RECT rect;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();

	// Setup list control
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, rect.right - 100 - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	// Fill list control with data
	for(int i = 0; i < m_pObject->pCustomAttrs->size(); i++)
	{
		item = m_wndListCtrl.InsertItem(i, m_pObject->pCustomAttrs->getKeyByIndex(i));
		if (item != -1)
			m_wndListCtrl.SetItemText(item, 1, m_pObject->pCustomAttrs->getValueByIndex(i));
	}
	
	EnableDlgItem(this, IDC_BUTTON_EDIT, m_wndListCtrl.GetSelectedCount() == 1);
	EnableDlgItem(this, IDC_BUTTON_DELETE, m_wndListCtrl.GetSelectedCount() > 0);

	return TRUE;
}


//
// PSN_OK handler
//

void CObjectPropsCustomAttrs::OnOK() 
{
	int i;

	if (m_pUpdate->qwFlags & OBJ_UPDATE_CUSTOM_ATTRS)
	{
		m_pUpdate->pCustomAttrs = &m_strMap;
		for(i = 0; i < m_wndListCtrl.GetItemCount(); i++)
		{
			m_strMap.set(m_wndListCtrl.GetItemText(i, 0), m_wndListCtrl.GetItemText(i, 1));
		}
	}
	CPropertyPage::OnOK();
}


//
// "Add" button handler
//

void CObjectPropsCustomAttrs::OnButtonAdd() 
{
	CEditVariableDlg dlg;
	int item;
	LVFINDINFO lvfi;

	dlg.m_pszTitle = _T("New Attribute");
	dlg.m_bNewVariable = TRUE;
	if (dlg.DoModal() == IDOK)
	{
		lvfi.flags = LVFI_STRING;
		lvfi.psz = dlg.m_strName;
		item = m_wndListCtrl.FindItem(&lvfi);
		if (item == -1)
		{
			item = m_wndListCtrl.InsertItem(0x7FFFFFFF, dlg.m_strName);
		}
		m_wndListCtrl.SetItemText(item, 1, dlg.m_strValue);
		
		m_pUpdate->qwFlags |= OBJ_UPDATE_CUSTOM_ATTRS;
		SetModified();
	}
}


//
// "Edit" button handler
//

void CObjectPropsCustomAttrs::OnButtonEdit() 
{
	CEditVariableDlg dlg;
	int item;

	if (m_wndListCtrl.GetSelectedCount() != 1)
		return;

	item = m_wndListCtrl.GetSelectionMark();
	dlg.m_strName = m_wndListCtrl.GetItemText(item, 0);
	dlg.m_strValue = m_wndListCtrl.GetItemText(item, 1);
	dlg.m_pszTitle = _T("Edit Attribute");
	if (dlg.DoModal() == IDOK)
	{
		m_wndListCtrl.SetItemText(item, 1, dlg.m_strValue);
		
		m_pUpdate->qwFlags |= OBJ_UPDATE_CUSTOM_ATTRS;
		SetModified();
	}
}


//
// "Delete" button handler
//

void CObjectPropsCustomAttrs::OnButtonDelete() 
{
	int item;

	while((item = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
		m_wndListCtrl.DeleteItem(item);
}


//
// Item change handler
//

void CObjectPropsCustomAttrs::OnItemchangedListAttributes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	EnableDlgItem(this, IDC_BUTTON_EDIT, m_wndListCtrl.GetSelectedCount() == 1);
	EnableDlgItem(this, IDC_BUTTON_DELETE, m_wndListCtrl.GetSelectedCount() > 0);
	*pResult = 0;
}


//
// Handler for double click in list control
//

void CObjectPropsCustomAttrs::OnDblclkListAttributes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	PostMessage(WM_COMMAND, IDC_BUTTON_EDIT, 0);
	*pResult = 0;
}
