// AgentParamSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "AgentParamSelDlg.h"
#include "DataQueryDlg.h"
#include "InputBox.h"

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
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_PARAMS, OnDblclkListParams)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentParamSelDlg message handlers

static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   return _tcsicmp(((CAgentParamSelDlg *)lParamSort)->m_pParamList[lParam1].szName,
                   ((CAgentParamSelDlg *)lParamSort)->m_pParamList[lParam2].szName);
}


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

   m_wndListCtrl.SortItems(CompareItems, (LPARAM)this);

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
      int iLen;
      BOOL bStart = TRUE;

      // Detect (*) at the end of parameter's name
      dwIndex = m_wndListCtrl.GetItemData(m_wndListCtrl.GetSelectionMark());
      dlg.m_dwObjectId = m_pNode->dwId;
      dlg.m_strNode = (LPCTSTR)m_pNode->szName;
      dlg.m_iOrigin = DS_NATIVE_AGENT;

      iLen = _tcslen(m_pParamList[dwIndex].szName);
      if (iLen > 3)
      {
         if (!_tcscmp(&m_pParamList[dwIndex].szName[iLen - 3], _T("(*)")))
         {
            CInputBox boxDlg;

            boxDlg.m_strTitle = _T("Arguments");
            boxDlg.m_strHeader = _T("Enter parameter's argument(s):");
            if (boxDlg.DoModal() == IDOK)
            {
               TCHAR szBuffer[MAX_DB_STRING];

               memcpy(szBuffer, m_pParamList[dwIndex].szName, (iLen - 2) * sizeof(TCHAR));
               _tcscpy(&szBuffer[iLen - 2], boxDlg.m_strText);
               _tcscat(szBuffer, _T(")"));
               dlg.m_strParameter = (LPCTSTR)szBuffer;
            }
            else
            {
               bStart = FALSE;
            }
         }
         else
         {
            dlg.m_strParameter = (LPCTSTR)m_pParamList[dwIndex].szName;
         }
      }
      else
      {
         dlg.m_strParameter = (LPCTSTR)m_pParamList[dwIndex].szName;
      }

      if (bStart)
         dlg.DoModal();
   }
   m_wndListCtrl.SetFocus();
}


//
// Process double click on list
//

void CAgentParamSelDlg::OnDblclkListParams(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDOK, 0);
	
	*pResult = 0;
}
