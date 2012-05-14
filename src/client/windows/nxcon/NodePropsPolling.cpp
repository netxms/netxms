// NodePropsPolling.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsPolling.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodePropsPolling property page

IMPLEMENT_DYNCREATE(CNodePropsPolling, CPropertyPage)

CNodePropsPolling::CNodePropsPolling() : CPropertyPage(CNodePropsPolling::IDD)
{
	//{{AFX_DATA_INIT(CNodePropsPolling)
	m_bDisableAgent = FALSE;
	m_bDisableICMP = FALSE;
	m_bDisableSNMP = FALSE;
	m_bDisableConfPolls = FALSE;
	m_bDisableDataCollection = FALSE;
	m_bDisableRoutePolls = FALSE;
	m_bDisableStatusPolls = FALSE;
	m_nUseIfXTable = -1;
	//}}AFX_DATA_INIT
}

CNodePropsPolling::~CNodePropsPolling()
{
}

void CNodePropsPolling::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsPolling)
	DDX_Check(pDX, IDC_CHECK_DISABLE_AGENT, m_bDisableAgent);
	DDX_Check(pDX, IDC_CHECK_DISABLE_ICMP, m_bDisableICMP);
	DDX_Check(pDX, IDC_CHECK_DISABLE_SNMP, m_bDisableSNMP);
	DDX_Check(pDX, IDC_CHECK_DISABLE_CONF_POLLS, m_bDisableConfPolls);
	DDX_Check(pDX, IDC_CHECK_DISABLE_DATACOLL, m_bDisableDataCollection);
	DDX_Check(pDX, IDC_CHECK_DISABLE_ROUTE_POLLS, m_bDisableRoutePolls);
	DDX_Check(pDX, IDC_CHECK_DISABLE_STATUS_POLL, m_bDisableStatusPolls);
	DDX_Radio(pDX, IDC_RADIO_DEFAULT, m_nUseIfXTable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsPolling, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsPolling)
	ON_BN_CLICKED(IDC_SELECT_POLLER, OnSelectPoller)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_AGENT, OnCheckDisableAgent)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_ICMP, OnCheckDisableIcmp)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_SNMP, OnCheckDisableSnmp)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_CONF_POLLS, OnCheckDisableConfPolls)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_DATACOLL, OnCheckDisableDatacoll)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_ROUTE_POLLS, OnCheckDisableRoutePolls)
	ON_BN_CLICKED(IDC_CHECK_DISABLE_STATUS_POLL, OnCheckDisableStatusPoll)
	ON_BN_CLICKED(IDC_RADIO_DEFAULT, OnRadioDefault)
	ON_BN_CLICKED(IDC_RADIO_DISABLE, OnRadioDisable)
	ON_BN_CLICKED(IDC_RADIO_ENABLE, OnRadioEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsPolling message handlers

//
// WM_INITDIALOG message handler
//

BOOL CNodePropsPolling::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   // Poller node
   if (m_dwPollerNode != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_POLLER, _T("<server>"));
   }

   return TRUE;
}


//
// Handler for "select poller node" button
//

void CNodePropsPolling::OnSelectPoller() 
{
   CObjectSelDlg dlg;

   dlg.m_dwAllowedClasses = SCL_NODE;
   dlg.m_bSingleSelection = TRUE;
   dlg.m_bAllowEmptySelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      if (dlg.m_dwNumObjects != 0)
      {
         NXC_OBJECT *pNode;

         m_dwPollerNode = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwPollerNode);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_POLLER, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_POLLER, _T("<invalid>"));
         }
      }
      else
      {
         m_dwPollerNode = 0;
         SetDlgItemText(IDC_EDIT_POLLER, _T("<server>"));
      }
      m_pUpdate->qwFlags |= OBJ_UPDATE_POLLER_NODE;
      SetModified();
   }
}


//
// Handler for "OK" button
//

void CNodePropsPolling::OnOK() 
{
	CPropertyPage::OnOK();

   m_pUpdate->dwPollerNode = m_dwPollerNode;
	m_pUpdate->nUseIfXTable = m_nUseIfXTable;
   if (m_bDisableAgent)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_NXCP;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_NXCP;

   if (m_bDisableSNMP)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_SNMP;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_SNMP;

   if (m_bDisableICMP)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_ICMP;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_ICMP;

   if (m_bDisableStatusPolls)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_STATUS_POLL;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_STATUS_POLL;

   if (m_bDisableConfPolls)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_CONF_POLL;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_CONF_POLL;

   if (m_bDisableRoutePolls)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_ROUTE_POLL;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_ROUTE_POLL;

   if (m_bDisableDataCollection)
      m_pUpdate->dwObjectFlags |= NF_DISABLE_DATA_COLLECT;
   else
      m_pUpdate->dwObjectFlags &= ~NF_DISABLE_DATA_COLLECT;
}


//
// Handlers for clicks on checkboxes
//

void CNodePropsPolling::OnCheckDisableAgent() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableIcmp() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableSnmp() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableConfPolls() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableDatacoll() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableRoutePolls() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnCheckDisableStatusPoll() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_FLAGS;
}

void CNodePropsPolling::OnRadioDefault() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_USE_IFXTABLE;
}

void CNodePropsPolling::OnRadioDisable() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_USE_IFXTABLE;
}

void CNodePropsPolling::OnRadioEnable() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_USE_IFXTABLE;
}
