// UserEditor.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "UserEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserEditor

IMPLEMENT_DYNCREATE(CUserEditor, CMDIChildWnd)

CUserEditor::CUserEditor()
{
   m_dwCurrentUser = INVALID_UID;
}

CUserEditor::~CUserEditor()
{
}


BEGIN_MESSAGE_MAP(CUserEditor, CMDIChildWnd)
	//{{AFX_MSG_MAP(CUserEditor)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_COMMAND(ID_USER_CREATE_GROUP, OnUserCreateGroup)
	ON_COMMAND(ID_USER_CREATE_USER, OnUserCreateUser)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_USER_PROPERTIES, OnUserProperties)
	ON_COMMAND(ID_USER_DELETE, OnUserDelete)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_USER_PROPERTIES, OnUpdateUserProperties)
	ON_UPDATE_COMMAND_UI(ID_USER_DELETE, OnUpdateUserDelete)
	ON_COMMAND(ID_USER_SETPASSWORD, OnUserSetpassword)
	ON_UPDATE_COMMAND_UI(ID_USER_SETPASSWORD, OnUpdateUserSetpassword)
	//}}AFX_MSG_MAP
   ON_MESSAGE(NXCM_USERDB_CHANGE, OnUserDBChange)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_VIEW, OnListViewDblClk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_VIEW, OnListViewItemChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserEditor message handlers

BOOL CUserEditor::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_USER_GROUP));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CLOSE message handler
//

void CUserEditor::OnClose() 
{
   DoRequestArg1(NXCUnlockUserDB, g_hSession, _T("Unlocking user database..."));
	CMDIChildWnd::OnClose();
}


//
// WM_DESTROY message handler
//

void CUserEditor::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_USER_MANAGER, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_CREATE message handler
//

int CUserEditor::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   CImageList *pImageList;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   theApp.OnViewCreate(VIEW_USER_MANAGER, this);
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL, rect, this, IDC_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   pImageList = new CImageList;
   pImageList->Create(16, 16, ILC_COLOR8 | ILC_MASK, 8, 8);
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_USER_GROUP));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_EVERYONE));
   pImageList->Add(AfxGetApp()->LoadIcon(IDI_OVL_STATUS_CRITICAL));
   pImageList->SetOverlayImage(3, 1);
   m_wndListCtrl.SetImageList(pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(1, _T("Full Name"), LVCFMT_LEFT, 170);
   m_wndListCtrl.InsertColumn(2, _T("Description"), LVCFMT_LEFT, 300);

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);

   return 0;
}


//
// Handler for WM_NOTIFY::NM_DBLCLK from IDC_LIST_VIEW
//

void CUserEditor::OnListViewDblClk(LPNMITEMACTIVATE pNMHDR, LRESULT *pResult)
{
   PostMessage(WM_COMMAND, ID_USER_PROPERTIES, 0);
   *pResult = 0;
}


//
// WM_SIZE message handler
//

void CUserEditor::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// Add new item to list
//

int CUserEditor::AddListItem(NXC_USER *pUser)
{
   int iItem;

   iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, pUser->szName,
                 (pUser->dwId == GROUP_EVERYONE) ? 2 : ((pUser->dwId & GROUP_FLAG) ? 1 : 0));
   if (iItem != -1)
   {
      m_wndListCtrl.SetItemData(iItem, pUser->dwId);
      m_wndListCtrl.SetItemText(iItem, 1, pUser->szFullName);
      m_wndListCtrl.SetItemText(iItem, 2, pUser->szDescription);
      m_wndListCtrl.SetItemState(iItem, INDEXTOOVERLAYMASK(pUser->wFlags & UF_DISABLED ? 1 : 0), LVIS_OVERLAYMASK);
   }
   return iItem;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CUserEditor::OnViewRefresh() 
{
   NXC_USER *pUserList;
   DWORD i, dwNumUsers;

   m_wndListCtrl.DeleteAllItems();
   
   // Fill in list view
   if (NXCGetUserDB(g_hSession, &pUserList, &dwNumUsers))
   {
      for(i = 0; i < dwNumUsers; i++)
         if (!(pUserList[i].wFlags & UF_DELETED))
            AddListItem(&pUserList[i]);
   }
}


//
// WM_COMMAND::ID_USER_CREATE_USER message handler
//

void CUserEditor::OnUserCreateUser() 
{
   CNewUserDlg wndDialog;

   wndDialog.m_strTitle = _T("New User");
   wndDialog.m_strHeader = _T("Login name");
   wndDialog.m_bDefineProperties = TRUE;
   if (wndDialog.DoModal() == IDOK)
      CreateUserObject((LPCTSTR)wndDialog.m_strName, FALSE, wndDialog.m_bDefineProperties);
}


//
// WM_COMMAND::ID_USER_CREATE_GROUP message handler
//

void CUserEditor::OnUserCreateGroup() 
{
   CNewUserDlg wndDialog;

   wndDialog.m_strTitle = _T("New Group");
   wndDialog.m_strHeader = _T("Group name");
   wndDialog.m_bDefineProperties = TRUE;
   if (wndDialog.DoModal() == IDOK)
      CreateUserObject((LPCTSTR)wndDialog.m_strName, TRUE, wndDialog.m_bDefineProperties);
}


//
// Create user or group
//

void CUserEditor::CreateUserObject(const TCHAR *pszName, BOOL bIsGroup, BOOL bShowProp)
{
   DWORD dwResult, dwNewId;

   // Send request to server
   dwResult = DoRequestArg4(NXCCreateUser, g_hSession, (void *)pszName, (void *)bIsGroup, 
                            &dwNewId, bIsGroup ? _T("Creating new group...") : _T("Creating new user..."));
   if (dwResult == RCC_SUCCESS)
   {
      int iItem, iOldItem;
      LVFINDINFO lvfi;

      // Find related item
      lvfi.flags = LVFI_PARAM;
      lvfi.lParam = dwNewId;
      iItem = m_wndListCtrl.FindItem(&lvfi, -1);

      if (iItem == -1)
      {
         NXC_USER user;

         memset(&user, 0, sizeof(NXC_USER));
         user.dwId = dwNewId;
         nx_strncpy(user.szName, pszName, MAX_USER_NAME);
         iItem = AddListItem(&user);
      }

      // Select newly created item
      iOldItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
      if (iOldItem != -1)
         m_wndListCtrl.SetItemState(iOldItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
      m_wndListCtrl.EnsureVisible(iItem, FALSE);
      m_wndListCtrl.SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, 
                                 LVIS_SELECTED | LVIS_FOCUSED);

      if (bShowProp)
         PostMessage(WM_COMMAND, ID_USER_PROPERTIES, 0);
   }
   else
   {
      TCHAR szBuffer[256];

      _stprintf(szBuffer, _T("Error creating %s object: %s"), 
                bIsGroup ? _T("group") : _T("user"), NXCGetErrorText(dwResult));
      MessageBox(szBuffer, _T("Error"), MB_OK | MB_ICONSTOP);
   }
}


//
// WM_USERDB_CHANGE message handler
//

void CUserEditor::OnUserDBChange(int iCode, NXC_USER *pUserInfo)
{
   int iItem;
   LVFINDINFO lvfi;

   // Find related item
   lvfi.flags = LVFI_PARAM;
   lvfi.lParam = pUserInfo->dwId;
   iItem = m_wndListCtrl.FindItem(&lvfi, -1);

   // Modify list control
   switch(iCode)
   {
      case USER_DB_CREATE:
         if (iItem == -1)
            AddListItem(pUserInfo);
         break;
      case USER_DB_MODIFY:
         if (iItem == -1)
         {
            AddListItem(pUserInfo);
         }
         else
         {
            m_wndListCtrl.SetItemText(iItem, 0, pUserInfo->szName);
            m_wndListCtrl.SetItemText(iItem, 1, pUserInfo->szFullName);
            m_wndListCtrl.SetItemText(iItem, 2, pUserInfo->szDescription);
            m_wndListCtrl.SetItemState(iItem, INDEXTOOVERLAYMASK(pUserInfo->wFlags & UF_DISABLED ? 1 : 0), LVIS_OVERLAYMASK);
         }
         break;
      case USER_DB_DELETE:
         if (iItem != -1)
            m_wndListCtrl.DeleteItem(iItem);
         break;
   }
}


//
// WM_SETFOCUS message handler
//

void CUserEditor::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndListCtrl.SetFocus();
}


//
// WM_COMMAND::ID_USER_PROPERTIES message handler
//

void CUserEditor::OnUserProperties() 
{
   int iItem;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      DWORD dwId;
      NXC_USER *pUser;

      dwId = m_wndListCtrl.GetItemData(iItem);
      pUser = NXCFindUserById(g_hSession, dwId);
      if (pUser != NULL)
      {
         NXC_USER userInfo;
         BOOL bModify = FALSE;

         memset(&userInfo, 0, sizeof(NXC_USER));
         if (dwId & GROUP_FLAG)
         {
            CGroupPropDlg dlg;

            dlg.m_pGroup = pUser;
            dlg.m_strName = pUser->szName;
            dlg.m_strDescription = pUser->szDescription;
            dlg.m_bDisabled = (pUser->wFlags & UF_DISABLED) ? TRUE : FALSE;
            dlg.m_bDropConn = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_SESSIONS) ? TRUE : FALSE;
            dlg.m_bManageConfig = (pUser->dwSystemRights & SYSTEM_ACCESS_SERVER_CONFIG) ? TRUE : FALSE;
            dlg.m_bConfigureTraps = (pUser->dwSystemRights & SYSTEM_ACCESS_CONFIGURE_TRAPS) ? TRUE : FALSE;
            dlg.m_bEditEventDB = (pUser->dwSystemRights & SYSTEM_ACCESS_EDIT_EVENT_DB) ? TRUE : FALSE;
            dlg.m_bViewEventDB = (pUser->dwSystemRights & SYSTEM_ACCESS_VIEW_EVENT_DB) ? TRUE : FALSE;
            dlg.m_bManageUsers = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_USERS) ? TRUE : FALSE;
            dlg.m_bManageActions = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_ACTIONS) ? TRUE : FALSE;
            dlg.m_bManageEPP = (pUser->dwSystemRights & SYSTEM_ACCESS_EPP) ? TRUE : FALSE;
            dlg.m_bDeleteAlarms = (pUser->dwSystemRights & SYSTEM_ACCESS_DELETE_ALARMS) ? TRUE : FALSE;
            dlg.m_bManagePkg = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_PACKAGES) ? TRUE : FALSE;
            dlg.m_bManageTools = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_TOOLS) ? TRUE : FALSE;
            dlg.m_bManageScripts = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_SCRIPTS) ? TRUE : FALSE;
            dlg.m_bViewTrapLog = (pUser->dwSystemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) ? TRUE : FALSE;
            dlg.m_bManageAgentCfg = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG) ? TRUE : FALSE;
            dlg.m_bManageModules = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_MODULES) ? TRUE : FALSE;
            if (dlg.DoModal() == IDOK)
            {
               userInfo.dwId = pUser->dwId;
               nx_strncpy(userInfo.szName, (LPCTSTR)dlg.m_strName, MAX_USER_NAME);
               nx_strncpy(userInfo.szDescription, (LPCTSTR)dlg.m_strDescription, MAX_USER_DESCR);
               userInfo.wFlags = dlg.m_bDisabled ? UF_DISABLED : 0;
               userInfo.dwSystemRights = (dlg.m_bDropConn ? SYSTEM_ACCESS_MANAGE_SESSIONS : 0) |
                                         (dlg.m_bManageUsers ? SYSTEM_ACCESS_MANAGE_USERS : 0) |
                                         (dlg.m_bManageActions ? SYSTEM_ACCESS_MANAGE_ACTIONS : 0) |
                                         (dlg.m_bManageEPP ? SYSTEM_ACCESS_EPP : 0) |
                                         (dlg.m_bManageConfig ? SYSTEM_ACCESS_SERVER_CONFIG : 0) |
                                         (dlg.m_bConfigureTraps ? SYSTEM_ACCESS_CONFIGURE_TRAPS : 0) |
                                         (dlg.m_bEditEventDB ? SYSTEM_ACCESS_EDIT_EVENT_DB : 0) |
                                         (dlg.m_bViewEventDB ? SYSTEM_ACCESS_VIEW_EVENT_DB : 0) |
                                         (dlg.m_bDeleteAlarms ? SYSTEM_ACCESS_DELETE_ALARMS : 0) |
                                         (dlg.m_bManagePkg ? SYSTEM_ACCESS_MANAGE_PACKAGES : 0) |
                                         (dlg.m_bManageTools ? SYSTEM_ACCESS_MANAGE_TOOLS : 0) |
                                         (dlg.m_bManageScripts ? SYSTEM_ACCESS_MANAGE_SCRIPTS : 0) |
                                         (dlg.m_bViewTrapLog ? SYSTEM_ACCESS_VIEW_TRAP_LOG : 0) |
                                         (dlg.m_bManageAgentCfg ? SYSTEM_ACCESS_MANAGE_AGENT_CFG : 0) |
                                         (dlg.m_bManageModules ? SYSTEM_ACCESS_MANAGE_MODULES : 0);
               userInfo.dwNumMembers = dlg.m_dwNumMembers;
               if (userInfo.dwNumMembers > 0)
               {
                  userInfo.pdwMemberList = (DWORD *)malloc(sizeof(DWORD) * userInfo.dwNumMembers);
                  memcpy(userInfo.pdwMemberList, dlg.m_pdwMembers, sizeof(DWORD) * userInfo.dwNumMembers);
               }
               bModify = TRUE;
            }
         }
         else
         {
            CUserPropDlg dlg;

            dlg.m_pUser = pUser;
            dlg.m_strLogin = pUser->szName;
            dlg.m_strFullName = pUser->szFullName;
            dlg.m_strDescription = pUser->szDescription;
            dlg.m_nAuthMethod = pUser->nAuthMethod;
				dlg.m_nMappingMethod = pUser->nCertMappingMethod;
				dlg.m_strMappingData = CHECK_NULL_EX(pUser->pszCertMappingData);
            dlg.m_bAccountDisabled = (pUser->wFlags & UF_DISABLED) ? TRUE : FALSE;
            dlg.m_bChangePassword = (pUser->wFlags & UF_CHANGE_PASSWORD) ? TRUE : FALSE;
            dlg.m_bDropConn = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_SESSIONS) ? TRUE : FALSE;
            dlg.m_bManageConfig = (pUser->dwSystemRights & SYSTEM_ACCESS_SERVER_CONFIG) ? TRUE : FALSE;
            dlg.m_bConfigureTraps = (pUser->dwSystemRights & SYSTEM_ACCESS_CONFIGURE_TRAPS) ? TRUE : FALSE;
            dlg.m_bEditEventDB = (pUser->dwSystemRights & SYSTEM_ACCESS_EDIT_EVENT_DB) ? TRUE : FALSE;
            dlg.m_bViewEventDB = (pUser->dwSystemRights & SYSTEM_ACCESS_VIEW_EVENT_DB) ? TRUE : FALSE;
            dlg.m_bManageUsers = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_USERS) ? TRUE : FALSE;
            dlg.m_bManageActions = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_ACTIONS) ? TRUE : FALSE;
            dlg.m_bManageEPP = (pUser->dwSystemRights & SYSTEM_ACCESS_EPP) ? TRUE : FALSE;
            dlg.m_bDeleteAlarms = (pUser->dwSystemRights & SYSTEM_ACCESS_DELETE_ALARMS) ? TRUE : FALSE;
            dlg.m_bManagePkg = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_PACKAGES) ? TRUE : FALSE;
            dlg.m_bManageTools = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_TOOLS) ? TRUE : FALSE;
            dlg.m_bManageScripts = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_SCRIPTS) ? TRUE : FALSE;
            dlg.m_bViewTrapLog = (pUser->dwSystemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) ? TRUE : FALSE;
            dlg.m_bManageAgentCfg = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG) ? TRUE : FALSE;
            dlg.m_bManageModules = (pUser->dwSystemRights & SYSTEM_ACCESS_MANAGE_MODULES) ? TRUE : FALSE;
            if (dlg.DoModal() == IDOK)
            {
               userInfo.dwId = pUser->dwId;
               nx_strncpy(userInfo.szName, (LPCTSTR)dlg.m_strLogin, MAX_USER_NAME);
               nx_strncpy(userInfo.szFullName, (LPCTSTR)dlg.m_strFullName, MAX_USER_FULLNAME);
               nx_strncpy(userInfo.szDescription, (LPCTSTR)dlg.m_strDescription, MAX_USER_DESCR);
               userInfo.nAuthMethod = dlg.m_nAuthMethod;
					userInfo.nCertMappingMethod = dlg.m_nMappingMethod;
					userInfo.pszCertMappingData = _tcsdup((LPCTSTR)dlg.m_strMappingData);
               userInfo.wFlags = (dlg.m_bAccountDisabled ? UF_DISABLED : 0) |
                                 (dlg.m_bChangePassword ? UF_CHANGE_PASSWORD : 0);
               userInfo.dwSystemRights = (dlg.m_bDropConn ? SYSTEM_ACCESS_MANAGE_SESSIONS : 0) |
                                         (dlg.m_bManageUsers ? SYSTEM_ACCESS_MANAGE_USERS : 0) |
                                         (dlg.m_bManageActions ? SYSTEM_ACCESS_MANAGE_ACTIONS : 0) |
                                         (dlg.m_bManageEPP ? SYSTEM_ACCESS_EPP : 0) |
                                         (dlg.m_bManageConfig ? SYSTEM_ACCESS_SERVER_CONFIG : 0) |
                                         (dlg.m_bConfigureTraps ? SYSTEM_ACCESS_CONFIGURE_TRAPS : 0) |
                                         (dlg.m_bEditEventDB ? SYSTEM_ACCESS_EDIT_EVENT_DB : 0) |
                                         (dlg.m_bViewEventDB ? SYSTEM_ACCESS_VIEW_EVENT_DB : 0) |
                                         (dlg.m_bDeleteAlarms ? SYSTEM_ACCESS_DELETE_ALARMS : 0) |
                                         (dlg.m_bManagePkg ? SYSTEM_ACCESS_MANAGE_PACKAGES : 0) |
                                         (dlg.m_bManageTools ? SYSTEM_ACCESS_MANAGE_TOOLS : 0) |
                                         (dlg.m_bManageScripts ? SYSTEM_ACCESS_MANAGE_SCRIPTS : 0) |
                                         (dlg.m_bViewTrapLog ? SYSTEM_ACCESS_VIEW_TRAP_LOG : 0) |
                                         (dlg.m_bManageAgentCfg ? SYSTEM_ACCESS_MANAGE_AGENT_CFG : 0) |
                                         (dlg.m_bManageModules ? SYSTEM_ACCESS_MANAGE_MODULES : 0);
               bModify = TRUE;
            }
         }

         // Modify user if OK was pressed
         if (bModify)
         {
            DWORD dwResult;

            dwResult = DoRequestArg2(NXCModifyUser, g_hSession, &userInfo, _T("Updating user database..."));
            if (dwResult != RCC_SUCCESS)
               theApp.ErrorBox(dwResult, _T("Cannot modify user record: %s"));

            // Cleanup
            safe_free(userInfo.pdwMemberList);
				safe_free(userInfo.pszCertMappingData);
         }
      }
      else
      {
         MessageBox(_T("Attempt to edit non-existing user record"), _T("Internal Error"), MB_ICONSTOP | MB_OK);
         PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
      }
   }
}


//
// WM_COMMAND::ID_USER_DELETE message handler
//

void CUserEditor::OnUserDelete() 
{
   int iItem;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      DWORD dwId;

      dwId = m_wndListCtrl.GetItemData(iItem);
      if (dwId == 0)
      {
         MessageBox(_T("System administrator account cannot be deleted"),
                    _T("Warning"), MB_ICONEXCLAMATION | MB_OK);
      }
      else if (dwId == GROUP_EVERYONE)
      {
         MessageBox(_T("Everyone group cannot be deleted"),
                    _T("Warning"), MB_ICONEXCLAMATION | MB_OK);
      }
      else
      {
         NXC_USER *pUser;

         pUser = NXCFindUserById(g_hSession, dwId);
         if (pUser != NULL)
         {
            TCHAR szBuffer[256];

            _stprintf(szBuffer, _T("Do you really wish to delete %s %s ?"),
                      dwId & GROUP_FLAG ? _T("group") : _T("user"), pUser->szName);
            if (MessageBox(szBuffer, _T("Delete account"), MB_ICONQUESTION | MB_YESNO) == IDYES)
            {
               DWORD dwResult;

               dwResult = DoRequestArg2(NXCDeleteUser, g_hSession, (void *)dwId, _T("Deleting user..."));
               if (dwResult != RCC_SUCCESS)
                  theApp.ErrorBox(dwResult, _T("Cannot delete user record: %s"));
            }
         }
         else
         {
            MessageBox(_T("Attempt to delete non-existing user record"), _T("Internal Error"), MB_ICONSTOP | MB_OK);
            PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
         }
      }
   }
}


//
// WM_CONTEXTMENU message handler
//

void CUserEditor::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu;

   pMenu = theApp.GetContextMenu(0);
   pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
}


//
// WM_NOTIFY::LVN_ITEMACTIVATE message handler
//

void CUserEditor::OnListViewItemChange(LPNMLISTVIEW pNMHDR, LRESULT *pResult)
{
   if (pNMHDR->iItem != -1)
      if (pNMHDR->uChanged & LVIF_STATE)
      {
         if (pNMHDR->uNewState & LVIS_FOCUSED)
         {
            m_dwCurrentUser = m_wndListCtrl.GetItemData(pNMHDR->iItem);
         }
         else
         {
            m_dwCurrentUser = INVALID_UID;
         }
      }
}


//
// WM_COMMAND::ID_USER_SETPASSWORD message handler
//

void CUserEditor::OnUserSetpassword() 
{
   int iItem;

   iItem = m_wndListCtrl.GetNextItem(-1, LVNI_FOCUSED);
   if (iItem != -1)
   {
      DWORD dwId;
      NXC_USER *pUser;

      dwId = m_wndListCtrl.GetItemData(iItem);

      pUser = NXCFindUserById(g_hSession, dwId);
      if (pUser != NULL)
      {
         CPasswordChangeDlg dlg(IDD_SET_PASSWORD);

         if (dlg.DoModal() == IDOK)
         {
            DWORD dwResult;

            dwResult = DoRequestArg3(NXCSetPassword, g_hSession, (void *)dwId, dlg.m_szPassword, _T("Changing password..."));
            if (dwResult != RCC_SUCCESS)
               theApp.ErrorBox(dwResult, _T("Cannot change password: %s"));
            else
               MessageBox(_T("Password was successfully changed"), _T("Information"), MB_ICONINFORMATION | MB_OK);
         }
      }
      else
      {
         MessageBox(_T("Attempt to modify non-existing user record"), _T("Internal Error"), MB_ICONSTOP | MB_OK);
         PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
      }
   }
}


//
// UPDATE_COMMAND_UI handlers
//

void CUserEditor::OnUpdateUserProperties(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_dwCurrentUser != INVALID_UID);
}

void CUserEditor::OnUpdateUserDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_dwCurrentUser != INVALID_UID);
}

void CUserEditor::OnUpdateUserSetpassword(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((m_dwCurrentUser != INVALID_UID) && (!(m_dwCurrentUser & GROUP_FLAG)));
}
