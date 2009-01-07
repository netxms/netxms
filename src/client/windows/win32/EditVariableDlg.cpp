// EditVariableDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditVariableDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditVariableDlg dialog


CEditVariableDlg::CEditVariableDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditVariableDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditVariableDlg)
	m_strName = _T("");
	m_strValue = _T("");
	//}}AFX_DATA_INIT
   m_bNewVariable = FALSE;
	m_pszTitle = NULL;
}


void CEditVariableDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditVariableDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	DDX_Text(pDX, IDC_EDIT_VALUE, m_strValue);
	DDV_MaxChars(pDX, m_strValue, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditVariableDlg, CDialog)
	//{{AFX_MSG_MAP(CEditVariableDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditVariableDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CEditVariableDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
   if (!m_bNewVariable)
   {
      SendDlgItemMessage(IDC_EDIT_NAME, EM_SETREADONLY, TRUE, 0);
      ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_VALUE));
   }
	if (m_pszTitle != NULL)
		SetWindowText(m_pszTitle);
	return m_bNewVariable;
}


//
// "OK" button handler
//

void CEditVariableDlg::OnOK() 
{
   if (m_bNewVariable)
   {
      TCHAR szBuffer[MAX_OBJECT_NAME];

      GetDlgItemText(IDC_EDIT_NAME, szBuffer, MAX_OBJECT_NAME);
      StrStrip(szBuffer);
      if (szBuffer[0] == 0)
      {
         MessageBox(_T("Variable name cannot be empty!"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
         return;
      }
   }
	CDialog::OnOK();
}
