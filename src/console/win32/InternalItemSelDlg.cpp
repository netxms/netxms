// InternalItemSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "InternalItemSelDlg.h"
#include "DataQueryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Static data
//

static struct __item_info
{
   DWORD dwMatchFlags;
   int iDataType;
   TCHAR *pszName;
   TCHAR *pszDescription;
} m_itemList[] =
{
   { NF_IS_LOCAL_MGMT, DCI_DT_FLOAT, _T("Server.AverageConfigurationPollerQueueSize"), _T("Average length of configuration poller queue for last minute") },
   { NF_IS_LOCAL_MGMT, DCI_DT_FLOAT, _T("Server.AverageDBWriterQueueSize"), _T("Average length of database writer's request queue for last minute") },
   { NF_IS_LOCAL_MGMT, DCI_DT_UINT, _T("Server.AverageDCIQueuingTime"), _T("Average time to queue DCI for polling for last minute") },
   { NF_IS_LOCAL_MGMT, DCI_DT_FLOAT, _T("Server.AverageDCPollerQueueSize"), _T("Average length of data collection poller's request queue for last minute") },
   { NF_IS_LOCAL_MGMT, DCI_DT_FLOAT, _T("Server.AverageStatusPollerQueueSize"), _T("Average length of status poller queue for last minute") },
   { 0, DCI_DT_INT, _T("Status"), _T("Status") },
   { 0, 0, NULL, NULL }
};


/////////////////////////////////////////////////////////////////////////////
// CInternalItemSelDlg dialog


CInternalItemSelDlg::CInternalItemSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInternalItemSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInternalItemSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CInternalItemSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInternalItemSelDlg)
	DDX_Control(pDX, IDC_LIST_PARAMETERS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInternalItemSelDlg, CDialog)
	//{{AFX_MSG_MAP(CInternalItemSelDlg)
	ON_BN_CLICKED(IDC_BUTTON_GET, OnButtonGet)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_PARAMETERS, OnDblclkListParameters)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInternalItemSelDlg message handlers

BOOL CInternalItemSelDlg::OnInitDialog() 
{
   DWORD i;
   int iItem;
   RECT rect;

	CDialog::OnInitDialog();

   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Parameter name"), LVCFMT_LEFT, 220);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT,
                              rect.right - 300 - GetSystemMetrics(SM_CXVSCROLL));
	
   for(i = 0; m_itemList[i].pszName != NULL; i++)
      if ((m_pNode->node.dwFlags & m_itemList[i].dwMatchFlags) ||
          (m_itemList[i].dwMatchFlags == 0))
      {
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, m_itemList[i].pszName);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, i);
            m_wndListCtrl.SetItemText(iItem, 1, g_pszItemDataType[m_itemList[i].iDataType]);
            m_wndListCtrl.SetItemText(iItem, 2, m_itemList[i].pszDescription);
         }
      }

	return TRUE;
}


//
// Handler for OK button
//

void CInternalItemSelDlg::OnOK() 
{
   if (m_wndListCtrl.GetSelectedCount() == 0)
   {
      MessageBox(_T("You must select parameter from the list before pressing OK!"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
	else
   {
      DWORD dwIndex;

      dwIndex = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
      _tcsncpy(m_szItemName, m_itemList[dwIndex].pszName, MAX_DB_STRING);
      _tcsncpy(m_szItemDescription, m_itemList[dwIndex].pszDescription, MAX_DB_STRING);
      m_iDataType = m_itemList[dwIndex].iDataType;
	   CDialog::OnOK();
   }
}


//
// Handler for "Get..." button
//

void CInternalItemSelDlg::OnButtonGet() 
{
   if (m_wndListCtrl.GetSelectedCount() != 0)
   {
      CDataQueryDlg dlg;
      DWORD dwIndex;

      dwIndex = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
      dlg.m_dwObjectId = m_pNode->dwId;
      dlg.m_strNode = (LPCTSTR)m_pNode->szName;
      dlg.m_strParameter = (LPCTSTR)m_itemList[dwIndex].pszName;
      dlg.m_iOrigin = DS_INTERNAL;
      dlg.DoModal();
   }
   m_wndListCtrl.SetFocus();
}


//
// Handle double click in list view
//

void CInternalItemSelDlg::OnDblclkListParameters(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDOK, 0);
	
	*pResult = 0;
}
