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

static NXC_DCI_LIST *m_pItemList = NULL;


//
// Process events coming from server
//

void ProcessDCI(CSCPMessage *pMsg)
{
   switch(pMsg->GetCode())
   {
      case CMD_NODE_DCI_LIST_END:
         CompleteSync(RCC_SUCCESS);
         break;
      case CMD_NODE_DCI:
         if (m_pItemList != NULL)
         {
            DWORD i, j, dwId;
            DCI_THRESHOLD dct;

            i = m_pItemList->dwNumItems;
            m_pItemList->dwNumItems++;
            m_pItemList->pItems = (NXC_DCI *)realloc(m_pItemList->pItems, 
                                          sizeof(NXC_DCI) * m_pItemList->dwNumItems);
            m_pItemList->pItems[i].dwId = pMsg->GetVariableLong(VID_DCI_ID);
            m_pItemList->pItems[i].iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
            m_pItemList->pItems[i].iPollingInterval = (int)pMsg->GetVariableLong(VID_POLLING_INTERVAL);
            m_pItemList->pItems[i].iRetentionTime = (int)pMsg->GetVariableLong(VID_RETENTION_TIME);
            m_pItemList->pItems[i].iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
            m_pItemList->pItems[i].iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
            m_pItemList->pItems[i].iDeltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
            m_pItemList->pItems[i].pszFormula = pMsg->GetVariableStr(VID_DCI_FORMULA);
            pMsg->GetVariableStr(VID_NAME, m_pItemList->pItems[i].szName, MAX_ITEM_NAME);
            pMsg->GetVariableStr(VID_DESCRIPTION, m_pItemList->pItems[i].szDescription,
                                 MAX_DB_STRING);
            m_pItemList->pItems[i].dwNumThresholds = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
            m_pItemList->pItems[i].pThresholdList = 
               (NXC_DCI_THRESHOLD *)malloc(sizeof(NXC_DCI_THRESHOLD) * m_pItemList->pItems[i].dwNumThresholds);
            for(j = 0, dwId = VID_DCI_THRESHOLD_BASE; j < m_pItemList->pItems[i].dwNumThresholds; j++, dwId++)
            {
               pMsg->GetVariableBinary(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
               m_pItemList->pItems[i].pThresholdList[j].dwId = ntohl(dct.dwId);
               m_pItemList->pItems[i].pThresholdList[j].dwEvent = ntohl(dct.dwEvent);
               m_pItemList->pItems[i].pThresholdList[j].dwArg1 = ntohl(dct.dwArg1);
               m_pItemList->pItems[i].pThresholdList[j].dwArg2 = ntohl(dct.dwArg2);
               m_pItemList->pItems[i].pThresholdList[j].wFunction = ntohs(dct.wFunction);
               m_pItemList->pItems[i].pThresholdList[j].wOperation = ntohs(dct.wOperation);
               switch(m_pItemList->pItems[i].iDataType)
               {
                  case DCI_DT_INTEGER:
                     m_pItemList->pItems[i].pThresholdList[j].value.dwInt32 = ntohl(dct.value.dwInt32);
                     break;
                  case DCI_DT_INT64:
                     m_pItemList->pItems[i].pThresholdList[j].value.qwInt64 = ntohq(dct.value.qwInt64);
                     break;
                  case DCI_DT_FLOAT:
                     m_pItemList->pItems[i].pThresholdList[j].value.dFloat = ntohd(dct.value.dFloat);
                     break;
                  case DCI_DT_STRING:
                     strcpy(m_pItemList->pItems[i].pThresholdList[j].value.szString, dct.value.szString);
                     break;
                  default:
                     break;
               }
            }
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
   PrepareForSync();

   m_pItemList = (NXC_DCI_LIST *)malloc(sizeof(NXC_DCI_LIST));
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
      // Wait for DCI list end or for disconnection
      dwRetCode = WaitForSync(INFINITE);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppItemList = m_pItemList;
      }
      else
      {
         free(m_pItemList);
      }
   }
   else
   {
      free(m_pItemList);
   }

   m_pItemList = NULL;
   return dwRetCode;
}


//
// Unlock and destroy previously opened node's DCI list
//

DWORD LIBNXCL_EXPORTABLE NXCCloseNodeDCIList(NXC_DCI_LIST *pItemList)
{
   DWORD i, dwRetCode = RCC_INVALID_ARGUMENT, dwRqId;
   CSCPMessage msg;

   if (pItemList != NULL)
   {
      dwRqId = g_dwMsgId++;

      msg.SetCode(CMD_UNLOCK_NODE_DCI_LIST);
      msg.SetId(dwRqId);
      msg.SetVariable(VID_OBJECT_ID, pItemList->dwNodeId);
      SendMsg(&msg);

      dwRetCode = WaitForRCC(dwRqId);

      for(i = 0; i < pItemList->dwNumItems; i++)
      {
         safe_free(pItemList->pItems[i].pThresholdList);
         safe_free(pItemList->pItems[i].pszFormula);
      }
      safe_free(pItemList->pItems);
      free(pItemList);
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
         pItemList->pItems = (NXC_DCI *)realloc(pItemList->pItems, 
                                                   sizeof(NXC_DCI) * (pItemList->dwNumItems + 1));
         memset(&pItemList->pItems[pItemList->dwNumItems], 0, sizeof(NXC_DCI));
         pItemList->pItems[pItemList->dwNumItems].dwId = *pdwItemId;
         pItemList->pItems[pItemList->dwNumItems].iStatus = ITEM_STATUS_ACTIVE;
         pItemList->pItems[pItemList->dwNumItems].iPollingInterval = 60;
         pItemList->pItems[pItemList->dwNumItems].iRetentionTime = 30;
         pItemList->pItems[pItemList->dwNumItems].iDeltaCalculation = DCM_ORIGINAL_VALUE;
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
   DWORD i, dwId, dwRqId, dwRetCode;
   CSCPMessage msg, *pResponce;
   DCI_THRESHOLD dct;

   dwRqId = g_dwMsgId++;

   msg.SetCode(CMD_MODIFY_NODE_DCI);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_DCI_ID, pItem->dwId);
   msg.SetVariable(VID_DCI_DATA_TYPE, (WORD)pItem->iDataType);
   msg.SetVariable(VID_POLLING_INTERVAL, (DWORD)pItem->iPollingInterval);
   msg.SetVariable(VID_RETENTION_TIME, (DWORD)pItem->iRetentionTime);
   msg.SetVariable(VID_DCI_SOURCE_TYPE, (WORD)pItem->iSource);
   msg.SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)pItem->iDeltaCalculation);
   msg.SetVariable(VID_DCI_STATUS, (WORD)pItem->iStatus);
   msg.SetVariable(VID_NAME, pItem->szName);
   msg.SetVariable(VID_DESCRIPTION, pItem->szDescription);
   msg.SetVariable(VID_DCI_FORMULA, pItem->pszFormula);
   msg.SetVariable(VID_NUM_THRESHOLDS, pItem->dwNumThresholds);
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < pItem->dwNumThresholds; i++, dwId++)
   {
      dct.dwId = htonl(pItem->pThresholdList[i].dwId);
      dct.dwEvent = htonl(pItem->pThresholdList[i].dwEvent);
      dct.dwArg1 = htonl(pItem->pThresholdList[i].dwArg1);
      dct.dwArg2 = htonl(pItem->pThresholdList[i].dwArg2);
      dct.wFunction = htons(pItem->pThresholdList[i].wFunction);
      dct.wOperation = htons(pItem->pThresholdList[i].wOperation);
      switch(pItem->iDataType)
      {
         case DCI_DT_INTEGER:
            dct.value.dwInt32 = htonl(pItem->pThresholdList[i].value.dwInt32);
            break;
         case DCI_DT_INT64:
            dct.value.qwInt64 = htonq(pItem->pThresholdList[i].value.qwInt64);
            break;
         case DCI_DT_FLOAT:
            dct.value.dFloat = htond(pItem->pThresholdList[i].value.dFloat);
            break;
         case DCI_DT_STRING:
            strcpy(dct.value.szString, pItem->pThresholdList[i].value.szString);
            break;
         default:
            break;
      }
      msg.SetVariable(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
   }
   SendMsg(&msg);

   pResponce = WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 2000);
   if (pResponce != NULL)
   {
      dwRetCode = pResponce->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         DWORD dwNumMaps, *pdwMapId, *pdwMapIndex;

         // Get index to id mapping for newly created thresholds
         dwNumMaps = pResponce->GetVariableLong(VID_DCI_NUM_MAPS);
         if (dwNumMaps > 0)
         {
            pdwMapId = (DWORD *)malloc(sizeof(DWORD) * dwNumMaps);
            pdwMapIndex = (DWORD *)malloc(sizeof(DWORD) * dwNumMaps);
            pResponce->GetVariableBinary(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(DWORD) * dwNumMaps);
            pResponce->GetVariableBinary(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(DWORD) * dwNumMaps);
            for(i = 0; i < dwNumMaps; i++)
               pItem->pThresholdList[ntohl(pdwMapIndex[i])].dwId = ntohl(pdwMapId[i]);
            free(pdwMapId);
            free(pdwMapIndex);
         }
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
            safe_free(pItemList->pItems[i].pThresholdList);
            safe_free(pItemList->pItems[i].pszFormula);
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
         *ppData = (NXC_DCI_DATA *)malloc(sizeof(NXC_DCI_DATA));
         (*ppData)->dwNumRows = ntohl(pHdr->dwNumRows);
         (*ppData)->dwNodeId = dwNodeId;
         (*ppData)->dwItemId = dwItemId;
         (*ppData)->wDataType = (WORD)ntohl(pHdr->dwDataType);
         (*ppData)->wRowSize = m_wRowSize[(*ppData)->wDataType];
         (*ppData)->pRows = ((*ppData)->dwNumRows > 0 ? 
               (NXC_DCI_ROW *)malloc((*ppData)->dwNumRows * (*ppData)->wRowSize) : NULL);

         // Convert and copy values from message to rows in result
         pSrc = (DCI_DATA_ROW *)(((char *)pHdr) + sizeof(DCI_DATA_HEADER));
         pDst = (*ppData)->pRows;
         for(i = 0; i < (*ppData)->dwNumRows; i++)
         {
            pDst->dwTimeStamp = ntohl(pSrc->dwTimeStamp);
            switch((*ppData)->wDataType)
            {
               case DCI_DT_INTEGER:
                  pDst->value.dwInt32 = ntohl(pSrc->value.dwInteger);
                  break;
               case DCI_DT_FLOAT:
                  pDst->value.dFloat = ntohd(pSrc->value.dFloat);
                  break;
               case DCI_DT_STRING:
                  strcpy(pDst->value.szString, pSrc->value.szString);
                  break;
            }

            pSrc = (DCI_DATA_ROW *)(((char *)pSrc) + (*ppData)->wRowSize);
            pDst = (NXC_DCI_ROW *)(((char *)pDst) + (*ppData)->wRowSize);
         }

         // Destroy message
         free(pRawMsg);
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
   if (pData != NULL)
   {
      safe_free(pData->pRows);
      free(pData);
   }
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


//
// Add threshold to item
//

DWORD LIBNXCL_EXPORTABLE NXCAddThresholdToItem(NXC_DCI *pItem, NXC_DCI_THRESHOLD *pThreshold)
{
   DWORD dwIndex;

   dwIndex = pItem->dwNumThresholds++;
   pItem->pThresholdList = (NXC_DCI_THRESHOLD *)realloc(pItem->pThresholdList,
                                    sizeof(NXC_DCI_THRESHOLD) * pItem->dwNumThresholds);
   memcpy(&pItem->pThresholdList[dwIndex], pThreshold, sizeof(NXC_DCI_THRESHOLD));
   return dwIndex;
}


//
// Delete threshold from item
//

BOOL LIBNXCL_EXPORTABLE NXCDeleteThresholdFromItem(NXC_DCI *pItem, DWORD dwIndex)
{
   BOOL bResult = FALSE;

   if (pItem->dwNumThresholds > dwIndex)
   {
      pItem->dwNumThresholds--;
      memmove(&pItem->pThresholdList[dwIndex], &pItem->pThresholdList[dwIndex + 1],
              sizeof(NXC_DCI_THRESHOLD) * (pItem->dwNumThresholds - dwIndex));
      bResult = TRUE;
   }
   return bResult;
}


//
// Swap two threshold items
//

BOOL LIBNXCL_EXPORTABLE NXCSwapThresholds(NXC_DCI *pItem, DWORD dwIndex1, DWORD dwIndex2)
{
   BOOL bResult = FALSE;
   NXC_DCI_THRESHOLD dct;

   if ((pItem->dwNumThresholds > dwIndex1) &&
       (pItem->dwNumThresholds > dwIndex2) &&
       (dwIndex1 != dwIndex2))
   {
      memcpy(&dct, &pItem->pThresholdList[dwIndex1], sizeof(NXC_DCI_THRESHOLD));
      memcpy(&pItem->pThresholdList[dwIndex1], &pItem->pThresholdList[dwIndex2], sizeof(NXC_DCI_THRESHOLD));
      memcpy(&pItem->pThresholdList[dwIndex2], &dct, sizeof(NXC_DCI_THRESHOLD));
      bResult = TRUE;
   }
   return bResult;
}
