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

#include "nms_core.h"


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

   // Load users
   hResult = DBSelect(g_hCoreDB, "SELECT id,name,password,access FROM users ORDER BY id");
   if (hResult == 0)
      return FALSE;

   g_dwNumUsers = DBGetNumRows(hResult);
   g_pUserList = (NMS_USER *)malloc(sizeof(NMS_USER) * g_dwNumUsers);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      g_pUserList[i].dwId = DBGetFieldULong(hResult, i, 0);
      strncpy(g_pUserList[i].szName, DBGetField(hResult, i, 1), MAX_USER_NAME);
      if (StrToBin(DBGetField(hResult, i, 2), g_pUserList[i].szPassword, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH)
      {
         WriteLog(MSG_INVALID_SHA1_HASH, EVENTLOG_WARNING_TYPE, "s", g_pUserList[i].szName);
         CreateSHA1Hash("netxms", g_pUserList[i].szPassword);
      }
      g_pUserList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 3);
      g_pUserList[i].wFlags = 0;
   }

   DBFreeResult(hResult);

   // Load groups
   hResult = DBSelect(g_hCoreDB, "SELECT id,name,access FROM user_groups ORDER BY id");
   if (hResult == 0)
      return FALSE;

   g_dwNumGroups = DBGetNumRows(hResult);
   g_pGroupList = (NMS_USER_GROUP *)malloc(sizeof(NMS_USER_GROUP) * g_dwNumGroups);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      g_pGroupList[i].dwId = DBGetFieldULong(hResult, i, 0);
      strncpy(g_pGroupList[i].szName, DBGetField(hResult, i, 1), MAX_USER_NAME);
      g_pGroupList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 2);
      g_pGroupList[i].wFlags = 0;
   }

   DBFreeResult(hResult);

   // Add users to groups
   hResult = DBSelect(g_hCoreDB, "SELECT user_id,group_id FROM user_group_members");
   if (hResult == 0)
      return FALSE;

   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
      AddUserToGroup(DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));

   return TRUE;
}


//
// Save user list to database
//

void SaveUsers(void)
{
   DWORD i;
   char szQuery[1024], szPassword[SHA_DIGEST_LENGTH * 2 + 1];

   // Save users
   MutexLock(m_hMutexUserAccess, INFINITE);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      if (g_pUserList[i].wFlags & UF_DELETED)
      {
         // Delete user record from database
         sprintf(szQuery, "DELETE FROM users WHERE id=%d", g_pUserList[i].dwId);
         DBQuery(g_hCoreDB, szQuery);

         // Delete user record from memory
         g_dwNumUsers--;
         memmove(&g_pUserList[i], &g_pUserList[i + 1], sizeof(NMS_USER) * (g_dwNumUsers - i));
         i--;
      }
      else if (g_pUserList[i].wFlags & UF_MODIFIED)
      {
         DB_RESULT hResult;
         BOOL bUserExists = FALSE;

         // Check if user record exists in database
         sprintf(szQuery, "SELECT name FROM users WHERE id=%d", g_pUserList[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bUserExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         BinToStr(g_pUserList[i].szPassword, SHA_DIGEST_LENGTH, szPassword);
         if (bUserExists)
            sprintf(szQuery, "UPDATE users SET name='%s',password='%s',access=%d WHERE id=%d",
                    g_pUserList[i].szName, szPassword, 
                    g_pUserList[i].wSystemRights, g_pUserList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO users (id,name,password,access) VALUES (%d,'%s','%s',%d)",
                    g_pUserList[i].dwId, g_pUserList[i].szName, 
                    szPassword, g_pUserList[i].wSystemRights);
         DBQuery(g_hCoreDB, szQuery);
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
         DBQuery(g_hCoreDB, szQuery);
         sprintf(szQuery, "DELETE FROM user_group_members WHERE group_id=%d", g_pGroupList[i].dwId);
         DBQuery(g_hCoreDB, szQuery);

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

         // Check if group record exists in database
         sprintf(szQuery, "SELECT name FROM user_groups WHERE id=%d", g_pGroupList[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bGroupExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         if (bGroupExists)
            sprintf(szQuery, "UPDATE user_groups SET name='%s',access=%d WHERE id=%d",
                    g_pGroupList[i].szName, g_pGroupList[i].wSystemRights, g_pGroupList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO user_groups (id,name,access) VALUES (%d,'%s',%d)",
                    g_pGroupList[i].dwId, g_pGroupList[i].szName, g_pGroupList[i].wSystemRights);
         DBQuery(g_hCoreDB, szQuery);

         if (bGroupExists)
         {
            sprintf(szQuery, "DELETE FROM user_group_members WHERE group_id=%d", g_pGroupList[i].dwId);
            DBQuery(g_hCoreDB, szQuery);
         }

         for(DWORD j = 0; j < g_pGroupList[i].dwNumMembers; j++)
         {
            sprintf(szQuery, "INSERT INTO user_group_members (group_id, user_id) VALUES (%d, %d)",
                    g_pGroupList[i].dwId, g_pGroupList[i].pMembers[j]);
            DBQuery(g_hCoreDB, szQuery);
         }
      }
   }
   MutexUnlock(m_hMutexGroupAccess);
}


//
// Authenticate user
// Checks if provided login name and password are correct, and returns TRUE
// on success and FALSE otherwise. On success authentication, user's ID is stored
// int pdwId.
//

BOOL AuthenticateUser(char *szName, BYTE *szPassword, DWORD *pdwId, DWORD *pdwSystemRights)
{
   DWORD i;
   BOOL bResult = FALSE;

   MutexLock(m_hMutexUserAccess, INFINITE);
   for(i = 0; i < g_dwNumUsers; i++)
      if (!strcmp(szName, g_pUserList[i].szName) &&
          !memcmp(szPassword, g_pUserList[i].szPassword, SHA_DIGEST_LENGTH) &&
          !(g_pUserList[i].wFlags & UF_DELETED))
      {
         *pdwId = g_pUserList[i].dwId;
         *pdwSystemRights = (DWORD)g_pUserList[i].wSystemRights;
         bResult = TRUE;
         break;
      }
   MutexUnlock(m_hMutexUserAccess);
   return bResult;
}


//
// Add user to group
//

void AddUserToGroup(DWORD dwUserId, DWORD dwGroupId)
{
}


//
// Delete user from group
//

void DeleteUserFromGroup(DWORD dwUserId, DWORD dwGroupId)
{
}


//
// Check if user is a member of specific group
//

BOOL CheckUserMembership(DWORD dwUserId, DWORD dwGroupId)
{
   return FALSE;
}


//
// Dump user list to stdout
//

void DumpUsers(void)
{
   DWORD i;

   printf("Login name           System rights\n"
          "----------------------------------\n");
   for(i = 0; i < g_dwNumUsers; i++)
      printf("%-20s 0x%08X\n", g_pUserList[i].szName, g_pUserList[i].wSystemRights);
   printf("\n");
}
