// DCIPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIPropPage.h"
#include "MIBBrowserDlg.h"
#include "InternalItemSelDlg.h"

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
   
   m_pNode = NULL;
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
   switch(m_iOrigin)
   {
      case DS_NATIVE_AGENT:
         break;
      case DS_SNMP_AGENT:
         SelectSNMPItem();
         break;
      case DS_INTERNAL:
         SelectInternalItem();
         break;
      default:
         break;
   }
}


//
// Handler for selection change in origin combo box
//

void CDCIPropPage::OnSelchangeComboOrigin() 
{
   TCHAR szBuffer[256];

   m_wndOriginList.GetWindowText(szBuffer, 256);
   for(m_iOrigin = 0; m_iOrigin < 2; m_iOrigin++)
      if (!strcmp(szBuffer, g_pszItemOriginLong[m_iOrigin]))
         break;
}


//
// Select SNMP parameter
//

void CDCIPropPage::SelectSNMPItem()
{
   CMIBBrowserDlg dlg;
   TCHAR *pDot, szBuffer[1024];

   dlg.m_pNode = m_pNode;
   m_wndEditName.GetWindowText(szBuffer, 1024);
   pDot = _tcsrchr(szBuffer, _T('.'));
   if (pDot != NULL)
   {
      *pDot = 0;
      pDot++;
      dlg.m_dwInstance = strtoul(pDot, NULL, 10);
   }
   else
   {
      dlg.m_dwInstance = 0;
   }
   dlg.m_strOID = szBuffer;
   if (dlg.DoModal() == IDOK)
   {
      _stprintf(szBuffer, _T(".%lu"), dlg.m_dwInstance);
      dlg.m_strOID += szBuffer;
      m_wndEditName.SetWindowText(dlg.m_strOID);
      m_wndTypeList.SelectString(-1, g_pszItemDataType[dlg.m_iDataType]);
      m_wndEditName.SetFocus();
   }
}


//
// Select internal item (like Status)
//

void CDCIPropPage::SelectInternalItem()
{
   CInternalItemSelDlg dlg;

   dlg.m_pNode = m_pNode;
   if (dlg.DoModal() == IDOK)
   {
      m_wndEditName.SetWindowText(dlg.m_szItemName);
      SetDlgItemText(IDC_EDIT_DESCRIPTION, dlg.m_szItemDescription);
      m_wndTypeList.SelectString(-1, g_pszItemDataType[dlg.m_iDataType]);
      m_wndEditName.SetFocus();
   }
}
