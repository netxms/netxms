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
// User structure
//

typedef struct
{
   DWORD dwId;
   char szName[MAX_USER_NAME];
   BYTE szPassword[SHA_DIGEST_LENGTH];
   WORD wSystemRights;      // System-wide user's rights
   WORD wFlags;
   char szFullName[MAX_USER_FULLNAME];
   char szDescription[MAX_USER_DESCR];
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
   char szDescription[MAX_USER_DESCR];
} NMS_USER_GROUP;


//
// Access list element structure
//

typedef struct
{
   DWORD dwUserId;
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
   void AddElement(DWORD dwUserId, DWORD dwAccessRights);
   BOOL DeleteElement(DWORD dwUserId);
   void DeleteAll(void);

   void EnumerateElements(void (* pHandler)(DWORD, DWORD, void *), void *pArg);

   void CreateMessage(CSCPMessage *pMsg);
};


//
// Functions
//

BOOL LoadUsers(void);
void SaveUsers(void);
void AddUserToGroup(DWORD dwUserId, DWORD dwGroupId);
BOOL CheckUserMembership(DWORD dwUserId, DWORD dwGroupId);
BOOL AuthenticateUser(char *szName, BYTE *szPassword, DWORD *pdwId, DWORD *pdwSystemRights);
void DumpUsers(void);
DWORD CreateNewUser(char *pszName, BOOL bIsGroup, DWORD *pdwId);
DWORD DeleteUserFromDB(DWORD dwId);
DWORD ModifyUser(NMS_USER *pUserInfo);
DWORD ModifyGroup(NMS_USER_GROUP *pGroupInfo);
void SendUserDBUpdate(int iCode, DWORD dwUserId, NMS_USER *pUser, NMS_USER_GROUP *pUserGroup);


//
// Global variables
//

extern NMS_USER *g_pUserList;
extern DWORD g_dwNumUsers;
extern NMS_USER_GROUP *g_pGroupList;
extern DWORD g_dwNumGroups;

#endif
