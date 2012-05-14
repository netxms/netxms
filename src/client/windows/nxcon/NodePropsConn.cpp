// NodePropsConn.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsConn.h"
#include "ObjectPropSheet.h"
#include "ObjectSelDlg.h"

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
// CNodePropsConn property page

IMPLEMENT_DYNCREATE(CNodePropsConn, CPropertyPage)

CNodePropsConn::CNodePropsConn() : CPropertyPage(CNodePropsConn::IDD)
{
	//{{AFX_DATA_INIT(CNodePropsConn)
	m_strSnmpAuthPassword = _T("");
	m_strSnmpAuthName = _T("");
	m_strSnmpPrivPassword = _T("");
	m_iAgentPort = 0;
	m_strSecret = _T("");
	m_bForceEncryption = FALSE;
	m_snmpPort = 0;
	//}}AFX_DATA_INIT
}

CNodePropsConn::~CNodePropsConn()
{
}

void CNodePropsConn::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsConn)
	DDX_Control(pDX, IDC_COMBO_USM_PRIV, m_wndSnmpPriv);
	DDX_Control(pDX, IDC_COMBO_USM_AUTH, m_wndSnmpAuth);
	DDX_Control(pDX, IDC_COMBO_SNMP_VERSION, m_wndSnmpVersion);
	DDX_Control(pDX, IDC_COMBO_AUTH, m_wndAuthList);
	DDX_Text(pDX, IDC_EDIT_AUTH_PASSWORD, m_strSnmpAuthPassword);
	DDV_MaxChars(pDX, m_strSnmpAuthPassword, 127);
	DDX_Text(pDX, IDC_EDIT_COMMUNITY, m_strSnmpAuthName);
	DDV_MaxChars(pDX, m_strSnmpAuthName, 127);
	DDX_Text(pDX, IDC_EDIT_PRIV_PASSWORD, m_strSnmpPrivPassword);
	DDV_MaxChars(pDX, m_strSnmpPrivPassword, 127);
	DDX_Text(pDX, IDC_EDIT_PORT, m_iAgentPort);
	DDV_MinMaxInt(pDX, m_iAgentPort, 1, 65535);
	DDX_Text(pDX, IDC_EDIT_SECRET, m_strSecret);
	DDV_MaxChars(pDX, m_strSecret, 63);
	DDX_Check(pDX, IDC_CHECK_ENCRYPTION, m_bForceEncryption);
	DDX_Text(pDX, IDC_EDIT_SNMP_PORT, m_snmpPort);
	DDV_MinMaxInt(pDX, m_snmpPort, 1, 65535);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsConn, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsConn)
	ON_BN_CLICKED(IDC_SELECT_PROXY, OnSelectProxy)
	ON_BN_CLICKED(IDC_SELECT_SNMPPROXY, OnSelectSnmpproxy)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE, OnButtonGenerate)
	ON_CBN_SELCHANGE(IDC_COMBO_AUTH, OnSelchangeComboAuth)
	ON_EN_CHANGE(IDC_EDIT_PORT, OnChangeEditPort)
	ON_EN_CHANGE(IDC_EDIT_SECRET, OnChangeEditSecret)
	ON_EN_CHANGE(IDC_EDIT_PRIV_PASSWORD, OnChangeEditPrivPassword)
	ON_EN_CHANGE(IDC_EDIT_COMMUNITY, OnChangeEditCommunity)
	ON_EN_CHANGE(IDC_EDIT_AUTH_PASSWORD, OnChangeEditAuthPassword)
	ON_CBN_SELCHANGE(IDC_COMBO_SNMP_VERSION, OnSelchangeComboSnmpVersion)
	ON_CBN_SELCHANGE(IDC_COMBO_USM_AUTH, OnSelchangeComboUsmAuth)
	ON_CBN_SELCHANGE(IDC_COMBO_USM_PRIV, OnSelchangeComboUsmPriv)
	ON_EN_CHANGE(IDC_EDIT_SNMP_PORT, OnChangeEditSnmpPort)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsConn message handlers

BOOL CNodePropsConn::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   // Initialize dropdown lists
   for(i = 0; i < 4; i++)
      m_wndAuthList.AddString(m_pszAuthStrings[i]);
   m_wndAuthList.SelectString(-1, m_pszAuthStrings[m_iAuthType]);

	m_wndSnmpVersion.AddString(_T("1"));
	m_wndSnmpVersion.AddString(_T("2c"));
	m_wndSnmpVersion.AddString(_T("3"));
	m_wndSnmpVersion.SelectString(-1, (m_iSNMPVersion == SNMP_VERSION_1) ? _T("1") : ((m_iSNMPVersion == SNMP_VERSION_2C) ? _T("2c") : _T("3")));

	static TCHAR *authMethods[] = { _T("NONE"), _T("MD5"), _T("SHA1") };
	for(i = 0; i < 3; i++)
		m_wndSnmpAuth.AddString(authMethods[i]);
	m_wndSnmpAuth.SelectString(-1, authMethods[m_iSnmpAuth]);
	
	static TCHAR *privMethods[] = { _T("NONE"), _T("DES"), _T("AES") };
	for(i = 0; i < 3; i++)
		m_wndSnmpPriv.AddString(privMethods[i]);
	m_wndSnmpPriv.SelectString(-1, privMethods[m_iSnmpPriv]);
	
	OnSnmpVersionChange();

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
// Handler for "OK" button
//

void CNodePropsConn::OnOK() 
{
   TCHAR szBuffer[256];
   int i;

	switch(m_wndSnmpVersion.GetCurSel())
	{
		case 0:
			m_iSNMPVersion = SNMP_VERSION_1;
			break;
		case 1:
			m_iSNMPVersion = SNMP_VERSION_2C;
			break;
		case 2:
			m_iSNMPVersion = SNMP_VERSION_3;
			break;
	}
	m_iSnmpAuth = m_wndSnmpAuth.GetCurSel();
	m_iSnmpPriv = m_wndSnmpPriv.GetCurSel();

	CPropertyPage::OnOK();

   m_pUpdate->iAgentPort = m_iAgentPort;
   m_pUpdate->pszAuthName = (TCHAR *)((LPCTSTR)m_strSnmpAuthName);
   m_pUpdate->pszAuthPassword = (TCHAR *)((LPCTSTR)m_strSnmpAuthPassword);
   m_pUpdate->pszPrivPassword = (TCHAR *)((LPCTSTR)m_strSnmpPrivPassword);
	m_pUpdate->wSnmpAuthMethod = (WORD)m_iSnmpAuth;
	m_pUpdate->wSnmpPrivMethod = (WORD)m_iSnmpPriv;
   m_pUpdate->pszSecret = (TCHAR *)((LPCTSTR)m_strSecret);
   m_pUpdate->wSNMPVersion = (WORD)m_iSNMPVersion;
	m_pUpdate->wSnmpPort = (WORD)m_snmpPort;
   m_pUpdate->dwProxyNode = m_dwProxyNode;
   m_pUpdate->dwSNMPProxy = m_dwSNMPProxy;
   if (m_bForceEncryption)
      m_pUpdate->dwObjectFlags |= NF_FORCE_ENCRYPTION;
   else
      m_pUpdate->dwObjectFlags &= ~NF_FORCE_ENCRYPTION;

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
// Update controls when SNMP version changed
//

void CNodePropsConn::OnSnmpVersionChange()
{
	switch(m_wndSnmpVersion.GetCurSel())
	{
		case 0:
			m_iSNMPVersion = SNMP_VERSION_1;
			break;
		case 1:
			m_iSNMPVersion = SNMP_VERSION_2C;
			break;
		case 2:
			m_iSNMPVersion = SNMP_VERSION_3;
			break;
	}

	BOOL isV3 = (m_iSNMPVersion == SNMP_VERSION_3);
	EnableDlgItem(this, IDC_EDIT_AUTH_PASSWORD, isV3);
	EnableDlgItem(this, IDC_EDIT_PRIV_PASSWORD, isV3);
	EnableDlgItem(this, IDC_COMBO_USM_AUTH, isV3);
	EnableDlgItem(this, IDC_COMBO_USM_PRIV, isV3);
	SetDlgItemText(IDC_STATIC_AUTH_NAME, isV3 ? _T("User name") : _T("Community string"));
}


//
// Handler for "Select proxy" button
//

void CNodePropsConn::OnSelectProxy() 
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

void CNodePropsConn::OnSelectSnmpproxy() 
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

void CNodePropsConn::OnButtonGenerate() 
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


//
// Change handlers
//

void CNodePropsConn::OnSelchangeComboAuth() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_AUTH;
   SetModified();
}

void CNodePropsConn::OnChangeEditPort() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_PORT;
   SetModified();
}

void CNodePropsConn::OnChangeEditSnmpPort() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_PORT;
   SetModified();
}

void CNodePropsConn::OnChangeEditSecret() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_AGENT_SECRET;
   SetModified();
}

void CNodePropsConn::OnChangeEditPrivPassword() 
{
	m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_AUTH;
	SetModified();
}

void CNodePropsConn::OnChangeEditCommunity() 
{
	m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_AUTH;
	SetModified();
}

void CNodePropsConn::OnChangeEditAuthPassword() 
{
	m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_AUTH;
	SetModified();
}

void CNodePropsConn::OnSelchangeComboSnmpVersion() 
{
   m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_VERSION;
   SetModified();
	OnSnmpVersionChange();
}

void CNodePropsConn::OnSelchangeComboUsmAuth() 
{
	m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_AUTH;
	SetModified();
}

void CNodePropsConn::OnSelchangeComboUsmPriv() 
{
	m_pUpdate->qwFlags |= OBJ_UPDATE_SNMP_AUTH;
	SetModified();
}
