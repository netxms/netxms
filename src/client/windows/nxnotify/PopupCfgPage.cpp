// PopupCfgPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxnotify.h"
#include "PopupCfgPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR *m_pszActions[] =
{
   _T("Acknowledge alarm"),
   _T("Dismiss popup"),
   _T("Open alarm list"),
   _T("Open NetXMS Console"),
   _T("Take no actions"),
   NULL
};

/////////////////////////////////////////////////////////////////////////////
// CPopupCfgPage property page

IMPLEMENT_DYNCREATE(CPopupCfgPage, CPropertyPage)

CPopupCfgPage::CPopupCfgPage() : CPropertyPage(CPopupCfgPage::IDD)
{
	//{{AFX_DATA_INIT(CPopupCfgPage)
	m_nTimeout = 0;
	//}}AFX_DATA_INIT

   m_nActionLeft = NXNOTIFY_ACTION_DISMISS;
   m_nActionRight = NXNOTIFY_ACTION_NONE;
   m_nActionDblClk = NXNOTIFY_ACTION_OPEN_LIST;
}

CPopupCfgPage::~CPopupCfgPage()
{
}

void CPopupCfgPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPopupCfgPage)
	DDX_Control(pDX, IDC_COMBO_RIGHT, m_wndComboRight);
	DDX_Control(pDX, IDC_COMBO_LEFT, m_wndComboLeft);
	DDX_Control(pDX, IDC_COMBO_DBLCLK, m_wndComboDblClk);
	DDX_Text(pDX, IDC_EDIT_TIMEOUT, m_nTimeout);
	DDV_MinMaxInt(pDX, m_nTimeout, 1, 1000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPopupCfgPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPopupCfgPage)
	ON_BN_CLICKED(IDC_CHANGE_SOUND, OnChangeSound)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPopupCfgPage message handlers

//
// WM_INITDIALOG message handler
//

BOOL CPopupCfgPage::OnInitDialog() 
{
   int i;

	CPropertyPage::OnInitDialog();

   for(i = 0; m_pszActions[i] != NULL; i++)
   {
      m_wndComboLeft.AddString(m_pszActions[i]);
      m_wndComboRight.AddString(m_pszActions[i]);
      m_wndComboDblClk.AddString(m_pszActions[i]);
   }
   m_wndComboLeft.SelectString(-1, m_pszActions[m_nActionLeft]);
   m_wndComboRight.SelectString(-1, m_pszActions[m_nActionRight]);
   m_wndComboDblClk.SelectString(-1, m_pszActions[m_nActionDblClk]);
	
	return TRUE;
}


//
// Handler for "Configure sound..." button
//

void CPopupCfgPage::OnChangeSound() 
{
   ConfigureAlarmSounds(&m_soundCfg);
}


//
// OK button handler
//

void CPopupCfgPage::OnOK() 
{
   int i;
   TCHAR szText[256];

	CPropertyPage::OnOK();

   m_nActionLeft = NXNOTIFY_ACTION_NONE;
   m_nActionRight = NXNOTIFY_ACTION_NONE;
   m_nActionDblClk = NXNOTIFY_ACTION_NONE;
   for(i = 0; m_pszActions[i] != NULL; i++)
   {
      m_wndComboLeft.GetWindowText(szText, 256);
      if (!_tcscmp(szText, m_pszActions[i]))
         m_nActionLeft = i;

      m_wndComboRight.GetWindowText(szText, 256);
      if (!_tcscmp(szText, m_pszActions[i]))
         m_nActionRight = i;

      m_wndComboDblClk.GetWindowText(szText, 256);
      if (!_tcscmp(szText, m_pszActions[i]))
         m_nActionDblClk = i;
   }
}
