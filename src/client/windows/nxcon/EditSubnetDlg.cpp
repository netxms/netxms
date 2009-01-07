// EditSubnetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "EditSubnetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditSubnetDlg dialog


CEditSubnetDlg::CEditSubnetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditSubnetDlg::IDD, pParent)
{
   memset(&m_subnet, 0, sizeof(IP_NETWORK));
	//{{AFX_DATA_INIT(CEditSubnetDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEditSubnetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditSubnetDlg)
	DDX_Control(pDX, IDC_IP_NETMASK, m_wndIPNetMask);
	DDX_Control(pDX, IDC_IP_ADDR, m_wndIPAddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditSubnetDlg, CDialog)
	//{{AFX_MSG_MAP(CEditSubnetDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditSubnetDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CEditSubnetDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   if (m_subnet.dwAddr != 0)
      m_wndIPAddr.SetAddress(m_subnet.dwAddr);
   if (m_subnet.dwMask != 0)
      m_wndIPNetMask.SetAddress(m_subnet.dwMask);
	
	return TRUE;
}


//
// "OK" button handler
//

void CEditSubnetDlg::OnOK() 
{
   DWORD i, dwAddr, dwMask, dwTemp;

   m_wndIPAddr.GetAddress(dwAddr);
   m_wndIPNetMask.GetAddress(dwMask);

   for(i = 0, dwTemp = 0xFFFFFFFF; i < 32; i++)
   {
      if (dwMask == dwTemp)
         break;
      dwTemp <<= 1;
   }
   if (i < 32)
   {
      m_subnet.dwAddr = dwAddr;
      m_subnet.dwMask = dwMask;
   	CDialog::OnOK();
   }
	else
   {
      MessageBox(_T("Invalid network mask"), _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
}
