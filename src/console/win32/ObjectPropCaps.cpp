// ObjectPropCaps.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropCaps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropCaps property page

IMPLEMENT_DYNCREATE(CObjectPropCaps, CPropertyPage)

CObjectPropCaps::CObjectPropCaps() : CPropertyPage(CObjectPropCaps::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropCaps)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CObjectPropCaps::~CObjectPropCaps()
{
}

void CObjectPropCaps::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropCaps)
	DDX_Control(pDX, IDC_LIST_CAPS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropCaps, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropCaps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropCaps message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropCaps::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

   // Initialize list control
   m_wndListCtrl.InsertColumn(0, "Capability", LVCFMT_LEFT, 180);
   m_wndListCtrl.InsertColumn(1, "Value", LVCFMT_LEFT, 80);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Fill list control with data
   AddListRecord("isSNMP", (m_pObject->node.dwFlags & NF_IS_SNMP));
   AddListRecord("isNetXMSAgent", (m_pObject->node.dwFlags & NF_IS_NATIVE_AGENT));
   AddListRecord("isManagementServer", (m_pObject->node.dwFlags & NF_IS_LOCAL_MGMT));
   AddListRecord("isBridge", (m_pObject->node.dwFlags & NF_IS_BRIDGE));
   AddListRecord("isRouter", (m_pObject->node.dwFlags & NF_IS_ROUTER));
	
	return TRUE;
}


//
// Add record to list
//

void CObjectPropCaps::AddListRecord(char *szName, BOOL bValue)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szName);
   if (iItem != -1)
      m_wndListCtrl.SetItemText(iItem, 1, bValue ? "True" : "False");
}
