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

DWORD LIBNXCL_EXPORTABLE NXCCreateNewDCI(NXC_DCI_LIST *pItemList, DWORD *pdwItemId)
{
   DWORD dwRetCode, dwRqId;
   CSCPMessage msg, *pResponce;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_CREATE_NEW_DCI);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, pItemList->dwNodeId);
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwItemId = pResponce->GetVariableLong(VID_DCI_ID);

         // Create new entry in list
         pItemList->pItems = (NXC_DCI *)MemReAlloc(pItemList->pItems, 
                                                   sizeof(NXC_DCI) * (pItemList->dwNumItems + 1));
         memset(&pItemList->pItems[pItemList->dwNumItems], 0, sizeof(NXC_DCI));
         pItemList->pItems[pItemList->dwNumItems].dwId = *pdwItemId;
         pItemList->pItems[pItemList->dwNumItems].iStatus = ITEM_STATUS_ACTIVE;
         pItemList->pItems[pItemList->dwNumItems].iPollingInterval = 60;
         pItemList->pItems[pItemList->dwNumItems].iRetentionTime = 30;
         pItemList->dwNumItems++;
      }
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

DWORD LIBNXCL_EXPORTABLE NXCDeleteDCI(NXC_DCI_LIST *pItemList, DWORD dwItemId)
{
   DWORD i, dwRqId, dwResult = RCC_INVALID_DCI_ID;
   CSCPMessage msg;

   // Find item with given ID in list
   for(i = 0; i < pItemList->dwNumItems; i++)
      if (pItemList->pItems[i].dwId == dwItemId)
      {
         dwRqId = g_dwMsgId++;

         msg.SetCode(CMD_DELETE_NODE_DCI);
         msg.SetId(dwRqId);
         msg.SetVariable(VID_OBJECT_ID, pItemList->dwNodeId);
         msg.SetVariable(VID_DCI_ID, dwItemId);
         SendMsg(&msg);

         dwResult = WaitForRCC(dwRqId);
         if (dwResult == RCC_SUCCESS)
         {
            // Item was successfully deleted on server, delete it from our list
            pItemList->dwNumItems--;
            memmove(&pItemList->pItems[i], &pItemList->pItems[i + 1], 
                    sizeof(NXC_DCI) * (pItemList->dwNumItems - i));
         }
         break;
      }
   return dwResult;
}


//
// Find item in list by id and return it's index
//

DWORD LIBNXCL_EXPORTABLE NXCItemIndex(NXC_DCI_LIST *pItemList, DWORD dwItemId)
{
   DWORD i;

   for(i = 0; i < pItemList->dwNumItems; i++)
      if (pItemList->pItems[i].dwId == dwItemId)
         return i;
   return INVALID_INDEX;
}


//
// Retrieve collected data from server
//

DWORD LIBNXCL_EXPORTABLE NXCGetDCIData(DWORD dwNodeId, DWORD dwItemId, DWORD dwMaxRows,
                                       DWORD dwTimeFrom, DWORD dwTimeTo, NXC_DCI_DATA **ppData)
{
   CSCPMessage msg;
   DWORD i, dwRqId, dwResult;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_GET_DCI_DATA);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_DCI_ID, dwItemId);
   msg.SetVariable(VID_MAX_ROWS, dwMaxRows);
   msg.SetVariable(VID_TIME_FROM, dwTimeFrom);
   msg.SetVariable(VID_TIME_TO, dwTimeTo);
   SendMsg(&msg);

   dwResult = WaitForRCC(dwRqId);
   if (dwResult == RCC_SUCCESS)
   {
      CSCP_MESSAGE *pRawMsg;

      // We wait a long time because data message can be quite large
      pRawMsg = WaitForRawMessage(CMD_DCI_DATA, dwRqId, 30000);
      if (pRawMsg != NULL)
      {
         DCI_DATA_HEADER *pHdr;
         DCI_DATA_ROW *pSrc;
         NXC_DCI_ROW *pDst;
         static WORD m_wRowSize[] = { 8, 12, 260, 12 };

         pHdr = (DCI_DATA_HEADER *)pRawMsg->df;

         // Allocate memory for results and initialize header
         *ppData = (NXC_DCI_DATA *)MemAlloc(sizeof(NXC_DCI_DATA));
         (*ppData)->dwNumRows = ntohl(pHdr->dwNumRows);
         (*ppData)->dwNodeId = dwNodeId;
         (*ppData)->dwItemId = dwItemId;
         (*ppData)->wDataType = (WORD)ntohl(pHdr->dwDataType);
         (*ppData)->wRowSize = m_wRowSize[(*ppData)->wDataType];
         (*ppData)->pRows = (NXC_DCI_ROW *)MemAlloc((*ppData)->dwNumRows * (*ppData)->wRowSize);

         // Convert and copy values from message to rows in result
         pSrc = (DCI_DATA_ROW *)(((char *)pHdr) + sizeof(DCI_DATA_HEADER));
         pDst = (*ppData)->pRows;
         for(i = 0; i < (*ppData)->dwNumRows; i++)
         {
            pDst->dwTimeStamp = ntohl(pSrc->dwTimeStamp);
            switch((*ppData)->wDataType)
            {
               case DTYPE_INTEGER:
                  pDst->value.dwInt32 = ntohl(pSrc->value.dwInteger);
                  break;
               case DTYPE_STRING:
                  strcpy(pDst->value.szString, pSrc->value.szString);
                  break;
            }

            pSrc = (DCI_DATA_ROW *)(((char *)pSrc) + (*ppData)->wRowSize);
            pDst = (NXC_DCI_ROW *)(((char *)pDst) + (*ppData)->wRowSize);
         }

         // Destroy message
         MemFree(pRawMsg);
      }
      else
      {
         dwResult = RCC_TIMEOUT;
      }
   }

   return dwResult;
}


//
// Destroy DCI result set
//

void LIBNXCL_EXPORTABLE NXCDestroyDCIData(NXC_DCI_DATA *pData)
{
   MemFree(pData->pRows);
   MemFree(pData);
}


//
// Get pointer to specific row in result set
//

NXC_DCI_ROW LIBNXCL_EXPORTABLE *NXCGetRowPtr(NXC_DCI_DATA *pData, DWORD dwRow)
{
   if (dwRow >= pData->dwNumRows)
      return NULL;

   return (NXC_DCI_ROW *)(((char *)(pData->pRows)) + dwRow * pData->wRowSize);
}
