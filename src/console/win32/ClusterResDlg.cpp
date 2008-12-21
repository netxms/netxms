// ClusterResDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ClusterResDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterResDlg dialog


CClusterResDlg::CClusterResDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClusterResDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClusterResDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_dwIpAddr = 0;
}


void CClusterResDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClusterResDlg)
	DDX_Control(pDX, IDC_IPADDR, m_wndIpAddr);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CClusterResDlg, CDialog)
	//{{AFX_MSG_MAP(CClusterResDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClusterResDlg message handlers


//
// WM_INITDIALOG
//

BOOL CClusterResDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if (m_dwIpAddr != 0)
		m_wndIpAddr.SetAddress(m_dwIpAddr);

	return TRUE;
}

void CClusterResDlg::OnOK() 
{
	m_wndIpAddr.GetAddress(m_dwIpAddr);
	CDialog::OnOK();
}
