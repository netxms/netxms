// AgentParamSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentParamSelDlg.h"
#include "DataQueryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAgentParamSelDlg dialog


CAgentParamSelDlg::CAgentParamSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAgentParamSelDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAgentParamSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_pNode = NULL;
}


void CAgentParamSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAgentParamSelDlg)
	DDX_Control(pDX, IDC_LIST_PARAMS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAgentParamSelDlg, CDialog)
	//{{AFX_MSG_MAP(CAgentParamSelDlg)
	ON_BN_CLICKED(IDC_BUTTON_GET, OnButtonGet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentParamSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CAgentParamSelDlg::OnInitDialog() 
{
   DWORD i;
   int iItem;
   RECT rect;

	CDialog::OnInitDialog();

   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Parameter name"), LVCFMT_LEFT, 160);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 90);
   m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT,
                              rect.right - 250 - GetSystemMetrics(SM_CXVSCROLL));
	
   for(i = 0; i < m_dwNumParams; i++)
   {
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, m_pParamList[i].szName);
      if (iItem != -1)
      {
         m_wndListCtrl.SetItemData(iItem, i);
         m_wndListCtrl.SetItemText(iItem, 1, g_pszItemDataType[m_pParamList[i].iDataType]);
         m_wndListCtrl.SetItemText(iItem, 2, m_pParamList[i].szDescription);
      }
   }

	return TRUE;
}


//
// Handler for OK button
//

void CAgentParamSelDlg::OnOK() 
{
   if (m_wndListCtrl.GetSelectedCount() == 0)
   {
      MessageBox(_T("You must select parameter from the list before pressing OK!"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
	else
   {
      m_dwSelectionIndex = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
	   CDialog::OnOK();
   }
}


//
// Handler for "Get..." button
//

void CAgentParamSelDlg::OnButtonGet() 
{
   if (m_wndListCtrl.GetSelectedCount() != 0)
   {
      CDataQueryDlg dlg;
      DWORD dwIndex;

      dwIndex = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
      dlg.m_dwObjectId = m_pNode->dwId;
      dlg.m_strNode = (LPCTSTR)m_pNode->szName;
      dlg.m_strParameter = (LPCTSTR)m_pParamList[dwIndex].szName;
      dlg.m_iOrigin = DS_NATIVE_AGENT;
      dlg.DoModal();
   }
   m_wndListCtrl.SetFocus();
}
