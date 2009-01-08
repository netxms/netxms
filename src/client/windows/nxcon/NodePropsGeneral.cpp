// NodePropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsGeneral.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Static data
//

static TCHAR *m_pszAuthStrings[] = 
{
   _T("None"),
   _T("Shared Secred (Plain Text)"),
   _T("Shared Secred (MD5 Hash)"),
   _T("Shared Secred (SHA1 Hash)")
};


/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral property page

IMPLEMENT_DYNCREATE(CNodePropsGeneral, CPropertyPage)

CNodePropsGeneral::CNodePropsGeneral() : CPropertyPage(CNodePropsGeneral::IDD)
{
	//{{AFX_DATA_INIT(CNodePropsGeneral)
	m_dwObjectId = 0;
	m_strName = _T("");
	m_strOID = _T("");
	m_iAgentPort = 0;
	m_strSecret = _T("");
	m_strCommunity = _T("");
	m_iAuthType = -1;
	m_iSNMPVersion = -1;
	m_bForceEncryption = FALSE;
	//}}AFX_DATA_INIT
}

CNodePropsGeneral::~CNodePropsGeneral()
{
}

void CNodePropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsGeneral)
	DDX_Control(pDX, IDC_COMBO_AUTH, m_wndAuthList);
	DDX_Text(pDX, IDC_EDIT_ID, m_dwObjectId);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_OID, m_strOID);
	DDX_Text(pDX, IDC_EDIT_PORT, m_iAgentPort);
	DDV_MinMaxInt(pDX, m_iAgentPort, 1, 65535);
	DDX_Text(pDX, IDC_EDIT_SECRET, m_strSecret);
	DDV_MaxChars(pDX, m_strSecret, 63);
	DDX_Text(pDX, IDC_EDIT_COMMUNITY, m_strCommunity);
	DDV_MaxChars(pDX, m_strCommunity, 63);
	DDX_CBIndex(pDX, IDC_COMBO_AUTH, m_iAuthType);
	DDX_Radio(pDX, IDC_RADIO_VERSION1, m_iSNMPVersion);
	DDX_Check(pDX, IDC_CHECK_ENCRYPTION, m_bForceEncryption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_SECRET, OnChangeEditSecret)
	ON_EN_CHANGE(IDC_EDIT_PORT, OnChangeEditPort)
	ON_EN_CHANGE(IDC_EDIT_COMMUNITY, OnChangeEditCommunity)
	ON_CBN_SELCHANGE(IDC_COMBO_AUTH, OnSelchangeComboAuth)
	ON_BN_CLICKED(IDC_RADIO_VERSION_2C, OnRadioVersion2c)
	ON_BN_CLICKED(IDC_RADIO_VERSION1, OnRadioVersion1)
	ON_BN_CLICKED(IDC_SELECT_IP, OnSelectIp)
	ON_BN_CLICKED(IDC_SELECT_PROXY, OnSelectProxy)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE, OnButtonGenerate)
	ON_BN_CLICKED(IDC_SELECT_SNMPPROXY, OnSelectSnmpproxy)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral message handlers

BOOL CNodePropsGeneral::OnInitDialog() 
{
   int i;
   TCHAR szBuffer[16];

	CPropertyPage::OnInitDialog();

   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   SetDlgItemText(IDC_EDIT_PRIMARY_IP, IpToStr(m_dwIpAddr, szBuffer));

   // Initialize dropdown lists
   for(i = 0; i < 4; i++)
      m_wndAuthList.AddString(m_pszAuthStrings[i]);
   m_wndAuthList.SelectString(-1, m_pszAuthStrings[m_iAuthType]);

   // Proxy node
   if (m_dwProxyNode != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwProxyNode);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_PROXY, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_PROXY, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_PROXY, _T("<none>"));
   }

   // SNMP proxy node
   if (m_dwSNMPProxy != 0)
   {
      NXC_OBJECT *pNode;

      pNode = NXCFindObjectById(g_hSession, m_dwSNMPProxy);
      if (pNode != NULL)
      {
         SetDlgItemText(IDC_EDIT_SNMPPROXY, pNode->szName);
      }
      else
      {
         SetDlgItemText(IDC_EDIT_SNMPPROXY, _T("<invalid>"));
      }
   }
   else
   {
      SetDlgItemText(IDC_EDIT_SNMPPROXY, _T("<none>"));
   }

   return TRUE;
}


//
// Handlers for changed controls
//

void CNodePropsGeneral::OnChangeEditName() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditSecret() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_SECRET;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditPort() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_PORT;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditCommunity() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_COMMUNITY;
   SetModified();
}

void CNodePropsGeneral::OnSelchangeComboAuth() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_AUTH;
   SetModified();
}

void CNodePropsGeneral::OnRadioVersion2c() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_VERSION;
   SetModified();
}

void CNodePropsGeneral::OnRadioVersion1() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_VERSION;
   SetModified();
}


//
// Handler for PSN_OK message
//

void CNodePropsGeneral::OnOK() 
{
   TCHAR szBuffer[256];
   int i;

	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (TCHAR *)((LPCTSTR)m_strName);
   m_pUpdate->iAgentPort = m_iAgentPort;
   m_pUpdate->pszCommunity = (TCHAR *)((LPCTSTR)m_strCommunity);
   m_pUpdate->pszSecret = (TCHAR *)((LPCTSTR)m_strSecret);
   m_pUpdate->wSNMPVersion = (m_iSNMPVersion == 0) ? SNMP_VERSION_1 : SNMP_VERSION_2C;
   m_pUpdate->dwIpAddr = m_dwIpAddr;
   m_pUpdate->dwProxyNode = m_dwProxyNode;
   m_pUpdate->dwSNMPProxy = m_dwSNMPProxy;
   if (m_bForceEncryption)
      m_pUpdate->dwNodeFlags |= NF_FORCE_ENCRYPTION;
   else
      m_pUpdate->dwNodeFlags &= ~NF_FORCE_ENCRYPTION;

   // Authentication type
   m_wndAuthList.GetWindowText(szBuffer, 255);
   for(i = 0; i < 4; i++)
      if (!_tcscmp(szBuffer, m_pszAuthStrings[i]))
      {
         m_pUpdate->iAuthType = i;
         break;
      }
}


//
// Select primary IP address for node
//

void CNodePropsGeneral::OnSelectIp() 
{
   CObjectSelDlg dlg;
   NXC_OBJECT *pObject;

   dlg.m_bSelectAddress = TRUE;
   dlg.m_bSingleSelection = TRUE;
   dlg.m_bAllowEmptySelection = FALSE;
   dlg.m_dwParentObject = m_dwObjectId;
   if (dlg.DoModal() == IDOK)
   {
      pObject = NXCFindObjectById(g_hSession, dlg.m_pdwObjectList[0]);
      if (pObject != NULL)
      {
         TCHAR szBuffer[16];

         m_dwIpAddr = pObject->dwIpAddr;
         SetDlgItemText(IDC_EDIT_PRIMARY_IP, IpToStr(m_dwIpAddr, szBuffer));
      }
      m_pUpdate->qwFlags |= OBJ_UPDATE_IP_ADDR;
      SetModified();
   }
}


//
// Handler for "Select proxy" button
//

void CNodePropsGeneral::OnSelectProxy() 
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

         m_dwProxyNode = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwProxyNode);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_PROXY, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_PROXY, _T("<invalid>"));
         }
      }
      else
      {
         m_dwProxyNode = 0;
         SetDlgItemText(IDC_EDIT_PROXY, _T("<none>"));
      }
      m_pUpdate->qwFlags |= OBJ_UPDATE_PROXY_NODE;
      SetModified();
   }
}


//
// Handler for "Select SNMP proxy" button
//

void CNodePropsGeneral::OnSelectSnmpproxy() 
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

         m_dwSNMPProxy = dlg.m_pdwObjectList[0];
         pNode = NXCFindObjectById(g_hSession, m_dwSNMPProxy);
         if (pNode != NULL)
         {
            SetDlgItemText(IDC_EDIT_SNMPPROXY, pNode->szName);
         }
         else
         {
            SetDlgItemText(IDC_EDIT_SNMPPROXY, _T("<invalid>"));
         }
      }
      else
      {
         m_dwSNMPProxy = 0;
         SetDlgItemText(IDC_EDIT_SNMPPROXY, _T("<none>"));
      }
      m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_PROXY;
      SetModified();
   }
}


//
// Generate random shared secret
//

void CNodePropsGeneral::OnButtonGenerate() 
{
   TCHAR szBuffer[256];
   int i;

   GetDlgItemText(IDC_EDIT_SECRET, szBuffer, 256);
   if (szBuffer[0] != 0)
      if (MessageBox(_T("Do you really wish to replace existing shared secret with a random one?"),
                     _T("Confirmation"), MB_YESNO | MB_ICONQUESTION) != IDYES)
         return;

   srand((unsigned int)time(NULL));
   for(i = 0; i < 15; i++)
   {
      szBuffer[i] = rand() * 90 / RAND_MAX + '!';
      if ((szBuffer[i] == '"') || (szBuffer[i] == '\'') || (szBuffer[i] == '='))
         szBuffer[i]++;
   }
   szBuffer[15] = 0;
   SetDlgItemText(IDC_EDIT_SECRET, szBuffer);
}
