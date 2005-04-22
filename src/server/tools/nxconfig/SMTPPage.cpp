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
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSMTPPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSMTPPage)
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

	CPropertyPage::OnInitDialog();

   SetDlgItemText(IDC_EDIT_SERVER, pc->m_szSMTPServer);
   SetDlgItemText(IDC_EDIT_EMAIL, pc->m_szSMTPMailFrom);
	
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
	
	return CPropertyPage::OnWizardNext();
}
