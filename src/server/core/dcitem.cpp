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

#include "nms_core.h"


//
// Default constructor for DCItem
//

DCItem::DCItem()
{
   m_dwId = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_iBusy = 0;
   m_iDataType = DTYPE_INTEGER;
   m_iPollingInterval = 3600;
   m_iRetentionTime = 0;
   m_iSource = DS_INTERNAL;
   m_iStatus = ITEM_STATUS_NOT_SUPPORTED;
   m_szName[0] = 0;
   m_tLastPoll = 0;
   m_pNode = NULL;
   m_hMutex = MutexCreate();
}


//
// Constructor for creating DCItem from database
// Assumes that fields in SELECT query are in following order:
// item_id,name,source,datatype,polling_interval,retention_time,status
//

DCItem::DCItem(DB_RESULT hResult, int iRow, Node *pNode)
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   strcpy(m_szName, DBGetField(hResult, iRow, 1));
   m_iSource = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_iDataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_iStatus = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
}


//
// Constructor for creating new DCItem from scratch
//

DCItem::DCItem(DWORD dwId, char *szName, int iSource, int iDataType, 
               int iPollingInterval, int iRetentionTime, Node *pNode)
{
   m_dwId = dwId;
   strncpy(m_szName, szName, MAX_ITEM_NAME);
   m_iSource = iSource;
   m_iDataType = iDataType;
   m_iPollingInterval = iPollingInterval;
   m_iRetentionTime = iRetentionTime;
   m_iStatus = ITEM_STATUS_ACTIVE;
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
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

   return bResult;
}


//
// Save to database
//

BOOL DCItem::SaveToDB(void)
{
   char szQuery[512];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE, bResult;

   // Paranoid check, DCItem::SaveToDB() normally called only
   // for items binded to some node object
   if (m_pNode == NULL)
      return FALSE;

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
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO items (item_id,node_id,name,description,source,"
                       "datatype,polling_interval,retention_time,status) VALUES "
                       "(%ld,%ld,'%s','',%d,%d,%ld,%ld,%d)", m_dwId, m_pNode->Id(),
                       m_szName, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus);
   else
      sprintf(szQuery, "UPDATE items SET node_id=%ld,name='%s',source=%d,datatype=%d,"
                       "polling_interval=%ld,retention_time=%ld,status=%d WHERE item_id=%ld",
                       m_pNode->Id(), m_szName, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_dwId);
   bResult = DBQuery(g_hCoreDB, szQuery);

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

void DCItem::CheckThresholds(const char *pszLastValue)
{
   DWORD i, iResult;

   Lock();
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      iResult = m_ppThresholdList[i]->Check(pszLastValue);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEvent(m_ppThresholdList[i]->EventCode(), m_pNode->Id(), "sss", m_szName,
                      m_ppThresholdList[i]->Value(), pszLastValue);
            i = m_dwNumThresholds;  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEvent(EVENT_THRESHOLD_REARMED, m_pNode->Id(), "s", m_szName);
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
   pMsg->SetVariable(VID_POLLING_INTERVAL, (DWORD)m_iPollingInterval);
   pMsg->SetVariable(VID_RETENTION_TIME, (DWORD)m_iRetentionTime);
   pMsg->SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_iSource);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_iDataType);
   pMsg->SetVariable(VID_DCI_STATUS, (WORD)m_iStatus);
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
   m_iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
   m_iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
   m_iPollingInterval = pMsg->GetVariableLong(VID_POLLING_INTERVAL);
   m_iRetentionTime = pMsg->GetVariableLong(VID_RETENTION_TIME);
   m_iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);

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
