/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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

#include "nxcore.h"


//
// Externals
//

int RadiusAuth(char *pszLogin, char *pszPasswd);


//
// Global variables
//

NMS_USER *g_pUserList = NULL;
DWORD g_dwNumUsers = 0;
NMS_USER_GROUP *g_pGroupList = NULL;
DWORD g_dwNumGroups = 0;


//
// Static data
//

static MUTEX m_hMutexUserAccess;
static MUTEX m_hMutexGroupAccess;


//
// Initialize user handling subsystem
//

void InitUsers(void)
{
   m_hMutexUserAccess = MutexCreate();
   m_hMutexGroupAccess = MutexCreate();
}


//
// Load user list from database
//

BOOL LoadUsers(void)
{
   DB_RESULT hResult;
   DWORD i, iNumRows;
   TCHAR szBuffer[256];

   // Load users
   hResult = DBSelect(g_hCoreDB, _T("SELECT id,name,password,system_access,flags,full_name,description,grace_logins,auth_method,guid FROM users ORDER BY id"));
   if (hResult == NULL)
      return FALSE;

   g_dwNumUsers = DBGetNumRows(hResult);
   g_pUserList = (NMS_USER *)malloc(sizeof(NMS_USER) * g_dwNumUsers);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      g_pUserList[i].dwId = DBGetFieldULong(hResult, i, 0);
      DBGetField(hResult, i, 1, g_pUserList[i].szName, MAX_USER_NAME);
      if (StrToBin(DBGetField(hResult, i, 2, szBuffer, 256), g_pUserList[i].szPassword, SHA1_DIGEST_SIZE) != SHA1_DIGEST_SIZE)
      {
         WriteLog(MSG_INVALID_SHA1_HASH, EVENTLOG_WARNING_TYPE, "s", g_pUserList[i].szName);
         CalculateSHA1Hash((BYTE *)"netxms", 6, g_pUserList[i].szPassword);
      }
      if (g_pUserList[i].dwId == 0)
         g_pUserList[i].wSystemRights = SYSTEM_ACCESS_FULL;    // Ignore database value for superuser
      else
         g_pUserList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 3);
      g_pUserList[i].wFlags = (WORD)DBGetFieldLong(hResult, i, 4);
      DBGetField(hResult, i, 5, g_pUserList[i].szFullName, MAX_USER_FULLNAME);
      DBGetField(hResult, i, 6, g_pUserList[i].szDescription, MAX_USER_DESCR);
      g_pUserList[i].nGraceLogins = DBGetFieldLong(hResult, i, 7);
      g_pUserList[i].nAuthMethod = DBGetFieldLong(hResult, i, 8);
      DBGetFieldGUID(hResult, i, 9, g_pUserList[i].guid);
   }

   DBFreeResult(hResult);

   // Check if user with UID 0 was loaded
   for(i = 0; i < g_dwNumUsers; i++)
      if (g_pUserList[i].dwId == 0)
         break;
   // Create superuser account if it doesn't exist
   if (i == g_dwNumUsers)
   {
      g_dwNumUsers++;
      g_pUserList = (NMS_USER *)realloc(g_pUserList, sizeof(NMS_USER) * g_dwNumUsers);
      g_pUserList[i].dwId = 0;
      strcpy(g_pUserList[i].szName, "admin");
      g_pUserList[i].wFlags = UF_MODIFIED | UF_CHANGE_PASSWORD;
      g_pUserList[i].wSystemRights = SYSTEM_ACCESS_FULL;
      g_pUserList[i].szFullName[0] = 0;
      strcpy(g_pUserList[i].szDescription, "Built-in system administrator account");
      CalculateSHA1Hash((BYTE *)"netxms", 6, g_pUserList[i].szPassword);
      g_pUserList[i].nGraceLogins = MAX_GRACE_LOGINS;
      g_pUserList[i].nAuthMethod = AUTH_NETXMS_PASSWORD;
      uuid_generate(g_pUserList[i].guid);
      WriteLog(MSG_SUPERUSER_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   // Load groups
   hResult = DBSelect(g_hCoreDB, "SELECT id,name,system_access,flags,description,guid FROM user_groups ORDER BY id");
   if (hResult == 0)
      return FALSE;

   g_dwNumGroups = DBGetNumRows(hResult);
   g_pGroupList = (NMS_USER_GROUP *)malloc(sizeof(NMS_USER_GROUP) * g_dwNumGroups);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      g_pGroupList[i].dwId = DBGetFieldULong(hResult, i, 0);
      DBGetField(hResult, i, 1, g_pGroupList[i].szName, MAX_USER_NAME);
      g_pGroupList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 2);
      g_pGroupList[i].wFlags = (WORD)DBGetFieldLong(hResult, i, 3);
      DBGetField(hResult, i, 4, g_pGroupList[i].szDescription, MAX_USER_DESCR);
      DBGetFieldGUID(hResult, i, 5, g_pGroupList[i].guid);
      g_pGroupList[i].dwNumMembers = 0;
      g_pGroupList[i].pMembers = NULL;
   }

   DBFreeResult(hResult);

   // Check if everyone group was loaded
   for(i = 0; i < g_dwNumGroups; i++)
      if (g_pGroupList[i].dwId == GROUP_EVERYONE)
         break;
   // Create everyone group if it doesn't exist
   if (i == g_dwNumGroups)
   {
      g_dwNumGroups++;
      g_pGroupList = (NMS_USER_GROUP *)realloc(g_pGroupList, sizeof(NMS_USER_GROUP) * g_dwNumGroups);
      g_pGroupList[i].dwId = GROUP_EVERYONE;
      g_pGroupList[i].dwNumMembers = 0;
      g_pGroupList[i].pMembers = NULL;
      strcpy(g_pGroupList[i].szName, "Everyone");
      g_pGroupList[i].wFlags = UF_MODIFIED;
      g_pGroupList[i].wSystemRights = 0;
      strcpy(g_pGroupList[i].szDescription, "Built-in everyone group");
      WriteLog(MSG_EVERYONE_GROUP_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   // Add users to groups
   hResult = DBSelect(g_hCoreDB, "SELECT user_id,group_id FROM user_group_members");
   if (hResult == NULL)
      return FALSE;

   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
      AddUserToGroup(DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));
   DBFreeResult(hResult);

   return TRUE;
}


//
// Save user list to database
//

void SaveUsers(DB_HANDLE hdb)
{
   DWORD i;
   char szQuery[1024], szPassword[SHA1_DIGEST_SIZE * 2 + 1], szGUID[64];

   // Save users
   MutexLock(m_hMutexUserAccess, INFINITE);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      if (g_pUserList[i].wFlags & UF_DELETED)
      {
         // Delete user record from database
         sprintf(szQuery, "DELETE FROM users WHERE id=%d", g_pUserList[i].dwId);
         DBQuery(hdb, szQuery);
         sprintf(szQuery, "DELETE FROM user_profiles WHERE user_id=%d", g_pUserList[i].dwId);
         DBQuery(hdb, szQuery);

         // Delete user record from memory
         g_dwNumUsers--;
         memmove(&g_pUserList[i], &g_pUserList[i + 1], sizeof(NMS_USER) * (g_dwNumUsers - i));
         i--;
      }
      else if (g_pUserList[i].wFlags & UF_MODIFIED)
      {
         DB_RESULT hResult;
         BOOL bUserExists = FALSE;

         // Clear modification flag
         g_pUserList[i].wFlags &= ~UF_MODIFIED;

         // Check if user record exists in database
         sprintf(szQuery, "SELECT name FROM users WHERE id=%d", g_pUserList[i].dwId);
         hResult = DBSelect(hdb, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bUserExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         BinToStr(g_pUserList[i].szPassword, SHA1_DIGEST_SIZE, szPassword);
         if (bUserExists)
            sprintf(szQuery, "UPDATE users SET name='%s',password='%s',system_access=%d,flags=%d,"
                             "full_name='%s',description='%s',grace_logins=%d,guid='%s',auth_method=%d WHERE id=%d",
                    g_pUserList[i].szName, szPassword, g_pUserList[i].wSystemRights,
                    g_pUserList[i].wFlags, g_pUserList[i].szFullName,
                    g_pUserList[i].szDescription, g_pUserList[i].nGraceLogins,
                    uuid_to_string(g_pUserList[i].guid, szGUID),
                    g_pUserList[i].nAuthMethod, g_pUserList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO users (id,name,password,system_access,flags,full_name,description,grace_logins,guid,auth_method) "
                             "VALUES (%d,'%s','%s',%d,%d,'%s','%s',%d,'%s',%d)",
                    g_pUserList[i].dwId, g_pUserList[i].szName, szPassword,
                    g_pUserList[i].wSystemRights, g_pUserList[i].wFlags,
                    g_pUserList[i].szFullName, g_pUserList[i].szDescription,
                    g_pUserList[i].nGraceLogins, uuid_to_string(g_pUserList[i].guid, szGUID),
                    g_pUserList[i].nAuthMethod);
         DBQuery(hdb, szQuery);
      }
   }
   MutexUnlock(m_hMutexUserAccess);

   // Save groups
   MutexLock(m_hMutexGroupAccess, INFINITE);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      if (g_pGroupList[i].wFlags & UF_DELETED)
      {
         // Delete group record from database
         sprintf(szQuery, "DELETE FROM user_groups WHERE id=%d", g_pGroupList[i].dwId);
         DBQuery(hdb, szQuery);
         sprintf(szQuery, "DELETE FROM user_group_members WHERE group_id=%d", g_pGroupList[i].dwId);
         DBQuery(hdb, szQuery);

         // Delete group record from memory
         if (g_pGroupList[i].pMembers != NULL)
            free(g_pGroupList[i].pMembers);
         g_dwNumGroups--;
         memmove(&g_pGroupList[i], &g_pGroupList[i + 1], sizeof(NMS_USER_GROUP) * (g_dwNumGroups - i));
         i--;
      }
      else if (g_pGroupList[i].wFlags & UF_MODIFIED)
      {
         DB_RESULT hResult;
         BOOL bGroupExists = FALSE;

         // Clear modification flag
         g_pGroupList[i].wFlags &= ~UF_MODIFIED;

         // Check if group record exists in database
         sprintf(szQuery, "SELECT name FROM user_groups WHERE id=%d", g_pGroupList[i].dwId);
         hResult = DBSelect(hdb, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bGroupExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         if (bGroupExists)
            sprintf(szQuery, "UPDATE user_groups SET name='%s',system_access=%d,flags=%d,"
                             "description='%s',guid='%s' WHERE id=%d",
                    g_pGroupList[i].szName, g_pGroupList[i].wSystemRights,
                    g_pGroupList[i].wFlags, g_pGroupList[i].szDescription,
                    uuid_to_string(g_pGroupList[i].guid, szGUID), g_pGroupList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO user_groups (id,name,system_access,flags,description,guid) "
                             "VALUES (%d,'%s',%d,%d,'%s','%s')",
                    g_pGroupList[i].dwId, g_pGroupList[i].szName, g_pGroupList[i].wSystemRights,
                    g_pGroupList[i].wFlags, g_pGroupList[i].szDescription,
                    uuid_to_string(g_pGroupList[i].guid, szGUID));
         DBBegin(hdb);
         DBQuery(hdb, szQuery);

         if (bGroupExists)
         {
            sprintf(szQuery, "DELETE FROM user_group_members WHERE group_id=%d", g_pGroupList[i].dwId);
            DBQuery(hdb, szQuery);
         }

         for(DWORD j = 0; j < g_pGroupList[i].dwNumMembers; j++)
         {
            sprintf(szQuery, "INSERT INTO user_group_members (group_id, user_id) VALUES (%d, %d)",
                    g_pGroupList[i].dwId, g_pGroupList[i].pMembers[j]);
            DBQuery(hdb, szQuery);
         }
         DBCommit(hdb);
      }
   }
   MutexUnlock(m_hMutexGroupAccess);
}


//
// Authenticate user
// Checks if provided login name and password are correct, and returns RCC_SUCCESS
// on success and appropriate RCC otherwise. On success authentication, user's ID is stored
// int pdwId. If password authentication is used, dwSigLen should be set to zero.
//

DWORD AuthenticateUser(TCHAR *pszName, TCHAR *pszPassword,
							  DWORD dwSigLen, void *pCert, BYTE *pChallenge,
							  DWORD *pdwId, DWORD *pdwSystemRights,
							  BOOL *pbChangePasswd)
{
   DWORD i, j;
   DWORD dwResult = RCC_ACCESS_DENIED;
   BOOL bPasswordValid;
   BYTE hash[SHA1_DIGEST_SIZE];

   MutexLock(m_hMutexUserAccess, INFINITE);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      if ((!strcmp(pszName, g_pUserList[i].szName)) &&
          (!(g_pUserList[i].wFlags & UF_DELETED)))
      {
         switch(g_pUserList[i].nAuthMethod)
         {
            case AUTH_NETXMS_PASSWORD:
					if (dwSigLen == 0)
					{
						CalculateSHA1Hash((BYTE *)pszPassword, strlen(pszPassword), hash);
						bPasswordValid = !memcmp(hash, g_pUserList[i].szPassword, SHA1_DIGEST_SIZE);
					}
					else
					{
						// We got certificate instead of password
						bPasswordValid = FALSE;
					}
               break;
            case AUTH_RADIUS:
					if (dwSigLen == 0)
					{
	               bPasswordValid = (RadiusAuth(pszName, pszPassword) == 0);
					}
					else
					{
						// We got certificate instead of password
						bPasswordValid = FALSE;
					}
               break;
				case AUTH_CERTIFICATE:
					if ((dwSigLen != 0) && (pCert != NULL))
					{
#ifdef _WITH_ENCRYPTION
						bPasswordValid = ValidateUserCertificate((X509 *)pCert, pszName, pChallenge,
						                                         (BYTE *)pszPassword, dwSigLen);
#else
						bPasswordValid = FALSE;
#endif
					}
					else
					{
						// We got password instead of certificate
						bPasswordValid = FALSE;
					}
					break;
            default:
               WriteLog(MSG_UNKNOWN_AUTH_METHOD, EVENTLOG_WARNING_TYPE, "ds",
                        g_pUserList[i].nAuthMethod, pszName);
               bPasswordValid = FALSE;
               break;
         }

         if (bPasswordValid)
         {
            if (!(g_pUserList[i].wFlags & UF_DISABLED))
            {
               if (g_pUserList[i].wFlags & UF_CHANGE_PASSWORD)
               {
                  if (g_pUserList[i].nGraceLogins <= 0)
                  {
                     dwResult = RCC_NO_GRACE_LOGINS;
                     break;
                  }
                  g_pUserList[i].nGraceLogins--;
                  *pbChangePasswd = TRUE;
               }
               else
               {
                  *pbChangePasswd = FALSE;
               }
               *pdwId = g_pUserList[i].dwId;
               *pdwSystemRights = (DWORD)g_pUserList[i].wSystemRights;
               dwResult = RCC_SUCCESS;
         
               // Collect system rights from groups this user belongs to
               MutexLock(m_hMutexGroupAccess, INFINITE);
               for(j = 0; j < g_dwNumGroups; j++)
                  if (CheckUserMembership(g_pUserList[i].dwId, g_pGroupList[j].dwId))
                     *pdwSystemRights |= (DWORD)g_pGroupList[j].wSystemRights;
               MutexUnlock(m_hMutexGroupAccess);
            }
            else
            {
               dwResult = RCC_ACCOUNT_DISABLED;
            }
         }
         break;
      }
   }
   MutexUnlock(m_hMutexUserAccess);
   return dwResult;
}


//
// Add user to group
//

void AddUserToGroup(DWORD dwUserId, DWORD dwGroupId)
{
   DWORD i;

   MutexLock(m_hMutexGroupAccess, INFINITE);

   // Find group
   for(i = 0; i < g_dwNumGroups; i++)
      if (g_pGroupList[i].dwId == dwGroupId)
         break;

   if (i != g_dwNumGroups)
   {
      DWORD j;

      // Check if user already in group
      for(j = 0; j < g_pGroupList[i].dwNumMembers; j++)
         if (g_pGroupList[i].pMembers[j] == dwUserId)
            break;

      // If not in group, add it
      if (j == g_pGroupList[i].dwNumMembers)
      {
         g_pGroupList[i].dwNumMembers++;
         g_pGroupList[i].pMembers = (DWORD *)realloc(g_pGroupList[i].pMembers,
                                         sizeof(DWORD) * g_pGroupList[i].dwNumMembers);
         g_pGroupList[i].pMembers[j] = dwUserId;
      }
   }

   MutexUnlock(m_hMutexGroupAccess);
}


//
// Delete user from group
//

static void DeleteUserFromGroup(NMS_USER_GROUP *pGroup, DWORD dwUserId)
{
   DWORD i;

   for(i = 0; i < pGroup->dwNumMembers; i++)
      if (pGroup->pMembers[i] == dwUserId)
      {
         pGroup->dwNumMembers--;
         memmove(&pGroup->pMembers[i], &pGroup->pMembers[i + 1], sizeof(DWORD) * (pGroup->dwNumMembers - i));
      }
}


//
// Check if user is a member of specific group
//

BOOL CheckUserMembership(DWORD dwUserId, DWORD dwGroupId)
{
   BOOL bResult = FALSE;

   MutexLock(m_hMutexGroupAccess, INFINITE);
   if (dwGroupId == GROUP_EVERYONE)
   {
      bResult = TRUE;
   }
   else
   {
      DWORD i, j;

      // Find group
      for(i = 0; i < g_dwNumGroups; i++)
         if (g_pGroupList[i].dwId == dwGroupId)
         {
            // Walk through group members
            for(j = 0; j < g_pGroupList[i].dwNumMembers; j++)
               if (g_pGroupList[i].pMembers[j] == dwUserId)
               {
                  bResult = TRUE;
                  break;
               }
            break;
         }
   }
   MutexUnlock(m_hMutexGroupAccess);
   return bResult;
}


//
// Dump user list to stdout
//

void DumpUsers(CONSOLE_CTX pCtx)
{
   DWORD i;
   char szGUID[64];

   ConsolePrintf(pCtx, "Login name           GUID                                 System rights\n"
                       "-----------------------------------------------------------------------\n");
   for(i = 0; i < g_dwNumUsers; i++)
      ConsolePrintf(pCtx, "%-20s %-36s 0x%08X\n", g_pUserList[i].szName,
                    uuid_to_string(g_pUserList[i].guid, szGUID), g_pUserList[i].wSystemRights);
   ConsolePrintf(pCtx, "\n");
}


//
// Delete user or group
// Will return RCC code
//

DWORD DeleteUserFromDB(DWORD dwId)
{
   DWORD i, j;

   DeleteUserFromAllObjects(dwId);

   if (dwId & GROUP_FLAG)
   {
      MutexLock(m_hMutexGroupAccess, INFINITE);

      // Find group
      for(i = 0; i < g_dwNumGroups; i++)
         if (g_pGroupList[i].dwId == dwId)
         {
            g_pGroupList[i].dwNumMembers = 0;
            safe_free(g_pGroupList[i].pMembers);
            g_pGroupList[i].pMembers = NULL;
            g_pGroupList[i].wFlags |= UF_DELETED;
            break;
         }

      MutexUnlock(m_hMutexGroupAccess);
   }
   else
   {
      MutexLock(m_hMutexUserAccess, INFINITE);
      
      // Find user
      for(i = 0; i < g_dwNumUsers; i++)
         if (g_pUserList[i].dwId == dwId)
         {
            g_pUserList[i].wFlags |= UF_DELETED;
            
            // Delete this user from all groups
            MutexLock(m_hMutexGroupAccess, INFINITE);
            for(j = 0; j < g_dwNumGroups; j++)
               DeleteUserFromGroup(&g_pGroupList[j], dwId);
            MutexUnlock(m_hMutexGroupAccess);
            break;
         }

      MutexUnlock(m_hMutexUserAccess);
   }

   SendUserDBUpdate(USER_DB_DELETE, dwId, NULL, NULL);

   return RCC_SUCCESS;
}


//
// Create new user or group
//

DWORD CreateNewUser(char *pszName, BOOL bIsGroup, DWORD *pdwId)
{
   DWORD i, dwResult = RCC_SUCCESS;
   DWORD dwNewId;

   if (bIsGroup)
   {
      MutexLock(m_hMutexGroupAccess, INFINITE);

      // Check for duplicate name
      for(i = 0; i < g_dwNumGroups; i++)
         if (!stricmp(g_pGroupList[i].szName, pszName))
         {
            dwResult = RCC_ALREADY_EXIST;
            break;
         }

      // If not exist, create it
      if (i == g_dwNumGroups)
      {
         g_dwNumGroups++;
         g_pGroupList = (NMS_USER_GROUP *)realloc(g_pGroupList, sizeof(NMS_USER_GROUP) * g_dwNumGroups);
         g_pGroupList[i].dwId = dwNewId = CreateUniqueId(IDG_USER_GROUP);
         g_pGroupList[i].dwNumMembers = 0;
         g_pGroupList[i].pMembers = NULL;
         nx_strncpy(g_pGroupList[i].szName, pszName, MAX_USER_NAME);
         g_pGroupList[i].wFlags = UF_MODIFIED;
         g_pGroupList[i].wSystemRights = 0;
         g_pGroupList[i].szDescription[0] = 0;
         uuid_generate(g_pGroupList[i].guid);
      }

      if (dwResult == RCC_SUCCESS)
         SendUserDBUpdate(USER_DB_CREATE, dwNewId, NULL, &g_pGroupList[i]);

      MutexUnlock(m_hMutexGroupAccess);
   }
   else
   {
      MutexLock(m_hMutexUserAccess, INFINITE);

      // Check for duplicate name
      for(i = 0; i < g_dwNumUsers; i++)
         if (!stricmp(g_pUserList[i].szName, pszName))
         {
            dwResult = RCC_ALREADY_EXIST;
            break;
         }

      // If not exist, create it
      if (i == g_dwNumUsers)
      {
         g_dwNumUsers++;
         g_pUserList = (NMS_USER *)realloc(g_pUserList, sizeof(NMS_USER) * g_dwNumUsers);
         g_pUserList[i].dwId = dwNewId = CreateUniqueId(IDG_USER);
         nx_strncpy(g_pUserList[i].szName, pszName, MAX_USER_NAME);
         g_pUserList[i].wFlags = UF_MODIFIED;
         g_pUserList[i].wSystemRights = 0;
         g_pUserList[i].szFullName[0] = 0;
         g_pUserList[i].szDescription[0] = 0;
         g_pUserList[i].nGraceLogins = MAX_GRACE_LOGINS;
         uuid_generate(g_pUserList[i].guid);
      }

      if (dwResult == RCC_SUCCESS)
         SendUserDBUpdate(USER_DB_CREATE, dwNewId, &g_pUserList[i], NULL);

      MutexUnlock(m_hMutexUserAccess);
   }

   *pdwId = dwNewId;

   return dwResult;
}


//
// Modify user record
//

DWORD ModifyUser(NMS_USER *pUserInfo)
{
   DWORD i, dwResult = RCC_INVALID_USER_ID;

   MutexLock(m_hMutexUserAccess, INFINITE);

   // Find user to be modified in list
   for(i = 0; i < g_dwNumUsers; i++)
      if (g_pUserList[i].dwId == pUserInfo->dwId)
      {
         if (!IsValidObjectName(pUserInfo->szName))
         {
            dwResult = RCC_INVALID_OBJECT_NAME;
            break;
         }

         strcpy(g_pUserList[i].szName, pUserInfo->szName);
         strcpy(g_pUserList[i].szFullName, pUserInfo->szFullName);
         strcpy(g_pUserList[i].szDescription, pUserInfo->szDescription);
         g_pUserList[i].nAuthMethod = pUserInfo->nAuthMethod;
         
         // We will not replace system access rights for UID 0
         if (g_pUserList[i].dwId != 0)
            g_pUserList[i].wSystemRights = pUserInfo->wSystemRights;

         // Modify UF_DISABLED and UF_CHANGE_PASSWORD flags from pUserInfo
         // Ignore DISABLED flag for UID 0
         g_pUserList[i].wFlags &= ~(UF_DISABLED | UF_CHANGE_PASSWORD);
         if (g_pUserList[i].dwId == 0)
            g_pUserList[i].wFlags |= pUserInfo->wFlags & UF_CHANGE_PASSWORD;
         else
            g_pUserList[i].wFlags |= pUserInfo->wFlags & (UF_DISABLED | UF_CHANGE_PASSWORD);

         // Mark user record as modified and notify clients
         g_pUserList[i].wFlags |= UF_MODIFIED;
         SendUserDBUpdate(USER_DB_MODIFY, g_pUserList[i].dwId, &g_pUserList[i], NULL);
         dwResult = RCC_SUCCESS;
         break;
      }

   MutexUnlock(m_hMutexUserAccess);
   return dwResult;
}


//
// Modify group record
//

DWORD ModifyGroup(NMS_USER_GROUP *pGroupInfo)
{
   DWORD i, dwResult = RCC_INVALID_USER_ID;

   MutexLock(m_hMutexGroupAccess, INFINITE);

   // Find group in list
   for(i = 0; i < g_dwNumGroups; i++)
      if (g_pGroupList[i].dwId == pGroupInfo->dwId)
      {
         if (!IsValidObjectName(pGroupInfo->szName))
         {
            dwResult = RCC_INVALID_OBJECT_NAME;
            break;
         }

         strcpy(g_pGroupList[i].szName, pGroupInfo->szName);
         strcpy(g_pGroupList[i].szDescription, pGroupInfo->szDescription);
         g_pGroupList[i].wSystemRights = pGroupInfo->wSystemRights;

         // Ignore DISABLED flag for EVERYONE group
         if (g_pGroupList[i].dwId != 0)
         {
            g_pGroupList[i].wFlags &= ~UF_DISABLED;
            g_pGroupList[i].wFlags |= pGroupInfo->wFlags & UF_DISABLED;
         }

         // Modify members list
         g_pGroupList[i].dwNumMembers = pGroupInfo->dwNumMembers;
         g_pGroupList[i].pMembers = (DWORD *)realloc(g_pGroupList[i].pMembers, sizeof(DWORD) * g_pGroupList[i].dwNumMembers);
         memcpy(g_pGroupList[i].pMembers, pGroupInfo->pMembers, sizeof(DWORD) * g_pGroupList[i].dwNumMembers);

         // Mark group record as modified and notify clients
         g_pGroupList[i].wFlags |= UF_MODIFIED;
         SendUserDBUpdate(USER_DB_MODIFY, g_pGroupList[i].dwId, NULL, &g_pGroupList[i]);
         dwResult = RCC_SUCCESS;
         break;
      }

   MutexUnlock(m_hMutexGroupAccess);
   return dwResult;
}


//
// Set user's password
//

DWORD SetUserPassword(DWORD dwId, BYTE *pszPassword, BOOL bResetChPasswd)
{
   DWORD i, dwResult = RCC_INVALID_USER_ID;

   MutexLock(m_hMutexUserAccess, INFINITE);

   // Find user
   for(i = 0; i < g_dwNumUsers; i++)
      if (g_pUserList[i].dwId == dwId)
      {
         memcpy(g_pUserList[i].szPassword, pszPassword, SHA1_DIGEST_SIZE);
         g_pUserList[i].nGraceLogins = MAX_GRACE_LOGINS;
         g_pUserList[i].wFlags |= UF_MODIFIED;
         if (bResetChPasswd)
            g_pUserList[i].wFlags &= ~UF_CHANGE_PASSWORD;
         dwResult = RCC_SUCCESS;
         break;
      }

   MutexUnlock(m_hMutexUserAccess);
   return dwResult;
}
