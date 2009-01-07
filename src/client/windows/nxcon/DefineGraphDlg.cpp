// DefineGraphDlg.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DefineGraphDlg.h"
#include "UserSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDefineGraphDlg dialog


CDefineGraphDlg::CDefineGraphDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDefineGraphDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDefineGraphDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_dwACLSize = 0;
	m_pACL = NULL;
	m_dwCurrAclEntry = INVALID_INDEX;
}

CDefineGraphDlg::~CDefineGraphDlg()
{
	safe_free(m_pACL);
}

void CDefineGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefineGraphDlg)
	DDX_Control(pDX, IDC_LIST_USERS, m_wndListCtrl);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDV_MaxChars(pDX, m_strName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefineGraphDlg, CDialog)
	//{{AFX_MSG_MAP(CDefineGraphDlg)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_USERS, OnItemchangedListUsers)
	ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
	ON_BN_CLICKED(IDC_CHECK_MODIFY, OnCheckModify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDefineGraphDlg message handlers

//
// WM_INITDIALOG message handler
//

BOOL CDefineGraphDlg::OnInitDialog() 
{
	RECT rect;
	DWORD i;

	CDialog::OnInitDialog();

	// Setup list control
	m_wndListCtrl.GetClientRect(&rect);
	m_wndListCtrl.InsertColumn(0, _T("User"), LVCFMT_LEFT, rect.right - 60 - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.InsertColumn(1, _T("Access"), LVCFMT_LEFT, 60);
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   // Create image list
   m_imageList.Create(16, 16, ILC_COLOR8 | ILC_MASK, 4, 1);
   m_imageList.Add(theApp.LoadIcon(IDI_USER));
   m_imageList.Add(theApp.LoadIcon(IDI_USER_GROUP));
   m_imageList.Add(theApp.LoadIcon(IDI_EVERYONE));
   m_wndListCtrl.SetImageList(&m_imageList, LVSIL_SMALL);

	// Fill ACL
	for(i = 0; i < m_dwACLSize; i++)
		AddListEntry(&m_pACL[i]);

	EnableDlgItem(this, IDC_CHECK_READ, FALSE);
	EnableDlgItem(this, IDC_CHECK_MODIFY, FALSE);
	EnableDlgItem(this, IDC_BUTTON_DELETE, FALSE);
	
	return TRUE;
}


//
// Add ACL entry to list control
//

void CDefineGraphDlg::AddListEntry(NXC_GRAPH_ACL_ENTRY *pEntry)
{
	NXC_USER *pUser;
	int nItem;
	TCHAR szBuffer[16];

	pUser = NXCFindUserById(g_hSession, pEntry->dwUserId);
	if (pUser != NULL)
	{
		nItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUser->szName,
			                              (pEntry->dwUserId == GROUP_EVERYONE) ? 2 :
					                           ((pEntry->dwUserId & GROUP_FLAG) ? 1 : 0));
		if (nItem != -1)
		{
			m_wndListCtrl.SetItemData(nItem, pEntry->dwUserId);
			szBuffer[0] = (pEntry->dwAccess & NXGRAPH_ACCESS_READ) ? _T('R') : _T('-');
			szBuffer[1] = (pEntry->dwAccess & NXGRAPH_ACCESS_WRITE) ? _T('M') : _T('-');
			szBuffer[2] = 0;
			m_wndListCtrl.SetItemText(nItem, 1, szBuffer);
		}
	}
}


//
// "Add" button handler
//

void CDefineGraphDlg::OnButtonAdd() 
{
	CUserSelectDlg dlg;
	DWORD i, j;

	if (dlg.DoModal() == IDOK)
	{
		for(i = 0; i < dlg.m_dwNumUsers; i++)
		{
			// Check if user already in ACL
			for(j = 0; j < m_dwACLSize; j++)
			{
				if (m_pACL[j].dwUserId == dlg.m_pdwUserList[i])
					break;
			}

			// If user not in ACL, add it
			if (j == m_dwACLSize)
			{
				m_dwACLSize++;
				m_pACL = (NXC_GRAPH_ACL_ENTRY *)realloc(m_pACL, sizeof(NXC_GRAPH_ACL_ENTRY) * m_dwACLSize);
				m_pACL[j].dwUserId = dlg.m_pdwUserList[i];
				m_pACL[j].dwAccess = NXGRAPH_ACCESS_READ;

				// Add list control item
				AddListEntry(&m_pACL[j]);
			}
		}
	}
}


//
// "Delete" button handler
//

void CDefineGraphDlg::OnButtonDelete() 
{
	int nItem;
	DWORD i, dwUserId;

	while((nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED)) != -1)
	{
		dwUserId = m_wndListCtrl.GetItemData(nItem);
		for(i = 0; i < m_dwACLSize; i++)
		{
			if (m_pACL[i].dwUserId == dwUserId)
			{
				m_dwACLSize--;
				memmove(&m_pACL[i], &m_pACL[i + 1], sizeof(NXC_GRAPH_ACL_ENTRY) * (m_dwACLSize - i));
				break;
			}
		}
		m_wndListCtrl.DeleteItem(nItem);
	}
}


//
// Handler for list control item changes
//

void CDefineGraphDlg::OnItemchangedListUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	DWORD i;
	NXC_USER *pUser;

	if (pNMListView->iItem != -1)
   {
      if (pNMListView->uChanged & LVIF_STATE)
      {
         if ((pNMListView->uNewState & LVIS_FOCUSED) && (pNMListView->uNewState & LVIS_SELECTED) &&
				 (m_wndListCtrl.GetSelectedCount() == 1))
         {
            pUser = NXCFindUserById(g_hSession, m_wndListCtrl.GetItemData(pNMListView->iItem));
            if (pUser != NULL)   // It should't be NULL
            {
               // Find user in ACL
               for(i = 0; i < m_dwACLSize; i++)
                  if (pUser->dwId == m_pACL[i].dwUserId)
                     break;

               if (i != m_dwACLSize)   // Again, just a paranoia check
					{
                  // Set checkboxes
						SendDlgItemMessage(IDC_CHECK_READ, BM_SETCHECK, (m_pACL[i].dwAccess & NXGRAPH_ACCESS_READ) ? BST_CHECKED : BST_UNCHECKED);
						SendDlgItemMessage(IDC_CHECK_MODIFY, BM_SETCHECK, (m_pACL[i].dwAccess & NXGRAPH_ACCESS_WRITE) ? BST_CHECKED : BST_UNCHECKED);

                  // Enable checkboxes if they was disabled
                  if (m_dwCurrAclEntry == INVALID_INDEX)
                  {
							EnableDlgItem(this, IDC_CHECK_READ, TRUE);
							EnableDlgItem(this, IDC_CHECK_MODIFY, TRUE);
                  }
                  m_dwCurrAclEntry = i;
					}
				}
			}
         else
         {
            // Deselect
            m_dwCurrAclEntry = INVALID_INDEX;
				EnableDlgItem(this, IDC_CHECK_READ, FALSE);
				EnableDlgItem(this, IDC_CHECK_MODIFY, FALSE);
			}
			EnableDlgItem(this, IDC_BUTTON_DELETE, m_wndListCtrl.GetSelectedCount() > 0);
		}
	}
	*pResult = 0;
}


//
// Handler for "Read" checkbox
//

void CDefineGraphDlg::OnCheckRead() 
{
	if (m_dwCurrAclEntry != INVALID_INDEX)
	{
		if (SendDlgItemMessage(IDC_CHECK_READ, BM_GETCHECK, 0, 0) == BST_CHECKED)
			m_pACL[m_dwCurrAclEntry].dwAccess |= NXGRAPH_ACCESS_READ;
		else
			m_pACL[m_dwCurrAclEntry].dwAccess &= ~NXGRAPH_ACCESS_READ;
	}
}


//
// Handler for "Modify" checkbox
//

void CDefineGraphDlg::OnCheckModify() 
{
	if (m_dwCurrAclEntry != INVALID_INDEX)
	{
		if (SendDlgItemMessage(IDC_CHECK_MODIFY, BM_GETCHECK, 0, 0) == BST_CHECKED)
			m_pACL[m_dwCurrAclEntry].dwAccess |= NXGRAPH_ACCESS_WRITE;
		else
			m_pACL[m_dwCurrAclEntry].dwAccess &= ~NXGRAPH_ACCESS_WRITE;
	}
}
