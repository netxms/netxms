// ConfigFilePage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "ConfigFilePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigFilePage property page

IMPLEMENT_DYNCREATE(CConfigFilePage, CPropertyPage)

CConfigFilePage::CConfigFilePage() : CPropertyPage(CConfigFilePage::IDD)
{
	//{{AFX_DATA_INIT(CConfigFilePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CConfigFilePage::~CConfigFilePage()
{
}

void CConfigFilePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigFilePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigFilePage, CPropertyPage)
	//{{AFX_MSG_MAP(CConfigFilePage)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigFilePage message handlers


//
// "Next" button handler
//

LRESULT CConfigFilePage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   GetDlgItemText(IDC_EDIT_FILE, pc->m_szConfigFile, MAX_PATH);
   StrStrip(pc->m_szConfigFile);
	
	return CPropertyPage::OnWizardNext();
}


//
// Handler for "Browse..." button
//

void CConfigFilePage::OnButtonBrowse() 
{
   CFileDialog dlg(TRUE, _T(".conf"), NULL, OFN_PATHMUSTEXIST,
                   _T("Configuration Files (*.conf)|*.conf|All Files (*.*)|*.*||"), this);

   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}


//
// Page activation handler
//

BOOL CConfigFilePage::OnSetActive() 
{
   ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}
