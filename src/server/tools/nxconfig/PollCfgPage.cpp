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

/**
 * WM_INITDIALOG message handler
 */
BOOL CPollCfgPage::OnInitDialog() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

	CPropertyPage::OnInitDialog();

   // Discovery
   SetDlgItemText(IDC_EDIT_INT_DP, pc->m_discoveryPollingInterval);
   SendDlgItemMessage(IDC_CHECK_RUN_DISCOVERY, BM_SETCHECK, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_DI, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_SEC, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_EDIT_INT_DP, pc->m_bRunAutoDiscovery);

   // Poll intervals
   SetDlgItemText(IDC_EDIT_INT_SP, pc->m_statusPollingInterval);
   SetDlgItemText(IDC_EDIT_INT_CP, pc->m_configurationPollingInterval);
   SetDlgItemText(IDC_EDIT_INT_TP, pc->m_topologyPollingInterval);

   // Poller thread pool
   SetDlgItemText(IDC_EDIT_POLLER_TP_BASE, pc->m_pollerPoolBaseSize);
   SetDlgItemText(IDC_EDIT_POLLER_TP_MAX, pc->m_pollerPoolMaxSize);
	
	return TRUE;
}

/**
 * Set dialog item text from numeric value
 */
void CPollCfgPage::SetDlgItemText(int nIDDlgItem, DWORD value)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, _T("%u"), value);
   SetDlgItemText(nIDDlgItem, buffer);
}

/**
 * Handler for "Run autodiscovery" check box
 */
void CPollCfgPage::OnCheckRunDiscovery() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;	

   pc->m_bRunAutoDiscovery = (SendDlgItemMessage(IDC_CHECK_RUN_DISCOVERY, BM_GETCHECK) == BST_CHECKED);
   EnableDlgItem(this, IDC_STATIC_DI, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_STATIC_SEC, pc->m_bRunAutoDiscovery);
   EnableDlgItem(this, IDC_EDIT_INT_DP, pc->m_bRunAutoDiscovery);
}

/**
 * Handler for "Next" button
 */
LRESULT CPollCfgPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   // Discovery poll
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_DP, &pc->m_discoveryPollingInterval, 5))
   {
      MessageBox(_T("Invalid value entered as discovery polling interval"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_DP)->SetFocus();
      return -1;
   }

   // Poller intervals
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_SP, &pc->m_statusPollingInterval, 5))
   {
      MessageBox(_T("Invalid value entered as status polling interval"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_SP)->SetFocus();
      return -1;
   }
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_CP, &pc->m_configurationPollingInterval, 30))
   {
      MessageBox(_T("Invalid value entered as configuration polling interval"),
         _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_CP)->SetFocus();
      return -1;
   }
   if (!GetEditBoxValueULong(this, IDC_EDIT_INT_TP, &pc->m_topologyPollingInterval, 10))
   {
      MessageBox(_T("Invalid value entered as topology polling interval"),
         _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_INT_TP)->SetFocus();
      return -1;
   }

   // Poller thread pool
   if (!GetEditBoxValueULong(this, IDC_EDIT_POLLER_TP_BASE, &pc->m_pollerPoolBaseSize, 1, 1000))
   {
      MessageBox(_T("Invalid value entered as base size of poller thread pool"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_POLLER_TP_BASE)->SetFocus();
      return -1;
   }
   if (!GetEditBoxValueULong(this, IDC_EDIT_POLLER_TP_MAX, &pc->m_pollerPoolMaxSize, pc->m_pollerPoolBaseSize, 4096))
   {
      MessageBox(_T("Invalid value entered as maximum size of poller thread pool"),
         _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
      GetDlgItem(IDC_EDIT_POLLER_TP_MAX)->SetFocus();
      return -1;
   }

	return CPropertyPage::OnWizardNext();
}
