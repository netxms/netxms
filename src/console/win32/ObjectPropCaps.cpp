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
   RECT rect;

	CPropertyPage::OnInitDialog();

   // Initialize list control
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Capability"), LVCFMT_LEFT, 180);
   m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 
                              rect.right - 180 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

   // Fill list control with data
   AddListRecord(_T("agentVersion"), m_pObject->node.szAgentVersion);
   AddListRecord(_T("isOSPF"), (m_pObject->node.dwFlags & NF_IS_OSPF));
   AddListRecord(_T("isSNMP"), (m_pObject->node.dwFlags & NF_IS_SNMP));
   AddListRecord(_T("isCheckPointSNMP"), (m_pObject->node.dwFlags & NF_IS_CPSNMP));
   AddListRecord(_T("isNetXMSAgent"), (m_pObject->node.dwFlags & NF_IS_NATIVE_AGENT));
   AddListRecord(_T("isManagementServer"), (m_pObject->node.dwFlags & NF_IS_LOCAL_MGMT));
   AddListRecord(_T("isBridge"), (m_pObject->node.dwFlags & NF_IS_BRIDGE));
   AddListRecord(_T("isRouter"), (m_pObject->node.dwFlags & NF_IS_ROUTER));
   AddListRecord(_T("isCDP"), (m_pObject->node.dwFlags & NF_IS_CDP));
   AddListRecord(_T("isNortelTopo"), (m_pObject->node.dwFlags & NF_IS_NORTEL_TOPO));
   AddListRecord(_T("nodeType"), CodeToText(m_pObject->node.dwNodeType, g_ctNodeType));
   AddListRecord(_T("platformName"), m_pObject->node.szPlatformName);
   AddListRecord(_T("snmpOID"), m_pObject->node.szObjectId);
	
	return TRUE;
}


//
// Add record to list
//

void CObjectPropCaps::AddListRecord(const TCHAR *pszName, BOOL bValue)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszName);
   if (iItem != -1)
      m_wndListCtrl.SetItemText(iItem, 1, bValue ? _T("True") : _T("False"));
}

void CObjectPropCaps::AddListRecord(const TCHAR *pszName, const TCHAR *pszValue)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pszName);
   if (iItem != -1)
      m_wndListCtrl.SetItemText(iItem, 1, pszValue);
}
