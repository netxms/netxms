// DiscoveryPropCommunities.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DiscoveryPropCommunities.h"
#include "InputBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropCommunities property page

IMPLEMENT_DYNCREATE(CDiscoveryPropCommunities, CPropertyPage)

CDiscoveryPropCommunities::CDiscoveryPropCommunities() : CPropertyPage(CDiscoveryPropCommunities::IDD)
{
	//{{AFX_DATA_INIT(CDiscoveryPropCommunities)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDiscoveryPropCommunities::~CDiscoveryPropCommunities()
{
}

void CDiscoveryPropCommunities::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiscoveryPropCommunities)
	DDX_Control(pDX, IDC_LIST_STRINGS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscoveryPropCommunities, CPropertyPage)
	//{{AFX_MSG_MAP(CDiscoveryPropCommunities)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropCommunities message handlers

BOOL CDiscoveryPropCommunities::OnInitDialog() 
{
	RECT rect;
	DWORD i;

	CPropertyPage::OnInitDialog();
	
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Community"), LVCFMT_LEFT, rect.right - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	for(i = 0; i < m_dwNumStrings; i++)
		m_wndListCtrl.InsertItem(i, m_ppStringList[i]);

	return TRUE;
}


//
// "Delete" button handler
//

void CDiscoveryPropCommunities::OnButtonDelete() 
{
   int nItem;

   nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   while(nItem != -1)
   {
      m_wndListCtrl.DeleteItem(nItem);
      nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   }
}


//
// "Add" button handler
//

void CDiscoveryPropCommunities::OnButtonAdd() 
{
	CInputBox dlg;

	dlg.m_strTitle = _T("New SNMP Community");
	dlg.m_strHeader = _T("Enter new SNMP community string");
	if (dlg.DoModal() == IDOK)
	{
		m_wndListCtrl.InsertItem(0x7FFFFFFF, dlg.m_strText);
	}
}


//
// "OK" button handler
//

void CDiscoveryPropCommunities::OnOK() 
{
   DWORD i;

   for(i = 0; i < m_dwNumStrings; i++)
		safe_free(m_ppStringList[i]);

   m_dwNumStrings = m_wndListCtrl.GetItemCount();
   m_ppStringList = (TCHAR **)realloc(m_ppStringList, sizeof(TCHAR *) * m_dwNumStrings);
   for(i = 0; i < m_dwNumStrings; i++)
		m_ppStringList[i] = _tcsdup(m_wndListCtrl.GetItemText(i, 0));

	CPropertyPage::OnOK();
}
