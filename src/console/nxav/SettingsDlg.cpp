// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxav.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg dialog


CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSettingsDlg)
	m_bAutoLogin = FALSE;
	m_strPassword = _T("");
	m_strServer = _T("");
	m_strUser = _T("");
	m_bRepeatSound = FALSE;
	//}}AFX_DATA_INIT
   memset(&m_soundCfg, 0, sizeof(ALARM_SOUND_CFG));
}


void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettingsDlg)
	DDX_Check(pDX, IDC_CHECK_AUTOLOGIN, m_bAutoLogin);
	DDX_Text(pDX, IDC_EDIT_PASSWD, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 63);
	DDX_Text(pDX, IDC_EDIT_SERVER_NAME, m_strServer);
	DDV_MaxChars(pDX, m_strServer, 63);
	DDX_Text(pDX, IDC_EDIT_USER, m_strUser);
	DDV_MaxChars(pDX, m_strUser, 63);
	DDX_Check(pDX, IDC_CHECK_REPEAT, m_bRepeatSound);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CSettingsDlg)
	ON_BN_CLICKED(IDC_CHECK_AUTOLOGIN, OnCheckAutologin)
	ON_BN_CLICKED(IDC_CONFIGURE_SOUNDS, OnConfigureSounds)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg message handlers

BOOL CSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   EnableControls();
	return TRUE;
}

void CSettingsDlg::OnCheckAutologin() 
{
   EnableControls();
}

void CSettingsDlg::EnableControls()
{
   BOOL bEnable;

   bEnable = (SendDlgItemMessage(IDC_CHECK_AUTOLOGIN, BM_GETCHECK) == BST_CHECKED);
   EnableDlgItem(this, IDC_STATIC_SERVER, bEnable);
   EnableDlgItem(this, IDC_STATIC_USER, bEnable);
   EnableDlgItem(this, IDC_STATIC_PASSWORD, bEnable);
   EnableDlgItem(this, IDC_EDIT_SERVER_NAME, bEnable);
   EnableDlgItem(this, IDC_EDIT_USER, bEnable);
   EnableDlgItem(this, IDC_EDIT_PASSWD, bEnable);
}


//
// Handler for "Configure sounds..." button
//

void CSettingsDlg::OnConfigureSounds() 
{
   ConfigureAlarmSounds(&m_soundCfg);
}
