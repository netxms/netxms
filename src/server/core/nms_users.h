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

#define GROUP_FLAG         0x01000000


//
// Global rights
//

#define SYSTEM_ACCESS_MANAGE_USERS  0x0001


//
// Object access rights
//

#define OBJECT_ACCESS_READ          0x00000001
#define OBJECT_ACCESS_MODIFY        0x00000002
#define OBJECT_ACCESS_CREATE        0x00000004
#define OBJECT_ACCESS_DELETE        0x00000008
#define OBJECT_ACCESS_MOVE          0x00000010


//
// User/group flags
//

#define UF_MODIFIED                 0x0001
#define UF_DELETED                  0x0002


//
// User structure
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_USER_NAME];
   char szPassword[MAX_USER_PASSWORD];
   WORD wSystemRights;      // System-wide user's rights
   WORD wFlags;
} NMS_USER;


//
// Group structure
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_USER_NAME];
   WORD wSystemRights;
   WORD wFlags;
   DWORD dwNumMembers;
   DWORD *pMembers;
} NMS_USER_GROUP;


//
// Access list element structure
//

typedef struct
{
   DWORD dwObjectId;
   DWORD dwAccessRights;
} ACL_ELEMENT;


//
// Access list class
//

class AccessList
{
private:
   DWORD m_dwNumElements;
   ACL_ELEMENT *m_pElements;
   MUTEX m_hMutex;

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

public:
   AccessList();
   ~AccessList();

   BOOL GetUserRights(DWORD dwUserId, DWORD *pdwAccessRights);
   void AddElement(DWORD dwObjectId, DWORD dwAccessRights);
   void DeleteElement(DWORD dwObjectId);

   void EnumerateElements(void (* pHandler)(DWORD, DWORD, void *), void *pArg);
};


//
// Functions
//

BOOL LoadUsers(void);
void SaveUsers(void);
void AddUserToGroup(DWORD dwUserId, DWORD dwGroupId);
void DeleteUserFromGroup(DWORD dwUserId, DWORD dwGroupId);
BOOL CheckUserMembership(DWORD dwUserId, DWORD dwGroupId);
BOOL AuthenticateUser(char *szName, char *szPassword, DWORD *pdwId);


//
// Global variables
//

extern NMS_USER *g_pUserList;


#endif
