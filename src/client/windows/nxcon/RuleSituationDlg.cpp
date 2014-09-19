// RuleSituationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleSituationDlg.h"
#include "SituationSelDlg.h"
#include "EditVariableDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleSituationDlg dialog


CRuleSituationDlg::CRuleSituationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleSituationDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleSituationDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRuleSituationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleSituationDlg)
	DDX_Control(pDX, IDC_LIST_ATTRIBUTES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleSituationDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleSituationDlg)
	ON_BN_CLICKED(IDC_CHECK_ENABLE, OnCheckEnable)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleSituationDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CRuleSituationDlg::OnInitDialog() 
{
	RECT rect;
	TCHAR buffer[MAX_DB_STRING];
	int item;

	CDialog::OnInitDialog();

	// Setup attribute list
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, rect.right - 100 - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	// Situation name and instance
	SetDlgItemText(IDC_EDIT_NAME, theApp.GetSituationName(m_dwSituation, buffer));
	SetDlgItemText(IDC_EDIT_INSTANCE, m_strInstance);

	// Attributes
   StructArray<KeyValuePair> *a = m_attrList.toArray();
	for(int i = 0; i < a->size(); i++)
	{
      KeyValuePair *p = a->get(i);
		item = m_wndListCtrl.InsertItem(i, p->key);
		m_wndListCtrl.SetItemText(item, 1, (const TCHAR *)p->value);
	}
   delete a;

	SendDlgItemMessage(IDC_CHECK_ENABLE, BM_SETCHECK, (m_dwSituation != 0) ? BST_CHECKED : BST_UNCHECKED, 0);
	EnableControls(m_dwSituation != 0);

	return TRUE;
}


//
// Enable or disable controls
//

void CRuleSituationDlg::EnableControls(BOOL enable)
{
	EnableDlgItem(this, IDC_EDIT_NAME, enable);
	EnableDlgItem(this, IDC_BUTTON_SELECT, enable);
	EnableDlgItem(this, IDC_EDIT_INSTANCE, enable);
	EnableDlgItem(this, IDC_LIST_ATTRIBUTES, enable);
	EnableDlgItem(this, IDC_BUTTON_ADD, enable);
	EnableDlgItem(this, IDC_BUTTON_DELETE, enable);
}


//
// "OK" button handler
//

void CRuleSituationDlg::OnOK() 
{
	int i, count;
	TCHAR attr[MAX_DB_STRING], value[MAX_DB_STRING];

	if (SendDlgItemMessage(IDC_CHECK_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		GetDlgItemText(IDC_EDIT_INSTANCE, m_strInstance);
		m_attrList.clear();
		count = m_wndListCtrl.GetItemCount();
		for(i = 0; i < count; i++)
		{
			m_wndListCtrl.GetItemText(i, 0, attr, MAX_DB_STRING);
			m_wndListCtrl.GetItemText(i, 1, value, MAX_DB_STRING);
			m_attrList.set(attr, value);
		}
	}
	else
	{
		m_dwSituation = 0;
		m_strInstance = _T("");
		m_attrList.clear();
	}
	CDialog::OnOK();
}


//
// Enable/disable situation update
//

void CRuleSituationDlg::OnCheckEnable() 
{
	EnableControls(SendDlgItemMessage(IDC_CHECK_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED);
}


//
// Handler for "Select..." button
//

void CRuleSituationDlg::OnButtonSelect() 
{
	CSituationSelDlg dlg;

	if (dlg.DoModal() == IDOK)
	{
		TCHAR buffer[MAX_DB_STRING];

		m_dwSituation = dlg.m_dwSituation;
		SetDlgItemText(IDC_EDIT_NAME, theApp.GetSituationName(m_dwSituation, buffer));
	}
}


//
// Add attribute
//

void CRuleSituationDlg::OnButtonAdd() 
{
	CEditVariableDlg dlg;
	int item;

	dlg.m_bNewVariable = TRUE;
	if (dlg.DoModal() == IDOK)
	{
		item = m_wndListCtrl.InsertItem(0x7FFFFFFF, dlg.m_strName);
		m_wndListCtrl.SetItemText(item, 1, dlg.m_strValue);
	}
}


//
// Delete attribute
//

void CRuleSituationDlg::OnButtonDelete() 
{
	int item;

	while((item = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
		m_wndListCtrl.DeleteItem(item);
}
