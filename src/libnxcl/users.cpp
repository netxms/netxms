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
void UpdateUserFromMessage(NXCPMessage *pMsg, NXC_USER *pUser)
{
   // Process common fields
   pUser->dwId = pMsg->getFieldAsUInt32(VID_USER_ID);
   pMsg->getFieldAsString(VID_USER_NAME, pUser->szName, MAX_USER_NAME);
   pUser->flags = pMsg->getFieldAsUInt16(VID_USER_FLAGS);
   pUser->dwSystemRights = pMsg->getFieldAsUInt64(VID_USER_SYS_RIGHTS);
   pMsg->getFieldAsString(VID_USER_DESCRIPTION, pUser->szDescription, MAX_USER_DESCR);
   pMsg->getFieldAsBinary(VID_GUID, pUser->guid, UUID_LENGTH);

   // Process group-specific fields
   if (pUser->dwId & GROUP_FLAG)
   {
      UINT32 i, dwId;

      pUser->dwNumMembers = pMsg->getFieldAsUInt32(VID_NUM_MEMBERS);
      pUser->pdwMemberList = (UINT32 *)realloc(pUser->pdwMemberList, sizeof(UINT32) * pUser->dwNumMembers);
      for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pUser->dwNumMembers; i++, dwId++)
         pUser->pdwMemberList[i] = pMsg->getFieldAsUInt32(dwId);
		pUser->pszCertMappingData = NULL;
   }
   else     // User-specific data
   {
      pUser->nAuthMethod = pMsg->getFieldAsUInt16(VID_AUTH_METHOD);
      pMsg->getFieldAsString(VID_USER_FULL_NAME, pUser->szFullName, MAX_USER_FULLNAME);
		pUser->nCertMappingMethod = pMsg->getFieldAsUInt16(VID_CERT_MAPPING_METHOD);
		pUser->pszCertMappingData = pMsg->getFieldAsString(VID_CERT_MAPPING_DATA);
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
   NXCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_CREATE_USER);
   msg.setId(dwRqId);
   msg.setField(VID_USER_NAME, pszName);
   msg.setField(VID_IS_GROUP, (WORD)bIsGroup);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwNewId = pResponse->getFieldAsUInt32(VID_USER_ID);
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_USER);
   msg.setId(dwRqId);
   msg.setField(VID_USER_ID, dwId);
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
   NXCPMessage msg;
   UINT32 i, dwId, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Fill in request
   msg.setCode(CMD_UPDATE_USER);
   msg.setId(dwRqId);
	msg.setField(VID_FIELDS, dwFields);
   msg.setField(VID_USER_ID, pUserInfo->dwId);
   msg.setField(VID_USER_NAME, pUserInfo->szName);
   msg.setField(VID_USER_DESCRIPTION, pUserInfo->szDescription);
   msg.setField(VID_USER_FLAGS, pUserInfo->flags);
   msg.setField(VID_USER_SYS_RIGHTS, pUserInfo->dwSystemRights);

   // Group-specific fields
   if (pUserInfo->dwId & GROUP_FLAG)
   {
      msg.setField(VID_NUM_MEMBERS, pUserInfo->dwNumMembers);
      for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pUserInfo->dwNumMembers; i++, dwId++)
         msg.setField(dwId, pUserInfo->pdwMemberList[i]);
   }
   else     // User-specific fields
   {
      msg.setField(VID_USER_FULL_NAME, pUserInfo->szFullName);
      msg.setField(VID_AUTH_METHOD, (WORD)pUserInfo->nAuthMethod);
		msg.setField(VID_CERT_MAPPING_METHOD, (WORD)pUserInfo->nCertMappingMethod);
		msg.setField(VID_CERT_MAPPING_DATA, CHECK_NULL_EX(pUserInfo->pszCertMappingData));
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SET_PASSWORD);
   msg.setId(dwRqId);
   msg.setField(VID_USER_ID, userId);
   msg.setField(VID_PASSWORD, newPassword);
	if (oldPassword != NULL)
	   msg.setField(VID_OLD_PASSWORD, oldPassword);

	((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get user variable
//

UINT32 LIBNXCL_EXPORTABLE NXCGetUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                            TCHAR *pszVarName, TCHAR *pszValue, UINT32 dwSize)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_USER_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   if (dwUserId != CURRENT_USER)
      msg.setField(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         pResponse->getFieldAsString(VID_VALUE, pszValue, dwSize);
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SET_USER_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   msg.setField(VID_VALUE, pszValue);
   if (dwUserId != CURRENT_USER)
      msg.setField(VID_USER_ID, dwUserId);
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_COPY_USER_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   if (dwSrcUserId != CURRENT_USER)
      msg.setField(VID_USER_ID, dwSrcUserId);
   msg.setField(VID_DST_USER_ID, dwDstUserId);
   msg.setField(VID_MOVE_FLAG, (WORD)bMove);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Delete user variable
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteUserVariable(NXC_SESSION hSession, UINT32 dwUserId,
                                               TCHAR *pszVarName)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_USER_VARIABLE);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, pszVarName);
   if (dwUserId != CURRENT_USER)
      msg.setField(VID_USER_ID, dwUserId);
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
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_ENUM_USER_VARIABLES);
   msg.setId(dwRqId);
   msg.setField(VID_SEARCH_PATTERN, pszPattern);
   if (dwUserId != CURRENT_USER)
      msg.setField(VID_USER_ID, dwUserId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumVars = pResponse->getFieldAsUInt32(VID_NUM_VARIABLES);
         if (*pdwNumVars > 0)
         {
            *pppszVarList = (TCHAR **)malloc(sizeof(TCHAR *) * (*pdwNumVars));
            for(i = 0, dwId = VID_VARLIST_BASE; i < *pdwNumVars; i++, dwId++)
               (*pppszVarList)[i] = pResponse->getFieldAsString(dwId);
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
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_SESSION_LIST);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *pdwNumSessions = 0;
   *ppList = NULL;

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumSessions = pResponse->getFieldAsUInt32(VID_NUM_SESSIONS);
         if (*pdwNumSessions > 0)
         {
            *ppList = (NXC_CLIENT_SESSION_INFO *)malloc(sizeof(NXC_CLIENT_SESSION_INFO) * (*pdwNumSessions));
            for(i = 0; i < *pdwNumSessions; i++)
            {
               dwId = i * 100;
               (*ppList)[i].dwSessionId = pResponse->getFieldAsUInt32(dwId++);
               (*ppList)[i].nCipher = pResponse->getFieldAsUInt16(dwId++);
               pResponse->getFieldAsString(dwId++, (*ppList)[i].szUserName, MAX_USER_NAME);
               pResponse->getFieldAsString(dwId++, (*ppList)[i].szClientApp, MAX_DB_STRING);
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
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_KILL_SESSION);
   msg.setId(dwRqId);
   msg.setField(VID_SESSION_ID, dwSessionId);
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
