// ObjectPropsSecurity.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectPropsSecurity.h"
#include "ObjectPropSheet.h"

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
   safe_free(m_pAccessList);
}

void CObjectPropsSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectPropsSecurity)
	DDX_Control(pDX, IDC_CHECK_TERM_ALARMS, m_wndCheckTermAlarms);
	DDX_Control(pDX, IDC_CHECK_PUSH_DATA, m_wndCheckPushData);
	DDX_Control(pDX, IDC_CHECK_SEND, m_wndCheckSend);
	DDX_Control(pDX, IDC_CHECK_CONTROL, m_wndCheckControl);
	DDX_Control(pDX, IDC_CHECK_ACK_ALARMS, m_wndCheckAckAlarms);
	DDX_Control(pDX, IDC_CHECK_VIEW_ALARMS, m_wndCheckViewAlarms);
	DDX_Control(pDX, IDC_CHECK_ACCESS, m_wndCheckAccess);
	DDX_Control(pDX, IDC_CHECK_CREATE, m_wndCheckCreate);
	DDX_Control(pDX, IDC_CHECK_READ, m_wndCheckRead);
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
	ON_BN_CLICKED(IDC_CHECK_INHERIT_RIGHTS, OnCheckInheritRights)
	ON_BN_CLICKED(IDC_DELETE_USER, OnDeleteUser)
	ON_BN_CLICKED(IDC_CHECK_READ, OnCheckRead)
	ON_BN_CLICKED(IDC_CHECK_MODIFY, OnCheckModify)
	ON_BN_CLICKED(IDC_CHECK_DELETE, OnCheckDelete)
	ON_BN_CLICKED(IDC_CHECK_CREATE, OnCheckCreate)
	ON_BN_CLICKED(IDC_CHECK_ACCESS, OnCheckAccess)
	ON_BN_CLICKED(IDC_CHECK_VIEW_ALARMS, OnCheckViewAlarms)
	ON_BN_CLICKED(IDC_CHECK_ACK_ALARMS, OnCheckAckAlarms)
	ON_BN_CLICKED(IDC_CHECK_SEND, OnCheckSend)
	ON_BN_CLICKED(IDC_CHECK_CONTROL, OnCheckControl)
	ON_BN_CLICKED(IDC_CHECK_TERM_ALARMS, OnCheckTermAlarms)
	ON_BN_CLICKED(IDC_CHECK_PUSH_DATA, OnCheckPushData)
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
   m_wndUserList.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 
                              rect.right - GetSystemMetrics(SM_CXVSCROLL) - 4);
	m_wndUserList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT);
   m_wndUserList.SetHoverTime(0x7FFFFFFF);

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
      pUser = NXCFindUserById(g_hSession, m_pAccessList[i].dwUserId);
      if (pUser != NULL)
      {
         iItem = m_wndUserList.InsertItem(0x7FFFFFFF, pUser->szName,
                                             (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
         if (iItem != -1)
            m_wndUserList.SetItemData(iItem, m_pAccessList[i].dwUserId);
      }
   }

   // We have no selection by default, so disable all checkboxes
   m_dwCurrAclEntry = INVALID_INDEX;
   m_wndCheckRead.EnableWindow(FALSE);
   m_wndCheckModify.EnableWindow(FALSE);
   m_wndCheckCreate.EnableWindow(FALSE);
   m_wndCheckDelete.EnableWindow(FALSE);
   m_wndCheckViewAlarms.EnableWindow(FALSE);
   m_wndCheckAckAlarms.EnableWindow(FALSE);
   m_wndCheckTermAlarms.EnableWindow(FALSE);
   m_wndCheckAccess.EnableWindow(FALSE);
   m_wndCheckSend.EnableWindow(FALSE);
   m_wndCheckPushData.EnableWindow(FALSE);
   m_wndCheckControl.EnableWindow(FALSE);
	
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
      m_pUpdate->qwFlags |= OBJ_UPDATE_ACL;
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
      DWORD i, j;
      int iItem = -1;

      for(j = 0; j < wndSelectDlg.m_dwNumUsers; j++)
      {
         // Check if we have this user in ACL already
         for(i = 0; i < m_dwAclSize; i++)
            if (m_pAccessList[i].dwUserId == wndSelectDlg.m_pdwUserList[j])
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
            m_pAccessList = (NXC_ACL_ENTRY *)realloc(m_pAccessList, sizeof(NXC_ACL_ENTRY) * (m_dwAclSize + 1));
            m_pAccessList[m_dwAclSize].dwUserId = wndSelectDlg.m_pdwUserList[j];
            m_pAccessList[m_dwAclSize].dwAccessRights = OBJECT_ACCESS_READ | OBJECT_ACCESS_READ_ALARMS;
            m_dwAclSize++;

            // Add new line to user list
            pUser = NXCFindUserById(g_hSession, wndSelectDlg.m_pdwUserList[j]);
            if (pUser != NULL)
            {
               iItem = m_wndUserList.InsertItem(0x7FFFFFFF, pUser->szName,
                                                (pUser->dwId == GROUP_EVERYONE) ? 2 :
                                                   ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
               m_wndUserList.SetItemData(iItem, wndSelectDlg.m_pdwUserList[j]);
            }
         }

         m_bIsModified = TRUE;
      }

      // Select new item
      if (iItem != -1)
         SelectListViewItem(&m_wndUserList, iItem);
   }
}


//
// Handler for "Delete user" button
//

void CObjectPropsSecurity::OnDeleteUser() 
{
   int iItem;

   if (m_dwCurrAclEntry != INVALID_INDEX)
   {
      iItem = m_wndUserList.GetSelectionMark();
      if (iItem != -1)
      {
         m_dwAclSize--;
         memmove(&m_pAccessList[m_dwCurrAclEntry], &m_pAccessList[m_dwCurrAclEntry + 1],
                 sizeof(NXC_ACL_ENTRY) * (m_dwAclSize - m_dwCurrAclEntry));
         m_wndUserList.DeleteItem(iItem);
         m_bIsModified = TRUE;
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
   {
      if (pNMListView->uChanged & LVIF_STATE)
      {
         if ((pNMListView->uNewState & LVIS_FOCUSED) && (pNMListView->uNewState & LVIS_SELECTED))
         {
            pUser = NXCFindUserById(g_hSession, (DWORD)m_wndUserList.GetItemData(pNMListView->iItem));
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
                  m_wndCheckViewAlarms.SetCheck((dwRights & OBJECT_ACCESS_READ_ALARMS) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckAckAlarms.SetCheck((dwRights & OBJECT_ACCESS_UPDATE_ALARMS) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckTermAlarms.SetCheck((dwRights & OBJECT_ACCESS_TERM_ALARMS) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckAccess.SetCheck((dwRights & OBJECT_ACCESS_ACL) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckSend.SetCheck((dwRights & OBJECT_ACCESS_SEND_EVENTS) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckPushData.SetCheck((dwRights & OBJECT_ACCESS_PUSH_DATA) ? BST_CHECKED : BST_UNCHECKED);
                  m_wndCheckControl.SetCheck((dwRights & OBJECT_ACCESS_CONTROL) ? BST_CHECKED : BST_UNCHECKED);

                  // Enable checkboxes if they was disabled
                  if (m_dwCurrAclEntry == INVALID_INDEX)
                  {
                     m_wndCheckRead.EnableWindow(TRUE);
                     m_wndCheckModify.EnableWindow(TRUE);
                     m_wndCheckCreate.EnableWindow(TRUE);
                     m_wndCheckDelete.EnableWindow(TRUE);
                     m_wndCheckViewAlarms.EnableWindow(TRUE);
                     m_wndCheckAckAlarms.EnableWindow(TRUE);
                     m_wndCheckTermAlarms.EnableWindow(TRUE);
                     m_wndCheckAccess.EnableWindow(TRUE);
                     m_wndCheckSend.EnableWindow(TRUE);
                     m_wndCheckPushData.EnableWindow(TRUE);
                     m_wndCheckControl.EnableWindow(TRUE);
                  }
                  m_dwCurrAclEntry = i;
               }
            }
         }
         else
         {
            // Deselect
            m_dwCurrAclEntry = INVALID_INDEX;
            m_wndCheckRead.EnableWindow(FALSE);
            m_wndCheckModify.EnableWindow(FALSE);
            m_wndCheckCreate.EnableWindow(FALSE);
            m_wndCheckDelete.EnableWindow(FALSE);
            m_wndCheckViewAlarms.EnableWindow(FALSE);
            m_wndCheckAckAlarms.EnableWindow(FALSE);
            m_wndCheckTermAlarms.EnableWindow(FALSE);
            m_wndCheckAccess.EnableWindow(FALSE);
            m_wndCheckSend.EnableWindow(FALSE);
            m_wndCheckPushData.EnableWindow(FALSE);
            m_wndCheckControl.EnableWindow(FALSE);
         }
      }
   }
   
	*pResult = 0;
}


//
// Handle clicks on checkboxes
//

void CObjectPropsSecurity::OnCheckInheritRights() 
{
   m_bIsModified = TRUE;
}

void CObjectPropsSecurity::OnCheckRead() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckRead.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_READ;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_READ;
   }
}

void CObjectPropsSecurity::OnCheckModify() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckModify.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_MODIFY;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_MODIFY;
   }
}

void CObjectPropsSecurity::OnCheckDelete() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckDelete.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_DELETE;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_DELETE;
   }
}

void CObjectPropsSecurity::OnCheckCreate() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckCreate.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_CREATE;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_CREATE;
   }
}

void CObjectPropsSecurity::OnCheckAccess() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckAccess.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_ACL;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_ACL;
   }
}

void CObjectPropsSecurity::OnCheckViewAlarms() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckViewAlarms.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_READ_ALARMS;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_READ_ALARMS;
   }
}

void CObjectPropsSecurity::OnCheckAckAlarms() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckAckAlarms.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_UPDATE_ALARMS;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_UPDATE_ALARMS;
   }
}

void CObjectPropsSecurity::OnCheckTermAlarms() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckTermAlarms.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_TERM_ALARMS;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_TERM_ALARMS;
   }
}

void CObjectPropsSecurity::OnCheckSend() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckSend.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_SEND_EVENTS;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_SEND_EVENTS;
   }
}

void CObjectPropsSecurity::OnCheckControl() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckControl.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_CONTROL;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_CONTROL;
   }
}

void CObjectPropsSecurity::OnCheckPushData() 
{
   m_bIsModified = TRUE;
   if (m_dwCurrAclEntry != INVALID_INDEX)    // It shouldn't be
   {
      if (m_wndCheckPushData.GetCheck() == BST_CHECKED)
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights |= OBJECT_ACCESS_PUSH_DATA;
      else
         m_pAccessList[m_dwCurrAclEntry].dwAccessRights &= ~OBJECT_ACCESS_PUSH_DATA;
   }
}
