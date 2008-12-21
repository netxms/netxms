// AddrChangeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AddrChangeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddrChangeDlg dialog


CAddrChangeDlg::CAddrChangeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddrChangeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddrChangeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddrChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddrChangeDlg)
	DDX_Control(pDX, IDC_IP_ADDR, m_wndIPAddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddrChangeDlg, CDialog)
	//{{AFX_MSG_MAP(CAddrChangeDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddrChangeDlg message handlers

void CAddrChangeDlg::OnOK() 
{
   int iNumBytes;

   iNumBytes = m_wndIPAddr.GetAddress(m_dwIpAddr);
   if ((iNumBytes != 4) || (m_dwIpAddr == INADDR_NONE) || (m_dwIpAddr == INADDR_ANY))
   {
      MessageBox(_T("Invalid IP address"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
   else
   {
	   CDialog::OnOK();
   }
}
