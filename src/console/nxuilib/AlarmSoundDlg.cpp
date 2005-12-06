// AlarmSoundDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "AlarmSoundDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlarmSoundDlg dialog


CAlarmSoundDlg::CAlarmSoundDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAlarmSoundDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAlarmSoundDlg)
	m_iSoundType = -1;
	m_strSound1 = _T("");
	m_strSound2 = _T("");
	m_bIncludeSource = FALSE;
	m_bIncludeSeverity = FALSE;
	m_bIncludeMessage = FALSE;
	m_bNotifyOnAck = FALSE;
	//}}AFX_DATA_INIT
}


void CAlarmSoundDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAlarmSoundDlg)
	DDX_Control(pDX, IDC_BUTTON_PLAY_2, m_wndButtonPlay2);
	DDX_Control(pDX, IDC_BUTTON_PLAY_1, m_wndButtonPlay1);
	DDX_Control(pDX, IDC_COMBO_SOUND2, m_wndCombo2);
	DDX_Control(pDX, IDC_COMBO_SOUND1, m_wndCombo1);
	DDX_Radio(pDX, IDC_RADIO_NO_SOUND, m_iSoundType);
	DDX_CBString(pDX, IDC_COMBO_SOUND1, m_strSound1);
	DDV_MaxChars(pDX, m_strSound1, 255);
	DDX_CBString(pDX, IDC_COMBO_SOUND2, m_strSound2);
	DDV_MaxChars(pDX, m_strSound2, 255);
	DDX_Check(pDX, IDC_CHECK_SOURCE, m_bIncludeSource);
	DDX_Check(pDX, IDC_CHECK_SEVERITY, m_bIncludeSeverity);
	DDX_Check(pDX, IDC_CHECK_MESSAGE, m_bIncludeMessage);
	DDX_Check(pDX, IDC_CHECK_ACK, m_bNotifyOnAck);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAlarmSoundDlg, CDialog)
	//{{AFX_MSG_MAP(CAlarmSoundDlg)
	ON_BN_CLICKED(IDC_BROWSE_1, OnBrowse1)
	ON_BN_CLICKED(IDC_BROWSE_2, OnBrowse2)
	ON_BN_CLICKED(IDC_RADIO_NO_SOUND, OnRadioNoSound)
	ON_BN_CLICKED(IDC_RADIO_SOUND, OnRadioSound)
	ON_BN_CLICKED(IDC_RADIO_SPEECH, OnRadioSpeech)
	ON_BN_CLICKED(IDC_BUTTON_PLAY_1, OnButtonPlay1)
	ON_BN_CLICKED(IDC_BUTTON_PLAY_2, OnButtonPlay2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlarmSoundDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CAlarmSoundDlg::OnInitDialog() 
{
   int i;

	CDialog::OnInitDialog();

   m_wndButtonPlay1.SetIcon(::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PLAY)));
   m_wndButtonPlay2.SetIcon(::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_PLAY)));

   m_wndCombo1.AddString(_T("<none>"));
   m_wndCombo2.AddString(_T("<none>"));
   for(i = 0; g_pszSoundNames[i] != NULL; i++)
   {
      m_wndCombo1.AddString(g_pszSoundNames[i]);
      m_wndCombo2.AddString(g_pszSoundNames[i]);
   }

   EnableItems(m_iSoundType);

	return TRUE;
}


//
// Handlers for browse ("...") buttons
//

void CAlarmSoundDlg::OnBrowse1() 
{
   SelectFile(IDC_COMBO_SOUND1);
}

void CAlarmSoundDlg::OnBrowse2() 
{
   SelectFile(IDC_COMBO_SOUND2);
}


//
// Browse for file and put it name into control
//

void CAlarmSoundDlg::SelectFile(int nCtrl)
{
   CFileDialog dlg(TRUE);

   dlg.m_ofn.Flags |= OFN_FILEMUSTEXIST;
   dlg.m_ofn.lpstrFilter = _T("Sounds (*.wav)\0*.wav\0All Files (*.*)\0*.*\0");
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(nCtrl, dlg.m_ofn.lpstrFile);
   }
}


//
// Handlers for radio buttons
//

void CAlarmSoundDlg::OnRadioNoSound() 
{
   EnableItems(AST_NONE);
}

void CAlarmSoundDlg::OnRadioSound() 
{
   EnableItems(AST_SOUND);
}

void CAlarmSoundDlg::OnRadioSpeech() 
{
   EnableItems(AST_VOICE);
}


//
// Enable or disable dialog box items depending on selected sound type
//

void CAlarmSoundDlg::EnableItems(int nSoundType)
{
   EnableDlgItem(this, IDC_COMBO_SOUND1, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_COMBO_SOUND2, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_BROWSE_1, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_BROWSE_2, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_BUTTON_PLAY_1, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_BUTTON_PLAY_2, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_STATIC_NEW_ALARM, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_STATIC_ALARM_ACK, nSoundType == AST_SOUND);
   EnableDlgItem(this, IDC_CHECK_SOURCE, nSoundType == AST_VOICE);
   EnableDlgItem(this, IDC_CHECK_SEVERITY, nSoundType == AST_VOICE);
   EnableDlgItem(this, IDC_CHECK_MESSAGE, nSoundType == AST_VOICE);
   EnableDlgItem(this, IDC_CHECK_ACK, nSoundType == AST_VOICE);
}


//
// Play selected sound
//

void CAlarmSoundDlg::OnButtonPlay1() 
{
   TCHAR szBuffer[256];

   m_wndCombo1.GetWindowText(szBuffer, 256);
   PlayDemoSound(szBuffer);
}

void CAlarmSoundDlg::OnButtonPlay2() 
{
   TCHAR szBuffer[256];

   m_wndCombo2.GetWindowText(szBuffer, 256);
   PlayDemoSound(szBuffer);
}


//
// PLay sample sound
//

void CAlarmSoundDlg::PlayDemoSound(TCHAR *pszSound)
{
   int i;

   if (_tcsicmp(pszSound, _T("<none>")))
   {
      for(i = 0; g_pszSoundNames[i] != NULL; i++)
         if (!_tcsicmp(pszSound, g_pszSoundNames[i]))
            break;
      if (g_pszSoundNames[i] != NULL)
      {
         PlaySound(MAKEINTRESOURCE(g_nSoundId[i]), g_hInstance,
                   SND_ASYNC | SND_NODEFAULT | SND_RESOURCE);
      }
      else
      {
         PlaySound(pszSound, NULL, SND_ASYNC | SND_NODEFAULT | SND_FILENAME);
      }
   }
}
