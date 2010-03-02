// SummaryPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "SummaryPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFFER_SIZE     1024

/////////////////////////////////////////////////////////////////////////////
// CSummaryPage property page

IMPLEMENT_DYNCREATE(CSummaryPage, CPropertyPage)

CSummaryPage::CSummaryPage() : CPropertyPage(CSummaryPage::IDD)
{
	//{{AFX_DATA_INIT(CSummaryPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSummaryPage::~CSummaryPage()
{
}

void CSummaryPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSummaryPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSummaryPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSummaryPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSummaryPage message handlers


//
// Add new parameter to buffer
//

static void AddParam(TCHAR *pszBuffer, TCHAR *pszText, void *pValue)
{
   int iLen;

   iLen = (int)_tcslen(pszBuffer);
   _sntprintf(&pszBuffer[iLen], BUFFER_SIZE - iLen, pszText, pValue);
}


//
// Handler for page activation
//

BOOL CSummaryPage::OnSetActive() 
{
   TCHAR szBuffer[1024];
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   szBuffer[0] = 0;
   AddParam(szBuffer, _T("Database engine: %s\r\n"), g_pszDBEngines[pc->m_iDBEngine]);
   AddParam(szBuffer, _T("Database driver: %s\r\n"), pc->m_szDBDriver);
   AddParam(szBuffer, _T("Create new database: %s\r\n"), pc->m_bCreateDB ? _T("yes") : _T("no"));
   if (!pc->m_bCreateDB)
      AddParam(szBuffer, _T("Initalize database: %s\r\n"), pc->m_bInitDB ? _T("yes") : _T("no"));

   if (!_tcsicmp(pc->m_szDBDriver, _T("sqlite.ddr")))
   {
      AddParam(szBuffer, _T("\r\nDatabase file: %s\r\n"), pc->m_szDBName);
   }
   else
   {
      if (!_tcsicmp(pc->m_szDBDriver, _T("odbc.ddr")))
      {
         AddParam(szBuffer, _T("\r\nODBC data source: %s\r\n"), pc->m_szDBServer);
      }
      else
      {
         AddParam(szBuffer, _T("\r\nDatabase server: %s\r\n"), pc->m_szDBServer);
         AddParam(szBuffer, _T("Database name: %s\r\n"), pc->m_szDBName);
      }
      AddParam(szBuffer, _T("Database user: %s\r\n"), pc->m_szDBLogin);
      if (pc->m_bCreateDB)
         AddParam(szBuffer, _T("Database administrator: %s\r\n"), pc->m_szDBALogin);
   }

   AddParam(szBuffer, _T("\r\nRun IP autodiscovery: %s\r\n"), pc->m_bRunAutoDiscovery ? _T("yes") : _T("no"));
   if (pc->m_bRunAutoDiscovery)
      AddParam(szBuffer, _T("Interval between discovery polls: %d\r\n"), CAST_TO_POINTER(pc->m_dwDiscoveryPI, void *));

   AddParam(szBuffer, _T("\r\nNumber of status pollers: %d\r\n"), CAST_TO_POINTER(pc->m_dwNumStatusPollers, void *));
   AddParam(szBuffer, _T("Interval between status polls: %d\r\n"), CAST_TO_POINTER(pc->m_dwStatusPI, void *));
   
   AddParam(szBuffer, _T("\r\nNumber of configuration pollers: %d\r\n"), CAST_TO_POINTER(pc->m_dwNumConfigPollers, void *));
   AddParam(szBuffer, _T("Interval between configuration polls: %d\r\n"), CAST_TO_POINTER(pc->m_dwConfigurationPI, void *));

   AddParam(szBuffer, _T("\r\nSMTP server: %s\r\n"), pc->m_szSMTPServer);
   AddParam(szBuffer, _T("System's email address: %s\r\n"), pc->m_szSMTPMailFrom);
   
   SetDlgItemText(IDC_EDIT_SUMMARY, szBuffer);
   SendDlgItemMessage(IDC_EDIT_SUMMARY, EM_SETSEL, -1, 0);

   ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}
