/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
// Global data
//

NMS_USER *g_pUserList;
int g_iNumUsers;


//
// Load user list from database
//

BOOL LoadUsers(void)
{
   DB_RESULT hResult;
   int i;

   hResult = DBSelect(g_hCoreDB, "SELECT id,name,password FROM users ORDER BY id");
   if (hResult == 0)
      return FALSE;

   g_iNumUsers = DBGetNumRows(hResult);
   g_pUserList = (NMS_USER *)malloc(sizeof(NMS_USER) * g_iNumUsers);
   for(i = 0; i < g_iNumUsers; i++)
   {
      g_pUserList[i].dwId = DBGetFieldULong(hResult, i, 0);
      strncpy(g_pUserList[i].szName, DBGetField(hResult, i, 1), MAX_USER_NAME);
      strncpy(g_pUserList[i].szPassword, DBGetField(hResult, i, 2), MAX_USER_PASSWORD);
   }

   DBFreeResult(hResult);
   return TRUE;
}


//
// Save user list to database
//

void SaveUsers(void)
{
}


//
// Authenticate user
// Checks if provided login name and password are correct, and returns TRUE
// on success and FALSE otherwise. On success authentication, user's ID is stored
// int pdwId.
//

BOOL AuthenticateUser(char *szName, char *szPassword, DWORD *pdwId)
{
   int i;

   for(i = 0; i < g_iNumUsers; i++)
      if (!strcmp(szName, g_pUserList[i].szName) &&
          !strcmp(szPassword, g_pUserList[i].szPassword))
      {
         *pdwId = g_pUserList[i].dwId;
         return TRUE;
      }
   return FALSE;
}
