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
** $module: datacoll.cpp
**
**/

#include "libnxcl.h"


//
// Static data
//

static CONDITION m_hCondSyncFinished;
static NXC_DCI_LIST *m_pItemList = NULL;


//
// Process events coming from server
//

void ProcessDCI(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_NODE_DCI_LIST_END:
         if (m_hCondSyncFinished != NULL)
            ConditionSet(m_hCondSyncFinished);
         break;
      case CMD_NODE_DCI:
         if (m_pItemList != NULL)
         {
            int i;

            i = m_pItemList->dwNumItems;
            m_pItemList->dwNumItems++;
            m_pItemList->pItems = (NXC_DCI *)MemReAlloc(m_pItemList->pItems, 
                                          sizeof(NXC_DCI) * m_pItemList->dwNumItems);
            m_pItemList->pItems[i].dwId = pMsg->GetVariableLong(VID_DCI_ID);
            m_pItemList->pItems[i].iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
            m_pItemList->pItems[i].iPollingInterval = (int)pMsg->GetVariableLong(VID_POLLING_INTERVAL);
            m_pItemList->pItems[i].iRetentionTime = (int)pMsg->GetVariableLong(VID_RETENTION_TIME);
            m_pItemList->pItems[i].iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
            m_pItemList->pItems[i].iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
            pMsg->GetVariableStr(VID_NAME, m_pItemList->pItems[i].szName, MAX_ITEM_NAME);
            m_pItemList->pItems[i].bModified = FALSE;
         }
         break;
      default:
         break;
   }
}


//
// Load data collection items list for specified node
// This function is NOT REENTRANT
//

DWORD LIBNXCL_EXPORTABLE NXCOpenNodeDCIList(DWORD dwNodeId, NXC_DCI_LIST **ppItemList)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = g_dwMsgId++;

   m_hCondSyncFinished = ConditionCreate(FALSE);
   m_pItemList = (NXC_DCI_LIST *)MemAlloc(sizeof(NXC_DCI_LIST));
   m_pItemList->dwNodeId = dwNodeId;
   m_pItemList->dwNumItems = 0;
   m_pItemList->pItems = NULL;

   msg.SetCode(CMD_GET_NODE_DCI_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   if (dwRetCode == RCC_SUCCESS)
   {
      ChangeState(STATE_LOAD_DCI);

      // Wait for DCI list end or for disconnection
      while(g_dwState != STATE_DISCONNECTED)
      {
         if (ConditionWait(m_hCondSyncFinished, 500))
            break;
      }

      if (g_dwState != STATE_DISCONNECTED)
      {
         ChangeState(STATE_IDLE);
         *ppItemList = m_pItemList;
      }
      else
      {
         dwRetCode = RCC_COMM_FAILURE;
         MemFree(m_pItemList);
      }
   }
   else
   {
      MemFree(m_pItemList);
   }

   m_pItemList = NULL;
   ConditionDestroy(m_hCondSyncFinished);
   m_hCondSyncFinished = NULL;
   return dwRetCode;
}


//
// Unlock and destroy previously opened node's DCI list
//

DWORD LIBNXCL_EXPORTABLE NXCCloseNodeDCIList(NXC_DCI_LIST *pItemList)
{
   DWORD dwRetCode = RCC_INVALID_ARGUMENT, dwRqId;
   CSCPMessage msg;

   if (pItemList != NULL)
   {
      dwRqId = g_dwMsgId++;

      msg.SetCode(CMD_UNLOCK_NODE_DCI_LIST);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_OBJECT_ID, pItemList->dwNodeId);
      SendMsg(&msg);

      dwRetCode = WaitForRCC(dwRqId);

      MemFree(pItemList->pItems);
      MemFree(pItemList);
   }
   return dwRetCode;
}


//
// Create new data collection item
//

DWORD LIBNXCL_EXPORTABLE NXCCreateNewDCI(DWORD dwNodeId, DWORD *pdwItemId)
{
   DWORD dwRetCode, dwRqId;
   CSCPMessage msg, *pResponce;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_CREATE_NEW_DCI);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
         *pdwItemId = pResponce->GetVariableLong(VID_DCI_ID);
      delete pResponce;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Update data collection item
//

DWORD LIBNXCL_EXPORTABLE NXCUpdateDCI(DWORD dwNodeId, NXC_DCI *pItem)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_MODIFY_NODE_DCI);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_DCI_ID, pItem->dwId);
   msg.SetVariable(VID_DCI_DATA_TYPE, (WORD)pItem->iDataType);
   msg.SetVariable(VID_POLLING_INTERVAL, (DWORD)pItem->iPollingInterval);
   msg.SetVariable(VID_RETENTION_TIME, (DWORD)pItem->iRetentionTime);
   msg.SetVariable(VID_DCI_SOURCE_TYPE, (WORD)pItem->iSource);
   msg.SetVariable(VID_DCI_STATUS, (WORD)pItem->iStatus);
   msg.SetVariable(VID_NAME, pItem->szName);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}


//
// Delete data collection item
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteDCI(DWORD dwNodeId, DWORD dwItemId)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_DELETE_NODE_DCI);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_DCI_ID, dwItemId);
   SendMsg(&msg);

   return WaitForRCC(dwRqId);
}


//
// Delete data collection item from list
//

void LIBNXCL_EXPORTABLE NXCDeleteDCIFromList(NXC_DCI_LIST *pItemList, DWORD dwItemId)
{
}
