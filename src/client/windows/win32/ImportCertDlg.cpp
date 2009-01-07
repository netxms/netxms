// ImportCertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ImportCertDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImportCertDlg dialog


CImportCertDlg::CImportCertDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImportCertDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CImportCertDlg)
	m_strComments = _T("");
	m_strFile = _T("");
	//}}AFX_DATA_INIT
}


void CImportCertDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CImportCertDlg)
	DDX_Text(pDX, IDC_EDIT_COMMENTS, m_strComments);
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CImportCertDlg, CDialog)
	//{{AFX_MSG_MAP(CImportCertDlg)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImportCertDlg message handlers

void CImportCertDlg::OnButtonBrowse() 
{
   TCHAR szBuffer[1024];

   GetDlgItemText(IDC_EDIT_FILE, szBuffer, 1024);
   CFileDialog dlg(TRUE, NULL, szBuffer, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                   _T("Certificate Files (*.cer;*.cert;*.pem)|*.cer;*.cert;*.pem|All Files (*.*)|*.*||"));
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
