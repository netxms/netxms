// ConnectionPage.cpp : implementation file
//

#include "stdafx.h"
#include "nxnotify.h"
#include "ConnectionPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionPage property page

IMPLEMENT_DYNCREATE(CConnectionPage, CPropertyPage)

CConnectionPage::CConnectionPage() : CPropertyPage(CConnectionPage::IDD)
{
	//{{AFX_DATA_INIT(CConnectionPage)
	m_bAutoLogin = FALSE;
	m_bEncrypt = FALSE;
	m_strLogin = _T("");
	m_strPassword = _T("");
	m_strServer = _T("");
	//}}AFX_DATA_INIT
}

CConnectionPage::~CConnectionPage()
{
}

void CConnectionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectionPage)
	DDX_Check(pDX, IDC_CHECK_AUTOLOGIN, m_bAutoLogin);
	DDX_Check(pDX, IDC_ENCRYPT, m_bEncrypt);
	DDX_Text(pDX, IDC_LOGIN, m_strLogin);
	DDV_MaxChars(pDX, m_strLogin, 255);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 255);
	DDX_Text(pDX, IDC_SERVER, m_strServer);
	DDV_MaxChars(pDX, m_strServer, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CConnectionPage)
	ON_BN_CLICKED(IDC_CHECK_AUTOLOGIN, OnCheckAutologin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionPage message handlers

//
// WM_INITDIALOG message handlers
//

BOOL CConnectionPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   SetItemState();
	return TRUE;
}


//
// Set state of dialog items
//

void CConnectionPage::SetItemState()
{
   BOOL bEnable;

   bEnable = (SendDlgItemMessage(IDC_CHECK_AUTOLOGIN, BM_GETCHECK) == BST_CHECKED);
   EnableDlgItem(this, IDC_SERVER, bEnable);
   EnableDlgItem(this, IDC_LOGIN, bEnable);
   EnableDlgItem(this, IDC_PASSWORD, bEnable);
   EnableDlgItem(this, IDC_ENCRYPT, bEnable);
}


//
// Handler for checking/unchecking of "auto login" checkbox
//

void CConnectionPage::OnCheckAutologin() 
{
   SetItemState();
}
