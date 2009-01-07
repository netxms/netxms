// NewObjectToolDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "NewObjectToolDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewObjectToolDlg dialog


CNewObjectToolDlg::CNewObjectToolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewObjectToolDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewObjectToolDlg)
	m_strName = _T("");
	m_iToolType = -1;
	//}}AFX_DATA_INIT
}


void CNewObjectToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewObjectToolDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	DDX_Radio(pDX, IDC_RADIO_ACTION, m_iToolType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewObjectToolDlg, CDialog)
	//{{AFX_MSG_MAP(CNewObjectToolDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewObjectToolDlg message handlers
