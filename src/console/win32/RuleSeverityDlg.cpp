// RuleSeverityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleSeverityDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Static data
//

static int m_iCheckBoxList[] = { IDC_CHECK_NORMAL, IDC_CHECK_WARNING, IDC_CHECK_MINOR,
                                 IDC_CHECK_MAJOR, IDC_CHECK_CRITICAL, -1 };


/////////////////////////////////////////////////////////////////////////////
// CRuleSeverityDlg dialog


CRuleSeverityDlg::CRuleSeverityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleSeverityDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleSeverityDlg)
	m_bCritical = FALSE;
	m_bMajor = FALSE;
	m_bMinor = FALSE;
	m_bNormal = FALSE;
	m_bWarning = FALSE;
	//}}AFX_DATA_INIT
}


void CRuleSeverityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleSeverityDlg)
	DDX_Control(pDX, IDOK, m_wndOkButton);
	DDX_Check(pDX, IDC_CHECK_CRITICAL, m_bCritical);
	DDX_Check(pDX, IDC_CHECK_MAJOR, m_bMajor);
	DDX_Check(pDX, IDC_CHECK_MINOR, m_bMinor);
	DDX_Check(pDX, IDC_CHECK_NORMAL, m_bNormal);
	DDX_Check(pDX, IDC_CHECK_WARNING, m_bWarning);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleSeverityDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleSeverityDlg)
	ON_BN_CLICKED(IDC_BUTTON_ALL, OnButtonAll)
	ON_BN_CLICKED(IDC_CHECK_CRITICAL, OnCheckCritical)
	ON_BN_CLICKED(IDC_CHECK_MAJOR, OnCheckMajor)
	ON_BN_CLICKED(IDC_CHECK_MINOR, OnCheckMinor)
	ON_BN_CLICKED(IDC_CHECK_NORMAL, OnCheckNormal)
	ON_BN_CLICKED(IDC_CHECK_WARNING, OnCheckWarning)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleSeverityDlg message handlers

void CRuleSeverityDlg::OnButtonAll() 
{
   int i;

   for(i = 0; m_iCheckBoxList[i] != -1; i++)
      SendDlgItemMessage(m_iCheckBoxList[i], BM_SETCHECK, BST_CHECKED, 0);
   m_wndOkButton.EnableWindow(TRUE);
}

void CRuleSeverityDlg::OnCheckCritical() 
{
   OnCheckBox();
}

void CRuleSeverityDlg::OnCheckMajor() 
{
   OnCheckBox();
}

void CRuleSeverityDlg::OnCheckMinor() 
{
   OnCheckBox();
}

void CRuleSeverityDlg::OnCheckNormal() 
{
   OnCheckBox();
}

void CRuleSeverityDlg::OnCheckWarning() 
{
   OnCheckBox();
}

void CRuleSeverityDlg::OnCheckBox()
{
   int i;

   for(i = 0; m_iCheckBoxList[i] != -1; i++)
      if (SendDlgItemMessage(m_iCheckBoxList[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
         break;

   m_wndOkButton.EnableWindow(m_iCheckBoxList[i] != -1);
}
