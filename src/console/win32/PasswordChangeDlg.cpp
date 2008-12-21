// PasswordChangeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "PasswordChangeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasswordChangeDlg dialog


CPasswordChangeDlg::CPasswordChangeDlg(int nTemplate, CWnd* pParent /*=NULL*/)
	: CDialog(nTemplate, pParent)
{
	//{{AFX_DATA_INIT(CPasswordChangeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPasswordChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasswordChangeDlg)
	DDX_Control(pDX, IDC_EDIT_PASSWD2, m_wndEditBox2);
	DDX_Control(pDX, IDC_EDIT_PASSWD1, m_wndEditBox1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasswordChangeDlg, CDialog)
	//{{AFX_MSG_MAP(CPasswordChangeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasswordChangeDlg message handlers

void CPasswordChangeDlg::OnOK() 
{
   TCHAR szPasswd1[MAX_PASSWORD_LENGTH], szPasswd2[MAX_PASSWORD_LENGTH];

   // Compare if two passwords match
   m_wndEditBox1.GetWindowText(szPasswd1, MAX_PASSWORD_LENGTH);
   m_wndEditBox2.GetWindowText(szPasswd2, MAX_PASSWORD_LENGTH);
   if (_tcscmp(szPasswd1, szPasswd2))
   {
      MessageBox(_T("The password was not correctly confirmed. ")
                 _T("Please ensure that password and confirmation match exactly."),
                 _T("Warning"), MB_ICONEXCLAMATION | MB_OK);
   }
   else
   {
      _tcscpy(m_szPassword, szPasswd1);
	   CDialog::OnOK();
   }
}
