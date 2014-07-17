/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: users.cpp
**
**/

#include "libnxcl.h"

/**
 * Fill user record with data from message
 */
void UpdateUserFromMessage(CSCPMessage *pMsg, NXC_USER *pUser)
{
   // Process common fields
   pUser->dwId = pMsg->GetVariableLong(VID_USER_ID);
   pMsg->GetVariableStr(VID_USER_NAME, pUser->szName, MAX_USER_NAME);
   pUser->wFlags = pMsg->GetVariableShort(VID_USER_FLAGS);
   pUser->dwSystemRights = pMsg->GetVariableInt64(VID_USER_SYS_RIGHTS);
   pMsg->GetVariableStr(VID_USER_DESCRIPTION, pUser->szDescription, MAX_USER_DESCR);
   pMsg->GetVariableBinary(VID_GUID, pUser->guid, UUID_LENGTH);

   // Process group-specific fields
   if (pUser->dwId & GROUP_FLAG)
   {
      UINT32 i, dwId;

      pUser->dwNumMembers = pMsg->GetVariableLong(VID_NUM_MEMBERS);
      pUser->pdwMemberList = (UINT32 *)realloc(pUser->pdwMemberList, sizeof(UINT32) * pUser->dwNumMembers);
      for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pUser->dwNumMembers; i++, dwId++)
         pUser->pdwMemberList[i] = pMsg->GetVariableLong(dwId);
		pUser->pszCertMappingData = NULL;
   }
   else     // User-specific data
   {
      pUser->nAuthMethod = pMsg->GetVariableShort(VID_AUTH_METHOD);
      pMsg->GetVariableStr(VID_USER_FULL_NAME, pUser->szFullName, MAX_USER_FULLNAME);
		pUser->nCertMappingMethod = pMsg->GetVariableShort(VID_CERT_MAPPING_METHOD);
		pUser->pszCertMappingData = pMsg->GetVariableStr(VID_CERT_MAPPING_DATA);
      pUser->pdwMemberList = NULL;
   }
}


//
// Load user database
// This function is NOT REENTRANT
//

UINT32 LIBNXCL_EXPORTABLE NXCLoadUserDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->LoadUserDB();
}


//
// Find user in database by ID
//

NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(NXC_SESSION hSession, UINT32 dwId)
{
   return ((NXCL_Session *)hSession)->FindUserById(dwId);
}


//
// Get pointer to user list and number of users
//

BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_SESSION hSession, NXC_USER **ppUserList,
                                     UINT32 *pdwNumUsers)
{
   return ((NXCL_Session *)hSession)->GetUserDB(ppUserList, pdwNumUsers);
}


//
// Create new user or group on server
//

UINT32 LIBNXCL_EXPORTABLE NXCCreateUser(NXC_SESSION hSession, TCHAR *pszName,
                                       BOOL bIsGroup, UINT32 *pdwNewId)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CREATE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_NAME, pszName);
   msg.SetVariable(VID_IS_GROUP, (WORD)bIsGroup);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwNewId = pResponse->GetVariableLong(VID_USER_ID);
      delete pResponse;
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

UINT32 LIBNXCL_EXPORTABLE NXCDeleteUser(NXC_SESSION hSession, UINT32 dwId)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Lock user database
//

UINT32 LIBNXCL_EXPORTABLE NXCLockUserDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_LOCK_USER_DB);
}


//
// Unlock user database
//

UINT32 LIBNXCL_EXPORTABLE NXCUnlockUserDB(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SimpleCommand(CMD_UNLOCK_USER_DB);
}


//
// Modify user record
//

UINT32 LIBNXCL_EXPORTABLE NXCModifyUser(NXC_SESSION hSession, NXC_USER *pUserInfo)
{
	return NXCModifyUserEx(hSession, pUserInfo, 0xFFFFFFFF);
}

UINT32 LIBNXCL_EXPORTABLE NXCModifyUserEx(NXC_SESSION hSession, NXC_USER *pUserInfo, UINT32 dwFields)
{
   CSCPMessage msg;
   UINT32 i, dwId, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Fill in request
   msg.SetCode(CMD_UPDATE_USER);
   msg.SetId(dwRqId);
	msg.SetVariable(VID_FIELDS, dwFields);
   msg.SetVariable(VID_USER_ID, pUserInfo->dwId);
   msg.SetVariable(VID_USER_NAME, pUserInfo->szName);
   msg.SetVariable(VID_USER_DESCRIPTION, pUserInfo->szDescription);
   msg.SetVariable(VID_USER_FLAGS, pUserInfo->wFlags);
   msg.SetVariable(VID_USER_SYS_RIGHTS, pUserInfo->dwSystemRights);

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
      msg.SetVariable(VID_AUTH_METHOD, (WORD)pUserInfo->nAuthMethod);
		msg.SetVariable(VID_CERT_MAPPING_METHOD, (WORD)pUserInfo->nCertMappingMethod);
		msg.SetVariable(VID_CERT_MAPPING_DATA, CHECK_NULL_EX(pUserInfo->pszCertMappingData));
   }

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Set password for user
//

UINT32 LIBNXCL_EXPORTABLE NXCSetPassword(NXC_SESSION hSession, UINT32 userId,
                                        const TCHAR *newPassword, const TCHAR *oldPassword)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_PASSWORD);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, userId);
   msg.SetVariable(VID_PASSWORD, newPassword);
	if (oldPassword != NULL)
	   msg.SetVariable(VID_OLD_PASSWORD, oldPassword);

	((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get user variable
//

UINT32 LIBNXCL_EXPORTABLE NXCGetUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                            TCHAR *pszVarName, TCHAR *pszValue, UINT32 dwSize)
{
   CSCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   if (dwUserId != CURRENT_USER)
      msg.SetVariable(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         pResponse->GetVariableStr(VID_VALUE, pszValue, dwSize);
      delete pResponse;
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

UINT32 LIBNXCL_EXPORTABLE NXCSetUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                            TCHAR *pszVarName, TCHAR *pszValue)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_SET_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   msg.SetVariable(VID_VALUE, pszValue);
   if (dwUserId != CURRENT_USER)
      msg.SetVariable(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Copy or move user variable
//

UINT32 LIBNXCL_EXPORTABLE NXCCopyUserVariable(NXC_SESSION hSession, UINT32 dwSrcUserId,
                                             UINT32 dwDstUserId, TCHAR *pszVarName,
                                             BOOL bMove)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_COPY_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   if (dwSrcUserId != CURRENT_USER)
      msg.SetVariable(VID_USER_ID, dwSrcUserId);
   msg.SetVariable(VID_DST_USER_ID, dwDstUserId);
   msg.SetVariable(VID_MOVE_FLAG, (WORD)bMove);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete user variable
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                               TCHAR *pszVarName)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_DELETE_USER_VARIABLE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_NAME, pszVarName);
   if (dwUserId != CURRENT_USER)
      msg.SetVariable(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Enumerate user variables
//

UINT32 LIBNXCL_EXPORTABLE NXCEnumUserVariables(NXC_SESSION hSession, UINT32 dwUserId,
                                              TCHAR *pszPattern, UINT32 *pdwNumVars,
                                              TCHAR ***pppszVarList)
{
   CSCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_ENUM_USER_VARIABLES);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SEARCH_PATTERN, pszPattern);
   if (dwUserId != CURRENT_USER)
      msg.SetVariable(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumVars = pResponse->GetVariableLong(VID_NUM_VARIABLES);
         if (*pdwNumVars > 0)
         {
            *pppszVarList = (TCHAR **)malloc(sizeof(TCHAR *) * (*pdwNumVars));
            for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++, dwId++)
               (*pppszVarList)[i] = pResponse->GetVariableStr(dwId);
         }
         else
         {
            *pppszVarList = NULL;
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   return dwResult;
}


//
// Get session list
//

UINT32 LIBNXCL_EXPORTABLE NXCGetSessionList(NXC_SESSION hSession, UINT32 *pdwNumSessions,
                                           NXC_CLIENT_SESSION_INFO **ppList)
{
   CSCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_SESSION_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumSessions = 0;
   *ppList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumSessions = pResponse->GetVariableLong(VID_NUM_SESSIONS);
         if (*pdwNumSessions > 0)
         {
            *ppList = (NXC_CLIENT_SESSION_INFO *)malloc(sizeof(NXC_CLIENT_SESSION_INFO) * (*pdwNumSessions));
            for(i = 0; i < *pdwNumSessions; i++)
            {
               dwId = i * 100;
               (*ppList)[i].dwSessionId = pResponse->GetVariableLong(dwId++);
               (*ppList)[i].nCipher = pResponse->GetVariableShort(dwId++);
               pResponse->GetVariableStr(dwId++, (*ppList)[i].szUserName, MAX_USER_NAME);
               pResponse->GetVariableStr(dwId++, (*ppList)[i].szClientApp, MAX_DB_STRING);
            }
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }

   return dwResult;
}


//
// Forcibly close client session
//

UINT32 LIBNXCL_EXPORTABLE NXCKillSession(NXC_SESSION hSession, UINT32 dwSessionId)
{
   CSCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_KILL_SESSION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_SESSION_ID, dwSessionId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}

/**
 * Get Id of currently logged in user
 */
UINT32 LIBNXCL_EXPORTABLE NXCGetCurrentUserId(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->getCurrentUserId();
}

/**
 * Get system access rights of currently logged in user
 */
UINT64 LIBNXCL_EXPORTABLE NXCGetCurrentSystemAccess(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->getCurrentSystemAccess();
}
