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
** File: datacoll.cpp
**
**/

#include "libnxcl.h"

/**
 * Load data collection items list for specified node
 * This function is NOT REENTRANT
 */
UINT32 LIBNXCL_EXPORTABLE NXCOpenNodeDCIList(NXC_SESSION hSession, UINT32 dwNodeId,
                                            NXC_DCI_LIST **ppItemList)
{
   return ((NXCL_Session *)hSession)->OpenNodeDCIList(dwNodeId, ppItemList);
}

/**
 * Unlock and destroy previously opened node's DCI list
 */
UINT32 LIBNXCL_EXPORTABLE NXCCloseNodeDCIList(NXC_SESSION hSession, NXC_DCI_LIST *pItemList)
{
   UINT32 i, j, dwRetCode = RCC_INVALID_ARGUMENT, dwRqId;
   NXCPMessage msg;

   if (pItemList != NULL)
   {
      dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

      msg.setCode(CMD_UNLOCK_NODE_DCI_LIST);
      msg.setId(dwRqId);
      msg.setField(VID_OBJECT_ID, pItemList->dwNodeId);
      ((NXCL_Session *)hSession)->SendMsg(&msg);

      dwRetCode = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);

      for(i = 0; i < pItemList->dwNumItems; i++)
      {
         for(j = 0; j < pItemList->pItems[i].dwNumSchedules; j++)
            free(pItemList->pItems[i].ppScheduleList[j]);
         safe_free(pItemList->pItems[i].ppScheduleList);
         for(j = 0; j < pItemList->pItems[i].dwNumThresholds; j++)
            free(pItemList->pItems[i].pThresholdList[j].script);
         safe_free(pItemList->pItems[i].pThresholdList);
         safe_free(pItemList->pItems[i].pszFormula);
			safe_free(pItemList->pItems[i].pszCustomUnitName);
			safe_free(pItemList->pItems[i].pszPerfTabSettings);
			safe_free(pItemList->pItems[i].comments);
      }
      safe_free(pItemList->pItems);
      free(pItemList);
   }
   return dwRetCode;
}


//
// Create new data collection item
//

UINT32 LIBNXCL_EXPORTABLE NXCCreateNewDCI(NXC_SESSION hSession, NXC_DCI_LIST *pItemList, UINT32 *pdwItemId)
{
   UINT32 dwRetCode, dwRqId;
   NXCPMessage msg, *pResponse;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_CREATE_NEW_DCI);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, pItemList->dwNodeId);
	msg.setField(VID_DCOBJECT_TYPE, (WORD)DCO_TYPE_ITEM);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwItemId = pResponse->getFieldAsUInt32(VID_DCI_ID);

         // Create new entry in list
         pItemList->pItems = (NXC_DCI *)realloc(pItemList->pItems,
                                                   sizeof(NXC_DCI) * (pItemList->dwNumItems + 1));
         memset(&pItemList->pItems[pItemList->dwNumItems], 0, sizeof(NXC_DCI));
         pItemList->pItems[pItemList->dwNumItems].dwId = *pdwItemId;
         pItemList->pItems[pItemList->dwNumItems].iStatus = ITEM_STATUS_ACTIVE;
         pItemList->pItems[pItemList->dwNumItems].iPollingInterval = 60;
         pItemList->pItems[pItemList->dwNumItems].iRetentionTime = 30;
         pItemList->pItems[pItemList->dwNumItems].iDeltaCalculation = DCM_ORIGINAL_VALUE;
         pItemList->pItems[pItemList->dwNumItems].flags = 0;
         pItemList->pItems[pItemList->dwNumItems].dwNumSchedules = 0;
         pItemList->pItems[pItemList->dwNumItems].ppScheduleList = NULL;
			pItemList->pItems[pItemList->dwNumItems].nBaseUnits = DCI_BASEUNITS_OTHER;
			pItemList->pItems[pItemList->dwNumItems].nMultiplier = 1;
         pItemList->dwNumItems++;
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}

/**
 * Update data collection item
 */
UINT32 LIBNXCL_EXPORTABLE NXCUpdateDCI(NXC_SESSION hSession, UINT32 dwNodeId, NXC_DCI *pItem)
{
   UINT32 i, dwId, dwRqId, dwRetCode;
   NXCPMessage msg, *pResponse;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_MODIFY_NODE_DCI);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_ID, pItem->dwId);
	msg.setField(VID_DCOBJECT_TYPE, (WORD)DCO_TYPE_ITEM);
   msg.setField(VID_DCI_DATA_TYPE, (WORD)pItem->iDataType);
   msg.setField(VID_POLLING_INTERVAL, (UINT32)pItem->iPollingInterval);
   msg.setField(VID_RETENTION_TIME, (UINT32)pItem->iRetentionTime);
   msg.setField(VID_DCI_SOURCE_TYPE, (WORD)pItem->iSource);
   msg.setField(VID_DCI_DELTA_CALCULATION, (WORD)pItem->iDeltaCalculation);
   msg.setField(VID_DCI_STATUS, (WORD)pItem->iStatus);
   msg.setField(VID_NAME, pItem->szName);
   msg.setField(VID_DESCRIPTION, pItem->szDescription);
   msg.setField(VID_INSTANCE, pItem->szInstance);
   msg.setField(VID_SYSTEM_TAG, pItem->szSystemTag);
	msg.setField(VID_TRANSFORMATION_SCRIPT, CHECK_NULL_EX(pItem->pszFormula));
   msg.setField(VID_FLAGS, pItem->flags);
	msg.setField(VID_SNMP_RAW_VALUE_TYPE, pItem->wSnmpRawType);
	msg.setField(VID_RESOURCE_ID, pItem->dwResourceId);
	msg.setField(VID_AGENT_PROXY, pItem->dwProxyNode);
	msg.setField(VID_BASE_UNITS, (WORD)pItem->nBaseUnits);
	msg.setField(VID_MULTIPLIER, (UINT32)pItem->nMultiplier);
	msg.setField(VID_SNMP_PORT, (WORD)pItem->nSnmpPort);
   msg.setField(VID_COMMENTS, (pItem->comments));
	if (pItem->pszCustomUnitName != NULL)
		msg.setField(VID_CUSTOM_UNITS_NAME, pItem->pszCustomUnitName);
	if (pItem->pszPerfTabSettings)
		msg.setField(VID_PERFTAB_SETTINGS, pItem->pszPerfTabSettings);
   if (pItem->flags & DCF_ADVANCED_SCHEDULE)
   {
      msg.setField(VID_NUM_SCHEDULES, pItem->dwNumSchedules);
      for(i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < pItem->dwNumSchedules; i++, dwId++)
         msg.setField(dwId, pItem->ppScheduleList[i]);
   }
   else
   {
      msg.setField(VID_NUM_SCHEDULES, (UINT32)0);
   }
   msg.setField(VID_NUM_THRESHOLDS, pItem->dwNumThresholds);
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < pItem->dwNumThresholds; i++, dwId++)
   {
		msg.setField(dwId++, pItem->pThresholdList[i].id);
      msg.setField(dwId++, pItem->pThresholdList[i].activationEvent);
      msg.setField(dwId++, pItem->pThresholdList[i].rearmEvent);
      msg.setField(dwId++, pItem->pThresholdList[i].function);
      msg.setField(dwId++, pItem->pThresholdList[i].operation);
      msg.setField(dwId++, pItem->pThresholdList[i].sampleCount);
      msg.setField(dwId++, CHECK_NULL_EX(pItem->pThresholdList[i].script));
		msg.setField(dwId++, (UINT32)pItem->pThresholdList[i].repeatInterval);
      msg.setField(dwId++, pItem->pThresholdList[i].value);
   }
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         UINT32 dwNumMaps, *pdwMapId, *pdwMapIndex;

         // Get index to id mapping for newly created thresholds
         dwNumMaps = pResponse->getFieldAsUInt32(VID_DCI_NUM_MAPS);
         if (dwNumMaps > 0)
         {
            pdwMapId = (UINT32 *)malloc(sizeof(UINT32) * dwNumMaps);
            pdwMapIndex = (UINT32 *)malloc(sizeof(UINT32) * dwNumMaps);
            pResponse->getFieldAsBinary(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(UINT32) * dwNumMaps);
            pResponse->getFieldAsBinary(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(UINT32) * dwNumMaps);
            for(i = 0; i < dwNumMaps; i++)
               pItem->pThresholdList[ntohl(pdwMapIndex[i])].id = ntohl(pdwMapId[i]);
            free(pdwMapId);
            free(pdwMapIndex);
         }
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}

/**
 * Delete data collection item
 */
UINT32 LIBNXCL_EXPORTABLE NXCDeleteDCI(NXC_SESSION hSession, NXC_DCI_LIST *pItemList, UINT32 dwItemId)
{
   UINT32 i, j, dwRqId, dwResult = RCC_INVALID_DCI_ID;
   NXCPMessage msg;

	CHECK_SESSION_HANDLE();

   // Find item with given ID in list
   for(i = 0; i < pItemList->dwNumItems; i++)
      if (pItemList->pItems[i].dwId == dwItemId)
      {
         dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

         msg.setCode(CMD_DELETE_NODE_DCI);
         msg.setId(dwRqId);
         msg.setField(VID_OBJECT_ID, pItemList->dwNodeId);
         msg.setField(VID_DCI_ID, dwItemId);
         ((NXCL_Session *)hSession)->SendMsg(&msg);

         dwResult = ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
         if (dwResult == RCC_SUCCESS)
         {
            // Item was successfully deleted on server, delete it from our list
            for(j = 0; j < pItemList->pItems[i].dwNumSchedules; j++)
               free(pItemList->pItems[i].ppScheduleList[j]);
            safe_free(pItemList->pItems[i].ppScheduleList);
            for(j = 0; j < pItemList->pItems[i].dwNumThresholds; j++)
               free(pItemList->pItems[i].pThresholdList[j].script);
            safe_free(pItemList->pItems[i].pThresholdList);
            safe_free(pItemList->pItems[i].pszFormula);
				safe_free(pItemList->pItems[i].pszCustomUnitName);
				safe_free(pItemList->pItems[i].pszPerfTabSettings);
				safe_free(pItemList->pItems[i].comments);
            pItemList->dwNumItems--;
            memmove(&pItemList->pItems[i], &pItemList->pItems[i + 1],
                    sizeof(NXC_DCI) * (pItemList->dwNumItems - i));
         }
         break;
      }
   return dwResult;
}

/**
 * Set status for multiple DCIs
 */
UINT32 LIBNXCL_EXPORTABLE NXCSetDCIStatus(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwNumItems,
                                         UINT32 *pdwItemList, int iStatus)
{
   NXCPMessage msg;
   UINT32 dwRqId;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_SET_DCI_STATUS);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_STATUS, (WORD)iStatus);
   msg.setField(VID_NUM_ITEMS, dwNumItems);
   msg.setFieldFromInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}

/**
 * Find item in list by id and return it's index
 */
UINT32 LIBNXCL_EXPORTABLE NXCItemIndex(NXC_DCI_LIST *pItemList, UINT32 dwItemId)
{
   UINT32 i;

   for(i = 0; i < pItemList->dwNumItems; i++)
      if (pItemList->pItems[i].dwId == dwItemId)
         return i;
   return INVALID_INDEX;
}

/**
 * Retrieve collected data from server
 */
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIData(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
                                       UINT32 dwMaxRows, UINT32 dwTimeFrom, UINT32 dwTimeTo,
                                       NXC_DCI_DATA **ppData)
{
	return NXCGetDCIDataEx(hSession, dwNodeId, dwItemId, dwMaxRows, dwTimeFrom, dwTimeTo, ppData, NULL, NULL);
}

/**
 * Retrieve collected data from server
 */
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIDataEx(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
                                         UINT32 dwMaxRows, UINT32 dwTimeFrom, UINT32 dwTimeTo,
                                         NXC_DCI_DATA **ppData, NXC_DCI_THRESHOLD **thresholds, UINT32 *numThresholds)
{
   NXCPMessage msg, *response;
   UINT32 i, dwRqId, dwId, dwResult;
   BOOL bRun = TRUE;

	CHECK_SESSION_HANDLE();

   msg.setCode(CMD_GET_DCI_DATA);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_ID, dwItemId);

   // Allocate memory for results and initialize header
   *ppData = (NXC_DCI_DATA *)malloc(sizeof(NXC_DCI_DATA));
   (*ppData)->dwNumRows = 0;
   (*ppData)->dwNodeId = dwNodeId;
   (*ppData)->dwItemId = dwItemId;
   (*ppData)->pRows = NULL;
	if (thresholds != NULL)
	{
		*thresholds = NULL;
		*numThresholds = 0;
	}

   do
   {
      dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

      msg.setId(dwRqId);
      msg.setField(VID_MAX_ROWS, dwMaxRows);
      msg.setField(VID_TIME_FROM, dwTimeFrom);
      msg.setField(VID_TIME_TO, dwTimeTo);
      ((NXCL_Session *)hSession)->SendMsg(&msg);

		response = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
		if (response == NULL)
		{
			dwResult = RCC_TIMEOUT;
			break;
		}
		dwResult = response->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         NXCP_MESSAGE *pRawMsg;

			if ((thresholds != NULL) && (*thresholds == NULL))
			{
				*numThresholds = response->getFieldAsUInt32(VID_NUM_THRESHOLDS);
				*thresholds = (NXC_DCI_THRESHOLD *)malloc(sizeof(NXC_DCI_THRESHOLD) * (*numThresholds));
				for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < *numThresholds; i++, dwId++)
				{
					(*thresholds)[i].id = response->getFieldAsUInt32(dwId++);
					(*thresholds)[i].activationEvent = response->getFieldAsUInt32(dwId++);
					(*thresholds)[i].rearmEvent = response->getFieldAsUInt32(dwId++);
					(*thresholds)[i].function = response->getFieldAsUInt16(dwId++);
					(*thresholds)[i].operation = response->getFieldAsUInt16(dwId++);
					(*thresholds)[i].sampleCount = response->getFieldAsUInt32(dwId++);
					(*thresholds)[i].script = response->getFieldAsString(dwId++);
					(*thresholds)[i].repeatInterval = (LONG)response->getFieldAsUInt32(dwId++);
					response->getFieldAsString(dwId++, (*thresholds)[i].value, MAX_STRING_VALUE);
				}
			}

         // We wait a long time because data message can be quite large
         pRawMsg = ((NXCL_Session *)hSession)->WaitForRawMessage(CMD_DCI_DATA, dwRqId, 60000);
         if (pRawMsg != NULL)
         {
            DCI_DATA_HEADER *pHdr;
            DCI_DATA_ROW *pSrc;
            NXC_DCI_ROW *pDst;
            UINT32 dwPrevRowCount, dwRecvRows;
				int netRowSize;
            static WORD s_wNetRowSize[] = { 8, 8, 16, 16, 516, 16 };
            static WORD s_wClientRowSize[] = { 8, 8, 16, 16, sizeof(TCHAR) * 256 + 4, 16 };

            pHdr = (DCI_DATA_HEADER *)pRawMsg->fields;
            dwRecvRows = ntohl(pHdr->numRows);

            // Allocate memory for results
            dwPrevRowCount = (*ppData)->dwNumRows;
            (*ppData)->dwNumRows += dwRecvRows;
            (*ppData)->wDataType = (WORD)ntohl(pHdr->dataType);
				if ((*ppData)->wDataType > 5)
					(*ppData)->wDataType = 0;
            (*ppData)->wRowSize = s_wClientRowSize[(*ppData)->wDataType];
				netRowSize = (int)s_wNetRowSize[(*ppData)->wDataType];
            if (dwRecvRows > 0)
               (*ppData)->pRows = (NXC_DCI_ROW *)realloc((*ppData)->pRows, (*ppData)->dwNumRows * (*ppData)->wRowSize);

            // Convert and copy values from message to rows in result
            pSrc = (DCI_DATA_ROW *)(((char *)pHdr) + sizeof(DCI_DATA_HEADER));
            pDst = (NXC_DCI_ROW *)(((char *)((*ppData)->pRows)) + dwPrevRowCount * (*ppData)->wRowSize);
            for(i = 0; i < dwRecvRows; i++)
            {
               pDst->dwTimeStamp = ntohl(pSrc->timeStamp);
               switch((*ppData)->wDataType)
               {
                  case DCI_DT_INT:
                  case DCI_DT_UINT:
                     pDst->value.dwInt32 = ntohl(pSrc->value.int32);
                     break;
                  case DCI_DT_INT64:
                  case DCI_DT_UINT64:
                     pDst->value.ext.v64.qwInt64 = ntohq(pSrc->value.ext.v64.int64);
                     break;
                  case DCI_DT_FLOAT:
                     pDst->value.ext.v64.dFloat = ntohd(pSrc->value.ext.v64.real);
                     break;
                  case DCI_DT_STRING:
                     SwapWideString(pSrc->value.string);
#ifdef UNICODE
#ifdef UNICODE_UCS4
                     ucs2_to_ucs4(pSrc->value.string, -1, pDst->value.szString, MAX_STRING_VALUE);
#else
                     wcscpy(pDst->value.szString, pSrc->value.string);
#endif
#else
                     ucs2_to_mb(pSrc->value.string, -1, pDst->value.szString, MAX_STRING_VALUE);
#endif
                     break;
               }

               pSrc = (DCI_DATA_ROW *)(((char *)pSrc) + netRowSize);
               pDst = (NXC_DCI_ROW *)(((char *)pDst) + (*ppData)->wRowSize);
            }

            // Shift boundaries
            if (((dwMaxRows == 0) || (dwMaxRows > MAX_DCI_DATA_RECORDS)) &&
                (dwRecvRows == MAX_DCI_DATA_RECORDS))
            {
               // Shift to last record
               pDst = (NXC_DCI_ROW *)(((char *)pDst) - (*ppData)->wRowSize);
               dwTimeTo = pDst->dwTimeStamp - 1;   // Assume that we have no more than one value per second
               if (dwMaxRows > 0)
                  dwMaxRows -= MAX_DCI_DATA_RECORDS;
            }
            else
            {
               bRun = FALSE;
            }

            // Destroy message
            free(pRawMsg);
         }
         else
         {
            dwResult = RCC_TIMEOUT;
         }
      }
      delete response;
   } while((dwResult == RCC_SUCCESS) && bRun);

   // Destroy already allocated buffer if request was unsuccessful
   if (dwResult != RCC_SUCCESS)
   {
      safe_free((*ppData)->pRows);
      free(*ppData);
		if (thresholds != NULL)
      {
         for(UINT32 i = 0; i < *numThresholds; i++)
            safe_free((*thresholds)[i].script);
			safe_free(*thresholds);
      }
   }

   return dwResult;
}

/**
 * Destroy DCI result set
 */
void LIBNXCL_EXPORTABLE NXCDestroyDCIData(NXC_DCI_DATA *pData)
{
   if (pData != NULL)
   {
      safe_free(pData->pRows);
      free(pData);
   }
}

/**
 * Get pointer to specific row in result set
 */
NXC_DCI_ROW LIBNXCL_EXPORTABLE *NXCGetRowPtr(NXC_DCI_DATA *pData, UINT32 dwRow)
{
   if (dwRow >= pData->dwNumRows)
      return NULL;

   return (NXC_DCI_ROW *)(((char *)(pData->pRows)) + dwRow * pData->wRowSize);
}

/**
 * Add threshold to item
 */
UINT32 LIBNXCL_EXPORTABLE NXCAddThresholdToItem(NXC_DCI *pItem, NXC_DCI_THRESHOLD *pThreshold)
{
   UINT32 dwIndex;

   dwIndex = pItem->dwNumThresholds++;
   pItem->pThresholdList = (NXC_DCI_THRESHOLD *)realloc(pItem->pThresholdList,
                                    sizeof(NXC_DCI_THRESHOLD) * pItem->dwNumThresholds);
   memcpy(&pItem->pThresholdList[dwIndex], pThreshold, sizeof(NXC_DCI_THRESHOLD));
   if (pThreshold->script != NULL)
      pItem->pThresholdList[dwIndex].script = _tcsdup(pThreshold->script);
   return dwIndex;
}

/**
 * Delete threshold from item
 */
BOOL LIBNXCL_EXPORTABLE NXCDeleteThresholdFromItem(NXC_DCI *pItem, UINT32 dwIndex)
{
   BOOL bResult = FALSE;

   if (pItem->dwNumThresholds > dwIndex)
   {
      pItem->dwNumThresholds--;
      if (pItem->dwNumThresholds > 0)
      {
         safe_free(pItem->pThresholdList[dwIndex].script);
         memmove(&pItem->pThresholdList[dwIndex], &pItem->pThresholdList[dwIndex + 1],
                 sizeof(NXC_DCI_THRESHOLD) * (pItem->dwNumThresholds - dwIndex));
      }
      bResult = TRUE;
   }
   return bResult;
}

/**
 * Swap two threshold items
 */
BOOL LIBNXCL_EXPORTABLE NXCSwapThresholds(NXC_DCI *pItem, UINT32 dwIndex1, UINT32 dwIndex2)
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

/**
 * Copy data collection items from one node to another
 */
UINT32 LIBNXCL_EXPORTABLE NXCCopyDCI(NXC_SESSION hSession, UINT32 dwSrcNodeId, UINT32 dwDstNodeId,
                                    UINT32 dwNumItems, UINT32 *pdwItemList, BOOL bMove)
{
   NXCPMessage msg;
   UINT32 dwRqId;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_COPY_DCI);
   msg.setId(dwRqId);
   msg.setField(VID_SOURCE_OBJECT_ID, dwSrcNodeId);
   msg.setField(VID_DESTINATION_OBJECT_ID, dwDstNodeId);
   msg.setField(VID_NUM_ITEMS, dwNumItems);
   msg.setFieldFromInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
   msg.setField(VID_MOVE_FLAG, (WORD)bMove);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}

/**
 * Query value of specific parameter from node
 */
UINT32 LIBNXCL_EXPORTABLE NXCQueryParameter(NXC_SESSION hSession, UINT32 dwNodeId, int iOrigin,
                                           TCHAR *pszParameter, TCHAR *pszBuffer,
                                           UINT32 dwBufferSize)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_QUERY_PARAMETER);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_SOURCE_TYPE, (WORD)iOrigin);
   msg.setField(VID_NAME, pszParameter);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 120000);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         pResponse->getFieldAsString(VID_VALUE, pszBuffer, dwBufferSize);
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}

/**
 * Get threshold list for given DCI
 */
UINT32 LIBNXCL_EXPORTABLE NXCGetDCIThresholds(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
															NXC_DCI_THRESHOLD **ppList, UINT32 *pdwSize)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

	CHECK_SESSION_HANDLE();

	*ppList = NULL;
	*pdwSize = 0;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_DCI_THRESHOLDS);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
	msg.setField(VID_DCI_ID, dwItemId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
		{
			*pdwSize = pResponse->getFieldAsUInt32(VID_NUM_THRESHOLDS);
			*ppList = (NXC_DCI_THRESHOLD *)malloc(sizeof(NXC_DCI_THRESHOLD) * (*pdwSize));
			for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < *pdwSize; i++, dwId++)
			{
				(*ppList)[i].id = pResponse->getFieldAsUInt32(dwId++);
				(*ppList)[i].activationEvent = pResponse->getFieldAsUInt32(dwId++);
				(*ppList)[i].rearmEvent = pResponse->getFieldAsUInt32(dwId++);
				(*ppList)[i].function = pResponse->getFieldAsUInt16(dwId++);
				(*ppList)[i].operation = pResponse->getFieldAsUInt16(dwId++);
				(*ppList)[i].sampleCount = pResponse->getFieldAsUInt32(dwId++);
				(*ppList)[i].script = pResponse->getFieldAsString(dwId++);
				(*ppList)[i].repeatInterval = (LONG)pResponse->getFieldAsUInt32(dwId++);
				pResponse->getFieldAsString(dwId++, (*ppList)[i].value, MAX_STRING_VALUE);
			}
		}
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}

/**
 * Get last values for all DCIs of selected node
 */
UINT32 LIBNXCL_EXPORTABLE NXCGetLastValues(NXC_SESSION hSession, UINT32 dwNodeId,
                                          UINT32 *pdwNumItems, NXC_DCI_VALUE **ppValueList)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

   *pdwNumItems = 0;
   *ppValueList = NULL;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_LAST_VALUES);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwNumItems = pResponse->getFieldAsUInt32(VID_NUM_ITEMS);
         *ppValueList = (NXC_DCI_VALUE *)malloc(sizeof(NXC_DCI_VALUE) * (*pdwNumItems));
         memset(*ppValueList, 0, sizeof(NXC_DCI_VALUE) * (*pdwNumItems));
         for(i = 0, dwId = VID_DCI_VALUES_BASE; i < *pdwNumItems; i++, dwId +=42)
         {
            (*ppValueList)[i].dwId = pResponse->getFieldAsUInt32(dwId++);
            pResponse->getFieldAsString(dwId++, (*ppValueList)[i].szName, MAX_ITEM_NAME);
            pResponse->getFieldAsString(dwId++, (*ppValueList)[i].szDescription, MAX_DB_STRING);
            (*ppValueList)[i].nSource = (BYTE)pResponse->getFieldAsUInt16(dwId++);
            (*ppValueList)[i].nDataType = (BYTE)pResponse->getFieldAsUInt16(dwId++);
            pResponse->getFieldAsString(dwId++, (*ppValueList)[i].szValue, MAX_DB_STRING);
            (*ppValueList)[i].dwTimestamp = pResponse->getFieldAsUInt32(dwId++);
            (*ppValueList)[i].nStatus = (BYTE)pResponse->getFieldAsUInt16(dwId++);
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Apply template to node
//

UINT32 LIBNXCL_EXPORTABLE NXCApplyTemplate(NXC_SESSION hSession, UINT32 dwTemplateId, UINT32 dwNodeId)
{
   UINT32 dwRqId;
   NXCPMessage msg;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_APPLY_TEMPLATE);
   msg.setId(dwRqId);
   msg.setField(VID_SOURCE_OBJECT_ID, dwTemplateId);
   msg.setField(VID_DESTINATION_OBJECT_ID, dwNodeId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Resolve DCI names
//

UINT32 LIBNXCL_EXPORTABLE NXCResolveDCINames(NXC_SESSION hSession, UINT32 dwNumDCI,
                                            INPUT_DCI *pDCIList, TCHAR ***pppszNames)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, j, dwId, dwRqId, dwResult, *pdwList;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_RESOLVE_DCI_NAMES);
   msg.setId(dwRqId);
   msg.setField(VID_NUM_ITEMS, dwNumDCI);

   pdwList = (UINT32 *)malloc(sizeof(UINT32) * dwNumDCI * 2);
   for(i = 0, j = dwNumDCI; i < dwNumDCI; i++, j++)
   {
      pdwList[i] = pDCIList[i].nodeId;
      pdwList[j] = pDCIList[i].id;
   }
   msg.setFieldFromInt32Array(VID_NODE_LIST, dwNumDCI, pdwList);
   msg.setFieldFromInt32Array(VID_DCI_LIST, dwNumDCI, &pdwList[dwNumDCI]);
   free(pdwList);

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pppszNames = (TCHAR **)malloc(sizeof(TCHAR *) * dwNumDCI);
         for(i = 0, dwId = VID_DCI_LIST_BASE; i < dwNumDCI; i++)
            (*pppszNames)[i] = pResponse->getFieldAsString(dwId++);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Push DCI data
//

UINT32 LIBNXCL_EXPORTABLE NXCPushDCIData(NXC_SESSION hSession, UINT32 dwNumItems,
                                        NXC_DCI_PUSH_DATA *pItems, UINT32 *pdwIndex)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwRqId, dwId, dwResult;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_PUSH_DCI_DATA);
   msg.setId(dwRqId);
   msg.setField(VID_NUM_ITEMS, dwNumItems);

   for(i = 0, dwId = VID_PUSH_DCI_DATA_BASE; i < dwNumItems; i++)
   {
      msg.setField(dwId++, pItems[i].dwNodeId);
      if (pItems[i].dwNodeId == 0)
      {
         msg.setField(dwId++, pItems[i].pszNodeName);
      }

      msg.setField(dwId++, pItems[i].dwId);
      if (pItems[i].dwId == 0)
      {
         msg.setField(dwId++, pItems[i].pszName);
      }

      msg.setField(dwId++, pItems[i].pszValue);
   }

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult != RCC_SUCCESS)
      {
         *pdwIndex = pResponse->getFieldAsUInt32(VID_FAILED_DCI_INDEX);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
      *pdwIndex = 0;
   }
   return dwResult;
}


//
// Get DCI info
//

UINT32 LIBNXCL_EXPORTABLE NXCGetDCIInfo(NXC_SESSION hSession, UINT32 dwNodeId,
                                       UINT32 dwItemId, NXC_DCI *pInfo)
{
   NXCPMessage msg, *pResponse;
   UINT32 dwRqId, dwResult;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_GET_DCI_INFO);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_ID, dwItemId);

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
			memset(pInfo, 0, sizeof(NXC_DCI));
			pInfo->dwId= dwItemId;
			pInfo->dwResourceId = pResponse->getFieldAsUInt32(VID_RESOURCE_ID);
			pInfo->dwTemplateId = pResponse->getFieldAsUInt32(VID_TEMPLATE_ID);
			pInfo->iDataType = (BYTE)pResponse->getFieldAsUInt16(VID_DCI_DATA_TYPE);
			pInfo->iSource = (BYTE)pResponse->getFieldAsUInt16(VID_DCI_SOURCE_TYPE);
			pResponse->getFieldAsString(VID_NAME, pInfo->szName, MAX_ITEM_NAME);
			pResponse->getFieldAsString(VID_DESCRIPTION, pInfo->szDescription, MAX_DB_STRING);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Get list of system DCIs
//

UINT32 LIBNXCL_EXPORTABLE NXCGetPerfTabDCIList(NXC_SESSION hSession, UINT32 dwNodeId,
                                             UINT32 *pdwNumItems, NXC_PERFTAB_DCI **ppList)
{
   NXCPMessage msg, *pResponse;
   UINT32 i, dwId, dwRqId, dwResult;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();
	*ppList = NULL;
	*pdwNumItems = 0;

   msg.setCode(CMD_GET_PERFTAB_DCI_LIST);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);

   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
			*pdwNumItems = pResponse->getFieldAsUInt32(VID_NUM_ITEMS);
			*ppList = (NXC_PERFTAB_DCI *)malloc(sizeof(NXC_PERFTAB_DCI) * (*pdwNumItems));
			for(i = 0, dwId = VID_SYSDCI_LIST_BASE; i < *pdwNumItems; i++, dwId += 6)
			{
				(*ppList)[i].dwId = pResponse->getFieldAsUInt32(dwId++);
				pResponse->getFieldAsString(dwId++, (*ppList)[i].szName, MAX_DB_STRING);
				(*ppList)[i].nStatus = pResponse->getFieldAsUInt16(dwId++);
				(*ppList)[i].pszSettings = pResponse->getFieldAsString(dwId++);
			}
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Clear collected data for DCI
//

UINT32 LIBNXCL_EXPORTABLE NXCClearDCIData(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId)
{
   UINT32 dwRqId;
   NXCPMessage msg;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_CLEAR_DCI_DATA);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_ID, dwItemId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Test DCI's transformation script
//

UINT32 LIBNXCL_EXPORTABLE NXCTestDCITransformation(NXC_SESSION hSession, UINT32 dwNodeId, UINT32 dwItemId,
																  const TCHAR *script, const TCHAR *value, BOOL *execStatus,
																  TCHAR *execResult, size_t resultBufSize)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg, *pResponse;

	CHECK_SESSION_HANDLE();

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.setCode(CMD_TEST_DCI_TRANSFORMATION);
   msg.setId(dwRqId);
   msg.setField(VID_OBJECT_ID, dwNodeId);
   msg.setField(VID_DCI_ID, dwItemId);
	msg.setField(VID_SCRIPT, script);
	msg.setField(VID_VALUE, value);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

	pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
			*execStatus = pResponse->getFieldAsUInt16(VID_EXECUTION_STATUS);
			pResponse->getFieldAsString(VID_EXECUTION_RESULT, execResult, (UINT32)resultBufSize);
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}
