// NodePropsGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NodePropsGeneral.h"

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
	m_iSnmpVersion = -1;
	m_iAuthType = -1;
	//}}AFX_DATA_INIT
}

CNodePropsGeneral::~CNodePropsGeneral()
{
}

void CNodePropsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropsGeneral)
	DDX_Control(pDX, IDC_COMBO_SNMP_VERSION, m_wndSnmpVersionList);
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
	DDX_CBIndex(pDX, IDC_COMBO_SNMP_VERSION, m_iSnmpVersion);
	DDX_CBIndex(pDX, IDC_COMBO_AUTH, m_iAuthType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropsGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CNodePropsGeneral)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT_SECRET, OnChangeEditSecret)
	ON_EN_CHANGE(IDC_EDIT_PORT, OnChangeEditPort)
	ON_EN_CHANGE(IDC_EDIT_COMMUNITY, OnChangeEditCommunity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropsGeneral message handlers

BOOL CNodePropsGeneral::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();
	
   // Initialize dropdown lists
   for(i = 0; i < 4; i++)
      m_wndAuthList.AddString(m_pszAuthStrings[i]);
   m_wndAuthList.SelectString(-1, m_pszAuthStrings[m_iAuthType]);

   m_wndSnmpVersionList.AddString("SNMP Version 1");
   m_wndSnmpVersionList.AddString("SNMP Version 2c");

   return TRUE;
}


//
// Handlers for changed controls
//

void CNodePropsGeneral::OnChangeEditName() 
{
   SetModified();
}

void CNodePropsGeneral::OnChangeEditSecret() 
{
   SetModified();
}

void CNodePropsGeneral::OnChangeEditPort() 
{
   SetModified();
}

void CNodePropsGeneral::OnChangeEditCommunity() 
{
   SetModified();
}
