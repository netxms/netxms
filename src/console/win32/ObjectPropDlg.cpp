// ObjectPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropDlg dialog


CObjectPropDlg::CObjectPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObjectPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CObjectPropDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CObjectPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropDlg)
	DDX_Control(pDX, IDC_LIST_VIEW, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropDlg, CDialog)
	//{{AFX_MSG_MAP(CObjectPropDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropDlg message handlers
