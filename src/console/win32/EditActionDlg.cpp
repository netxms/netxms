// EditActionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditActionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_CTRL_COUNT     6

/////////////////////////////////////////////////////////////////////////////
// CEditActionDlg dialog


CEditActionDlg::CEditActionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditActionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditActionDlg)
	m_strData = _T("");
	m_strName = _T("");
	m_strRcpt = _T("");
	m_strSubject = _T("");
	m_iType = -1;
	//}}AFX_DATA_INIT
}


void CEditActionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditActionDlg)
	DDX_Text(pDX, IDC_EDIT_DATA, m_strData);
	DDV_MaxChars(pDX, m_strData, 4095);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_RCPT, m_strRcpt);
	DDV_MaxChars(pDX, m_strRcpt, 255);
	DDX_Text(pDX, IDC_EDIT_SUBJECT, m_strSubject);
	DDV_MaxChars(pDX, m_strSubject, 255);
	DDX_Radio(pDX, IDC_RADIO_EXEC, m_iType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditActionDlg, CDialog)
	//{{AFX_MSG_MAP(CEditActionDlg)
	ON_BN_CLICKED(IDC_RADIO_EMAIL, OnRadioEmail)
	ON_BN_CLICKED(IDC_RADIO_EXEC, OnRadioExec)
	ON_BN_CLICKED(IDC_RADIO_REXEC, OnRadioRexec)
	ON_BN_CLICKED(IDC_RADIO_SMS, OnRadioSms)
	ON_BN_CLICKED(IDC_RADIO_FORWARD, OnRadioForward)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditActionDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CEditActionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   OnTypeChange();	
	return TRUE;
}


//
// Change dialog behavior when action type changes
//

void CEditActionDlg::OnTypeChange()
{
   static int nCtrlList[MAX_CTRL_COUNT] = 
      { IDC_STATIC_RCPT, IDC_EDIT_RCPT, IDC_STATIC_SUBJ,
        IDC_EDIT_SUBJECT, IDC_STATIC_DATA, IDC_EDIT_DATA };
   static BOOL bStateTable[][MAX_CTRL_COUNT] =
   {
      { FALSE, FALSE, FALSE, FALSE, TRUE, TRUE },
      { TRUE, TRUE, FALSE, FALSE, TRUE, TRUE },
      { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE },
      { TRUE, TRUE, FALSE, FALSE, TRUE, TRUE },
      { TRUE, TRUE, FALSE, FALSE, FALSE, FALSE }
   };
   static TCHAR *pszRcptTitles[] = 
   { 
      _T("Recipient address"),
      _T("Remote host"),
      _T("Recipient address"),
      _T("Recipient phone number"),
      _T("Remote NetXMS server")
   };
   static TCHAR *pszDataTitles[] = 
   { 
      _T("Command"),
      _T("Action"),
      _T("Message text"),
      _T("Message text"),
      _T("Message text")
   };
   int i;

   SetDlgItemText(IDC_STATIC_RCPT, pszRcptTitles[m_iType]);
   SetDlgItemText(IDC_STATIC_DATA, pszDataTitles[m_iType]);
   for(i = 0; i < MAX_CTRL_COUNT; i++)
      EnableDlgItem(this, nCtrlList[i], bStateTable[m_iType][i]);
}


//
// Radio button handlers
//

void CEditActionDlg::OnRadioEmail() 
{
   m_iType = ACTION_SEND_EMAIL;
   OnTypeChange();
}

void CEditActionDlg::OnRadioExec() 
{
   m_iType = ACTION_EXEC;
   OnTypeChange();
}

void CEditActionDlg::OnRadioRexec() 
{
   m_iType = ACTION_REMOTE;
   OnTypeChange();
}

void CEditActionDlg::OnRadioSms() 
{
   m_iType = ACTION_SEND_SMS;
   OnTypeChange();
}

void CEditActionDlg::OnRadioForward() 
{
   m_iType = ACTION_FORWARD_EVENT;
   OnTypeChange();
}
