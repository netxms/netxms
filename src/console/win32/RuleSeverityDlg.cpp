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
	DDX_Check(pDX, IDC_CHECK_CRITICAL, m_bCritical);
	DDX_Check(pDX, IDC_CHECK_MAJOR, m_bMajor);
	DDX_Check(pDX, IDC_CHECK_MINOR, m_bMinor);
	DDX_Check(pDX, IDC_CHECK_NORMAL, m_bNormal);
	DDX_Check(pDX, IDC_CHECK_WARNING, m_bWarning);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleSeverityDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleSeverityDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleSeverityDlg message handlers
