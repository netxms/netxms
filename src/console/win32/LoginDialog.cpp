// LoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
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
	//{{AFX_DATA_INIT(CLoginDialog)
	m_szLogin = _T("");
	m_szPassword = _T("");
	m_szServer = _T("");
	//}}AFX_DATA_INIT
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
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDialog, CDialog)
	//{{AFX_MSG_MAP(CLoginDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDialog message handlers
