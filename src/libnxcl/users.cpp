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
// Static data
//

static DWORD m_dwNumUsers;
static NXC_USER *m_pUserList = NULL;
static BOOL m_bUserDBLoaded = FALSE;
static CONDITION m_hCondLoadFinished;


//
// Process record from network
//

void ProcessUserDBRecord(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_USER_DB_EOF:
         if (g_dwState == STATE_LOAD_USER_DB)
            ConditionSet(m_hCondLoadFinished);
         break;
      case CMD_USER_DATA:
      case CMD_GROUP_DATA:
         m_pUserList = (NXC_USER *)MemReAlloc(m_pUserList, sizeof(NXC_USER) * (m_dwNumUsers + 1));
         memset(&m_pUserList[m_dwNumUsers], 0, sizeof(NXC_USER));

         // Process common fields
         m_pUserList[m_dwNumUsers].dwId = pMsg->GetVariableLong(VID_USER_ID);
         pMsg->GetVariableStr(VID_USER_NAME, m_pUserList[m_dwNumUsers].szName, MAX_USER_NAME);
         m_pUserList[m_dwNumUsers].wFlags = pMsg->GetVariableShort(VID_USER_FLAGS);
         m_pUserList[m_dwNumUsers].wSystemRights = pMsg->GetVariableShort(VID_USER_SYS_RIGHTS);
         pMsg->GetVariableStr(VID_USER_DESCRIPTION, m_pUserList[m_dwNumUsers].szDescription, MAX_USER_DESCR);

         // Process group-specific fields
         if (pMsg->GetCode() == CMD_GROUP_DATA)
         {
            DWORD i, dwId;

            m_pUserList[m_dwNumUsers].dwNumMembers = pMsg->GetVariableLong(VID_NUM_MEMBERS);
            m_pUserList[m_dwNumUsers].pdwMemberList = (DWORD *)MemAlloc(sizeof(DWORD) * m_pUserList[m_dwNumUsers].dwNumMembers);
            for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < m_pUserList[m_dwNumUsers].dwNumMembers; i++, dwId++)
               m_pUserList[m_dwNumUsers].pdwMemberList[i] = pMsg->GetVariableLong(dwId);
         }
         else     // User-specific data
         {
            pMsg->GetVariableStr(VID_USER_FULL_NAME, m_pUserList[m_dwNumUsers].szFullName, MAX_USER_FULLNAME);
         }

         m_dwNumUsers++;
         break;
         break;
      default:
         break;
   }
}


//
// Load user database
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCLoadUserDB(void)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode, dwRqId;

   m_hCondLoadFinished = ConditionCreate(FALSE);
   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_LOAD_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   if (dwRetCode == RCC_SUCCESS)
   {
      ChangeState(STATE_LOAD_USER_DB);

      // Wait for object list end or for disconnection
      while(g_dwState != STATE_DISCONNECTED)
      {
         if (ConditionWait(m_hCondLoadFinished, 500))
            break;
      }

      if (g_dwState != STATE_DISCONNECTED)
      {
         ChangeState(STATE_IDLE);
         m_bUserDBLoaded = TRUE;
      }
      else
      {
         dwRetCode = RCC_COMM_FAILURE;
      }
   }

   ConditionDestroy(m_hCondLoadFinished);
   return dwRetCode;
}


//
// Find user in database by ID
//

NXC_USER LIBNXCL_EXPORTABLE *NXCFindUserById(DWORD dwId)
{
   DWORD i;
   NXC_USER *pUser = NULL;

   if (m_bUserDBLoaded)
   {
      for(i = 0; i < m_dwNumUsers; i++)
         if (m_pUserList[i].dwId == dwId)
         {
            pUser = &m_pUserList[i];
            break;
         }
   }

   return pUser;
}


//
// Get pointer to user list and number of users
//

BOOL LIBNXCL_EXPORTABLE NXCGetUserDB(NXC_USER **ppUserList, DWORD *pdwNumUsers)
{
   if (!m_bUserDBLoaded)
      return FALSE;

   *ppUserList = m_pUserList;
   *pdwNumUsers = m_dwNumUsers;
   return TRUE;
}


//
// Create new user or group on server
//

DWORD CreateUser(DWORD dwRqId, char *pszName, BOOL bIsGroup)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode;

   msg.SetCode(CMD_CREATE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_NAME, pszName);
   msg.SetVariable(VID_IS_GROUP, (WORD)bIsGroup);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
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

DWORD DeleteUser(DWORD dwRqId, DWORD dwId)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode;

   msg.SetCode(CMD_DELETE_USER);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_USER_ID, dwId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Lock/unlock user database
//

DWORD LockUserDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg, *pResponce;
   DWORD dwRetCode;

   msg.SetCode(bLock ? CMD_LOCK_USER_DB : CMD_UNLOCK_USER_DB);
   msg.SetId(dwRqId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}
