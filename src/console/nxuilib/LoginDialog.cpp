// LoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "nxuilib.h"
#include "LoginDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginDialog dialog


CLoginDialog::CLoginDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDialog::IDD, pParent)
{
   LOGBRUSH lb;

	//{{AFX_DATA_INIT(CLoginDialog)
	m_szLogin = _T("");
	m_szPassword = _T("");
	m_szServer = _T("");
	m_bClearCache = FALSE;
	m_bMatchVersion = FALSE;
	m_bNoCache = FALSE;
	m_bEncrypt = FALSE;
	//}}AFX_DATA_INIT

   lb.lbColor = 0;
   lb.lbStyle = BS_NULL;
   lb.lbHatch = 0;
   m_hNullBrush = CreateBrushIndirect(&lb);

   m_dwFlags = 0;
}

CLoginDialog::~CLoginDialog()
{
   DeleteObject(m_hNullBrush);
}


void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDialog)
	DDX_Text(pDX, IDC_EDIT_LOGIN, m_szLogin);
	DDV_MaxChars(pDX, m_szLogin, 64);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_szPassword);
	DDV_MaxChars(pDX, m_szPassword, 64);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_szServer);
	DDV_MaxChars(pDX, m_szServer, 64);
	DDX_Check(pDX, IDC_CHECK_CACHE, m_bClearCache);
	DDX_Check(pDX, IDC_CHECK_VERSION_MATCH, m_bMatchVersion);
	DDX_Check(pDX, IDC_CHECK_NOCACHE, m_bNoCache);
	DDX_Check(pDX, IDC_CHECK_ENCRYPT, m_bEncrypt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDialog, CDialog)
	//{{AFX_MSG_MAP(CLoginDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CHECK_NOCACHE, OnCheckNocache)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDialog message handlers


//
// WM_INITDIALOG message handler
//

BOOL CLoginDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

   if (m_font.m_hObject == NULL)
      m_font.CreateFont(-MulDiv(8, GetDeviceCaps(GetDC()->m_hDC, LOGPIXELSY), 72),
                        0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                        VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));

   SetDlgItemText(IDC_STATIC_VERSION, _T("Ver. ") NETXMS_VERSION_STRING);

   if (m_dwFlags & LOGIN_DLG_NO_OBJECT_CACHE)
   {
      EnableDlgItem(this, IDC_CHECK_CACHE, FALSE);
      EnableDlgItem(this, IDC_CHECK_NOCACHE, FALSE);
   }
   if (m_dwFlags & LOGIN_DLG_NO_ENCRYPTION)
   {
      EnableDlgItem(this, IDC_CHECK_ENCRYPT, FALSE);
   }
   if (m_bNoCache)
   {
      EnableDlgItem(this, IDC_CHECK_CACHE, FALSE);
   }
	
   if (m_szLogin.IsEmpty() || m_szServer.IsEmpty())
		return TRUE;

   // Server and login already entered, change focus to password field
   ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD));
   return FALSE;
}


//
// WM_CTLCOLOR message handler
//

HBRUSH CLoginDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   if (pWnd->GetDlgCtrlID() == IDC_STATIC_VERSION)
   {
      pDC->SelectObject(&m_font);
      pDC->SetTextColor(RGB(255, 255, 255));
      pDC->SetBkColor(RGB(43, 60, 142));
      //pDC->SetBkMode(TRANSPARENT);
      return m_hNullBrush;
   }
   else
   {
	   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	   
	   return hbr;
   }
}


//
// Handler for "Don't cache this session" selection
//

void CLoginDialog::OnCheckNocache() 
{
   BOOL bNoCache;

   bNoCache = (SendDlgItemMessage(IDC_CHECK_NOCACHE, BM_GETCHECK) == BST_CHECKED);
   if (bNoCache)
   {
      SendDlgItemMessage(IDC_CHECK_CACHE, BM_SETCHECK, BST_CHECKED);
      EnableDlgItem(this, IDC_CHECK_CACHE, FALSE);
   }
   else
   {
      EnableDlgItem(this, IDC_CHECK_CACHE, TRUE);
   }
}
