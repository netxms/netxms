// PollCfgPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "PollCfgPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPollCfgPage property page

IMPLEMENT_DYNCREATE(CPollCfgPage, CPropertyPage)

CPollCfgPage::CPollCfgPage() : CPropertyPage(CPollCfgPage::IDD)
{
	//{{AFX_DATA_INIT(CPollCfgPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPollCfgPage::~CPollCfgPage()
{
}

void CPollCfgPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPollCfgPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPollCfgPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPollCfgPage)
	ON_BN_CLICKED(IDC_CHECK_RUN_DISCOVERY, OnCheckRunDiscovery)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPollCfgPage message handlers

//
// Handler for "Back" button
//

LRESULT CPollCfgPage::OnWizardBack() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

   return !_tcsicmp(pc->m_szDBDriver, _T("odbc.ddr")) ? IDD_ODBC : IDD_SELECT_DB;
}


//
// WM_INITDIALOG message handler
//

BOOL CPollCfgPage::OnInitDialog() 
{
   TCHAR szBuffer[32];
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

	CPropertyPage::OnInitDialog();

   // Discovery
   _sntprintf(szBuffer, 32, _T("%d"), pc->m_dwDiscoveryPI);
   SetDlgItemText(IDC_EDIT_INT_DP, szBuffer);
   SendDlgItemMessage(IDC_CHECK_RUN_DISCOVERY, BM_SETCHECK, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_DI, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_SEC, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_EDIT_INT_DP, pc->m_bRunAutoDiscovery);

   // Status poll
   _sntprintf(szBuffer, 32, _T("%d"), pc->m_dwNumStatusPollers);
   SetDlgItemText(IDC_EDIT_NUM_SP, szBuffer);
   _sntprintf(szBuffer, 32, _T("%d"), pc->m_dwStatusPI);
   SetDlgItemText(IDC_EDIT_INT_SP, szBuffer);
	
   // Configuration poll
   _sntprintf(szBuffer, 32, _T("%d"), pc->m_dwNumConfigPollers);
   SetDlgItemText(IDC_EDIT_NUM_CP, szBuffer);
   _sntprintf(szBuffer, 32, _T("%d"), pc->m_dwConfigurationPI);
   SetDlgItemText(IDC_EDIT_INT_CP, szBuffer);
	
	return TRUE;
}


//
// Handler for "Run autodiscovery" check box
//

void CPollCfgPage::OnCheckRunDiscovery() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

   pc->m_bRunAutoDiscovery = (SendDlgItemMessage(IDC_CHECK_RUN_DISCOVERY, BM_GETCHECK) == BST_CHECKED);
   EnableDlgItem(this, IDC_STATIC_DI, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_SEC, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_EDIT_INT_DP, pc->m_bRunAutoDiscovery);
}


//
// Handler for "Next" button
//

LRESULT CPollCfgPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   // Discovery poll
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_DP, &pc->m_dwDiscoveryPI, 5))
   {
      MessageBox(_T("Invalid value entered as discovery polling interval"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_DP)->SetFocus();
      return -1;
   }

   // Status poll
   if (!GetEditBoxValueULong(this, IDC_EDIT_NUM_SP, &pc->m_dwNumStatusPollers, 1, 100000))
   {
      MessageBox(_T("Invalid value entered as number of status pollers"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_NUM_SP)->SetFocus();
      return -1;
   }
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_SP, &pc->m_dwStatusPI, 5))
   {
      MessageBox(_T("Invalid value entered as status polling interval"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_SP)->SetFocus();
      return -1;
   }

   // Configuration poll
   if (!GetEditBoxValueULong(this, IDC_EDIT_NUM_CP, &pc->m_dwNumConfigPollers, 1, 1000))
   {
      MessageBox(_T("Invalid value entered as number of configuration pollers"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_NUM_CP)->SetFocus();
      return -1;
   }
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_CP, &pc->m_dwConfigurationPI, 30))
   {
      MessageBox(_T("Invalid value entered as configuration polling interval"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_CP)->SetFocus();
      return -1;
   }

	return CPropertyPage::OnWizardNext();
}
