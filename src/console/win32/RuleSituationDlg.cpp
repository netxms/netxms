// RuleSituationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleSituationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleSituationDlg dialog


CRuleSituationDlg::CRuleSituationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleSituationDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleSituationDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CRuleSituationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleSituationDlg)
	DDX_Control(pDX, IDC_LIST_ATTRIBUTES, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleSituationDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleSituationDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleSituationDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CRuleSituationDlg::OnInitDialog() 
{
	RECT rect;

	CDialog::OnInitDialog();

	// Setup attribute list
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
	m_wndListCtrl.InsertColumn(1, _T("Value"), LVCFMT_LEFT, rect.right - 100 - GetSystemMetrics(SM_CXVSCROLL));

	return TRUE;
}
