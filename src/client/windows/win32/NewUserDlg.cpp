// NewUserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NewUserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewUserDlg dialog


CNewUserDlg::CNewUserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewUserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewUserDlg)
	m_bDefineProperties = FALSE;
	m_strName = _T("");
	m_strHeader = _T("");
	//}}AFX_DATA_INIT
}


void CNewUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewUserDlg)
	DDX_Check(pDX, IDC_CHECK_PROPERTIES, m_bDefineProperties);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_STATIC_HEADER, m_strHeader);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewUserDlg, CDialog)
	//{{AFX_MSG_MAP(CNewUserDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewUserDlg message handlers

BOOL CNewUserDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   SetWindowText((LPCTSTR)m_strTitle);

	return TRUE;
}
