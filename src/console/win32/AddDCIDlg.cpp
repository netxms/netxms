// AddDCIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AddDCIDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddDCIDlg dialog


CAddDCIDlg::CAddDCIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddDCIDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddDCIDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddDCIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDCIDlg)
	DDX_Control(pDX, IDC_LIST_NODES, m_wndListNodes);
	DDX_Control(pDX, IDC_LIST_DCI, m_wndListDCI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDCIDlg, CDialog)
	//{{AFX_MSG_MAP(CAddDCIDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddDCIDlg message handlers
