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
	m_bGenerateAlarm = FALSE;
	//}}AFX_DATA_INIT
}


void CRuleAlarmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleAlarmDlg)
	DDX_Radio(pDX, IDC_RADIO_NORMAL, m_iSeverity);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_strMessage);
	DDV_MaxChars(pDX, m_strMessage, 255);
	DDX_Text(pDX, IDC_EDIT_KEY, m_strKey);
	DDV_MaxChars(pDX, m_strKey, 255);
	DDX_Text(pDX, IDC_EDIT_KEYACK, m_strAckKey);
	DDV_MaxChars(pDX, m_strAckKey, 255);
	DDX_Check(pDX, IDC_CHECK_ALARM, m_bGenerateAlarm);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleAlarmDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleAlarmDlg)
	ON_BN_CLICKED(IDC_CHECK_ALARM, OnCheckAlarm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleAlarmDlg message handlers

void CRuleAlarmDlg::OnCheckAlarm() 
{
	EnableControls(SendDlgItemMessage(IDC_CHECK_ALARM, BM_GETCHECK, 0, 0) == BST_CHECKED);
}


//
// WM_INITDIALOG message handler
//

BOOL CRuleAlarmDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   EnableControls(m_bGenerateAlarm);
	return TRUE;
}


//
// Enable or disable all controls except "generate alarm" checkbox
//

void CRuleAlarmDlg::EnableControls(BOOL bEnable)
{
   static int iCtrlList[] = { IDC_EDIT_MESSAGE, IDC_EDIT_KEY, IDC_EDIT_KEYACK,
                              IDC_RADIO_NORMAL, IDC_RADIO_WARNING, IDC_RADIO_MINOR,
                              IDC_RADIO_MAJOR, IDC_RADIO_CRITICAL, IDC_RADIO_EVENT,
                              IDC_RADIO_NONE, IDC_STATIC_MESSAGE, IDC_STATIC_KEY,
                              IDC_STATIC_KEYACK, IDC_STATIC_ACK, IDC_STATIC_SEVERITY, -1 };
   int i;

   for(i = 0; iCtrlList[i] != -1; i++)
      GetDlgItem(iCtrlList[i])->EnableWindow(bEnable);
   
   if (bEnable)
      GetDlgItem(IDC_EDIT_MESSAGE)->SetFocus();
}
