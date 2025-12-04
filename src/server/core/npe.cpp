/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
#include <npe.h>

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
void PredictionEngine::train(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass)
{
}

/**
 * Get series of predicted values starting with current time. Default implementation
 * calls getPredictedValue with incrementing timestamp.
 *
 * @param nodeId Node object ID
 * @param dciId DCI ID
 * @param storageClass DCI storage class
 * @param count number of values to retrieve
 * @param series buffer for values
 * @return true on success
 */
bool PredictionEngine::getPredictedSeries(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, int count, double *series)
{
   shared_ptr<NetObj> object = FindObjectById(nodeId);
   if ((object == nullptr) || !object->isDataCollectionTarget())
      return false;

   shared_ptr<DCObject> dci = static_cast<DataCollectionTarget*>(object.get())->getDCObjectById(dciId, 0);
   if (dci->getType() != DCO_TYPE_ITEM)
      return false;

   time_t interval = dci->getEffectivePollingInterval();
   time_t t = time(nullptr);
   for(int i = 0; i < count; i++)
   {
      series[i] = getPredictedValue(nodeId, dciId, storageClass, t);
      t += interval;
   }
   return true;
}

/**
 * Helper method to read last N values of given DCI
 */
StructArray<DciValue> *PredictionEngine::getDciValues(UINT32 nodeId, UINT32 dciId, DCObjectStorageClass storageClass, int maxRows)
{
   TCHAR query[1024];
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
            _sntprintf(query, 1024, _T("SELECT TOP %d idata_timestamp,idata_value FROM idata WHERE node_id=%u AND item_id=%u ORDER BY idata_timestamp DESC"), maxRows, nodeId, dciId);
         else
            _sntprintf(query, 1024, _T("SELECT TOP %d idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC"), maxRows, nodeId, dciId);
         break;
      case DB_SYNTAX_ORACLE:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
            _sntprintf(query, 1024, _T("SELECT * FROM (SELECT idata_timestamp,idata_value FROM idata WHERE node_id=%u AND item_id=%u ORDER BY idata_timestamp DESC) WHERE ROWNUM<=%d"), nodeId, dciId, maxRows);
         else
            _sntprintf(query, 1024, _T("SELECT * FROM (SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC) WHERE ROWNUM<=%d"), nodeId, dciId, maxRows);
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
            _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata WHERE node_id=%u AND item_id=%u ORDER BY idata_timestamp DESC LIMIT %d"), nodeId, dciId, maxRows);
         else
            _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %d"), nodeId, dciId, maxRows);
         break;
      case DB_SYNTAX_TSDB:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
            _sntprintf(query, 1024, _T("SELECT timestamptz_to_ms(idata_timestamp),idata_value FROM idata_sc_%s WHERE node_id=%u AND item_id=%u ORDER BY idata_timestamp DESC LIMIT %d"),
                     DCObject::getStorageClassName(storageClass), nodeId, dciId, maxRows);
         else
            _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %d"), nodeId, dciId, maxRows);
         break;
      case DB_SYNTAX_DB2:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
            _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata WHERE node_id=%u AND item_id=%u ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"), nodeId, dciId, maxRows);
         else
            _sntprintf(query, 1024, _T("SELECT idata_timestamp,idata_value FROM idata_%u WHERE item_id=%u ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"), nodeId, dciId, maxRows);
         break;
      default:
         nxlog_debug(1, _T("INTERNAL ERROR: unsupported database in PredictionEngine::getDciValues"));
         return nullptr;   // Unsupported database
   }

   StructArray<DciValue> *values = nullptr;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      values = new StructArray<DciValue>(count);
      for(int i = 0; i < count; i++)
      {
         DciValue v;
         v.timestamp = DBGetFieldTimestamp(hResult, i, 0);
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
static StringObjectMap<PredictionEngine> s_engines(Ownership::True);

/**
 * Prediction engine thread pool
 */
static ThreadPool *s_npeThreadPool = nullptr;

/**
 * Register prediction engines on startup
 */
void RegisterPredictionEngines()
{
   s_npeThreadPool = ThreadPoolCreate(_T("NPE"), 0, 1024);
   ENUMERATE_MODULES(pfGetPredictionEngines)
   {
      ObjectArray<PredictionEngine> *engines = CURRENT_MODULE.pfGetPredictionEngines();
      engines->setOwner(Ownership::False);
      for(int i = 0; i < engines->size(); i++)
      {
         PredictionEngine *e = engines->get(i);
         TCHAR errorMessage[MAX_NPE_ERROR_MESSAGE_LEN];
         if (e->initialize(errorMessage))
         {
            s_engines.set(e->getName(), e);
            nxlog_write(NXLOG_INFO, _T("Prediction engine %s version %s registered"), e->getName(), e->getVersion());
         }
         else
         {
            nxlog_write(NXLOG_ERROR, _T("Initialization of prediction engine %s version %s failed"), e->getName(), e->getVersion());
            delete e;
         }
      }
      delete engines;
   }
}

/**
 * Shutdown prediction engines
 */
void ShutdownPredictionEngines()
{
   ThreadPoolDestroy(s_npeThreadPool);
   s_engines.clear();
}

/**
 * Callback for ShowPredictionEngines
 */
static EnumerationCallbackResult ShowEngineDetails(const TCHAR *key, const PredictionEngine *p, ServerConsole *console)
{
   ConsolePrintf(console, _T("%-16s | %-24s | %s\n"), key, p->getVersion(), p->getVendor());
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
   StructArray<KeyValuePair<PredictionEngine>> *a = s_engines.toArray();
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < a->size(); i++)
   {
      const PredictionEngine *e = a->get(i)->value;
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
bool GetPredictedData(ClientSession *session, const NXCPMessage& request, NXCPMessage *response, const DataCollectionTarget& dcTarget)
{
   // Find DCI object
   shared_ptr<DCObject> dci = dcTarget.getDCObjectById(request.getFieldAsUInt32(VID_DCI_ID), session->getUserId());
   if (dci == nullptr)
   {
      response->setField(VID_RCC, RCC_INVALID_DCI_ID);
      return false;
   }

   if (dci->getType() != DCO_TYPE_ITEM)
   {
      response->setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      return false;
   }

   PredictionEngine *engine = FindPredictionEngine(static_cast<DCItem*>(dci.get())->getPredictionEngine());

   // Send CMD_REQUEST_COMPLETED message
   response->setField(VID_RCC, RCC_SUCCESS);
   static_cast<DCItem*>(dci.get())->fillMessageWithThresholds(response, false);
   session->sendMessage(response);

   int16_t dataType = static_cast<DCItem*>(dci.get())->getTransformedDataType();
   time_t timeFrom = request.getFieldAsTime(VID_TIME_FROM);
   time_t timestamp = request.getFieldAsTime(VID_TIME_TO);
   time_t interval = dci->getEffectivePollingInterval();

   // Allocate memory for data and prepare data header
   char buffer[64];
   int32_t count = std::min(static_cast<int>((timestamp - timeFrom) / interval), MAX_DCI_DATA_RECORDS);

   ByteStream data(32768);
   data.writeB(count);
   data.writeB(dataType);
   data.writeB(static_cast<uint16_t>(0));   // Options

   // Fill memory block with records
   double *series = MemAllocArray<double>(count);
   if (timestamp >= timeFrom)
   {
      engine->getPredictedSeries(dci->getOwner()->getId(), dci->getId(), dci->getStorageClass(), count, series);

      for(int i = 0; i < count; i++)
      {
         data.writeB(static_cast<int64_t>(timestamp) * 1000);  // Timestamp in milliseconds
         switch(dataType)
         {
            case DCI_DT_INT:
               data.writeB(static_cast<int32_t>(series[i]));
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               data.writeB(static_cast<uint32_t>(series[i]));
               break;
            case DCI_DT_INT64:
               data.writeB(static_cast<int64_t>(series[i]));
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               data.writeB(static_cast<uint64_t>(series[i]));
               break;
            case DCI_DT_FLOAT:
               data.writeB(series[i]);
               break;
            case DCI_DT_STRING:
               snprintf(buffer, 64, "%f", series[i]);
               size_t l = strlen(buffer);
               data.writeB(static_cast<uint16_t>(l));
               data.write(buffer, l);
               break;
         }
         timestamp -= interval;
      }
   }
   MemFree(series);

   // Prepare and send raw message with fetched data
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_DCI_DATA, request.getId(), 0, data.buffer(), data.size(), nullptr, session->isCompressionEnabled());
   session->sendRawMessage(msg);
   MemFree(msg);
   return true;
}

/**
 * Queue training run for prediction engine
 */
void QueuePredictionEngineTraining(PredictionEngine *engine, DCItem *dci)
{
   ThreadPoolExecute(s_npeThreadPool, engine, &PredictionEngine::train, dci->getOwner()->getId(), dci->getId(), dci->getStorageClass());
}
