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
** $module: actions.cpp
**
**/

#include "nms_core.h"


//
// Static data
//

static DWORD m_dwNumActions = 0;
static NXC_ACTION *m_pActionList = NULL;
static RWLOCK m_rwlockActionListAccess;
static DWORD m_dwUpdateCode;


//
// Send updates to all connected clients
//

static void SendActionDBUpdate(ClientSession *pSession, void *pArg)
{
   pSession->OnActionDBUpdate(m_dwUpdateCode, (NXC_ACTION *)pArg);
}


//
// Destroy action list
//

static void DestroyActionList(void)
{
   DWORD i;

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);
   if (m_pActionList != NULL)
   {
      for(i = 0; i < m_dwNumActions; i++)
         safe_free(m_pActionList[i].pszData);
      free(m_pActionList);
      m_pActionList = NULL;
      m_dwNumActions = 0;
   }
   RWLockUnlock(m_rwlockActionListAccess);
}


//
// Load actions list from database
//

static BOOL LoadActions(void)
{
   DB_RESULT hResult;
   BOOL bResult = FALSE;
   DWORD i;
   char *pStr;

   hResult = DBSelect(g_hCoreDB, "SELECT action_id,action_name,action_type,"
                                 "is_disabled,rcpt_addr,email_subject,action_data "
                                 "FROM actions ORDER BY action_id");
   if (hResult != NULL)
   {
      DestroyActionList();
      m_dwNumActions = (DWORD)DBGetNumRows(hResult);
      m_pActionList = (NXC_ACTION *)malloc(sizeof(NXC_ACTION) * m_dwNumActions);
      for(i = 0; i < m_dwNumActions; i++)
      {
         m_pActionList[i].dwId = DBGetFieldULong(hResult, i, 0);
         strncpy(m_pActionList[i].szName, DBGetField(hResult, i, 1), MAX_OBJECT_NAME);
         m_pActionList[i].iType = DBGetFieldLong(hResult, i, 2);
         m_pActionList[i].bIsDisabled = DBGetFieldLong(hResult, i, 3);

         pStr = DBGetField(hResult, i, 4);
         strcpy(m_pActionList[i].szRcptAddr, CHECK_NULL(pStr));
         DecodeSQLString(m_pActionList[i].szRcptAddr);

         pStr = DBGetField(hResult, i, 5);
         strcpy(m_pActionList[i].szEmailSubject, CHECK_NULL(pStr));
         DecodeSQLString(m_pActionList[i].szEmailSubject);

         m_pActionList[i].pszData = strdup(DBGetField(hResult, i, 6));
         DecodeSQLString(m_pActionList[i].pszData);
      }
      DBFreeResult(hResult);
      bResult = TRUE;
   }
   else
   {
      WriteLog(MSG_ACTIONS_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
   }
   return bResult;
}


//
// Initialize action-related stuff
//

BOOL InitActions(void)
{
   BOOL bSuccess = FALSE;

   m_rwlockActionListAccess = RWLockCreate();
   if (m_rwlockActionListAccess != NULL)
      bSuccess = LoadActions();
   return bSuccess;
}


//
// Cleanup action-related stuff
//

void CleanupActions(void)
{
   DestroyActionList();
   RWLockDestroy(m_rwlockActionListAccess);
}


//
// Save action record to database
//

static void SaveActionToDB(NXC_ACTION *pAction)
{
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   char szQuery[4096];

   // Check if action with given ID already exist in database
   sprintf(szQuery, "SELECT action_id FROM actions WHERE action_id=%ld", pAction->dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      bExist = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }

   // Prepare and execute INSERT or UPDATE query
   if (bExist)
      sprintf(szQuery, "UPDATE actions SET action_name='%s',action_type=%d,is_disabled=%d,"
                       "rcpt_addr='%s',email_subject='%s',action_data='%s'"
                       "WHERE action_id=%ld",
              pAction->szName, pAction->iType, pAction->bIsDisabled,
              pAction->szRcptAddr, pAction->szEmailSubject,
              (pAction->pszData == NULL ? "" : pAction->pszData), pAction->dwId);
   else
      sprintf(szQuery, "INSERT INTO actions (action_id,action_name,action_type,"
                       "is_disabled,rcpt_addr,email_subject,action_data) VALUES"
                       " (%ld,'%s',%d,%d,'%s','%s','%s')",
              pAction->dwId,pAction->szName, pAction->iType, pAction->bIsDisabled,
              pAction->szRcptAddr, pAction->szEmailSubject, CHECK_NULL_EX(pAction->pszData));
   DBQuery(g_hCoreDB, szQuery);
}


//
// Compare action's id for bsearch()
//

static int CompareId(const void *key, const void *elem)
{
   return (DWORD)key < ((NXC_ACTION *)elem)->dwId ? -1 : 
            ((DWORD)key > ((NXC_ACTION *)elem)->dwId ? 1 : 0);
}


//
// Compare action id for qsort()
//

static int CompareElements(const void *p1, const void *p2)
{
   return ((NXC_ACTION *)p1)->dwId < ((NXC_ACTION *)p2)->dwId ? -1 : 
            (((NXC_ACTION *)p1)->dwId > ((NXC_ACTION *)p2)->dwId ? 1 : 0);
}


//
// Execute action on specific event
//

BOOL ExecuteAction(DWORD dwActionId, Event *pEvent)
{
   NXC_ACTION *pAction;
   BOOL bSuccess = FALSE;

   RWLockReadLock(m_rwlockActionListAccess, INFINITE);
   pAction = (NXC_ACTION *)bsearch((void *)dwActionId, m_pActionList, 
                                   m_dwNumActions, sizeof(NXC_ACTION), CompareId);
   if (pAction != NULL)
   {
      if (pAction->bIsDisabled)
      {
         DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Action %d (%s) is disabled and will not be executed",
                   dwActionId, pAction->szName);
         bSuccess = TRUE;
      }
      else
      {
         char *pszExpandedData, *pszExpandedSubject;

         pszExpandedData = pEvent->ExpandText(CHECK_NULL_EX(pAction->pszData));
         switch(pAction->iType)
         {
            case ACTION_EXEC:
               DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Executing command \"%s\"", pszExpandedData);
               bSuccess = ExecCommand(pszExpandedData);
               break;
            case ACTION_SEND_EMAIL:
               DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Sending mail to %s: \"%s\"", 
                         pAction->szRcptAddr, pszExpandedData);
               pszExpandedSubject = pEvent->ExpandText(pAction->szEmailSubject);
               PostMail(pAction->szRcptAddr, pszExpandedSubject, pszExpandedData);
               free(pszExpandedSubject);
               bSuccess = TRUE;
               break;
            case ACTION_SEND_SMS:
               DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Sending SMS to %s: \"%s\"", 
                         pAction->szRcptAddr, pszExpandedData);
               break;
            case ACTION_REMOTE:
               DbgPrintf(AF_DEBUG_ACTIONS, "*actions* Executing on \"%s\": \"%s\"", 
                         pAction->szRcptAddr, pszExpandedData);
               break;
            default:
               break;
         }
         free(pszExpandedData);
      }
   }
   RWLockUnlock(m_rwlockActionListAccess);
   return bSuccess;
}


//
// Create new action
//

DWORD CreateNewAction(char *pszName, DWORD *pdwId)
{
   DWORD i, dwResult = RCC_SUCCESS;

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   // Check for duplicate name
   for(i = 0; i < m_dwNumActions; i++)
      if (!stricmp(m_pActionList[i].szName, pszName))
      {
         dwResult = RCC_ALREADY_EXIST;
         break;
      }

   // If not exist, create it
   if (i == m_dwNumActions)
   {
      m_dwNumActions++;
      m_pActionList = (NXC_ACTION *)realloc(m_pActionList, sizeof(NXC_ACTION) * m_dwNumActions);
      m_pActionList[i].dwId = CreateUniqueId(IDG_ACTION);
      strncpy(m_pActionList[i].szName, pszName, MAX_OBJECT_NAME);
      m_pActionList[i].bIsDisabled = TRUE;
      m_pActionList[i].iType = ACTION_EXEC;
      m_pActionList[i].szEmailSubject[0] = 0;
      m_pActionList[i].szRcptAddr[0] = 0;
      m_pActionList[i].pszData = NULL;

      qsort(m_pActionList, m_dwNumActions, sizeof(NXC_ACTION), CompareElements);

      SaveActionToDB(&m_pActionList[i]);
      m_dwUpdateCode = NX_NOTIFY_ACTION_CREATED;
      EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);
      *pdwId = m_pActionList[i].dwId;
   }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Delete action
//

DWORD DeleteActionFromDB(DWORD dwActionId)
{
   DWORD i, dwResult = RCC_INVALID_ACTION_ID;
   char szQuery[256];

   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   for(i = 0; i < m_dwNumActions; i++)
      if (m_pActionList[i].dwId == dwActionId)
      {
         m_dwNumActions--;
         memmove(&m_pActionList[i], &m_pActionList[i + 1], sizeof(NXC_ACTION) * (m_dwNumActions - i));
         sprintf(szQuery, "DELETE FROM actions WHERE action_id=%ld", dwActionId);
         DBQuery(g_hCoreDB, szQuery);

         m_dwUpdateCode = NX_NOTIFY_ACTION_DELETED;
         EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);

         dwResult = RCC_SUCCESS;
         break;
      }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Modify action record from message
//

DWORD ModifyActionFromMessage(CSCPMessage *pMsg)
{
   DWORD i, dwResult = RCC_INVALID_ACTION_ID;
   DWORD dwActionId;

   dwActionId = pMsg->GetVariableLong(VID_ACTION_ID);
   RWLockWriteLock(m_rwlockActionListAccess, INFINITE);

   // Find action with given id
   for(i = 0; i < m_dwNumActions; i++)
      if (m_pActionList[i].dwId == dwActionId)
      {
         m_pActionList[i].bIsDisabled = pMsg->GetVariableShort(VID_IS_DISABLED);
         m_pActionList[i].iType = pMsg->GetVariableShort(VID_ACTION_TYPE);
         safe_free(m_pActionList[i].pszData);
         m_pActionList[i].pszData = pMsg->GetVariableStr(VID_ACTION_DATA);
         pMsg->GetVariableStr(VID_EMAIL_SUBJECT, m_pActionList[i].szEmailSubject, MAX_EMAIL_SUBJECT_LEN);
         pMsg->GetVariableStr(VID_ACTION_NAME, m_pActionList[i].szName, MAX_OBJECT_NAME);
         pMsg->GetVariableStr(VID_RCPT_ADDR, m_pActionList[i].szRcptAddr, MAX_RCPT_ADDR_LEN);

         SaveActionToDB(&m_pActionList[i]);

         m_dwUpdateCode = NX_NOTIFY_ACTION_MODIFIED;
         EnumerateClientSessions(SendActionDBUpdate, &m_pActionList[i]);

         dwResult = RCC_SUCCESS;
         break;
      }

   RWLockUnlock(m_rwlockActionListAccess);
   return dwResult;
}


//
// Fill CSCP message with action's data
//

void FillActionInfoMessage(CSCPMessage *pMsg, NXC_ACTION *pAction)
{
   pMsg->SetVariable(VID_IS_DISABLED, (WORD)pAction->bIsDisabled);
   pMsg->SetVariable(VID_ACTION_TYPE, (WORD)pAction->iType);
   pMsg->SetVariable(VID_ACTION_DATA, CHECK_NULL_EX(pAction->pszData));
   pMsg->SetVariable(VID_EMAIL_SUBJECT, pAction->szEmailSubject);
   pMsg->SetVariable(VID_ACTION_NAME, pAction->szName);
   pMsg->SetVariable(VID_RCPT_ADDR, pAction->szRcptAddr);
}


//
// Send all actions to client
//

void SendActionsToClient(ClientSession *pSession, DWORD dwRqId)
{
   DWORD i;
   CSCPMessage msg;

   // Prepare message
   msg.SetCode(CMD_ACTION_DATA);
   msg.SetId(dwRqId);

   RWLockReadLock(m_rwlockActionListAccess, INFINITE);

   for(i = 0; i < m_dwNumActions; i++)
   {
      msg.SetVariable(VID_ACTION_ID, m_pActionList[i].dwId);
      FillActionInfoMessage(&msg, &m_pActionList[i]);
      pSession->SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   RWLockUnlock(m_rwlockActionListAccess);

   // Send end-of-list flag
   msg.SetVariable(VID_ACTION_ID, (DWORD)0);
   pSession->SendMessage(&msg);
}
