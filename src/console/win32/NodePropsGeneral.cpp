// NodePropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsGeneral.h"
#include <nxsnmp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Static data
//

char *m_pszAuthStrings[] = { "None", "Shared Secred (Plain Text)",
                             "Shared Secred (MD5 Hash)", "Shared Secred (SHA1 Hash)" };

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
	m_strPrimaryIp = _T("");
	m_strSecret = _T("");
	m_strCommunity = _T("");
	m_iAuthType = -1;
	m_iSNMPVersion = -1;
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
	DDX_Text(pDX, IDC_EDIT_PRIMARY_IP, m_strPrimaryIp);
	DDX_Text(pDX, IDC_EDIT_SECRET, m_strSecret);
	DDV_MaxChars(pDX, m_strSecret, 63);
	DDX_Text(pDX, IDC_EDIT_COMMUNITY, m_strCommunity);
	DDV_MaxChars(pDX, m_strCommunity, 63);
	DDX_CBIndex(pDX, IDC_COMBO_AUTH, m_iAuthType);
	DDX_Radio(pDX, IDC_RADIO_VERSION1, m_iSNMPVersion);
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral message handlers

BOOL CNodePropsGeneral::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();

   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
	
   // Initialize dropdown lists
   for(i = 0; i < 4; i++)
      m_wndAuthList.AddString(m_pszAuthStrings[i]);
   m_wndAuthList.SelectString(-1, m_pszAuthStrings[m_iAuthType]);

   return TRUE;
}


//
// Handlers for changed controls
//

void CNodePropsGeneral::OnChangeEditName() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_NAME;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditSecret() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_AGENT_SECRET;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditPort() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_AGENT_PORT;
   SetModified();
}

void CNodePropsGeneral::OnChangeEditCommunity() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_SNMP_COMMUNITY;
   SetModified();
}

void CNodePropsGeneral::OnSelchangeComboAuth() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_AGENT_AUTH;
   SetModified();
}

void CNodePropsGeneral::OnRadioVersion2c() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_SNMP_VERSION;
   SetModified();
}

void CNodePropsGeneral::OnRadioVersion1() 
{
   m_pUpdate->dwFlags |= OBJ_UPDATE_SNMP_VERSION;
   SetModified();
}


//
// Handler for PSN_OK message
//

void CNodePropsGeneral::OnOK() 
{
   char szBuffer[256];
   int i;

	CPropertyPage::OnOK();

   // Set fields in update structure
   m_pUpdate->pszName = (char *)((LPCTSTR)m_strName);
   m_pUpdate->iAgentPort = m_iAgentPort;
   m_pUpdate->pszCommunity = (char *)((LPCTSTR)m_strCommunity);
   m_pUpdate->pszSecret = (char *)((LPCTSTR)m_strSecret);
   m_pUpdate->wSNMPVersion = (m_iSNMPVersion == 0) ? SNMP_VERSION_1 : SNMP_VERSION_2C;

   // Authentication type
   m_wndAuthList.GetWindowText(szBuffer, 255);
   for(i = 0; i < 4; i++)
      if (!strcmp(szBuffer, m_pszAuthStrings[i]))
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
}
