/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: users.cpp
**
**/

#include "libnxcl.h"


//
// Static data
//

static DWORD m_dwNumUsers;
static NXC_USER *m_pUserList = NULL;
static BOOL m_bUserDBLoaded = FALSE;


//
// Fill user record with data from message
//

static void UpdateUserFromMessage(CSCPMessage *pMsg, NXC_USER *pUser)
{
   // Process common fields
   pUser->dwId = pMsg->GetVariableLong(VID_USER_ID);
   pMsg->GetVariableStr(VID_USER_NAME, pUser->szName, MAX_USER_NAME);
   pUser->wFlags = pMsg->GetVariableShort(VID_USER_FLAGS);
   pUser->wSystemRights = pMsg->GetVariableShort(VID_USER_SYS_RIGHTS);
   pMsg->GetVariableStr(VID_USER_DESCRIPTION, pUser->szDescription, MAX_USER_DESCR);

   // Process group-specific fields
   if (pUser->dwId & GROUP_FLAG)
   {
      DWORD i, dwId;

      pUser->dwNumMembers = pMsg->GetVariableLong(VID_NUM_MEMBERS);
      pUser->pdwMemberList = (DWORD *)realloc(pUser->pdwMemberList, sizeof(DWORD) * pUser->dwNumMembers);
      for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pUser->dwNumMembers; i++, dwId++)
         pUser->pdwMemberList[i] = pMsg->GetVariableLong(dwId);
   }
   else     // User-specific data
   {
      pMsg->GetVariableStr(VID_USER_FULL_NAME, pUser->szFullName, MAX_USER_FULLNAME);
   }
}


//
// Process user database update
//

void ProcessUserDBUpdate(CSCPMessage *pMsg)
{
   int iCode;
   DWORD dwUserId;
   NXC_USER *pUser;

   iCode = pMsg->GetVariableShort(VID_UPDATE_TYPE);
   dwUserId = pMsg->GetVariableLong(VID_USER_ID);
   pUser = NXCFindUserById(dwUserId);

   switch(iCode)
   {
      case USER_DB_CREATE:
         if (pUser == NULL)
         {
            // No user with this id, create one
            m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
            memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));

            // Process common fields
            m_pUserList[m_dwNumUsers].dwId = dwUserId;
            pMsg->GetVariableStr(VID_USER_NAME, m_pUserList[m_dwNumUsers].szName, MAX_USER_NAME);
            pUser = &m_pUserList[m_dwNumUsers];
            m_dwNumUsers++;
         }
         break;
      case USER_DB_MODIFY:
         if (pUser == NULL)
         {
            // No user with this id, create one
            m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
            memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));
            pUser = &m_pUserList[m_dwNumUsers];
            m_dwNumUsers++;
         }
         UpdateUserFromMessage(pMsg, pUser);
         break;
      case USER_DB_DELETE:
         if (pUser != NULL)
            pUser->wFlags |= UF_DELETED;
         break;
      default:
         break;
   }

   if (pUser != NULL)
      CallEventHandler(NXC_EVENT_USER_DB_CHANGED, iCode, pUser);
}


//
// Process record from network
//

void ProcessUserDBRecord(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_USER_DB_EOF:
         CompleteSync(RCC_SUCCESS);
         break;
      case CMD_USER_DATA:
      case CMD_GROUP_DATA:
         m_pUserList = (NXC_USER *)realloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
         memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));
         UpdateUserFromMessage(pMsg, &m_pUserList[m_dwNumUsers]);
         m_dwNumUsers++;
         break;
      default:
         break;
   }
}


//
// Load user database
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(void)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;
   PrepareForSync();

   safe_free(m_pUserList);
   m_pUserList = NULL;
   m_dwNumUsers = 0;

   msg.SetCode(CMD_LOAD_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   if (dwRetCode == RCC_SUCCESS)
   {
      dwRetCode = WaitForSync(INFINITE);
      if (dwRetCode == RCC_SUCCESS)
         m_bUserDBLoaded = TRUE;
   }

   return dwRetCode;
}


//
// Find user in database by ID
//

NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(DWORD dwId)
{
   DWORD i;
   NXC_USER *pUser = NULL;

   if (m_bUserDBLoaded)
   {
      for(i = 0; i < m_dwNumUsers; i++)
         if (m_pUserList[i].dwId == dwId)
         {
            pUser = &m_pUserList[i];
            break;
         }
   }

   return pUser;
}


//
// Get pointer to user list and number of users
//

BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_USER **ppUserList, DWORD *pdwNumUsers)
{
   if (!m_bUserDBLoaded)
      return FALSE;

   *ppUserList = m_pUserList;
   *pdwNumUsers = m_dwNumUsers;
   return TRUE;
}


//
// Create new user or group on server
//

DWORD LIBNXCL_EXPORTABLE NXCCreateUser(TCHAR *pszName, BOOL bIsGroup, DWORD *pdwNewId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_CREATE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_NAME, pszName);
   msg.SetVariable(VID_IS_GROUP, (WORD)bIsGroup);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, g_dwCommandTimeout);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwNewId = pResponce->GetVariableLong(VID_USER_ID);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Delete user or group
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteUser(DWORD dwId)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_DELETE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);
   return dwRetCode;
}


//
// Lock/unlock user database
//

static DWORD LockUserDB(BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;

   msg.SetCode(bLock ? CMD_LOCK_USER_DB : CMD_UNLOCK_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);
   return dwRetCode;
}


//
// Client interface: lock user database
//

DWORD LIBNXCL_EXPORTABLE NXCLockUserDB(void)
{
   return LockUserDB(TRUE);
}


//
// Client interface: unlock user database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockUserDB(void)
{
   return LockUserDB(FALSE);
}


//
// Modify user record
//

DWORD LIBNXCL_EXPORTABLE NXCModifyUser(NXC_USER *pUserInfo)
{
   CSCPMessage msg;
   DWORD i, dwId, dwRqId;

   dwRqId = g_dwMsgId++;

   // Fill in request
   msg.SetCode(CMD_UPDATE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, pUserInfo->dwId);
   msg.SetVariable(VID_USER_NAME, pUserInfo->szName);
   msg.SetVariable(VID_USER_DESCRIPTION, pUserInfo->szDescription);
   msg.SetVariable(VID_USER_FLAGS, pUserInfo->wFlags);
   msg.SetVariable(VID_USER_SYS_RIGHTS, pUserInfo->wSystemRights);

   // Group-specific fields
   if (pUserInfo->dwId & GROUP_FLAG)
   {
      msg.SetVariable(VID_NUM_MEMBERS, pUserInfo->dwNumMembers);
      for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pUserInfo->dwNumMembers; i++, dwId++)
         msg.SetVariable(dwId, pUserInfo->pdwMemberList[i]);
   }
   else     // User-specific fields
   {
      msg.SetVariable(VID_USER_FULL_NAME, pUserInfo->szFullName);
   }

   SendMsg(&msg);

   // Wait for responce
   return WaitForRCC(dwRqId);
}


//
// Set password for user
//

DWORD LIBNXCL_EXPORTABLE NXCSetPassword(DWORD dwUserId, char *pszNewPassword)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;
   BYTE hash[SHA1_DIGEST_SIZE];

   dwRqId = g_dwMsgId++;

   CalculateSHA1Hash((BYTE *)pszNewPassword, strlen(pszNewPassword), hash);

   msg.SetCode(CMD_SET_PASSWORD);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwUserId);
   msg.SetVariable(VID_PASSWORD, hash, SHA1_DIGEST_SIZE);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);
   return dwRetCode;
}
