// ActionSelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ActionSelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionSelDlg dialog


CActionSelDlg::CActionSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CActionSelDlg::IDD, pParent)
{
   m_pdwActionList = NULL;

	//{{AFX_DATA_INIT(CActionSelDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CActionSelDlg::~CActionSelDlg()
{
   safe_free(m_pdwActionList);
}


void CActionSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionSelDlg)
	DDX_Control(pDX, IDC_LIST_ACTIONS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionSelDlg, CDialog)
	//{{AFX_MSG_MAP(CActionSelDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ACTIONS, OnDblclkListActions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionSelDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CActionSelDlg::OnInitDialog() 
{
   DWORD i;
   int iItem;
   RECT rect;

	CDialog::OnInitDialog();
	
   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_REXEC));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_EMAIL));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_SMS));
   m_imageList.Add(AfxGetApp()->LoadIcon(IDI_FORWARD_EVENT));

   // Setup list control
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 200);
   m_wndListCtrl.InsertColumn(1, _T("Type"), LVCFMT_LEFT, 
                              rect.right - 200 - GetSystemMetrics(SM_CXVSCROLL));
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	
   // Fill in event list
   LockActions();
   for(i = 0; i < g_dwNumActions; i++)
   {
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, g_pActionList[i].szName, g_pActionList[i].iType);
      if (iItem != -1)
      {
         m_wndListCtrl.SetItemText(iItem, 1, g_szActionType[g_pActionList[i].iType]);
         m_wndListCtrl.SetItemData(iItem, g_pActionList[i].dwId);
      }
   }
   UnlockActions();

	return TRUE;
}


//
// WM_COMMAND::IDOK message handler
//

void CActionSelDlg::OnOK() 
{
   int iItem;
   DWORD i;

   m_dwNumActions = m_wndListCtrl.GetSelectedCount();
   if (m_dwNumActions > 0)
   {
      // Build list of selected objects
      m_pdwActionList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumActions);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; iItem != -1; i++)
      {
         m_pdwActionList[i] = m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
	   CDialog::OnOK();
   }
   else
   {
      MessageBox(_T("You should select at least one action"),
                 _T("Warning"), MB_OK | MB_ICONEXCLAMATION);
   }
}


//
// Handler for list control double click
//

void CActionSelDlg::OnDblclkListActions(NMHDR* pNMHDR, LRESULT* pResult) 
{
   if (m_wndListCtrl.GetSelectedCount() > 0)
      PostMessage(WM_COMMAND, IDOK, 0);
	*pResult = 0;
}
