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


//
// Compare certificate names
//

static int CompareCertificates(const void *p1, const void *p2)
{
	return _tcsicmp(((LOGIN_CERTIFICATE *)p1)->szName, ((LOGIN_CERTIFICATE *)p2)->szName);
}


/////////////////////////////////////////////////////////////////////////////
// CLoginDialog dialog


CLoginDialog::CLoginDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDialog::IDD, pParent)
{
   LOGBRUSH lb;
	HCERTSTORE hStore;
	const CERT_CONTEXT *pCert = NULL;

	//{{AFX_DATA_INIT(CLoginDialog)
	m_bClearCache = FALSE;
	m_bMatchVersion = FALSE;
	m_bNoCache = FALSE;
	m_bEncrypt = FALSE;
	m_strServer = _T("");
	m_strLogin = _T("");
	m_strPassword = _T("");
	m_nAuthType = -1;
	m_nCertificateIndex = -1;
	//}}AFX_DATA_INIT

   lb.lbColor = 0;
   lb.lbStyle = BS_NULL;
   lb.lbHatch = 0;
   m_hNullBrush = CreateBrushIndirect(&lb);

   m_dwFlags = 0;
	memset(m_pszServerHistory, 0, sizeof(TCHAR *) * MAX_LOGINDLG_HISTORY_SIZE);

	// Fill certificate list
	m_pCertList = NULL;
	m_dwNumCerts = 0;
	hStore = CertOpenSystemStore(NULL, _T("MY"));
	if (hStore != NULL)
	{
		// Create certificate list
		while((pCert = CertEnumCertificatesInStore(hStore, pCert)) != NULL)
		{
			m_pCertList = (LOGIN_CERTIFICATE *)realloc(m_pCertList, sizeof(LOGIN_CERTIFICATE) * (m_dwNumCerts + 1));
			CertGetNameString(pCert, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0,
									NULL, m_pCertList[m_dwNumCerts].szName, MAX_CERT_NAME);
			m_pCertList[m_dwNumCerts].pCert = CertDuplicateCertificateContext(pCert);
			m_dwNumCerts++;
		}
		CertCloseStore(hStore, 0);

		// Sort certificates by name and fill combo box
		if (m_dwNumCerts > 0)
			qsort(m_pCertList, m_dwNumCerts, sizeof(LOGIN_CERTIFICATE), CompareCertificates);
	}
}

CLoginDialog::~CLoginDialog()
{
	DWORD i;

	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
		safe_free(m_pszServerHistory[i]);
   DeleteObject(m_hNullBrush);

	for(i = 0; i < m_dwNumCerts; i++)
		CertFreeCertificateContext(m_pCertList[i].pCert);
	safe_free(m_pCertList);
}


void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDialog)
	DDX_Control(pDX, IDC_COMBO_CERTS, m_wndComboCerts);
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
	DDX_Radio(pDX, IDC_RADIO_PASSWORD, m_nAuthType);
	DDX_CBIndex(pDX, IDC_COMBO_CERTS, m_nCertificateIndex);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDialog, CDialog)
	//{{AFX_MSG_MAP(CLoginDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CHECK_NOCACHE, OnCheckNocache)
	ON_BN_CLICKED(IDC_RADIO_CERT, OnRadioCert)
	ON_BN_CLICKED(IDC_RADIO_PASSWORD, OnRadioPassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDialog message handlers


//
// WM_INITDIALOG message handler
//

BOOL CLoginDialog::OnInitDialog() 
{
	DWORD i;

	CDialog::OnInitDialog();

   if (m_font.m_hObject == NULL)
	{
		HDC hdc = ::GetDC(m_hWnd);
      m_font.CreateFont(-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72),
                        0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                        VARIABLE_PITCH | FF_DONTCARE, _T("Verdana"));
		::ReleaseDC(m_hWnd, hdc);
	}

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

	if (m_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE)
	{
		::ShowWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD), SW_HIDE);
		SetDlgItemText(IDC_STATIC_PASSWORD, _T("Certificate:"));
	}
	else
	{
		m_wndComboCerts.ShowWindow(SW_HIDE);
	}

	// Fill certificate list
	for(i = 0; i < m_dwNumCerts; i++)
		m_wndComboCerts.AddString(m_pCertList[i].szName);
	m_wndComboCerts.SelectString(-1, m_strLastCertName);

	LoadServerHistory();
	
   if (m_strLogin.IsEmpty() || m_strServer.IsEmpty())
		return TRUE;

   // Server and login already entered, change focus to password field
   ::SetFocus(::GetDlgItem(m_hWnd, (m_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE) ? IDC_COMBO_CERTS : IDC_EDIT_PASSWORD));
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
		_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("HistorySrv_%d"), i);
		dwSize = 256;
		if (RegQueryValueEx(hKey, szBuffer, NULL, NULL, (BYTE *)szData, &dwSize) == ERROR_SUCCESS)
		{
			m_wndComboServer.AddString(szData);
			safe_free(m_pszServerHistory[i]);
			m_pszServerHistory[i] = _tcsdup(szData);
		}
		else
		{
			safe_free_and_null(m_pszServerHistory[i]);
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
			_sntprintf_s(szBuffer, 64, _TRUNCATE, _T("HistorySrv_%d"), i);
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
	TCHAR szBuffer[1024];
	int i;

	// Check if certificate is selected
	if (m_nAuthType == NETXMS_AUTH_TYPE_CERTIFICATE)
	{
		m_wndComboCerts.GetWindowText(szBuffer, 1024);
		if (szBuffer[0] == 0)
		{
			MessageBox(_T("You must select certificate from the list!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	m_wndComboServer.GetWindowText(szBuffer, 256);
	StrStrip(szBuffer);
	for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
		if (m_pszServerHistory[i] != NULL)
			if (!_tcsicmp(szBuffer, m_pszServerHistory[i]))
			{
				free(m_pszServerHistory[i]);
				m_pszServerHistory[i] = _tcsdup(szBuffer);
				break;
			}
	if (i == MAX_LOGINDLG_HISTORY_SIZE)
	{
		// No such entry in history, add it
		for(i = 0; i < MAX_LOGINDLG_HISTORY_SIZE; i++)
			if (m_pszServerHistory[i] == NULL)
			{
				m_pszServerHistory[i] = _tcsdup(szBuffer);
				break;
			}

		if (i == MAX_LOGINDLG_HISTORY_SIZE)
		{
			// No room for new entry
			free(m_pszServerHistory[0]);
			memmove(&m_pszServerHistory[0], &m_pszServerHistory[1], sizeof(TCHAR *) * (MAX_LOGINDLG_HISTORY_SIZE - 1));
			m_pszServerHistory[MAX_LOGINDLG_HISTORY_SIZE - 1] = _tcsdup(szBuffer);
		}
	}
	SaveServerHistory();

	CDialog::OnOK();
}


//
// Switch to certificate authentication
//

void CLoginDialog::OnRadioCert() 
{
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD), SW_HIDE);
	SetDlgItemText(IDC_STATIC_PASSWORD, _T("Certificate:"));
	m_wndComboCerts.ShowWindow(SW_SHOW);
	m_nAuthType = NETXMS_AUTH_TYPE_CERTIFICATE;
}


//
// Switch to password authentication
//

void CLoginDialog::OnRadioPassword() 
{
	m_wndComboCerts.ShowWindow(SW_HIDE);
	SetDlgItemText(IDC_STATIC_PASSWORD, _T("Password:"));
	::ShowWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD), SW_SHOW);
	m_nAuthType = NETXMS_AUTH_TYPE_PASSWORD;
}
