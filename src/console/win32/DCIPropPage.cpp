// DCIPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIPropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDCIPropPage dialog

IMPLEMENT_DYNCREATE(CDCIPropPage, CPropertyPage)

CDCIPropPage::CDCIPropPage()
	: CPropertyPage(CDCIPropPage::IDD)
{
	//{{AFX_DATA_INIT(CDCIPropPage)
	m_iPollingInterval = 0;
	m_iRetentionTime = 0;
	m_strName = _T("");
	m_iDataType = -1;
	m_iOrigin = -1;
	m_iStatus = -1;
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}


void CDCIPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDCIPropPage)
	DDX_Control(pDX, IDC_EDIT_NAME, m_wndEditName);
	DDX_Control(pDX, IDC_COMBO_ORIGIN, m_wndOriginList);
	DDX_Control(pDX, IDC_COMBO_DT, m_wndTypeList);
	DDX_Control(pDX, IDC_BUTTON_SELECT, m_wndSelectButton);
	DDX_Text(pDX, IDC_EDIT_INTERVAL, m_iPollingInterval);
	DDV_MinMaxInt(pDX, m_iPollingInterval, 5, 100000);
	DDX_Text(pDX, IDC_EDIT_RETENTION, m_iRetentionTime);
	DDV_MinMaxInt(pDX, m_iRetentionTime, 1, 100000);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	DDX_CBIndex(pDX, IDC_COMBO_DT, m_iDataType);
	DDX_CBIndex(pDX, IDC_COMBO_ORIGIN, m_iOrigin);
	DDX_Radio(pDX, IDC_RADIO_ACTIVE, m_iStatus);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDV_MaxChars(pDX, m_strDescription, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCIPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCIPropPage)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_CBN_SELCHANGE(IDC_COMBO_ORIGIN, OnSelchangeComboOrigin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCIPropPage message handlers


//
// WM_INITDIALOG message handler
//

BOOL CDCIPropPage::OnInitDialog() 
{
   DWORD i;

	CPropertyPage::OnInitDialog();

   // Add origins
   for(i = 0; i < 3; i++)
      m_wndOriginList.AddString(g_pszItemOriginLong[i]);
   m_wndOriginList.SelectString(-1, g_pszItemOriginLong[m_iOrigin]);
   m_wndSelectButton.EnableWindow(m_iOrigin == DS_SNMP_AGENT);
	
   // Add data types
   for(i = 0; i < 6; i++)
      m_wndTypeList.AddString(g_pszItemDataType[i]);
   m_wndTypeList.SelectString(-1, g_pszItemDataType[m_iDataType]);
	
	return TRUE;
}


//
// Select MIB variable from tree
//

void CDCIPropPage::OnButtonSelect() 
{
   CMIBBrowserDlg dlg;

   m_wndEditName.GetWindowText(dlg.m_strOID);
   if (dlg.DoModal() == IDOK)
      m_wndEditName.SetWindowText(dlg.m_strOID);
}


//
// Handler for selection change in origin combo box
//

void CDCIPropPage::OnSelchangeComboOrigin() 
{
   char szBuffer[256];

   m_wndOriginList.GetWindowText(szBuffer, 256);
   m_wndSelectButton.EnableWindow(!strcmp(szBuffer, g_pszItemOriginLong[DS_SNMP_AGENT]));
}
