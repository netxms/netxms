// SelectMPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SelectMPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectMPDlg dialog


CSelectMPDlg::CSelectMPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectMPDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectMPDlg)
	m_strFile = _T("");
	m_bReplaceEventByName = FALSE;
	m_bReplaceEventByCode = FALSE;
	//}}AFX_DATA_INIT
}


void CSelectMPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectMPDlg)
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFile);
	DDV_MaxChars(pDX, m_strFile, 1023);
	DDX_Check(pDX, IDC_CHECK_REPLACE_BY_NAME, m_bReplaceEventByName);
	DDX_Check(pDX, IDC_CHECK_REPLACE_BY_CODE, m_bReplaceEventByCode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectMPDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectMPDlg)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectMPDlg message handlers

void CSelectMPDlg::OnButtonBrowse() 
{
   TCHAR szBuffer[1024];

   GetDlgItemText(IDC_EDIT_FILE, szBuffer, 1024);
   CFileDialog dlg(TRUE, NULL, szBuffer, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                   _T("NetXMS Management Packs (*.nxmp)|*.nxmp|All Files (*.*)|*.*||"));
   if (dlg.DoModal() == IDOK)
   {
      SetDlgItemText(IDC_EDIT_FILE, dlg.m_ofn.lpstrFile);
   }
}
