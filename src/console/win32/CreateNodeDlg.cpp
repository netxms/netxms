// CreateNodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateNodeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateNodeDlg dialog


CCreateNodeDlg::CCreateNodeDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateNodeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateNodeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCreateNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateNodeDlg)
	DDX_Control(pDX, IDC_IP_MASK, m_wndIPMask);
	DDX_Control(pDX, IDC_IP_ADDR, m_wndIPAddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateNodeDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateNodeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateNodeDlg message handlers


//
// "OK" button handler
//

void CCreateNodeDlg::OnOK() 
{
   m_wndIPAddr.GetAddress(m_dwIpAddr);
   m_dwIpAddr = htonl(m_dwIpAddr);
   m_wndIPMask.GetAddress(m_dwNetMask);
   m_dwNetMask = htonl(m_dwNetMask);
	
	CCreateObjectDlg::OnOK();
}
