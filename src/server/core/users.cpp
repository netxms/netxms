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
// Static data
//

static NMS_USER *m_pUserList = NULL;
static DWORD m_dwNumUsers = 0;
static NMS_USER_GROUP *m_pGroupList = NULL;
static DWORD m_dwNumGroups = 0;
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

   m_dwNumUsers = DBGetNumRows(hResult);
   m_pUserList = (NMS_USER *)malloc(sizeof(NMS_USER) * m_dwNumUsers);
   for(i = 0; i < m_dwNumUsers; i++)
   {
      m_pUserList[i].dwId = DBGetFieldULong(hResult, i, 0);
      strncpy(m_pUserList[i].szName, DBGetField(hResult, i, 1), MAX_USER_NAME);
printf("%s %d %d\n",DBGetField(hResult, i, 2),StrToBin(DBGetField(hResult, i, 2), m_pUserList[i].szPassword, SHA_DIGEST_LENGTH),SHA_DIGEST_LENGTH);
char buf[256];
BinToStr(m_pUserList[i].szPassword,SHA_DIGEST_LENGTH,buf);
printf("recover: %s\n",buf);
      if (StrToBin(DBGetField(hResult, i, 2), m_pUserList[i].szPassword, SHA_DIGEST_LENGTH) != SHA_DIGEST_LENGTH)
      {
         WriteLog(MSG_INVALID_SHA1_HASH, EVENTLOG_WARNING_TYPE, "s", m_pUserList[i].szName);
         CreateSHA1Hash("netxms", m_pUserList[i].szPassword);
      }
      m_pUserList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 3);
      m_pUserList[i].wFlags = 0;
   }

   DBFreeResult(hResult);

   // Load groups
   hResult = DBSelect(g_hCoreDB, "SELECT id,name,access FROM UserGroups ORDER BY id");
   if (hResult == 0)
      return FALSE;

   m_dwNumGroups = DBGetNumRows(hResult);
   m_pGroupList = (NMS_USER_GROUP *)malloc(sizeof(NMS_USER_GROUP) * m_dwNumGroups);
   for(i = 0; i < m_dwNumGroups; i++)
   {
      m_pGroupList[i].dwId = DBGetFieldULong(hResult, i, 0);
      strncpy(m_pGroupList[i].szName, DBGetField(hResult, i, 1), MAX_USER_NAME);
      m_pGroupList[i].wSystemRights = (WORD)DBGetFieldLong(hResult, i, 2);
      m_pGroupList[i].wFlags = 0;
   }

   DBFreeResult(hResult);

   // Add users to groups
   hResult = DBSelect(g_hCoreDB, "SELECT user_id,group_id FROM UserGroupMembers");
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
   for(i = 0; i < m_dwNumUsers; i++)
   {
      if (m_pUserList[i].wFlags & UF_DELETED)
      {
         // Delete user record from database
         sprintf(szQuery, "DELETE FROM users WHERE id=%d", m_pUserList[i].dwId);
         DBQuery(g_hCoreDB, szQuery);

         // Delete user record from memory
         m_dwNumUsers--;
         memmove(&m_pUserList[i], &m_pUserList[i + 1], sizeof(NMS_USER) * (m_dwNumUsers - i));
         i--;
      }
      else if (m_pUserList[i].wFlags & UF_MODIFIED)
      {
         DB_RESULT hResult;
         BOOL bUserExists = FALSE;

         // Check if user record exists in database
         sprintf(szQuery, "SELECT name FROM users WHERE id=%d", m_pUserList[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bUserExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         BinToStr(m_pUserList[i].szPassword, SHA_DIGEST_LENGTH, szPassword);
         if (bUserExists)
            sprintf(szQuery, "UPDATE users SET name='%s',password='%s',access=%d WHERE id=%d",
                    m_pUserList[i].szName, szPassword, 
                    m_pUserList[i].wSystemRights, m_pUserList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO users (id,name,password,access) VALUES (%d,'%s','%s',%d)",
                    m_pUserList[i].dwId, m_pUserList[i].szName, 
                    szPassword, m_pUserList[i].wSystemRights);
         DBQuery(g_hCoreDB, szQuery);
      }
   }
   MutexUnlock(m_hMutexUserAccess);

   // Save groups
   MutexLock(m_hMutexGroupAccess, INFINITE);
   for(i = 0; i < m_dwNumGroups; i++)
   {
      if (m_pGroupList[i].wFlags & UF_DELETED)
      {
         // Delete group record from database
         sprintf(szQuery, "DELETE FROM usergroups WHERE id=%d", m_pGroupList[i].dwId);
         DBQuery(g_hCoreDB, szQuery);
         sprintf(szQuery, "DELETE FROM usergroupmembers WHERE group_id=%d", m_pGroupList[i].dwId);
         DBQuery(g_hCoreDB, szQuery);

         // Delete group record from memory
         if (m_pGroupList[i].pMembers != NULL)
            free(m_pGroupList[i].pMembers);
         m_dwNumGroups--;
         memmove(&m_pGroupList[i], &m_pGroupList[i + 1], sizeof(NMS_USER_GROUP) * (m_dwNumGroups - i));
         i--;
      }
      else if (m_pGroupList[i].wFlags & UF_MODIFIED)
      {
         DB_RESULT hResult;
         BOOL bGroupExists = FALSE;

         // Check if group record exists in database
         sprintf(szQuery, "SELECT name FROM usergroups WHERE id=%d", m_pGroupList[i].dwId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) != 0)
               bGroupExists = TRUE;
            DBFreeResult(hResult);
         }

         // Create or update record in database
         if (bGroupExists)
            sprintf(szQuery, "UPDATE usergroups SET name='%s',access=%d WHERE id=%d",
                    m_pGroupList[i].szName, m_pGroupList[i].wSystemRights, m_pGroupList[i].dwId);
         else
            sprintf(szQuery, "INSERT INTO usergroupss (id,name,access) VALUES (%d,'%s',%d)",
                    m_pGroupList[i].dwId, m_pGroupList[i].szName, m_pGroupList[i].wSystemRights);
         DBQuery(g_hCoreDB, szQuery);

         if (bGroupExists)
         {
            sprintf(szQuery, "DELETE FROM usergroupmembers WHERE group_id=%d", m_pGroupList[i].dwId);
            DBQuery(g_hCoreDB, szQuery);
         }

         for(DWORD j = 0; j < m_pGroupList[i].dwNumMembers; j++)
         {
            sprintf(szQuery, "INSERT INTO UserGroupMembers (group_id, user_id) VALUES (%d, %d)",
                    m_pGroupList[i].dwId, m_pGroupList[i].pMembers[j]);
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
   for(i = 0; i < m_dwNumUsers; i++)
      if (!strcmp(szName, m_pUserList[i].szName) &&
          !memcmp(szPassword, m_pUserList[i].szPassword, SHA_DIGEST_LENGTH) &&
          !(m_pUserList[i].wFlags & UF_DELETED))
      {
         *pdwId = m_pUserList[i].dwId;
         *pdwSystemRights = (DWORD)m_pUserList[i].wSystemRights;
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

   for(i = 0; i < m_dwNumUsers; i++)
      printf("%-16s \"%s\" %d\n", m_pUserList[i].szName, m_pUserList[i].szPassword,
             m_pUserList[i].wSystemRights);
   printf("*** done ***\n");
}
