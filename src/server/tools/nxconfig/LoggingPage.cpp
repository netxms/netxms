// LoggingPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "LoggingPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoggingPage property page

IMPLEMENT_DYNCREATE(CLoggingPage, CPropertyPage)

CLoggingPage::CLoggingPage() : CPropertyPage(CLoggingPage::IDD)
{
	//{{AFX_DATA_INIT(CLoggingPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CLoggingPage::~CLoggingPage()
{
}

void CLoggingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoggingPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoggingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CLoggingPage)
	ON_BN_CLICKED(IDC_RADIO_SYSLOG, OnRadioSyslog)
	ON_BN_CLICKED(IDC_RADIO_FILE, OnRadioFile)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoggingPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CLoggingPage::OnInitDialog() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

	CPropertyPage::OnInitDialog();
	
   if (pc->m_bLogToSyslog)
   {
      SendDlgItemMessage(IDC_RADIO_SYSLOG, BM_SETCHECK, BST_CHECKED);
      EnableDlgItem(this, IDC_EDIT_FILE, FALSE);
      EnableDlgItem(this, IDC_BUTTON_BROWSE, FALSE);
   }
   else
   {
      SendDlgItemMessage(IDC_RADIO_FILE, BM_SETCHECK, BST_CHECKED);
      SetDlgItemText(IDC_EDIT_FILE, pc->m_szLogFile);
   }
	
	return TRUE;
}


//
// Radio butons handlers
//

void CLoggingPage::OnRadioSyslog() 
{
   EnableDlgItem(this, IDC_EDIT_FILE, FALSE);
   EnableDlgItem(this, IDC_BUTTON_BROWSE, FALSE);
}

void CLoggingPage::OnRadioFile() 
{
   EnableDlgItem(this, IDC_EDIT_FILE, TRUE);
   EnableDlgItem(this, IDC_BUTTON_BROWSE, TRUE);
}


//
// "Next" button handler
//

LRESULT CLoggingPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   pc->m_bLogToSyslog = (SendDlgItemMessage(IDC_RADIO_SYSLOG, BM_GETCHECK) == BST_CHECKED);
   GetDlgItemText(IDC_EDIT_FILE, pc->m_szLogFile, MAX_PATH);
	return CPropertyPage::OnWizardNext();
}


//
// Handler for "Browse..." button
//

void CLoggingPage::OnButtonBrowse() 
{
   CFileDialog dlg(TRUE, _T(".log"), NULL, OFN_PATHMUSTEXIST,
                   _T("Log Files (*.log)|*.log|All Files (*.*)|*.*||"), this);

   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
