// SMTPPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "SMTPPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSMTPPage property page

IMPLEMENT_DYNCREATE(CSMTPPage, CPropertyPage)

CSMTPPage::CSMTPPage() : CPropertyPage(CSMTPPage::IDD)
{
	//{{AFX_DATA_INIT(CSMTPPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSMTPPage::~CSMTPPage()
{
}

void CSMTPPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSMTPPage)
	DDX_Control(pDX, IDC_COMBO_SMSDRV, m_wndDrvList);
	DDX_Control(pDX, IDC_COMBO_PORT, m_wndPortList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSMTPPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSMTPPage)
	ON_CBN_SELCHANGE(IDC_COMBO_SMSDRV, OnSelchangeComboSmsdrv)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSMTPPage message handlers

//
// WM_INITDIALOG message handler
//

BOOL CSMTPPage::OnInitDialog() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   BOOL bEnable;

	CPropertyPage::OnInitDialog();

   SetDlgItemText(IDC_EDIT_SERVER, pc->m_szSMTPServer);
   SetDlgItemText(IDC_EDIT_EMAIL, pc->m_szSMTPMailFrom);

   m_wndDrvList.AddString(_T("<none>"));
   m_wndDrvList.AddString(_T("generic.sms"));
   m_wndDrvList.SelectString(-1, pc->m_szSMSDriver);

   m_wndPortList.AddString(_T("COM1:"));
   m_wndPortList.AddString(_T("COM2:"));
   m_wndPortList.AddString(_T("COM3:"));
   m_wndPortList.AddString(_T("COM4:"));
   m_wndPortList.AddString(_T("COM5:"));
   m_wndPortList.AddString(_T("COM6:"));
   m_wndPortList.AddString(_T("COM7:"));
   m_wndPortList.AddString(_T("COM8:"));
   m_wndPortList.AddString(_T("COM9:"));
   m_wndPortList.SelectString(-1, pc->m_szSMSDrvParam);
	
   bEnable = (_tcsicmp(pc->m_szSMSDriver, _T("<none>")) != 0);
   EnableDlgItem(this, IDC_STATIC_PORT, bEnable);
   EnableDlgItem(this, IDC_COMBO_PORT, bEnable);

	return TRUE;
}


//
// Handler for "Next" button
//

LRESULT CSMTPPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   GetDlgItemText(IDC_EDIT_SERVER, pc->m_szSMTPServer, MAX_DB_STRING);
   GetDlgItemText(IDC_EDIT_EMAIL, pc->m_szSMTPMailFrom, MAX_DB_STRING);

   m_wndDrvList.GetWindowText(pc->m_szSMSDriver, MAX_PATH);
   m_wndPortList.GetWindowText(pc->m_szSMSDrvParam, MAX_DB_STRING);
	
	return CPropertyPage::OnWizardNext();
}


//
// Handler for SMS driver selection change
//

void CSMTPPage::OnSelchangeComboSmsdrv() 
{
   TCHAR szBuffer[256];
   BOOL bEnable;

   m_wndDrvList.GetWindowText(szBuffer, 256);
   bEnable = (_tcsicmp(szBuffer, _T("<none>")) != 0);
   EnableDlgItem(this, IDC_STATIC_PORT, bEnable);
   EnableDlgItem(this, IDC_COMBO_PORT, bEnable);
}
