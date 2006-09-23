// DCIPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DCIPropPage.h"
#include "MIBBrowserDlg.h"
#include "InternalItemSelDlg.h"
#include "AgentParamSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int g_nCurrentDCIDataType;

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
	m_bAdvSchedule = FALSE;
	//}}AFX_DATA_INIT
   
   m_pNode = NULL;
   m_pParamList = NULL;
   m_dwNumParams = 0;
}

CDCIPropPage::~CDCIPropPage()
{
   safe_free(m_pParamList);
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
	DDX_Check(pDX, IDC_CHECK_SCHEDULE, m_bAdvSchedule);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDCIPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDCIPropPage)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_CBN_SELCHANGE(IDC_COMBO_ORIGIN, OnSelchangeComboOrigin)
	ON_BN_CLICKED(IDC_CHECK_SCHEDULE, OnCheckSchedule)
	ON_CBN_SELCHANGE(IDC_COMBO_DT, OnSelchangeComboDt)
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
   for(i = 0; i < 4; i++)
      m_wndOriginList.AddString(g_pszItemOriginLong[i]);
   m_wndOriginList.SelectString(-1, g_pszItemOriginLong[m_iOrigin]);
	
   // Add data types
   for(i = 0; i < 6; i++)
      m_wndTypeList.AddString(g_pszItemDataType[i]);
   m_wndTypeList.SelectString(-1, g_pszItemDataType[m_iDataType]);
   g_nCurrentDCIDataType = m_iDataType;

   if (m_bAdvSchedule)
      EnablePollingInterval(FALSE);
	
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
         SelectAgentItem();
         break;
      case DS_SNMP_AGENT:
      case DS_CHECKPOINT_AGENT:
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
   for(m_iOrigin = 0; m_iOrigin < 3; m_iOrigin++)
      if (!_tcscmp(szBuffer, g_pszItemOriginLong[m_iOrigin]))
         break;
}


//
// Handler for selection change in data type combo box
//

void CDCIPropPage::OnSelchangeComboDt() 
{
   TCHAR szBuffer[256];

   m_wndTypeList.GetWindowText(szBuffer, 256);
   for(g_nCurrentDCIDataType = 0; g_nCurrentDCIDataType < 6; g_nCurrentDCIDataType++)
      if (!_tcscmp(szBuffer, g_pszItemDataType[g_nCurrentDCIDataType]))
         break;
}


//
// Select SNMP parameter
//

void CDCIPropPage::SelectSNMPItem(void)
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
      dlg.m_dwInstance = _tcstoul(pDot, NULL, 10);
   }
   else
   {
      dlg.m_dwInstance = 0;
   }
   dlg.m_strOID = szBuffer;
   dlg.m_iOrigin = m_iOrigin;
   if (dlg.DoModal() == IDOK)
   {
      _stprintf(szBuffer, _T(".%lu"), dlg.m_dwInstance);
      dlg.m_strOID += szBuffer;
      m_wndEditName.SetWindowText(dlg.m_strOID);
      m_wndTypeList.SelectString(-1, g_pszItemDataType[dlg.m_iDataType]);
      g_nCurrentDCIDataType = dlg.m_iDataType;
      m_wndEditName.SetFocus();
   }
}


//
// Select internal item (like Status)
//

void CDCIPropPage::SelectInternalItem(void)
{
   CInternalItemSelDlg dlg;

   dlg.m_pNode = m_pNode;
   if (dlg.DoModal() == IDOK)
   {
      TCHAR *p;

      m_wndEditName.SetWindowText(dlg.m_szItemName);
      SetDlgItemText(IDC_EDIT_DESCRIPTION, dlg.m_szItemDescription);
      m_wndTypeList.SelectString(-1, g_pszItemDataType[dlg.m_iDataType]);
      g_nCurrentDCIDataType = dlg.m_iDataType;
      m_wndEditName.SetFocus();

      // Replace (*) in parameter's name
      p = _tcschr(dlg.m_szItemName, _T('('));
      if (p != NULL)
      {
         int iPos;

         p++;
         if (*p == _T('*'))
         {
            iPos = (int)((char *)p - (char *)dlg.m_szItemName) / sizeof(TCHAR);
            m_wndEditName.SetSel(iPos, iPos + 1, FALSE);
            m_wndEditName.ReplaceSel(_T("<insert arguments here>"));
            m_wndEditName.SetSel(iPos, iPos + 23, FALSE);
         }
      }
   }
}


//
// Select agent item
//

void CDCIPropPage::SelectAgentItem(void)
{
   DWORD dwResult;

   // Fetch list of items supported by current node from server
   if (m_pParamList == NULL)
   {
      dwResult = DoRequestArg4(NXCGetSupportedParameters, g_hSession, 
                               (void *)m_pNode->dwId, &m_dwNumParams, &m_pParamList,
                               _T("Downloading supported parameters list..."));
      if (dwResult != RCC_SUCCESS)
         theApp.ErrorBox(dwResult, _T("Error retrieving supported parameters list: %s"));
   }
   else
   {
      dwResult = RCC_SUCCESS;
   }

   // If everything is OK, display selection dialog
   if (dwResult == RCC_SUCCESS)
   {
      CAgentParamSelDlg dlg;
      TCHAR *p;

      dlg.m_pNode = m_pNode;
      dlg.m_dwNumParams = m_dwNumParams;
      dlg.m_pParamList = m_pParamList;
      if (dlg.DoModal() == IDOK)
      {
         m_wndEditName.SetWindowText(m_pParamList[dlg.m_dwSelectionIndex].szName);
         SetDlgItemText(IDC_EDIT_DESCRIPTION,
                        m_pParamList[dlg.m_dwSelectionIndex].szDescription);
         m_wndTypeList.SelectString(-1, g_pszItemDataType[m_pParamList[dlg.m_dwSelectionIndex].iDataType]);
         g_nCurrentDCIDataType = m_pParamList[dlg.m_dwSelectionIndex].iDataType;
         m_wndEditName.SetFocus();

         // Replace (*) in parameter's name
         p = _tcschr(m_pParamList[dlg.m_dwSelectionIndex].szName, _T('('));
         if (p != NULL)
         {
            int iPos;

            p++;
            if (*p == _T('*'))
            {
               iPos = (int)((char *)p - (char *)m_pParamList[dlg.m_dwSelectionIndex].szName) / sizeof(TCHAR);
               m_wndEditName.SetSel(iPos, iPos + 1, FALSE);
               m_wndEditName.ReplaceSel(_T("<insert arguments here>"));
               m_wndEditName.SetSel(iPos, iPos + 23, FALSE);
            }
         }
      }
   }
}


//
// Enable or disable polling interval entry field
//

void CDCIPropPage::EnablePollingInterval(BOOL bEnable)
{
   EnableDlgItem(this, IDC_EDIT_INTERVAL, bEnable);
   EnableDlgItem(this, IDC_STATIC_INTERVAL, bEnable);
   EnableDlgItem(this, IDC_STATIC_SECONDS, bEnable);
}


//
// Handle click on "advanced schedule" checkbox
//

void CDCIPropPage::OnCheckSchedule() 
{
   EnablePollingInterval(SendDlgItemMessage(IDC_CHECK_SCHEDULE, BM_GETCHECK) != BST_CHECKED);
}
