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
** $module: nms_users.h
**
**/

#ifndef _nms_users_h_
#define _nms_users_h_


//
// Constants
//

#define MAX_USER_NAME      64
#define MAX_USER_PASSWORD  64


//
// User structure
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_USER_NAME];
   char szPassword[MAX_USER_PASSWORD];
} NMS_USER;


//
// Functions
//

BOOL LoadUsers(void);
void SaveUsers(void);
BOOL AuthenticateUser(char *szName, char *szPassword, DWORD *pdwId);


//
// Global variables
//

extern NMS_USER *g_pUserList;


#endif
