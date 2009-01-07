// RuleCommentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RuleCommentDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRuleCommentDlg dialog


CRuleCommentDlg::CRuleCommentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRuleCommentDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRuleCommentDlg)
	m_strText = _T("");
	//}}AFX_DATA_INIT
}


void CRuleCommentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRuleCommentDlg)
	DDX_Text(pDX, IDC_EDIT_TEXT, m_strText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRuleCommentDlg, CDialog)
	//{{AFX_MSG_MAP(CRuleCommentDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRuleCommentDlg message handlers
