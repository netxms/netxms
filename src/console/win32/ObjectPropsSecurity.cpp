// ObjectPropsSecurity.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsSecurity.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsSecurity property page

IMPLEMENT_DYNCREATE(CObjectPropsSecurity, CPropertyPage)

CObjectPropsSecurity::CObjectPropsSecurity() : CPropertyPage(CObjectPropsSecurity::IDD)
{
	//{{AFX_DATA_INIT(CObjectPropsSecurity)
	m_bInheritRights = FALSE;
	//}}AFX_DATA_INIT
   m_pAccessList = NULL;
   m_dwAclSize = 0;
}

CObjectPropsSecurity::~CObjectPropsSecurity()
{
   MemFree(m_pAccessList);
}

void CObjectPropsSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsSecurity)
	DDX_Control(pDX, IDC_CHECK_CREATE, m_wndCheckCreate);
	DDX_Control(pDX, IDC_CHECK_READ, m_wndCheckRead);
	DDX_Control(pDX, IDC_CHECK_MOVE, m_wndCheckMove);
	DDX_Control(pDX, IDC_CHECK_MODIFY, m_wndCheckModify);
	DDX_Control(pDX, IDC_CHECK_DELETE, m_wndCheckDelete);
	DDX_Control(pDX, IDC_LIST_USERS, m_wndUserList);
	DDX_Check(pDX, IDC_CHECK_INHERIT_RIGHTS, m_bInheritRights);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectPropsSecurity, CPropertyPage)
	//{{AFX_MSG_MAP(CObjectPropsSecurity)
	ON_BN_CLICKED(IDC_ADD_USER, OnAddUser)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_USERS, OnItemchangedListUsers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropsSecurity message handlers


//
// WM_INITDIALOG message handler
//

BOOL CObjectPropsSecurity::OnInitDialog() 
{
   DWORD i;
   NXC_USER *pUser;
   int iItem;
   CImageList *pImageList;
   RECT rect;

	CPropertyPage::OnInitDialog();
	
   m_pUpdate = ((CObjectPropSheet *)GetParent())->GetUpdateStruct();
   m_bIsModified = FALSE;

   // Setup list view
   m_wndUserList.GetClientRect(&rect);
   m_wndUserList.InsertColumn(0, "Name", LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndUserList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndUserList.SetHoverTime();

   // Create image list
   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVERYONE));
   m_wndUserList.SetImageList(pImageList, LVSIL_SMALL);

   // Copy object's ACL
   m_dwAclSize = m_pObject->dwAclSize;
   m_pAccessList = (NXC_ACL_ENTRY *)nx_memdup(m_pObject->pAccessList, 
                                              sizeof(NXC_ACL_ENTRY) * m_dwAclSize);

   // Populate user list with current ACL data
   for(i = 0; i < m_dwAclSize; i++)
   {
      pUser = NXCFindUserById(m_pAccessList[i].dwUserId);
      if (pUser != NULL)
      {
         iItem = m_wndUserList.InsertItem(0x7FFFFFFF, pUser->szName,
                                             (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
         if (iItem != -1)
            m_wndUserList.SetItemData(iItem, m_pAccessList[i].dwUserId);
      }
   }
	
	return TRUE;
}


//
// PSN_OK handler
//

void CObjectPropsSecurity::OnOK() 
{
	CPropertyPage::OnOK();
   
   if (m_bIsModified)
   {
      m_pUpdate->dwFlags |= OBJ_UPDATE_ACL;
      m_pUpdate->bInheritRights = m_bInheritRights;
      m_pUpdate->dwAclSize = m_dwAclSize;
      m_pUpdate->pAccessList = (NXC_ACL_ENTRY *)nx_memdup(m_pAccessList,
                                                          sizeof(NXC_ACL_ENTRY) * m_dwAclSize);
   }
}


//
// Handler for "Add user..." button
//

void CObjectPropsSecurity::OnAddUser() 
{
   CUserSelectDlg wndSelectDlg;

   if (wndSelectDlg.DoModal() == IDOK)
   {
      DWORD i;
      int iItem = -1;

      // Check if we have this user in ACL already
      for(i = 0; i < m_dwAclSize; i++)
         if (m_pAccessList[i].dwUserId == wndSelectDlg.m_dwUserId)
         {
            LVFINDINFO lvfi;

            // Find appropriate item in list
            lvfi.flags = LVFI_PARAM;
            lvfi.lParam = m_pAccessList[i].dwUserId;
            iItem = m_wndUserList.FindItem(&lvfi);
            break;
         }

      if (i == m_dwAclSize)
      {
         NXC_USER *pUser;

         // Create new entry in ACL
         m_pAccessList = (NXC_ACL_ENTRY *)MemReAlloc(m_pAccessList, sizeof(NXC_ACL_ENTRY) * (m_dwAclSize + 1));
         m_pAccessList[m_dwAclSize].dwUserId = wndSelectDlg.m_dwUserId;
         m_pAccessList[m_dwAclSize].dwAccessRights = OBJECT_ACCESS_READ;
         m_dwAclSize++;

         // Add new line to user list
         pUser = NXCFindUserById(wndSelectDlg.m_dwUserId);
         if (pUser != NULL)
         {
            iItem = m_wndUserList.InsertItem(0x7FFFFFFF, pUser->szName,
                                             (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
            m_wndUserList.SetItemData(iItem, wndSelectDlg.m_dwUserId);
         }

         m_bIsModified = TRUE;
      }

      // Select new item
      if (iItem != -1)
      {
         int iOldItem;

         iOldItem = m_wndUserList.GetNextItem(-1, LVNI_SELECTED);
         m_wndUserList.SetItemState(iItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
         m_wndUserList.EnsureVisible(iItem, FALSE);
         m_wndUserList.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, 
                                    LVIS_SELECTED | LVIS_FOCUSED);
      }
   }
}


//
// LVN_ITEMCHANGED handler for IDC_LIST_USERS
//

void CObjectPropsSecurity::OnItemchangedListUsers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
   NXC_USER *pUser;
   DWORD i;

   if (pNMListView->iItem != -1)
      if ((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVIS_FOCUSED))
      {
         pUser = NXCFindUserById(m_wndUserList.GetItemData(pNMListView->iItem));
         if (pUser != NULL)   // It should't be NULL
         {
            // Find user in ACL
            for(i = 0; i < m_dwAclSize; i++)
               if (pUser->dwId == m_pAccessList[i].dwUserId)
                  break;

            if (i != m_dwAclSize)   // Again, just a paranoia check
            {
               DWORD dwRights = m_pAccessList[i].dwAccessRights;
               
               // Set checkboxes
               m_wndCheckRead.SetCheck((dwRights & OBJECT_ACCESS_READ) ? BST_CHECKED : BST_UNCHECKED);
               m_wndCheckModify.SetCheck((dwRights & OBJECT_ACCESS_MODIFY) ? BST_CHECKED : BST_UNCHECKED);
               m_wndCheckCreate.SetCheck((dwRights & OBJECT_ACCESS_CREATE) ? BST_CHECKED : BST_UNCHECKED);
               m_wndCheckDelete.SetCheck((dwRights & OBJECT_ACCESS_DELETE) ? BST_CHECKED : BST_UNCHECKED);
               m_wndCheckMove.SetCheck((dwRights & OBJECT_ACCESS_MOVE) ? BST_CHECKED : BST_UNCHECKED);
            }
         }
      }
   
	*pResult = 0;
}
