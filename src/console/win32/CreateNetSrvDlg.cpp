// CreateNetSrvDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "CreateNetSrvDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCreateNetSrvDlg dialog


CCreateNetSrvDlg::CCreateNetSrvDlg(CWnd* pParent /*=NULL*/)
	: CCreateObjectDlg(CCreateNetSrvDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateNetSrvDlg)
	m_iPort = 0;
	m_iProtocolType = -1;
	m_iProtocolNumber = 0;
	m_strRequest = _T("");
	m_strResponce = _T("");
	//}}AFX_DATA_INIT
}


void CCreateNetSrvDlg::DoDataExchange(CDataExchange* pDX)
{
	CCreateObjectDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateNetSrvDlg)
	DDX_Control(pDX, IDC_COMBO_TYPES, m_wndTypeList);
	DDX_Text(pDX, IDC_EDIT_PORT, m_iPort);
	DDV_MinMaxLong(pDX, m_iPort, 0, 65535);
	DDX_Radio(pDX, IDC_RADIO_TCP, m_iProtocolType);
	DDX_Text(pDX, IDC_EDIT_PROTO, m_iProtocolNumber);
	DDV_MinMaxLong(pDX, m_iProtocolNumber, 0, 255);
	DDX_Text(pDX, IDC_EDIT_REQUEST, m_strRequest);
	DDX_Text(pDX, IDC_EDIT_RESPONCE, m_strResponce);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateNetSrvDlg, CCreateObjectDlg)
	//{{AFX_MSG_MAP(CCreateNetSrvDlg)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPES, OnSelchangeComboTypes)
	ON_BN_CLICKED(IDC_RADIO_TCP, OnRadioTcp)
	ON_BN_CLICKED(IDC_RADIO_UDP, OnRadioUdp)
	ON_BN_CLICKED(IDC_RADIO_ICMP, OnRadioIcmp)
	ON_BN_CLICKED(IDC_RADIO_OTHER, OnRadioOther)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateNetSrvDlg message handlers

BOOL CCreateNetSrvDlg::OnInitDialog() 
{
   int i;

	CCreateObjectDlg::OnInitDialog();

   // Add service types to combo box
   for(i = 0; g_szServiceType[i] != NULL; i++)
      m_wndTypeList.AddString(g_szServiceType[i]);
   m_wndTypeList.SelectString(-1, g_szServiceType[m_iServiceType]);

   SetProtocolCtrls();
	
	return TRUE;
}


//
// Handler for OK button
//

void CCreateNetSrvDlg::OnOK() 
{
   GetServiceType();	
	CCreateObjectDlg::OnOK();
}


//
// Handler for selection change in service type list
//

void CCreateNetSrvDlg::OnSelchangeComboTypes() 
{
   GetServiceType();
   if (m_iServiceType != NETSRV_CUSTOM)
   {
      SetDlgItemText(IDC_EDIT_PROTO, _T("6"));
      SendDlgItemMessage(IDC_RADIO_TCP, BM_SETCHECK, BST_CHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_UDP, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_ICMP, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(IDC_RADIO_OTHER, BM_SETCHECK, BST_UNCHECKED, 0);
   }
   SetProtocolCtrls();
}


//
// Set states of IP protocol selection controls
//

void CCreateNetSrvDlg::SetProtocolCtrls()
{
   BOOL bEnable;

   bEnable = (m_iServiceType == NETSRV_CUSTOM) ? TRUE : FALSE;
   EnableDlgItem(this, IDC_RADIO_TCP, bEnable);
   EnableDlgItem(this, IDC_RADIO_UDP, bEnable);
   EnableDlgItem(this, IDC_RADIO_ICMP, bEnable);
   EnableDlgItem(this, IDC_RADIO_OTHER, bEnable);
   EnableDlgItem(this, IDC_EDIT_PROTO, 
                 SendDlgItemMessage(IDC_RADIO_OTHER, BM_GETCHECK) == BST_CHECKED);
}


//
// Get selected service type from combo box
//

void CCreateNetSrvDlg::GetServiceType()
{
   TCHAR szBuffer[256];

   // Determine service type
   m_wndTypeList.GetWindowText(szBuffer, 256);
   for(m_iServiceType = 0; g_szServiceType[m_iServiceType] != NULL; m_iServiceType++)
      if (!_tcscmp(szBuffer, g_szServiceType[m_iServiceType]))
         break;
}


//
// Handle radio button clicks
//

void CCreateNetSrvDlg::OnRadioTcp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("6"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
}

void CCreateNetSrvDlg::OnRadioUdp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("17"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
}

void CCreateNetSrvDlg::OnRadioIcmp() 
{
   SetDlgItemText(IDC_EDIT_PROTO, _T("1"));
   EnableDlgItem(this, IDC_EDIT_PROTO, FALSE);
}

void CCreateNetSrvDlg::OnRadioOther() 
{
   EnableDlgItem(this, IDC_EDIT_PROTO, TRUE);
}
