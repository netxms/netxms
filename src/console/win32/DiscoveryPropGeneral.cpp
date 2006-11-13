// DiscoveryPropGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DiscoveryPropGeneral.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropGeneral property page

IMPLEMENT_DYNCREATE(CDiscoveryPropGeneral, CPropertyPage)

CDiscoveryPropGeneral::CDiscoveryPropGeneral() : CPropertyPage(CDiscoveryPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CDiscoveryPropGeneral)
	m_bAllowAgent = FALSE;
	m_bAllowRange = FALSE;
	m_bAllowSNMP = FALSE;
	m_strScript = _T("");
	m_strCommunity = _T("");
	m_nMode = -1;
	m_nFilter = -1;
	//}}AFX_DATA_INIT
}

CDiscoveryPropGeneral::~CDiscoveryPropGeneral()
{
}

void CDiscoveryPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiscoveryPropGeneral)
	DDX_Check(pDX, IDC_CHECK_AGENT, m_bAllowAgent);
	DDX_Check(pDX, IDC_CHECK_RANGE, m_bAllowRange);
	DDX_Check(pDX, IDC_CHECK_SNMP, m_bAllowSNMP);
	DDX_Text(pDX, IDC_EDIT_SCRIPT, m_strScript);
	DDV_MaxChars(pDX, m_strScript, 255);
	DDX_Text(pDX, IDC_EDIT_COMMUNITY, m_strCommunity);
	DDV_MaxChars(pDX, m_strCommunity, 63);
	DDX_Radio(pDX, IDC_RADIO_DISABLED, m_nMode);
	DDX_Radio(pDX, IDC_RADIO_NONE, m_nFilter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscoveryPropGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CDiscoveryPropGeneral)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	ON_BN_CLICKED(IDC_RADIO_PASSIVE, OnRadioPassive)
	ON_BN_CLICKED(IDC_RADIO_ACTIVE, OnRadioActive)
	ON_BN_CLICKED(IDC_RADIO_DISABLED, OnRadioDisabled)
	ON_BN_CLICKED(IDC_RADIO_AUTO, OnRadioAuto)
	ON_BN_CLICKED(IDC_RADIO_CUSTOM, OnRadioCustom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropGeneral message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDiscoveryPropGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

   OnModeSelection();
	
	return TRUE;
}


//
// Change dialog behaviour based on current discovery mode
//

void CDiscoveryPropGeneral::OnModeSelection()
{
   UpdateData(TRUE);

   EnableDlgItem(this, IDC_EDIT_COMMUNITY, m_nMode > 0);
   EnableDlgItem(this, IDC_RADIO_NONE, m_nMode > 0);
   EnableDlgItem(this, IDC_RADIO_AUTO, m_nMode > 0);
   EnableDlgItem(this, IDC_RADIO_CUSTOM, m_nMode > 0);
   EnableDlgItem(this, IDC_EDIT_SCRIPT, (m_nMode > 0) && (m_nFilter == 1));
   EnableDlgItem(this, IDC_BUTTON_SELECT, (m_nMode > 0) && (m_nFilter == 1));
   EnableDlgItem(this, IDC_CHECK_AGENT, (m_nMode > 0) && (m_nFilter == 2));
   EnableDlgItem(this, IDC_CHECK_SNMP, (m_nMode > 0) && (m_nFilter == 2));
   EnableDlgItem(this, IDC_CHECK_RANGE, (m_nMode > 0) && (m_nFilter == 2));
}


//
// Handlers for discovery mode selection radio buttons
//

void CDiscoveryPropGeneral::OnRadioDisabled() 
{
   OnModeSelection();
}

void CDiscoveryPropGeneral::OnRadioPassive() 
{
   OnModeSelection();
}

void CDiscoveryPropGeneral::OnRadioActive() 
{
   OnModeSelection();
}


//
// Handlers for discovery filter selection radio buttons
//

void CDiscoveryPropGeneral::OnRadioNone() 
{
   OnFilterSelection();
}

void CDiscoveryPropGeneral::OnRadioAuto() 
{
   OnFilterSelection();
}

void CDiscoveryPropGeneral::OnRadioCustom() 
{
   OnFilterSelection();
}


//
// Change dialog behaviour based on current discovery filter
//

void CDiscoveryPropGeneral::OnFilterSelection()
{
   UpdateData(TRUE);

   EnableDlgItem(this, IDC_CHECK_AGENT, m_nFilter == 2);
   EnableDlgItem(this, IDC_CHECK_SNMP, m_nFilter == 2);
   EnableDlgItem(this, IDC_CHECK_RANGE, m_nFilter == 2);
   EnableDlgItem(this, IDC_EDIT_SCRIPT, m_nFilter == 1);
   EnableDlgItem(this, IDC_BUTTON_SELECT, m_nFilter == 1);
}
