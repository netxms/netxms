// ProgressDialog.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ProgressDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgressDialog dialog


CProgressDialog::CProgressDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProgressDialog)
	m_szStatusText = _T("");
	//}}AFX_DATA_INIT
}


void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDialog)
	DDX_Text(pDX, IDC_STATIC_TEXT, m_szStatusText);
	DDV_MaxChars(pDX, m_szStatusText, 1024);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	//{{AFX_MSG_MAP(CProgressDialog)
   ON_MESSAGE(WM_CLOSE_STATUS_DLG, OnCloseStatusDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDialog message handlers


//
// WM_CLOSE_STATUS_DLG message handler
//

LRESULT CProgressDialog::OnCloseStatusDlg(WPARAM wParam, LPARAM lParam)
{
   EndDialog(lParam);
   return 0;
}
