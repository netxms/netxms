// ModifiedAgentCfgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ModifiedAgentCfgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModifiedAgentCfgDlg dialog


CModifiedAgentCfgDlg::CModifiedAgentCfgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModifiedAgentCfgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModifiedAgentCfgDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CModifiedAgentCfgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModifiedAgentCfgDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModifiedAgentCfgDlg, CDialog)
	//{{AFX_MSG_MAP(CModifiedAgentCfgDlg)
	ON_BN_CLICKED(IDC_SAVE, OnSave)
	ON_BN_CLICKED(IDC_DISCARD, OnDiscard)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModifiedAgentCfgDlg message handlers

void CModifiedAgentCfgDlg::OnSave() 
{
   EndDialog(IDC_SAVE);
}

void CModifiedAgentCfgDlg::OnDiscard() 
{
   EndDialog(IDC_DISCARD);
}

void CModifiedAgentCfgDlg::OnApply() 
{
   EndDialog(IDC_APPLY);
}
