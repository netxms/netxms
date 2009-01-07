// RemoveTemplateDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "RemoveTemplateDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRemoveTemplateDlg dialog


CRemoveTemplateDlg::CRemoveTemplateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRemoveTemplateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRemoveTemplateDlg)
	m_iRemoveDCI = -1;
	//}}AFX_DATA_INIT
}


void CRemoveTemplateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRemoveTemplateDlg)
	DDX_Radio(pDX, IDC_RADIO_UNBIND, m_iRemoveDCI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoveTemplateDlg, CDialog)
	//{{AFX_MSG_MAP(CRemoveTemplateDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRemoveTemplateDlg message handlers
