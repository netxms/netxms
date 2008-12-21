// NewActionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NewActionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewActionDlg dialog


CNewActionDlg::CNewActionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewActionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewActionDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
}


void CNewActionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewActionDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 63);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewActionDlg, CDialog)
	//{{AFX_MSG_MAP(CNewActionDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewActionDlg message handlers
