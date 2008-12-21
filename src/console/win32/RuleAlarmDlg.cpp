// RuleAlarmDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleAlarmDlg.h"
#include "EventSelDlg.h"

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
	m_dwTimeout = 0;
	//}}AFX_DATA_INIT
	m_nMode = -1;
	m_iSeverity = 0;
	m_dwTimeout = 0;
	m_dwTimeoutEvent = EVENT_ALARM_TIMEOUT;
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
	DDX_Text(pDX, IDC_EDIT_ACKKEY, m_strAckKey);
	DDV_MaxChars(pDX, m_strAckKey, 255);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_dwTimeout);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleAlarmDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleAlarmDlg)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	ON_BN_CLICKED(IDC_RADIO_NEWALARM, OnRadioNewalarm)
	ON_BN_CLICKED(IDC_RADIO_TERMINATE, OnRadioTerminate)
	ON_BN_CLICKED(IDC_SELECT_EVENT, OnSelectEvent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleAlarmDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CRuleAlarmDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   if (m_nMode == 0)
	{
		SendDlgItemMessage(IDC_RADIO_NEWALARM, BM_SETCHECK, BST_CHECKED, 0);
	}
   else if (m_nMode == 1)
	{
		SendDlgItemMessage(IDC_RADIO_TERMINATE, BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
		SendDlgItemMessage(IDC_RADIO_NONE, BM_SETCHECK, BST_CHECKED, 0);
	}
   SetDlgItemText(IDC_EDIT_EVENT, NXCGetEventName(g_hSession, m_dwTimeoutEvent));
   EnableControls(m_nMode);
	return FALSE;
}


//
// Enable or disable all controls except "generate alarm" checkbox
//

void CRuleAlarmDlg::EnableControls(int nMode)
{
   static int iCtrlList[2][18] = 
	{
		{ 
	      IDC_EDIT_MESSAGE, IDC_EDIT_KEY,
         IDC_RADIO_NORMAL, IDC_RADIO_WARNING, IDC_RADIO_MINOR,
         IDC_RADIO_MAJOR, IDC_RADIO_CRITICAL, IDC_RADIO_EVENT,
         IDC_STATIC_MESSAGE, IDC_STATIC_KEY, IDC_STATIC_SECONDS,
         IDC_STATIC_SEVERITY, IDC_STATIC_TIMEOUT,
			IDC_STATIC_EVENT, IDC_EDIT_EVENT, IDC_EDIT_TIMEOUT,
			IDC_SELECT_EVENT, -1
		},
		{
			IDC_EDIT_ACKKEY, IDC_STATIC_ACKKEY, -1
		}
	};
   int i, j;

	for(j = 0; j < 2; j++)
		for(i = 0; iCtrlList[j][i] != -1; i++)
			GetDlgItem(iCtrlList[j][i])->EnableWindow(j == nMode);
   
   if (nMode == 0)
	{
      GetDlgItem(IDC_EDIT_MESSAGE)->SetFocus();
	}
   else if (nMode == 1)
	{
      GetDlgItem(IDC_EDIT_ACKKEY)->SetFocus();
	}
	else
	{
      GetDlgItem(IDC_RADIO_NONE)->SetFocus();
	}
}


//
// Handlers for radio buttons
//

void CRuleAlarmDlg::OnRadioNone() 
{
	if (SendDlgItemMessage(IDC_RADIO_NONE, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		m_nMode = -1;
		SendDlgItemMessage(IDC_RADIO_NEWALARM, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_RADIO_TERMINATE, BM_SETCHECK, BST_UNCHECKED, 0);
		EnableControls(m_nMode);
	}
}

void CRuleAlarmDlg::OnRadioNewalarm() 
{
	if (SendDlgItemMessage(IDC_RADIO_NEWALARM, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		m_nMode = 0;
		SendDlgItemMessage(IDC_RADIO_NONE, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_RADIO_TERMINATE, BM_SETCHECK, BST_UNCHECKED, 0);
		EnableControls(m_nMode);
	}
}

void CRuleAlarmDlg::OnRadioTerminate() 
{
	if (SendDlgItemMessage(IDC_RADIO_TERMINATE, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		m_nMode = 1;
		SendDlgItemMessage(IDC_RADIO_NONE, BM_SETCHECK, BST_UNCHECKED, 0);
		SendDlgItemMessage(IDC_RADIO_NEWALARM, BM_SETCHECK, BST_UNCHECKED, 0);
		EnableControls(m_nMode);
	}
}


//
// "Select..." button handler
//

void CRuleAlarmDlg::OnSelectEvent() 
{
   CEventSelDlg dlg;

   dlg.m_bSingleSelection = TRUE;
   if (dlg.DoModal() == IDOK)
   {
      m_dwTimeoutEvent = dlg.m_pdwEventList[0];
	   SetDlgItemText(IDC_EDIT_EVENT, NXCGetEventName(g_hSession, m_dwTimeoutEvent));
   }
}
