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
   DWORD i, dwNumUsers;
   int iItem;
   CImageList *pImageList;

	CDialog::OnInitDialog();

   // Setup list view
   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, "Name", LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndListCtrl.SetHoverTime();

   // Create image list
   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(pImageList, LVSIL_SMALL);

   // Fill in list view
   if (NXCGetUserDB(&pUserList, &dwNumUsers))
   {
      for(i = 0; i < dwNumUsers; i++)
         if (!(pUserList[i].wFlags & UF_DELETED))
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

   iItem = m_wndListCtrl.GetSelectionMark();
   if (iItem != -1)
   {
      m_dwUserId = m_wndListCtrl.GetItemData(iItem);
	   CDialog::OnOK();
   }
   else
   {
      MessageBox("You should select user or group first", "User selection", MB_OK | MB_ICONSTOP);
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
