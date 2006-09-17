/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
   m_dwTemplateItemId = 0;
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
   m_pszFormula = NULL;
   m_pScript = NULL;
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_iAdvSchedule = 0;
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;
}


//
// Create DCItem from another DCItem
//

DCItem::DCItem(const DCItem *pSrc)
{
   DWORD i;

   m_dwId = pSrc->m_dwId;
   m_dwTemplateId = pSrc->m_dwTemplateId;
   m_dwTemplateItemId = pSrc->m_dwTemplateItemId;
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
   m_pszFormula = NULL;
   m_pScript = NULL;
   NewFormula(pSrc->m_pszFormula);
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_tLastCheck = 0;
   m_iAdvSchedule = pSrc->m_iAdvSchedule;

   // Copy schedules
   m_dwNumSchedules = pSrc->m_dwNumSchedules;
   m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
   for(i = 0; i < m_dwNumSchedules; i++)
      m_ppScheduleList[i] = _tcsdup(pSrc->m_ppScheduleList[i]);

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
// delta_calculation,transformation,template_id,description,instance,
// template_item_id,adv_schedule
//

DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode)
{
   TCHAR *pszTmp, szQuery[256];
   DB_RESULT hTempResult;
   DWORD i;

   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   nx_strncpy(m_szName, DBGetField(hResult, iRow, 1), MAX_ITEM_NAME);
   DecodeSQLString(m_szName);
   m_iSource = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_iDataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_iStatus = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_iDeltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   m_pszFormula = NULL;
   m_pScript = NULL;
   pszTmp = _tcsdup(DBGetField(hResult, iRow, 8));
   DecodeSQLString(pszTmp);
   NewFormula(pszTmp);
   free(pszTmp);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   nx_strncpy(m_szDescription, DBGetField(hResult, iRow, 10), MAX_DB_STRING);
   DecodeSQLString(m_szDescription);
   nx_strncpy(m_szInstance, DBGetField(hResult, iRow, 11), MAX_DB_STRING);
   DecodeSQLString(m_szInstance);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_tLastCheck = 0;
   m_iAdvSchedule = (BYTE)DBGetFieldLong(hResult, iRow, 13);

   if (m_iAdvSchedule)
   {
      sprintf(szQuery, "SELECT schedule FROM dci_schedules WHERE item_id=%d", m_dwId);
      hTempResult = DBSelect(g_hCoreDB, szQuery);
      if (hTempResult != NULL)
      {
         m_dwNumSchedules = DBGetNumRows(hTempResult);
         m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
         for(i = 0; i < m_dwNumSchedules; i++)
         {
            m_ppScheduleList[i] = _tcsdup(DBGetField(hTempResult, i, 0));
            DecodeSQLString(m_ppScheduleList[i]);
         }
         DBFreeResult(hTempResult);
      }
      else
      {
         m_dwNumSchedules = 0;
         m_ppScheduleList = NULL;
      }
   }
   else
   {
      m_dwNumSchedules = 0;
      m_ppScheduleList = NULL;
   }

   // Load last raw value from database
   sprintf(szQuery, "SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d", m_dwId);
   hTempResult = DBSelect(g_hCoreDB, szQuery);
   if (hTempResult != NULL)
   {
      if (DBGetNumRows(hTempResult) > 0)
      {
         m_prevRawValue = DBGetField(hTempResult, 0, 0);
         m_tPrevValueTimeStamp = DBGetFieldULong(hTempResult, 0, 1);
         m_tLastPoll = m_tPrevValueTimeStamp;
      }
      DBFreeResult(hTempResult);
   }
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
   m_dwTemplateItemId = 0;
   nx_strncpy(m_szName, szName, MAX_ITEM_NAME);
   if (pszDescription != NULL)
      nx_strncpy(m_szDescription, pszDescription, MAX_DB_STRING);
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
   m_pszFormula = NULL;
   m_pScript = NULL;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_iAdvSchedule = 0;
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;

   UpdateCacheSize();
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
   for(i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);
   safe_free(m_ppScheduleList);
   safe_free(m_pszFormula);
   delete m_pScript;
   ClearCache();
   MutexDestroy(m_hMutex);
}


//
// Clear data cache
//

void DCItem::ClearCache(void)
{
   DWORD i;

   for(i = 0; i < m_dwCacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   m_ppValueCache = NULL;
   m_dwCacheSize = 0;
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
                    "WHERE item_id=%d ORDER BY sequence_number", m_dwId);
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

   //UpdateCacheSize();

   return bResult;
}


//
// Save to database
//

BOOL DCItem::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszEscName, *pszEscFormula, *pszEscDescr, *pszEscInstance, szQuery[2048];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE, bResult;

   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT item_id FROM items WHERE item_id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
   pszEscName = EncodeSQLString(m_szName);
   pszEscFormula = EncodeSQLString(CHECK_NULL_EX(m_pszFormula));
   pszEscDescr = EncodeSQLString(m_szDescription);
   pszEscInstance = EncodeSQLString(m_szInstance);
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO items (item_id,node_id,template_id,name,description,source,"
                       "datatype,polling_interval,retention_time,status,delta_calculation,"
                       "transformation,instance,template_item_id,adv_schedule)"
                       " VALUES (%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,"
                       "%d,'%s','%s',%d,%d)",
                       m_dwId, (m_pNode == NULL) ? (DWORD)0 : m_pNode->Id(), m_dwTemplateId,
                       pszEscName, pszEscDescr, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation,
                       pszEscFormula, pszEscInstance, m_dwTemplateItemId, m_iAdvSchedule);
   else
      sprintf(szQuery, "UPDATE items SET node_id=%d,template_id=%d,name='%s',source=%d,"
                       "datatype=%d,polling_interval=%d,retention_time=%d,status=%d,"
                       "delta_calculation=%d,transformation='%s',description='%s',"
                       "instance='%s',template_item_id=%d,adv_schedule=%d WHERE item_id=%d",
                       (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                       pszEscName, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation, pszEscFormula,
                       pszEscDescr, pszEscInstance, m_dwTemplateItemId,
                       m_iAdvSchedule, m_dwId);
   bResult = DBQuery(hdb, szQuery);
   free(pszEscName);
   free(pszEscFormula);
   free(pszEscDescr);
   free(pszEscInstance);

   // Save thresholds
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumThresholds; i++)
         m_ppThresholdList[i]->SaveToDB(hdb, i);
   }

   // Delete non-existing thresholds
   sprintf(szQuery, "SELECT threshold_id FROM thresholds WHERE item_id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
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
            sprintf(szQuery, "DELETE FROM thresholds WHERE threshold_id=%d", dwId);
            DBQuery(hdb, szQuery);
         }
      }
      DBFreeResult(hResult);
   }

   // Create record in raw_dci_values if needed
   sprintf(szQuery, "SELECT item_id FROM raw_dci_values WHERE item_id=%d", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         char *pszEscValue;

         pszEscValue = EncodeSQLString(m_prevRawValue.String());
         sprintf(szQuery, "INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time)"
                          " VALUES (%d,'%s',%ld)",
                 m_dwId, pszEscValue, m_tPrevValueTimeStamp);
         free(pszEscValue);
         DBQuery(hdb, szQuery);
      }
      DBFreeResult(hResult);
   }

   // Save schedules
   sprintf(szQuery, "DELETE FROM dci_schedules WHERE item_id=%d", m_dwId);
   DBQuery(hdb, szQuery);
   if (m_iAdvSchedule)
   {
      TCHAR *pszEscSchedule;
      DWORD i;

      for(i = 0; i < m_dwNumSchedules; i++)
      {
         pszEscSchedule = EncodeSQLString(m_ppScheduleList[i]);
         sprintf(szQuery, "INSERT INTO dci_schedules (item_id,schedule) VALUES (%d,'%s')",
                 m_dwId, pszEscSchedule);
         free(pszEscSchedule);
         DBQuery(hdb, szQuery);
      }
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
   pMsg->SetVariable(VID_TEMPLATE_ID, m_dwTemplateId);
   pMsg->SetVariable(VID_NAME, m_szName);
   pMsg->SetVariable(VID_DESCRIPTION, m_szDescription);
   pMsg->SetVariable(VID_INSTANCE, m_szInstance);
   pMsg->SetVariable(VID_POLLING_INTERVAL, (DWORD)m_iPollingInterval);
   pMsg->SetVariable(VID_RETENTION_TIME, (DWORD)m_iRetentionTime);
   pMsg->SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_iSource);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_iDataType);
   pMsg->SetVariable(VID_DCI_STATUS, (WORD)m_iStatus);
   pMsg->SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)m_iDeltaCalculation);
   pMsg->SetVariable(VID_DCI_FORMULA, CHECK_NULL_EX(m_pszFormula));
   pMsg->SetVariable(VID_NUM_THRESHOLDS, m_dwNumThresholds);
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < m_dwNumThresholds; i++, dwId++)
   {
      m_ppThresholdList[i]->CreateMessage(&dct);
      pMsg->SetVariable(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
   }
   pMsg->SetVariable(VID_ADV_SCHEDULE, (WORD)m_iAdvSchedule);
   if (m_iAdvSchedule)
   {
      pMsg->SetVariable(VID_NUM_SCHEDULES, m_dwNumSchedules);
      for(i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
         pMsg->SetVariable(dwId, m_ppScheduleList[i]);
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
   TCHAR *pszStr;

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
   pszStr = pMsg->GetVariableStr(VID_DCI_FORMULA);
   NewFormula(pszStr);
   free(pszStr);

   // Update schedules
   for(i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);
   m_iAdvSchedule = (BYTE)pMsg->GetVariableShort(VID_ADV_SCHEDULE);
   if (m_iAdvSchedule)
   {
      m_dwNumSchedules = pMsg->GetVariableLong(VID_NUM_SCHEDULES);
      m_ppScheduleList = (TCHAR **)realloc(m_ppScheduleList, sizeof(TCHAR *) * m_dwNumSchedules);
      for(i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
      {
         pszStr = pMsg->GetVariableStr(dwId);
         if (pszStr != NULL)
         {
            m_ppScheduleList[i] = pszStr;
         }
         else
         {
            m_ppScheduleList[i] = _tcsdup(_T("(null)"));
         }
      }
   }
   else
   {
      if (m_ppScheduleList != NULL)
      {
         free(m_ppScheduleList);
         m_ppScheduleList = NULL;
      }
      m_dwNumSchedules = 0;
   }

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
   UpdateCacheSize();
   Unlock();
}


//
// Process new value
//

void DCItem::NewValue(time_t tmTimeStamp, const char *pszOriginalValue)
{
   char *pszEscValue, szQuery[MAX_LINE_SIZE + 128];
   ItemValue rawValue, *pValue;

   Lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
   {
      Unlock();
      return;
   }

   // Save raw value into database
   pszEscValue = EncodeSQLString(pszOriginalValue);
   sprintf(szQuery, "UPDATE raw_dci_values SET raw_value='%s',last_poll_time=" TIME_T_FMT " WHERE item_id=%d",
           pszEscValue, tmTimeStamp, m_dwId);
   free(pszEscValue);
   QueueSQLRequest(szQuery);

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue(pszOriginalValue, (DWORD)tmTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;
   Transform(*pValue, tmTimeStamp - m_tPrevValueTimeStamp);
   m_prevRawValue = rawValue;
   m_tPrevValueTimeStamp = tmTimeStamp;

   // Save transformed value to database
   pszEscValue = EncodeSQLString(pValue->String());
   sprintf(szQuery, "INSERT INTO idata_%d (item_id,idata_timestamp,idata_value)"
                    " VALUES (%d," TIME_T_FMT ",'%s')", m_pNode->Id(), m_dwId, tmTimeStamp, pszEscValue);
   free(pszEscValue);
   QueueSQLRequest(szQuery);

   // Check thresholds and add value to cache
   CheckThresholds(*pValue);

   if (m_dwCacheSize > 0)
   {
      delete m_ppValueCache[m_dwCacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_dwCacheSize - 1));
      m_ppValueCache[0] = pValue;
   }
   else
   {
      delete pValue;
   }

   Unlock();
}


//
// Transform received value
//

void DCItem::Transform(ItemValue &value, time_t nElapsedTime)
{
   switch(m_iDeltaCalculation)
   {
      case DCM_SIMPLE:
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               value = (LONG)value - (LONG)m_prevRawValue;
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
            case DCI_DT_STRING:
               value = (LONG)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      case DCM_AVERAGE_PER_MINUTE:
         nElapsedTime /= 60;  // Convert to minutes
      case DCM_AVERAGE_PER_SECOND:
         // Check elapsed time to prevent divide-by-zero exception
         if (nElapsedTime == 0)
            nElapsedTime++;

         switch(m_iDataType)
         {
            case DCI_DT_INT:
               value = ((LONG)value - (LONG)m_prevRawValue) / (LONG)nElapsedTime;
               break;
            case DCI_DT_UINT:
               value = ((DWORD)value - (DWORD)m_prevRawValue) / (DWORD)nElapsedTime;
               break;
            case DCI_DT_INT64:
               value = ((INT64)value - (INT64)m_prevRawValue) / (INT64)nElapsedTime;
               break;
            case DCI_DT_UINT64:
               value = ((QWORD)value - (QWORD)m_prevRawValue) / (QWORD)nElapsedTime;
               break;
            case DCI_DT_FLOAT:
               value = ((double)value - (double)m_prevRawValue) / (double)nElapsedTime;
               break;
            case DCI_DT_STRING:
               // I don't see any meaning in "average delta per second (minute)" for string
               // values, so result will be 0 if there are no difference between
               // current and previous values, and 1 otherwise
               value = (LONG)((strcmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      default:    // Default is no transformation
         break;
   }

   if (m_pScript != NULL)
   {
      NXSL_Value *pValue;
      NXSL_Environment *pEnv;

      switch(m_iDataType)
      {
         case DCI_DT_INT:
            pValue = new NXSL_Value((LONG)value);
            break;
         case DCI_DT_UINT:
            pValue = new NXSL_Value((DWORD)value);
            break;
         case DCI_DT_INT64:
            pValue = new NXSL_Value((INT64)value);
            break;
         case DCI_DT_UINT64:
            pValue = new NXSL_Value((QWORD)value);
            break;
         case DCI_DT_FLOAT:
            pValue = new NXSL_Value((double)value);
            break;
         case DCI_DT_STRING:
            pValue = new NXSL_Value((char *)((const char *)value));
            break;
         default:
            pValue = new NXSL_Value;
            break;
      }

      pEnv = new NXSL_Environment;
      pEnv->SetLibrary(g_pScriptLibrary);

      if (m_pScript->Run(pEnv, 1, &pValue) == 0)
      {
         pValue = m_pScript->GetResult();
         if (pValue != NULL)
         {
            switch(m_iDataType)
            {
               case DCI_DT_INT:
                  value = pValue->GetValueAsInt32();
                  break;
               case DCI_DT_UINT:
                  value = pValue->GetValueAsUInt32();
                  break;
               case DCI_DT_INT64:
                  value = pValue->GetValueAsInt64();
                  break;
               case DCI_DT_UINT64:
                  value = pValue->GetValueAsUInt64();
                  break;
               case DCI_DT_FLOAT:
                  value = pValue->GetValueAsReal();
                  break;
               case DCI_DT_STRING:
                  value = pValue->GetValueAsCString();
                  break;
               default:
                  break;
            }
         }
      }
      else
      {
         TCHAR szBuffer[1024];

         _sntprintf(szBuffer, 1024, _T("DCI::%s::%d"),
                    (m_pNode != NULL) ? m_pNode->Name() : _T("(null)"), m_dwId);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, _T("ssd"), szBuffer,
                   m_pScript->GetErrorText(), m_dwId);
      }
   }
}


//
// Set new ID
//

void DCItem::ChangeBinding(DWORD dwNewId, Template *pNewNode)
{
   DWORD i;

   Lock();
   m_pNode = pNewNode;
   m_dwId = dwNewId;
   for(i = 0; i < m_dwNumThresholds; i++)
      m_ppThresholdList[i]->BindToItem(m_dwId);
   ClearCache();
   UpdateCacheSize();
   Unlock();
}


//
// Update required cache size depending on thresholds
// dwCondId is an identifier of calling condition object id. If it is not 0,
// GetCacheSizeForDCI should be called with bNoLock == TRUE for appropriate
// condition object
//

void DCItem::UpdateCacheSize(DWORD dwCondId)
{
   DWORD i, dwSize, dwRequiredSize;

   // Sanity check
   if (m_pNode == NULL)
   {
      DbgPrintf(AF_DEBUG_DC, _T("DCItem::UpdateCacheSize() called for DCI %d when m_pNode == NULL"), m_dwId);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if (m_pNode->Type() == OBJECT_NODE)
   {
      dwRequiredSize = 1;

      // Calculate required cache size
      for(i = 0; i < m_dwNumThresholds; i++)
         if (dwRequiredSize < m_ppThresholdList[i]->RequiredCacheSize())
            dwRequiredSize = m_ppThresholdList[i]->RequiredCacheSize();

      RWLockReadLock(g_rwlockConditionIndex, INFINITE);
      for(i = 0; i < g_dwConditionIndexSize; i++)
      {
         dwSize = ((Condition *)g_pConditionIndex[i].pObject)->GetCacheSizeForDCI(m_dwId, dwCondId == g_pConditionIndex[i].dwKey);
         if (dwSize > dwRequiredSize)
            dwRequiredSize = dwSize;
      }
      RWLockUnlock(g_rwlockConditionIndex);
   }
   else
   {
      dwRequiredSize = 0;
   }

   // Update cache if needed
   if (dwRequiredSize < m_dwCacheSize)
   {
      // Destroy unneeded values
      if (m_dwCacheSize > 0)
         for(i = dwRequiredSize; i < m_dwCacheSize; i++)
            delete m_ppValueCache[i];

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

         switch(g_dwDBSyntax)
         {
            case DB_SYNTAX_MSSQL:
               sprintf(szBuffer, "SELECT TOP %d idata_value,idata_timestamp FROM idata_%d "
                                 "WHERE item_id=%d ORDER BY idata_timestamp DESC",
                       dwRequiredSize, m_pNode->Id(), m_dwId);
               break;
            case DB_SYNTAX_ORACLE:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%d "
                                 "WHERE item_id=%d AND ROWNUM <= %d ORDER BY idata_timestamp DESC",
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            case DB_SYNTAX_MYSQL:
            case DB_SYNTAX_PGSQL:
            case DB_SYNTAX_SQLITE:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%d "
                                 "WHERE item_id=%d ORDER BY idata_timestamp DESC LIMIT %d",
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            default:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%d "
                                 "WHERE item_id=%d ORDER BY idata_timestamp DESC",
                       m_pNode->Id(), m_dwId);
               break;
         }
         hResult = DBAsyncSelect(g_hCoreDB, szBuffer);
         if (hResult != NULL)
         {
            // Skip already cached values
            for(i = 0, bHasData = TRUE; i < m_dwCacheSize; i++)
               bHasData = DBFetch(hResult);

            // Create new cache entries
            for(; (i < dwRequiredSize) && bHasData; i++)
            {
               bHasData = DBFetch(hResult);
               if (bHasData)
               {
                  DBGetFieldAsync(hResult, 0, szBuffer, MAX_DB_STRING);
                  DecodeSQLString(szBuffer);
                  m_ppValueCache[i] = 
                     new ItemValue(szBuffer, DBGetFieldAsyncULong(hResult, 1));
               }
               else
               {
                  m_ppValueCache[i] = new ItemValue(_T(""), 1);   // Empty value
               }
            }

            // Fill up cache with empty values if we don't have enough values in database
            for(; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);

            DBFreeAsyncResult(g_hCoreDB, hResult);
         }
         else
         {
            // Error reading data from database, fill cache with empty values
            for(i = m_dwCacheSize; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
      }
      m_dwCacheSize = dwRequiredSize;
   }
   m_bCacheLoaded = TRUE;
}


//
// Put last value into CSCP message
//

void DCItem::GetLastValue(CSCPMessage *pMsg, DWORD dwId)
{
   pMsg->SetVariable(dwId++, m_dwId);
   pMsg->SetVariable(dwId++, m_szName);
   pMsg->SetVariable(dwId++, m_szDescription);
   pMsg->SetVariable(dwId++, (WORD)m_iSource);
   if (m_dwCacheSize > 0)
   {
      pMsg->SetVariable(dwId++, (WORD)m_iDataType);
      pMsg->SetVariable(dwId++, (TCHAR *)m_ppValueCache[0]->String());
      pMsg->SetVariable(dwId++, m_ppValueCache[0]->GetTimeStamp());
   }
   else
   {
      pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);
      pMsg->SetVariable(dwId++, _T(""));
      pMsg->SetVariable(dwId++, (DWORD)0);
   }
}


//
// Get item's last value for use in NXSL
//

NXSL_Value *DCItem::GetValueForNXSL(int nFunction, int nPolls)
{
   NXSL_Value *pValue;

   switch(nFunction)
   {
      case F_LAST:
         pValue = (m_dwCacheSize > 0) ? new NXSL_Value((TCHAR *)m_ppValueCache[0]->String()) : new NXSL_Value;
         break;
      case F_DIFF:
         if (m_dwCacheSize >= 2)
         {
            ItemValue result;

            CalculateItemValueDiff(result, m_iDataType, *m_ppValueCache[0], *m_ppValueCache[1]);
            pValue = new NXSL_Value((TCHAR *)result.String());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_AVERAGE:
         if (m_dwCacheSize > 0)
         {
            ItemValue result;

            CalculateItemValueAverage(result, m_iDataType,
                                      min(m_dwCacheSize, (DWORD)nPolls), m_ppValueCache);
            pValue = new NXSL_Value((TCHAR *)result.String());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      default:
         pValue = new NXSL_Value;
         break;
   }
   return pValue;
}


//
// Clean expired data
//

void DCItem::CleanData(void)
{
   TCHAR szQuery[256];
   time_t now;

   now = time(NULL);
   Lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%d WHERE (item_id=%d) AND (idata_timestamp<%ld)"),
              m_pNode->Id(), m_dwId, now - (time_t)m_iRetentionTime * 86400);
   Unlock();
   QueueSQLRequest(szQuery);
}


//
// Prepare item for deletion
//

void DCItem::PrepareForDeletion(void)
{
   Lock();

   m_iStatus = ITEM_STATUS_DISABLED;   // Prevent future polls

   // Wait until current poll ends, if any
   // while() is not a very good solution, and probably need to be
   // rewrited using conditions
   while(m_iBusy)
   {
      Unlock();
      ThreadSleepMs(100);
      Lock();
   }

   Unlock();
}


//
// Match schedule element
// NOTE: We assume that pattern can be modified during processing
//

static BOOL MatchScheduleElement(TCHAR *pszPattern, int nValue)
{
   TCHAR *ptr, *curr;
   int nStep, nCurr, nPrev;
   BOOL bRun = TRUE, bRange = FALSE;

   // Check if step was specified
   ptr = _tcschr(pszPattern, _T('/'));
   if (ptr != NULL)
   {
      *ptr = 0;
      ptr++;
      nStep = atoi(ptr);
   }
   else
   {
      nStep = 1;
   }

   if (*pszPattern == _T('*'))
      goto check_step;

   for(curr = pszPattern; bRun; curr = ptr + 1)
   {
      for(ptr = curr; (*ptr != 0) && (*ptr != '-') && (*ptr != ','); ptr++);
      switch(*ptr)
      {
         case '-':
            if (bRange)
               return FALSE;  // Form like 1-2-3 is invalid
            bRange = TRUE;
            *ptr = 0;
            nPrev = atoi(curr);
            break;
         case 0:
            bRun = FALSE;
         case ',':
            *ptr = 0;
            nCurr = atoi(curr);
            if (bRange)
            {
               if ((nValue >= nPrev) && (nValue <= nCurr))
                  goto check_step;
               bRange = FALSE;
            }
            else
            {
               if (nValue == nCurr)
                  return TRUE;
            }
            break;
      }
   }

   return FALSE;

check_step:
   return (nValue % nStep) == 0;
}


//
// Match schedule to current time
//

static BOOL MatchSchedule(struct tm *pCurrTime, TCHAR *pszSchedule)
{
   TCHAR *pszCurr, szValue[256];

   // Minute
   pszCurr = ExtractWord(pszSchedule, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_min))
         return FALSE;

   // Hour
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_hour))
         return FALSE;

   // Day of month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_mday))
         return FALSE;

   // Month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, pCurrTime->tm_mon + 1))
         return FALSE;

   // Day of week
   ExtractWord(pszCurr, szValue);
   TranslateStr(szValue, _T("7"), _T("0"));
   return MatchScheduleElement(szValue, pCurrTime->tm_wday);
}


//
// Check if DCI have to be polled
//

BOOL DCItem::ReadyForPolling(time_t currTime)
{
   BOOL bResult;

   Lock();
   if ((m_iStatus == ITEM_STATUS_ACTIVE) && (!m_iBusy) && m_bCacheLoaded)
   {
      if (m_iAdvSchedule)
      {
         DWORD i;
         struct tm tmCurrLocal, tmLastLocal;

         memcpy(&tmCurrLocal, localtime(&currTime), sizeof(struct tm));
         memcpy(&tmLastLocal, localtime(&m_tLastCheck), sizeof(struct tm));
         for(i = 0, bResult = FALSE; i < m_dwNumSchedules; i++)
         {
            if (MatchSchedule(&tmCurrLocal, m_ppScheduleList[i]))
            {
               if (!MatchSchedule(&tmLastLocal, m_ppScheduleList[i]))
               {
                  bResult = TRUE;
                  break;
               }
            }
         }
         m_tLastCheck = currTime;
      }
      else
      {
         bResult = (m_tLastPoll + m_iPollingInterval <= currTime);
      }
   }
   else
   {
      bResult = FALSE;
   }
   Unlock();
   return bResult;
}


//
// Update from template item
//

void DCItem::UpdateFromTemplate(DCItem *pItem)
{
   DWORD i, dwCount;

   Lock();

   m_iDataType = pItem->m_iDataType;
   m_iPollingInterval = pItem->m_iPollingInterval;
   m_iRetentionTime = pItem->m_iRetentionTime;
   m_iDeltaCalculation = pItem->m_iDeltaCalculation;
   m_iSource = pItem->m_iSource;
   m_iStatus = pItem->m_iStatus;
   _tcscpy(m_szName, pItem->m_szName);
   _tcscpy(m_szDescription, pItem->m_szDescription);
   _tcscpy(m_szInstance, pItem->m_szInstance);
   NewFormula(pItem->m_pszFormula);
   m_iAdvSchedule = pItem->m_iAdvSchedule;

   // Copy schedules
   for(i = 0; i < m_dwNumSchedules; i++)
      safe_free(m_ppScheduleList[i]);
   safe_free(m_ppScheduleList);
   m_dwNumSchedules = pItem->m_dwNumSchedules;
   m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
   for(i = 0; i < m_dwNumSchedules; i++)
      m_ppScheduleList[i] = _tcsdup(pItem->m_ppScheduleList[i]);

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
   dwCount = min(m_dwNumThresholds, pItem->m_dwNumThresholds);
   for(i = 0; i < dwCount; i++)
      if (!m_ppThresholdList[i]->Compare(pItem->m_ppThresholdList[i]))
         break;
   dwCount = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
   for(; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];

   // (Re)create thresholds starting from first unmatched
   m_dwNumThresholds = pItem->m_dwNumThresholds;
   m_ppThresholdList = (Threshold **)realloc(m_ppThresholdList, sizeof(Threshold *) * m_dwNumThresholds);
   for(i = dwCount; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i] = new Threshold(pItem->m_ppThresholdList[i]);
      m_ppThresholdList[i]->CreateId();
      m_ppThresholdList[i]->BindToItem(m_dwId);
   }

   UpdateCacheSize();
   
   Unlock();
}


//
// Set new formula
//

void DCItem::NewFormula(TCHAR *pszFormula)
{
   safe_free(m_pszFormula);
   delete m_pScript;
   if (pszFormula != NULL)
   {
      m_pszFormula = _tcsdup(pszFormula);
      StrStrip(m_pszFormula);
      if (m_pszFormula[0] != 0)
      {
         m_pScript = (NXSL_Program *)NXSLCompile(m_pszFormula, NULL, 0);
      }
      else
      {
         m_pScript = NULL;
      }
   }
   else
   {
      m_pszFormula = NULL;
      m_pScript = NULL;
   }
}
