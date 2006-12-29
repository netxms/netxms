// ConsoleUpgradeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ConsoleUpgradeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConsoleUpgradeDlg dialog


CConsoleUpgradeDlg::CConsoleUpgradeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConsoleUpgradeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConsoleUpgradeDlg)
	m_strURL = _T("");
	//}}AFX_DATA_INIT
}


void CConsoleUpgradeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConsoleUpgradeDlg)
	DDX_Text(pDX, IDC_EDIT_URL, m_strURL);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConsoleUpgradeDlg, CDialog)
	//{{AFX_MSG_MAP(CConsoleUpgradeDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConsoleUpgradeDlg message handlers
