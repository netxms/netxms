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
// Fill user record with data from message
//

void UpdateUserFromMessage(CSCPMessage *pMsg, NXC_USER *pUser)
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
      pUser->pdwMemberList = NULL;
   }
}


//
// Load user database
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->LoadUserDB();
}


//
// Find user in database by ID
//

NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(NXC_SESSION hSession, DWORD dwId)
{
   return ((NXCL_Session *)hSession)->FindUserById(dwId);
}


//
// Get pointer to user list and number of users
//

BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_SESSION hSession, NXC_USER **ppUserList, 
                                     DWORD *pdwNumUsers)
{
   return ((NXCL_Session *)hSession)->GetUserDB(ppUserList, pdwNumUsers);
}


//
// Create new user or group on server
//

DWORD LIBNXCL_EXPORTABLE NXCCreateUser(NXC_SESSION hSession, TCHAR *pszName,
                                       BOOL bIsGroup, DWORD *pdwNewId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CREATE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_NAME, pszName);
   msg.SetVariable(VID_IS_GROUP, (WORD)bIsGroup);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
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

DWORD LIBNXCL_EXPORTABLE NXCDeleteUser(NXC_SESSION hSession, DWORD dwId)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Lock/unlock user database
//

static DWORD LockUserDB(NXCL_Session *pSession, BOOL bLock)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = pSession->CreateRqId();

   msg.SetCode(bLock ? CMD_LOCK_USER_DB : CMD_UNLOCK_USER_DB);
   msg.SetId(dwRqId);
   pSession->SendMsg(&msg);

   return pSession->WaitForRCC(dwRqId);
}


//
// Client interface: lock user database
//

DWORD LIBNXCL_EXPORTABLE NXCLockUserDB(NXC_SESSION hSession)
{
   return LockUserDB((NXCL_Session *)hSession, TRUE);
}


//
// Client interface: unlock user database
//

DWORD LIBNXCL_EXPORTABLE NXCUnlockUserDB(NXC_SESSION hSession)
{
   return LockUserDB((NXCL_Session *)hSession, FALSE);
}


//
// Modify user record
//

DWORD LIBNXCL_EXPORTABLE NXCModifyUser(NXC_SESSION hSession, NXC_USER *pUserInfo)
{
   CSCPMessage msg;
   DWORD i, dwId, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

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

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for responce
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Set password for user
//

DWORD LIBNXCL_EXPORTABLE NXCSetPassword(NXC_SESSION hSession, DWORD dwUserId, 
                                        char *pszNewPassword)
{
   CSCPMessage msg;
   DWORD dwRqId;
   BYTE hash[SHA1_DIGEST_SIZE];

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   CalculateSHA1Hash((BYTE *)pszNewPassword, strlen(pszNewPassword), hash);

   msg.SetCode(CMD_SET_PASSWORD);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwUserId);
   msg.SetVariable(VID_PASSWORD, hash, SHA1_DIGEST_SIZE);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get user variable
//

DWORD LIBNXCL_EXPORTABLE NXCGetUserVariable(NXC_SESSION hSession, TCHAR *pszVarName,
                                            TCHAR *pszValue, DWORD dwSize)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponce != NULL)
   {
      dwResult = pResponce->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         pResponce->GetVariableStr(VID_VALUE, pszValue, dwSize);
      delete pResponce;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   return dwResult;
}


//
// Set user variable
//

DWORD LIBNXCL_EXPORTABLE NXCSetUserVariable(NXC_SESSION hSession, TCHAR *pszVarName, TCHAR *pszValue)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   msg.SetVariable(VID_VALUE, pszValue);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete user variable
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteUserVariable(NXC_SESSION hSession, TCHAR *pszVarName)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Enumerate user variables
//

DWORD LIBNXCL_EXPORTABLE NXCEnumUserVariables(NXC_SESSION hSession, TCHAR *pszPattern,
                                              DWORD *pdwNumVars, TCHAR ***pppszVarList)
{
   CSCPMessage msg, *pResponce;
   DWORD i, dwId, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_ENUM_USER_VARIABLES);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SEARCH_PATTERN, pszPattern);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponce = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponce != NULL)
   {
      dwResult = pResponce->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumVars = pResponce->GetVariableLong(VID_NUM_VARIABLES);
         if (*pdwNumVars > 0)
         {
            *pppszVarList = (TCHAR **)malloc(sizeof(TCHAR *) * (*pdwNumVars));
            for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++, dwId++)
               (*pppszVarList)[i] = pResponce->GetVariableStr(dwId);
         }
      }
      delete pResponce;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   return dwResult;
}
