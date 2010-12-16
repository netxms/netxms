// WinSrvPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxconfig.h"
#include "WinSrvPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinSrvPage property page

IMPLEMENT_DYNCREATE(CWinSrvPage, CPropertyPage)

CWinSrvPage::CWinSrvPage() : CPropertyPage(CWinSrvPage::IDD)
{
	//{{AFX_DATA_INIT(CWinSrvPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWinSrvPage::~CWinSrvPage()
{
}

void CWinSrvPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinSrvPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWinSrvPage, CPropertyPage)
	//{{AFX_MSG_MAP(CWinSrvPage)
	ON_BN_CLICKED(IDC_RADIO_SYSTEM, OnRadioSystem)
	ON_BN_CLICKED(IDC_RADIO_USER, OnRadioUser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinSrvPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CWinSrvPage::OnInitDialog() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

	CPropertyPage::OnInitDialog();
	
   SendDlgItemMessage(IDC_RADIO_SYSTEM, BM_SETCHECK, BST_CHECKED);
   EnableDlgItem(this, IDC_EDIT_LOGIN, FALSE);
   EnableDlgItem(this, IDC_EDIT_PASSWD1, FALSE);
   EnableDlgItem(this, IDC_EDIT_PASSWD2, FALSE);
	
   if (_tcscmp(pc->m_szDBLogin, _T("*")) ||
       (pc->m_iDBEngine != DB_ENGINE_MSSQL))
   {
      ::ShowWindow(::GetDlgItem(m_hWnd, IDC_ICON_WARNING), SW_HIDE);
      ::ShowWindow(::GetDlgItem(m_hWnd, IDC_STATIC_WARNING), SW_HIDE);
   }

	return TRUE;
}


//
// Handler for "Next" button
//

LRESULT CWinSrvPage::OnWizardNext() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   if (SendDlgItemMessage(IDC_RADIO_SYSTEM, BM_GETCHECK) == BST_CHECKED)
   {
      pc->m_szServiceLogin[0] = 0;
      pc->m_szServicePassword[0] = 0;
   }
   else
   {
      TCHAR szPasswd1[MAX_DB_STRING], szPasswd2[MAX_DB_STRING];

      GetDlgItemText(IDC_EDIT_LOGIN, pc->m_szServiceLogin, MAX_DB_STRING);
      GetDlgItemText(IDC_EDIT_PASSWD1, szPasswd1, MAX_DB_STRING);
      GetDlgItemText(IDC_EDIT_PASSWD2, szPasswd2, MAX_DB_STRING);
      if (!_tcscmp(szPasswd1, szPasswd2))
      {
         _tcscpy(pc->m_szServicePassword, szPasswd1);
      }
      else
      {
         MessageBox(_T("Entered passwords does not match"), _T("Warning"),
                    MB_OK | MB_ICONEXCLAMATION);
         return -1;
      }
   }
	
	return CPropertyPage::OnWizardNext();
}


//
// Handlers for radio buttons
//

void CWinSrvPage::OnRadioSystem() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;

   EnableDlgItem(this, IDC_EDIT_LOGIN, FALSE);
   EnableDlgItem(this, IDC_EDIT_PASSWD1, FALSE);
   EnableDlgItem(this, IDC_EDIT_PASSWD2, FALSE);

   if ((!_tcscmp(pc->m_szDBLogin, _T("*"))) &&
       (pc->m_iDBEngine == DB_ENGINE_MSSQL))
   {
      ::ShowWindow(::GetDlgItem(m_hWnd, IDC_ICON_WARNING), SW_SHOW);
      ::ShowWindow(::GetDlgItem(m_hWnd, IDC_STATIC_WARNING), SW_SHOW);
   }
}

void CWinSrvPage::OnRadioUser() 
{
   EnableDlgItem(this, IDC_EDIT_LOGIN, TRUE);
   EnableDlgItem(this, IDC_EDIT_PASSWD1, TRUE);
   EnableDlgItem(this, IDC_EDIT_PASSWD2, TRUE);

   ::ShowWindow(::GetDlgItem(m_hWnd, IDC_ICON_WARNING), SW_HIDE);
   ::ShowWindow(::GetDlgItem(m_hWnd, IDC_STATIC_WARNING), SW_HIDE);
}


//
// Page activation handler
//

BOOL CWinSrvPage::OnSetActive() 
{
   WIZARD_CFG_INFO *pc = &((CConfigWizard *)GetParent())->m_cfg;
   BOOL bShow;

   bShow = ((!_tcscmp(pc->m_szDBLogin, _T("*"))) && (pc->m_iDBEngine == DB_ENGINE_MSSQL));
   ::ShowWindow(::GetDlgItem(m_hWnd, IDC_ICON_WARNING), bShow ? SW_SHOW : SW_HIDE);
   ::ShowWindow(::GetDlgItem(m_hWnd, IDC_STATIC_WARNING), bShow ? SW_SHOW : SW_HIDE);
	
	return CPropertyPage::OnSetActive();
}
