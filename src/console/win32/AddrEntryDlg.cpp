// AddrEntryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AddrEntryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddrEntryDlg dialog


CAddrEntryDlg::CAddrEntryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddrEntryDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddrEntryDlg)
	m_nType = -1;
	//}}AFX_DATA_INIT
}


void CAddrEntryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddrEntryDlg)
	DDX_Control(pDX, IDC_IPADDR2, m_wndAddr2);
	DDX_Control(pDX, IDC_IPADDR1, m_wndAddr1);
	DDX_Radio(pDX, IDC_RADIO_SUBNET, m_nType);
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      m_wndAddr1.GetAddress(m_dwAddr1);
      m_wndAddr2.GetAddress(m_dwAddr2);
   }
   else
   {
      m_wndAddr1.SetAddress(m_dwAddr1);
      m_wndAddr2.SetAddress(m_dwAddr2);
   }
}


BEGIN_MESSAGE_MAP(CAddrEntryDlg, CDialog)
	//{{AFX_MSG_MAP(CAddrEntryDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddrEntryDlg message handlers
