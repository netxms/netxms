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
	//}}AFX_DATA_INIT
}


void CCreateNodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateNodeDlg)
	DDX_Control(pDX, IDC_EDIT_NAME, m_wndObjectName);
	DDX_Control(pDX, IDC_IP_ADDR, m_wndIPAddr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateNodeDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateNodeDlg)
	ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnButtonResolve)
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
	CCreateObjectDlg::OnOK();
}


//
// "Resolve" button handler
//

void CCreateNodeDlg::OnButtonResolve() 
{
   TCHAR szHostName[256];
   struct hostent *hs;
   DWORD dwAddr = INADDR_NONE;

   m_wndObjectName.GetWindowText(szHostName, 256);
   hs = gethostbyname(szHostName);
   if (hs != NULL)
   {
      memcpy(&dwAddr, hs->h_addr, sizeof(DWORD));
   }
   else
   {
      dwAddr = inet_addr(szHostName);
   }

   if (dwAddr != INADDR_NONE)
   {
      m_wndIPAddr.SetAddress(ntohl(dwAddr));
   }
   else
   {
      MessageBox(_T("Unable to resolve host name!"), _T("Error"), MB_OK | MB_ICONSTOP);
   }
}
