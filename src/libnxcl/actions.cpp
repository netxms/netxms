/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: actions.cpp
**
**/

#include "libnxcl.h"


//
// Fill action record from message
//

static void ActionFromMsg(NXCPMessage *pMsg, NXC_ACTION *pAction)
{
   pAction->bIsDisabled = pMsg->getFieldAsUInt16(VID_IS_DISABLED);
   pAction->iType = pMsg->getFieldAsUInt16(VID_ACTION_TYPE);
   pAction->pszData = pMsg->getFieldAsString(VID_ACTION_DATA);
   pMsg->getFieldAsString(VID_EMAIL_SUBJECT, pAction->szEmailSubject, MAX_EMAIL_SUBJECT_LEN);
   pMsg->getFieldAsString(VID_ACTION_NAME, pAction->szName, MAX_OBJECT_NAME);
   pMsg->getFieldAsString(VID_RCPT_ADDR, pAction->szRcptAddr, MAX_RCPT_ADDR_LEN);
}


//
// Process CMD_ACTION_DB_UPDATE message
//

void ProcessActionUpdate(NXCL_Session *pSession, NXCPMessage *pMsg)
{
   NXC_ACTION action;
   UINT32 dwCode;

   dwCode = pMsg->getFieldAsUInt32(VID_NOTIFICATION_CODE);
   action.dwId = pMsg->getFieldAsUInt32(VID_ACTION_ID);
   if (dwCode != NX_NOTIFY_ACTION_DELETED)
      ActionFromMsg(pMsg, &action);
   pSession->callEventHandler(NXC_EVENT_NOTIFICATION, dwCode, &action);
}

/**
 * Load all actions from server
 */
UINT32 LIBNXCL_EXPORTABLE NXCLoadActions(NXC_SESSION hSession, UINT32 *pdwNumActions, NXC_ACTION **ppActionList)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwRetCode = RCC_SUCCESS, dwNumActions = 0, dwActionId = 0;
   NXC_ACTION *pList = NULL;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_LOAD_ACTIONS);
   msg.setId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
   if (dwRetCode == RCC_SUCCESS)
   {
      do
      {
         pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_ACTION_DATA, dwRqId);
         if (pResponse != NULL)
         {
            dwActionId = pResponse->getFieldAsUInt32(VID_ACTION_ID);
            if (dwActionId != 0)  // 0 is end of list indicator
            {
               pList = (NXC_ACTION *)realloc(pList, sizeof(NXC_ACTION) * (dwNumActions + 1));
               pList[dwNumActions].dwId = dwActionId;
               ActionFromMsg(pResponse, &pList[dwNumActions]);
               dwNumActions++;
            }
            delete pResponse;
         }
         else
         {
            dwRetCode = RCC_TIMEOUT;
            dwActionId = 0;
         }
      }
      while(dwActionId != 0);
   }

   // Destroy results on failure or save on success
   if (dwRetCode == RCC_SUCCESS)
   {
      *ppActionList = pList;
      *pdwNumActions = dwNumActions;
   }
   else
   {
      safe_free(pList);
      *ppActionList = NULL;
      *pdwNumActions = 0;
   }

   return dwRetCode;
}


//
// Create new action on server
//

UINT32 LIBNXCL_EXPORTABLE NXCCreateAction(NXC_SESSION hSession, TCHAR *pszName, UINT32 *pdwNewId)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRetCode, dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_CREATE_ACTION);
   msg.setId(dwRqId);
   msg.setField(VID_ACTION_NAME, pszName);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwNewId = pResponse->getFieldAsUInt32(VID_ACTION_ID);
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }
   return dwRetCode;
}


//
// Delete action by ID
//

UINT32 LIBNXCL_EXPORTABLE NXCDeleteAction(NXC_SESSION hSession, UINT32 dwActionId)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_DELETE_ACTION);
   msg.setId(dwRqId);
   msg.setField(VID_ACTION_ID, dwActionId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Modify action record
//

UINT32 LIBNXCL_EXPORTABLE NXCModifyAction(NXC_SESSION hSession, NXC_ACTION *pAction)
{
   NXCPMessage msg;
   UINT32 dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Fill in request
   msg.setCode(CMD_MODIFY_ACTION);
   msg.setId(dwRqId);
   msg.setField(VID_IS_DISABLED, (WORD)pAction->bIsDisabled);
   msg.setField(VID_ACTION_ID, pAction->dwId);
   msg.setField(VID_ACTION_TYPE, (WORD)pAction->iType);
   msg.setField(VID_ACTION_DATA, pAction->pszData);
   msg.setField(VID_EMAIL_SUBJECT, pAction->szEmailSubject);
   msg.setField(VID_ACTION_NAME, pAction->szName);
   msg.setField(VID_RCPT_ADDR, pAction->szRcptAddr);

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}
