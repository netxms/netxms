// FatalErrorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "FatalErrorDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFatalErrorDlg dialog


CFatalErrorDlg::CFatalErrorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFatalErrorDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFatalErrorDlg)
	m_strText = _T("");
	m_strFile = _T("");
	//}}AFX_DATA_INIT
}


void CFatalErrorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFatalErrorDlg)
	DDX_Text(pDX, IDC_EDIT_TEXT, m_strText);
	DDX_Text(pDX, IDC_EDIT_FILE, m_strFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFatalErrorDlg, CDialog)
	//{{AFX_MSG_MAP(CFatalErrorDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFatalErrorDlg message handlers
