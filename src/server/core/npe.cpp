/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: npe.cpp
**
**/

#include "nxcore.h"

/**
 * Base class constructor
 */
PredictionEngine::PredictionEngine()
{
}

/**
 * Base class destructor
 */
PredictionEngine::~PredictionEngine()
{
}

/**
 * Default initialization method - always returns true
 */
bool PredictionEngine::initialize(TCHAR *errorMessage)
{
   errorMessage[0] = 0;
   return true;
}

/**
 * Default implementation - always return false
 */
bool PredictionEngine::requiresTraining()
{
   return false;
}

/**
 * Default trainig method - do nothing
 */
void PredictionEngine::train(UINT32 nodeId, UINT32 dciId)
{
}

/**
 * Helper method to read last N values of given DCI
 */
StructArray<DciValue> *PredictionEngine::getDciValues(UINT32 nodeId, UINT32 dciId, int maxRows)
{
   TCHAR query[1024];
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         _sntprintf(query, 1024, _T("SELECT TOP %d idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC"), maxRows, nodeId, dciId);
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(query, 1024, _T("SELECT * FROM (SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC) WHERE ROWNUM<=%d"), nodeId, dciId, maxRows);
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %d"), nodeId, dciId, maxRows);
         break;
      case DB_SYNTAX_DB2:
         _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC ETCH FIRST %d ROWS ONLY"), nodeId, dciId, maxRows);
         break;
      default:
         nxlog_debug(1, _T("INTERNAL ERROR: unsupported database in PredictionEngine::getDciValues"));
         return NULL;   // Unsupported database
   }

   StructArray<DciValue> *values = NULL;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      values = new StructArray<DciValue>(count);
      for(int i = 0; i < count; i++)
      {
         DciValue v;
         v.timestamp = DBGetFieldULong(hResult, i, 0);
         v.value = DBGetFieldDouble(hResult, i, 1);
         values->add(&v);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return values;
}

/**
 * Prediction engine registry
 */
static StringObjectMap<PredictionEngine> s_engines(true);

/**
 * Prediction engine thread pool
 */
ThreadPool *g_npeThreadPool = NULL;

/**
 * Register prediction engines on startup
 */
void RegisterPredictionEngines()
{
   g_npeThreadPool = ThreadPoolCreate(0, 1024, _T("NPE"));
   ENUMERATE_MODULES(pfGetPredictionEngines)
   {
      ObjectArray<PredictionEngine> *engines = g_pModuleList[__i].pfGetPredictionEngines();
      engines->setOwner(false);
      for(int i = 0; i < engines->size(); i++)
      {
         PredictionEngine *e = engines->get(i);
         TCHAR errorMessage[MAX_NPE_ERROR_MESSAGE_LEN];
         if (e->initialize(errorMessage))
         {
            s_engines.set(e->getName(), e);
            nxlog_write(MSG_NPE_REGISTERED, NXLOG_INFO, "ss", e->getName(), e->getVersion());
         }
         else
         {
            nxlog_write(MSG_NPE_INIT_FAILED, NXLOG_ERROR, "ss", e->getName(), e->getVersion());
            delete e;
         }
      }
      delete engines;
   }
}

/**
 * Callback for ShowPredictionEngines
 */
static EnumerationCallbackResult ShowEngineDetails(const TCHAR *key, const void *value, void *data)
{
   const PredictionEngine *p = (const PredictionEngine *)value;
   ConsolePrintf((CONSOLE_CTX)data, _T("%-16s | %-24s | %s\n"), key, p->getVersion(), p->getVendor());
   return _CONTINUE;
}

/**
 * Show registered prediction engines on console
 */
void ShowPredictionEngines(CONSOLE_CTX console)
{
   if (s_engines.size() == 0)
   {
      ConsolePrintf(console, _T("No prediction engines registered\n"));
      return;
   }

   ConsolePrintf(console, _T("Name             | Version                  | Vendor\n"));
   ConsolePrintf(console, _T("-----------------+--------------------------+------------------------------\n"));
   s_engines.forEach(ShowEngineDetails, console);
}

/**
 * Find prediction engine by name
 */
PredictionEngine NXCORE_EXPORTABLE *FindPredictionEngine(const TCHAR *name)
{
   return s_engines.get(name);
}

/**
 * Get list of registered engines into NXCP message
 */
void GetPredictionEngines(NXCPMessage *msg)
{
   StructArray<KeyValuePair> *a = s_engines.toArray();
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < a->size(); i++)
   {
      const PredictionEngine *e = (const PredictionEngine *)a->get(i)->value;
      msg->setField(fieldId++, e->getName());
      msg->setField(fieldId++, e->getDescription());
      msg->setField(fieldId++, e->getVersion());
      msg->setField(fieldId++, e->getVendor());
      fieldId += 6;
   }
   msg->setField(VID_NUM_ELEMENTS, a->size());
   delete a;
}

/**
 * Get predicted data for DCI
 */
bool GetPredictedData(ClientSession *session, const NXCPMessage *request, NXCPMessage *response, DataCollectionTarget *dcTarget)
{
   static UINT32 s_rowSize[] = { 8, 8, 16, 16, 516, 16 };

   // Find DCI object
   DCObject *dci = dcTarget->getDCObjectById(request->getFieldAsUInt32(VID_DCI_ID), session->getUserId());
   if (dci == NULL)
   {
      response->setField(VID_RCC, RCC_INVALID_DCI_ID);
      return false;
   }

   if (dci->getType() != DCO_TYPE_ITEM)
   {
      response->setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      return false;
   }

   PredictionEngine *engine = FindPredictionEngine(((DCItem *)dci)->getPredictionEngine());

   // Send CMD_REQUEST_COMPLETED message
   response->setField(VID_RCC, RCC_SUCCESS);
   ((DCItem *)dci)->fillMessageWithThresholds(response, false);
   session->sendMessage(response);

   int dataType = ((DCItem *)dci)->getDataType();
   time_t timeFrom = request->getFieldAsTime(VID_TIME_FROM);
   time_t timestamp = request->getFieldAsTime(VID_TIME_TO);
   time_t interval = dci->getEffectivePollingInterval();

   // Allocate memory for data and prepare data header
   char buffer[64];
   int allocated = 8192;
   int rows = 0;
   DCI_DATA_HEADER *pData = (DCI_DATA_HEADER *)malloc(allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
   pData->dataType = htonl((UINT32)dataType);
   pData->dciId = htonl(dci->getId());

   // Fill memory block with records
   DCI_DATA_ROW *pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
   while((timestamp >= timeFrom) && (rows < MAX_DCI_DATA_RECORDS))
   {
      if (rows == allocated)
      {
         allocated += 8192;
         pData = (DCI_DATA_HEADER *)realloc(pData, allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
         pCurr = (DCI_DATA_ROW *)(((char *)pData + s_rowSize[dataType] * rows) + sizeof(DCI_DATA_HEADER));
      }
      rows++;

      double value = engine->getPredictedValue(dci->getOwner()->getId(), dci->getId(), timestamp);
      pCurr->timeStamp = htonl((UINT32)timestamp);
      switch(dataType)
      {
         case DCI_DT_INT:
            pCurr->value.int32 = htonl((UINT32)((INT32)value));
            break;
         case DCI_DT_UINT:
            pCurr->value.int32 = htonl((UINT32)value);
            break;
         case DCI_DT_INT64:
            pCurr->value.ext.v64.int64 = htonq((UINT64)((INT64)value));
            break;
         case DCI_DT_UINT64:
            pCurr->value.ext.v64.int64 = htonq((UINT64)value);
            break;
         case DCI_DT_FLOAT:
            pCurr->value.ext.v64.real = htond(value);
            break;
         case DCI_DT_STRING:
            snprintf(buffer, 64, "%f", value);
            mb_to_ucs2(buffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
            SwapUCS2String(pCurr->value.string);
            break;
      }
      pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + s_rowSize[dataType]);
      timestamp -= interval;
   }
   pData->numRows = htonl(rows);

   // Prepare and send raw message with fetched data
   NXCP_MESSAGE *msg =
      CreateRawNXCPMessage(CMD_DCI_DATA, request->getId(), 0,
                           pData, rows * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER),
                           NULL, session->isCompressionEnabled());
   free(pData);
   session->sendRawMessage(msg);
   free(msg);
   return true;
}

/**
 * Queue training run for prediction engine
 */
void QueuePredictionEngineTraining(PredictionEngine *engine, UINT32 nodeId, UINT32 dciId)
{
   ThreadPoolExecute(g_npeThreadPool, engine, &PredictionEngine::train, nodeId, dciId);
}
