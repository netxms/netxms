// RuleAlarmDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleAlarmDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleAlarmDlg dialog


CRuleAlarmDlg::CRuleAlarmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleAlarmDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleAlarmDlg)
	m_iSeverity = -1;
	m_strMessage = _T("");
	m_strKey = _T("");
	m_strAckKey = _T("");
	//}}AFX_DATA_INIT
}


void CRuleAlarmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleAlarmDlg)
	DDX_Radio(pDX, IDC_RADIO_NORMAL, m_iSeverity);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_strMessage);
	DDX_Text(pDX, IDC_EDIT_KEY, m_strKey);
	DDX_Text(pDX, IDC_EDIT_KEYACK, m_strAckKey);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleAlarmDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleAlarmDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleAlarmDlg message handlers
