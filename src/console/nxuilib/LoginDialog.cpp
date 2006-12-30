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
	m_bClearCache = FALSE;
	m_bMatchVersion = FALSE;
	m_bNoCache = FALSE;
	m_bEncrypt = FALSE;
	m_strServer = _T("");
	m_strLogin = _T("");
	m_strPassword = _T("");
	//}}AFX_DATA_INIT

   lb.lbColor = 0;
   lb.lbStyle = BS_NULL;
   lb.lbHatch = 0;
   m_hNullBrush = CreateBrushIndirect(&lb);

   m_dwFlags = 0;
	memset(m_pszServerHistory, 0, sizeof(TCHAR *) * MAX_LOGINDLG_HISTORY_SIZE);
}

CLoginDialog::~CLoginDialog()
{
	int i;

	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
		safe_free(m_pszServerHistory[i]);
   DeleteObject(m_hNullBrush);
}


void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDialog)
	DDX_Control(pDX, IDC_COMBO_SERVER, m_wndComboServer);
	DDX_Check(pDX, IDC_CHECK_CACHE, m_bClearCache);
	DDX_Check(pDX, IDC_CHECK_VERSION_MATCH, m_bMatchVersion);
	DDX_Check(pDX, IDC_CHECK_NOCACHE, m_bNoCache);
	DDX_Check(pDX, IDC_CHECK_ENCRYPT, m_bEncrypt);
	DDX_CBString(pDX, IDC_COMBO_SERVER, m_strServer);
	DDV_MaxChars(pDX, m_strServer, 63);
	DDX_Text(pDX, IDC_EDIT_LOGIN, m_strLogin);
	DDV_MaxChars(pDX, m_strLogin, 63);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 63);
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

	LoadServerHistory();
	
   if (m_strLogin.IsEmpty() || m_strServer.IsEmpty())
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


//
// Load history of connected servers
//

void CLoginDialog::LoadServerHistory()
{
	HKEY hKey;
	DWORD i, dwSize;
	TCHAR szBuffer[64], szData[256];

	if (RegOpenKeyEx(HKEY_CURRENT_USER, NXUILIB_BASE_REGISTRY_KEY _T("LoginDialog"),
	                 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return;

	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
	{
		_stprintf(szBuffer, _T("HistorySrv_%d"), i);
		dwSize = 256;
		if (RegQueryValueEx(hKey, szBuffer, NULL, NULL, (BYTE *)szData, &dwSize) == ERROR_SUCCESS)
		{
			m_wndComboServer.AddString(szData);
			m_pszServerHistory[i] = _tcsdup(szData);
		}
	}

	RegCloseKey(hKey);
}


//
// Save history of connected servers
//

void CLoginDialog::SaveServerHistory()
{
	HKEY hKey;
	DWORD i;
	TCHAR szBuffer[64];

	if (RegCreateKeyEx(HKEY_CURRENT_USER, NXUILIB_BASE_REGISTRY_KEY _T("LoginDialog"),
	                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
							 NULL, &hKey, NULL) != ERROR_SUCCESS)
		return;

	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
	{
		if (m_pszServerHistory[i] != NULL)
		{
			_stprintf(szBuffer, _T("HistorySrv_%d"), i);
			RegSetValueEx(hKey, szBuffer, 0, REG_SZ, (BYTE *)m_pszServerHistory[i],
			              (_tcslen(m_pszServerHistory[i]) + 1) * sizeof(TCHAR));
		}
	}

	RegCloseKey(hKey);
}


//
// OK button handler
//

void CLoginDialog::OnOK() 
{
	TCHAR szServer[256];
	int i;

	m_wndComboServer.GetWindowText(szServer, 256);
	StrStrip(szServer);
	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
		if (m_pszServerHistory[i] != NULL)
			if (!_tcsicmp(szServer, m_pszServerHistory[i]))
			{
				free(m_pszServerHistory[i]);
				m_pszServerHistory[i] = _tcsdup(szServer);
				break;
			}
	if (i == MAX_LOGINDLG_HISTORY_SIZE)
	{
		// No such entry in history, add it
		for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
			if (m_pszServerHistory[i] == NULL)
			{
				m_pszServerHistory[i] = _tcsdup(szServer);
				break;
			}

		if (i == MAX_LOGINDLG_HISTORY_SIZE)
		{
			// No room for new entry
			free(m_pszServerHistory[0]);
			memmove(&m_pszServerHistory[0], &m_pszServerHistory[1], sizeof(TCHAR *) * (MAX_LOGINDLG_HISTORY_SIZE - 1));
			m_pszServerHistory[MAX_LOGINDLG_HISTORY_SIZE - 1] = _tcsdup(szServer);
		}
	}
	SaveServerHistory();

	CDialog::OnOK();
}
