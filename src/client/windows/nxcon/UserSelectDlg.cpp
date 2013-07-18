// UserSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "UserSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserSelectDlg dialog


CUserSelectDlg::CUserSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUserSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUserSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

   m_bOnlyUsers = FALSE;
   m_bAddPublic = FALSE;
   m_bSingleSelection = FALSE;
   m_pdwUserList = NULL;
}

CUserSelectDlg::~CUserSelectDlg()
{
   safe_free(m_pdwUserList);
}

void CUserSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUserSelectDlg)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUserSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CUserSelectDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_USERS, OnDblclkListUsers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserSelectDlg message handlers


//
// WM_INITDIALOG message handler
//

BOOL CUserSelectDlg::OnInitDialog() 
{
   RECT rect;
   NXC_USER *pUserList;
   UINT32 i, dwNumUsers;
   int iItem;

	CDialog::OnInitDialog();

   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   if (m_bSingleSelection)
   {
      ::SetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE, 
         ::GetWindowLong(m_wndListCtrl.m_hWnd, GWL_STYLE) | LVS_SINGLESEL);
   }

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   m_imageList.Add(theApp.LoadIcon(IDI_USER));
   m_imageList.Add(theApp.LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

   // Fill in list view
   if (m_bAddPublic)
   {
      iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, _T("[public]"), 2);
      if (iItem != -1)
         m_wndListCtrl.SetItemData(iItem, GROUP_EVERYONE);
   }
   if (NXCGetUserDB(g_hSession, &pUserList, &dwNumUsers))
   {
      for(i = 0; i < dwNumUsers; i++)
         if (!(pUserList[i].wFlags & UF_DELETED))
            if (!m_bOnlyUsers || !(pUserList[i].dwId & GROUP_FLAG))
            {
               iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUserList[i].szName,
                  (pUserList[i].dwId == GROUP_EVERYONE) ? 2 :
                     ((pUserList[i].dwId & GROUP_FLAG) ? 1 : 0));
               if (iItem != -1)
                  m_wndListCtrl.SetItemData(iItem, pUserList[i].dwId);
            }
   }

	return TRUE;
}


//
// Handler for OK button
//

void CUserSelectDlg::OnOK() 
{
   int iItem;
   DWORD i;

   m_dwNumUsers = m_wndListCtrl.GetSelectedCount();
   if (m_dwNumUsers > 0)
   {
      m_pdwUserList = (DWORD *)malloc(sizeof(DWORD) * m_dwNumUsers);
      iItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
      for(i = 0; (iItem != -1) && (i < m_dwNumUsers); i++)
      {
         m_pdwUserList[i] = (DWORD)m_wndListCtrl.GetItemData(iItem);
         iItem = m_wndListCtrl.GetNextItem(iItem, LVIS_SELECTED);
      }
	   CDialog::OnOK();
   }
   else
   {
      MessageBox(_T("You should select user or group first"),
                 _T("User selection"), MB_OK | MB_ICONSTOP);
   }
}


//
// Handler for doble click in user list
//

void CUserSelectDlg::OnDblclkListUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
   PostMessage(WM_COMMAND, IDOK, 0);
   *pResult = 0;
}
