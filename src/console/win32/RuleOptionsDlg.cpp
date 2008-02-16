// RuleOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleOptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleOptionsDlg dialog


CRuleOptionsDlg::CRuleOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleOptionsDlg)
	m_bStopProcessing = FALSE;
	//}}AFX_DATA_INIT
}


void CRuleOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleOptionsDlg)
	DDX_Check(pDX, IDC_CHECK_STOP, m_bStopProcessing);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleOptionsDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleOptionsDlg message handlers
