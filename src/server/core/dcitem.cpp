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
** $module: dcitem.cpp
**
**/

#include "nxcore.h"


//
// Default constructor for DCItem
//

DCItem::DCItem()
{
   m_dwId = 0;
   m_dwTemplateId = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_iBusy = 0;
   m_iDataType = DCI_DT_INT;
   m_iPollingInterval = 3600;
   m_iRetentionTime = 0;
   m_iDeltaCalculation = DCM_ORIGINAL_VALUE;
   m_iSource = DS_INTERNAL;
   m_iStatus = ITEM_STATUS_NOT_SUPPORTED;
   m_szName[0] = 0;
   m_szDescription[0] = 0;
   m_szInstance[0] = 0;
   m_tLastPoll = 0;
   m_pszFormula = _tcsdup(_T(""));
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
}


//
// Create DCItem from another DCItem
//

DCItem::DCItem(const DCItem *pSrc)
{
   DWORD i;

   m_dwId = pSrc->m_dwId;
   m_dwTemplateId = pSrc->m_dwTemplateId;
   m_iBusy = 0;
   m_iDataType = pSrc->m_iDataType;
   m_iPollingInterval = pSrc->m_iPollingInterval;
   m_iRetentionTime = pSrc->m_iRetentionTime;
   m_iDeltaCalculation = pSrc->m_iDeltaCalculation;
   m_iSource = pSrc->m_iSource;
   m_iStatus = pSrc->m_iStatus;
   _tcscpy(m_szName, pSrc->m_szName);
   _tcscpy(m_szDescription, pSrc->m_szDescription);
   _tcscpy(m_szInstance, pSrc->m_szInstance);
   m_tLastPoll = 0;
   m_pszFormula = _tcsdup(pSrc->m_pszFormula);
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;

   // Copy thresholds
   m_dwNumThresholds = pSrc->m_dwNumThresholds;
   m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i] = new Threshold(pSrc->m_ppThresholdList[i]);
      m_ppThresholdList[i]->CreateId();
   }
}


//
// Constructor for creating DCItem from database
// Assumes that fields in SELECT query are in following order:
// item_id,name,source,datatype,polling_interval,retention_time,status,
// delta_calculation,transformation,template_id,description,instance
//

DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode)
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   strncpy(m_szName, DBGetField(hResult, iRow, 1), MAX_ITEM_NAME);
   m_iSource = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_iDataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_iStatus = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_iDeltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   m_pszFormula = strdup(DBGetField(hResult, iRow, 8));
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   strncpy(m_szDescription, DBGetField(hResult, iRow, 10), MAX_DB_STRING);
   DecodeSQLString(m_szDescription);
   strncpy(m_szInstance, DBGetField(hResult, iRow, 11), MAX_DB_STRING);
   DecodeSQLString(m_szInstance);
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
}


//
// Constructor for creating new DCItem from scratch
//

DCItem::DCItem(DWORD dwId, char *szName, int iSource, int iDataType, 
               int iPollingInterval, int iRetentionTime, Template *pNode,
               char *pszDescription)
{
   m_dwId = dwId;
   m_dwTemplateId = 0;
   strncpy(m_szName, szName, MAX_ITEM_NAME);
   if (pszDescription != NULL)
      strncpy(m_szDescription, pszDescription, MAX_DB_STRING);
   else
      strcpy(m_szDescription, m_szName);
   m_szInstance[0] = 0;
   m_iSource = iSource;
   m_iDataType = iDataType;
   m_iPollingInterval = iPollingInterval;
   m_iRetentionTime = iRetentionTime;
   m_iDeltaCalculation = DCM_ORIGINAL_VALUE;
   m_iStatus = ITEM_STATUS_ACTIVE;
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_pszFormula = strdup("");
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
}


//
// Destructor for DCItem
//

DCItem::~DCItem()
{
   DWORD i;

   for(i = 0; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];
   safe_free(m_ppThresholdList);
   safe_free(m_pszFormula);
   for(i = 0; i < m_dwCacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   MutexDestroy(m_hMutex);
}


//
// Load data collection items thresholds from database
//

BOOL DCItem::LoadThresholdsFromDB(void)
{
   DWORD i;
   char szQuery[256];
   DB_RESULT hResult;
   BOOL bResult = FALSE;

   sprintf(szQuery, "SELECT threshold_id,fire_value,rearm_value,check_function,"
                    "check_operation,parameter_1,parameter_2,event_code FROM thresholds "
                    "WHERE item_id=%ld ORDER BY sequence_number", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumThresholds = DBGetNumRows(hResult);
      if (m_dwNumThresholds > 0)
      {
         m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
         for(i = 0; i < m_dwNumThresholds; i++)
            m_ppThresholdList[i] = new Threshold(hResult, i, this);
      }
      DBFreeResult(hResult);
      bResult = TRUE;
   }

   UpdateCacheSize();

   return bResult;
}


//
// Save to database
//

BOOL DCItem::SaveToDB(void)
{
   char *pszEscFormula, *pszEscDescr, *pszEscInstance, szQuery[1024];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE, bResult;

   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT item_id FROM items WHERE item_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
   pszEscFormula = EncodeSQLString(m_pszFormula);
   pszEscDescr = EncodeSQLString(m_szDescription);
   pszEscInstance = EncodeSQLString(m_szInstance);
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO items (item_id,node_id,template_id,name,description,source,"
                       "datatype,polling_interval,retention_time,status,delta_calculation,"
                       "transformation,instance) VALUES (%ld,%ld,%ld,'%s','%s',%d,%d,%ld,%ld,%d,%d,'%s','%s')",
                       m_dwId, (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                       m_szName, pszEscDescr, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation, pszEscFormula, pszEscInstance);
   else
      sprintf(szQuery, "UPDATE items SET node_id=%ld,template_id=%ld,name='%s',source=%d,"
                       "datatype=%d,polling_interval=%ld,retention_time=%ld,status=%d,"
                       "delta_calculation=%d,transformation='%s',description='%s',"
                       "instance='%s' WHERE item_id=%ld",
                       (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                       m_szName, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation, pszEscFormula,
                       pszEscDescr, pszEscInstance, m_dwId);
   bResult = DBQuery(g_hCoreDB, szQuery);
   free(pszEscFormula);
   free(pszEscDescr);
   free(pszEscInstance);

   // Save thresholds
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumThresholds; i++)
         m_ppThresholdList[i]->SaveToDB(i);
   }

   // Delete non-existing thresholds
   sprintf(szQuery, "SELECT threshold_id FROM thresholds WHERE item_id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      int i, iNumRows;
      DWORD j, dwId;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         for(j = 0; j < m_dwNumThresholds; j++)
            if (m_ppThresholdList[j]->Id() == dwId)
               break;
         if (j == m_dwNumThresholds)
         {
            sprintf(szQuery, "DELETE FROM thresholds WHERE threshold_id=%ld", dwId);
            DBQuery(g_hCoreDB, szQuery);
         }
      }
      DBFreeResult(hResult);
   }

   Unlock();
   return bResult;
}


//
// Check last value for threshold breaches
//

void DCItem::CheckThresholds(ItemValue &value)
{
   DWORD i, iResult;
   ItemValue checkValue;

   Lock();
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      iResult = m_ppThresholdList[i]->Check(value, m_ppValueCache, checkValue);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEvent(m_ppThresholdList[i]->EventCode(), m_pNode->Id(), "ssssis", m_szName,
                      m_szDescription, m_ppThresholdList[i]->StringValue(), 
                      (const char *)checkValue, m_dwId, m_szInstance);
            i = m_dwNumThresholds;  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEvent(EVENT_THRESHOLD_REARMED, m_pNode->Id(), "ssi", m_szName, 
                      m_szDescription, m_dwId);
            break;
         case NO_ACTION:
            if (m_ppThresholdList[i]->IsReached())
               i = m_dwNumThresholds;  // Threshold condition still true, stop processing
            break;
      }
   }
   Unlock();
}


//
// Create CSCP message with item data
//

void DCItem::CreateMessage(CSCPMessage *pMsg)
{
   DCI_THRESHOLD dct;
   DWORD i, dwId;

   Lock();
   pMsg->SetVariable(VID_DCI_ID, m_dwId);
   pMsg->SetVariable(VID_NAME, m_szName);
   pMsg->SetVariable(VID_DESCRIPTION, m_szDescription);
   pMsg->SetVariable(VID_INSTANCE, m_szInstance);
   pMsg->SetVariable(VID_POLLING_INTERVAL, (DWORD)m_iPollingInterval);
   pMsg->SetVariable(VID_RETENTION_TIME, (DWORD)m_iRetentionTime);
   pMsg->SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_iSource);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_iDataType);
   pMsg->SetVariable(VID_DCI_STATUS, (WORD)m_iStatus);
   pMsg->SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)m_iDeltaCalculation);
   pMsg->SetVariable(VID_DCI_FORMULA, m_pszFormula);
   pMsg->SetVariable(VID_NUM_THRESHOLDS, m_dwNumThresholds);
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < m_dwNumThresholds; i++, dwId++)
   {
      m_ppThresholdList[i]->CreateMessage(&dct);
      pMsg->SetVariable(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
   }
   Unlock();
}


//
// Delete item and collected data from database
//

void DCItem::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "DELETE FROM items WHERE item_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
   sprintf(szQuery, "DELETE FROM idata_%d WHERE item_id=%d", m_pNode->Id(), m_dwId);
   QueueSQLRequest(szQuery);
   sprintf(szQuery, "DELETE FROM thresholds WHERE item_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
}


//
// Update item from CSCP message
//

void DCItem::UpdateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                               DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i, j, dwNum, dwId;
   DCI_THRESHOLD *pNewThresholds;
   Threshold **ppNewList;

   Lock();

   pMsg->GetVariableStr(VID_NAME, m_szName, MAX_ITEM_NAME);
   pMsg->GetVariableStr(VID_DESCRIPTION, m_szDescription, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_INSTANCE, m_szInstance, MAX_DB_STRING);
   m_iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
   m_iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
   m_iPollingInterval = pMsg->GetVariableLong(VID_POLLING_INTERVAL);
   m_iRetentionTime = pMsg->GetVariableLong(VID_RETENTION_TIME);
   m_iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
   m_iDeltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
   safe_free(m_pszFormula);
   m_pszFormula = pMsg->GetVariableStr(VID_DCI_FORMULA);

   // Update thresholds
   dwNum = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
   pNewThresholds = (DCI_THRESHOLD *)malloc(sizeof(DCI_THRESHOLD) * dwNum);
   *ppdwMapIndex = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *ppdwMapId = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *pdwNumMaps = 0;

   // Read all thresholds from message
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId++)
   {
      pMsg->GetVariableBinary(dwId, (BYTE *)&pNewThresholds[i], sizeof(DCI_THRESHOLD));
      pNewThresholds[i].dwId = ntohl(pNewThresholds[i].dwId);
   }
   
   // Check if some thresholds was deleted, and reposition others if needed
   ppNewList = (Threshold **)malloc(sizeof(Threshold *) * dwNum);
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      for(j = 0; j < dwNum; j++)
         if (m_ppThresholdList[i]->Id() == pNewThresholds[j].dwId)
            break;
      if (j == dwNum)
      {
         // No threshold with that id in new list, delete it
         delete m_ppThresholdList[i];
         m_dwNumThresholds--;
         memmove(&m_ppThresholdList[i], &m_ppThresholdList[i + 1], sizeof(Threshold *) * (m_dwNumThresholds - i));
         i--;
      }
      else
      {
         // Move existing thresholds to appropriate positions in new list
         ppNewList[j] = m_ppThresholdList[i];
      }
   }
   safe_free(m_ppThresholdList);
   m_ppThresholdList = ppNewList;
   m_dwNumThresholds = dwNum;

   // Add or update thresholds
   for(i = 0; i < dwNum; i++)
   {
      if (pNewThresholds[i].dwId == 0)    // New threshold?
      {
         m_ppThresholdList[i] = new Threshold(this);
         m_ppThresholdList[i]->CreateId();

         // Add index -> id mapping
         (*ppdwMapIndex)[*pdwNumMaps] = i;
         (*ppdwMapId)[*pdwNumMaps] = m_ppThresholdList[i]->Id();
         (*pdwNumMaps)++;
      }
      m_ppThresholdList[i]->UpdateFromMessage(&pNewThresholds[i]);
   }
      
   safe_free(pNewThresholds);
   Unlock();
}


//
// Process new value
//

void DCItem::NewValue(DWORD dwTimeStamp, const char *pszOriginalValue)
{
   char szQuery[MAX_LINE_SIZE + 128];
   ItemValue rawValue, *pValue;

   // Normally m_pNode shouldn't be NULL for polled items,
   // but who knows...
   if (m_pNode == NULL)
      return;

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue(pszOriginalValue);
   if (m_tLastPoll == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;
   Transform(*pValue);
   m_prevRawValue = rawValue;

   // Save transformed value to database
   sprintf(szQuery, "INSERT INTO idata_%ld (item_id,idata_timestamp,idata_value)"
                    " VALUES (%ld,%ld,'%s')", m_pNode->Id(), m_dwId, dwTimeStamp, 
           pValue->String());
   QueueSQLRequest(szQuery);

   // Check thresholds and add value to cache
   CheckThresholds(*pValue);

   if (m_dwCacheSize > 0)
   {
      Lock();
      delete m_ppValueCache[m_dwCacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_dwCacheSize - 1));
      m_ppValueCache[0] = pValue;
      Unlock();
   }
   else
   {
      delete pValue;
   }
}


//
// Transform received value
//

void DCItem::Transform(ItemValue &value)
{
   switch(m_iDeltaCalculation)
   {
      case DCM_SIMPLE:
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               value = (long)value - (long)m_prevRawValue;
               break;
            case DCI_DT_UINT:
               value = (DWORD)value - (DWORD)m_prevRawValue;
               break;
            case DCI_DT_INT64:
               value = (INT64)value - (INT64)m_prevRawValue;
               break;
            case DCI_DT_UINT64:
               value = (QWORD)value - (QWORD)m_prevRawValue;
               break;
            case DCI_DT_FLOAT:
               value = (double)value - (double)m_prevRawValue;
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      case DCM_AVERAGE_PER_SECOND:
         break;
      case DCM_AVERAGE_PER_MINUTE:
         break;
      default:    // Default is no transformation
         break;
   }
}


//
// Set new ID
//

void DCItem::SetId(DWORD dwNewId)
{
   DWORD i;

   Lock();
   m_dwId = dwNewId;
   for(i = 0; i < m_dwNumThresholds; i++)
      m_ppThresholdList[i]->BindToItem(m_dwId);
   Unlock();
}


//
// Update required cache size depending on thresholds
//

void DCItem::UpdateCacheSize(void)
{
   DWORD i, dwRequiredSize;

   for(i = 0, dwRequiredSize = 0; i < m_dwNumThresholds; i++)
      if (dwRequiredSize < m_ppThresholdList[i]->RequiredCacheSize())
         dwRequiredSize = m_ppThresholdList[i]->RequiredCacheSize();
   if (dwRequiredSize < m_dwCacheSize)
   {
      m_dwCacheSize = dwRequiredSize;
      if (m_dwCacheSize > 0)
      {
         m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * m_dwCacheSize);
      }
      else
      {
         safe_free(m_ppValueCache);
         m_ppValueCache = NULL;
      }
   }
   else if (dwRequiredSize > m_dwCacheSize)
   {
      // Expand cache
      m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * dwRequiredSize);
      for(i = m_dwCacheSize; i < dwRequiredSize; i++)
         m_ppValueCache[i] = NULL;

      // Load missing values from database
      if (m_pNode != NULL)
      {
         DB_ASYNC_RESULT hResult;
         char szBuffer[MAX_DB_STRING];
         BOOL bHasData;

         sprintf(szBuffer, "SELECT idata_value FROM idata_%ld "
                           "WHERE item_id=%ld ORDER BY idata_timestamp DESC",
                 m_pNode->Id(), m_dwId);
         hResult = DBAsyncSelect(g_hCoreDB, szBuffer);
         if (hResult != NULL)
         {
            // Skip already cached values
            for(i = 0, bHasData = TRUE; (i < m_dwCacheSize) && bHasData; i++)
               bHasData = DBFetch(hResult);

            // Create new cache entries
            for(; (i < dwRequiredSize) && bHasData; i++)
            {
               bHasData = DBFetch(hResult);
               if (bHasData)
               {
                  m_ppValueCache[i] = new ItemValue(DBGetFieldAsync(hResult, 0, szBuffer, MAX_DB_STRING));
               }
               else
               {
                  m_ppValueCache[i] = new ItemValue(_T(""));   // Empty value
               }
            }

            // Fill up cache with empty values if we don't have enough values in database
            for(; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""));

            DBFreeAsyncResult(hResult);
         }
         else
         {
            // Error reading data from database, fill cache with empty values
            for(i = 0; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""));
         }
      }
      m_dwCacheSize = dwRequiredSize;
   }
}
