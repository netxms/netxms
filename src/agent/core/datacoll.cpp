/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: datacoll.cpp
**
**/

#include "nxagentd.h"

/**
 * Externals
 */
void UpdateSnmpTarget(SNMPTarget *target);
UINT32 GetSnmpValue(const uuid& target, UINT16 port, const TCHAR *oid, TCHAR *value, int interpretRawValue);

extern UINT32 g_dcReconciliationBlockSize;
extern UINT32 g_dcReconciliationTimeout;
extern UINT32 g_dcMaxCollectorPoolSize;

/**
 * Data collector start indicator
 */
static bool s_dataCollectorStarted = false;

/**
 * Unsaved poll time indicator
 */
static bool s_pollTimeChanged = false;

/**
 * Data collection item
 */
class DataCollectionItem : public RefCountObject
{
private:
   UINT64 m_serverId;
   UINT32 m_id;
   INT32 m_pollingInterval;
   TCHAR *m_name;
   BYTE m_type;
   BYTE m_origin;
   UINT16 m_snmpPort;
   BYTE m_snmpRawValueType;
   BYTE m_busy;
	uuid m_snmpTargetGuid;
   time_t m_lastPollTime;

public:
   DataCollectionItem(UINT64 serverId, NXCPMessage *msg, UINT32 baseId);
   DataCollectionItem(DB_RESULT hResult, int row);
   DataCollectionItem(const DataCollectionItem *item);
   virtual ~DataCollectionItem();

   UINT32 getId() const { return m_id; }
   UINT64 getServerId() const { return m_serverId; }
   const TCHAR *getName() const { return m_name; }
   int getType() const { return (int)m_type; }
   int getOrigin() const { return (int)m_origin; }
   const uuid& getSnmpTargetGuid() const { return m_snmpTargetGuid; }
   UINT16 getSnmpPort() const { return m_snmpPort; }
   int getSnmpRawValueType() const { return (int)m_snmpRawValueType; }
   UINT32 getPollingInterval() const { return (UINT32)m_pollingInterval; }
   time_t getLastPollTime() { return m_lastPollTime; }

   bool equals(const DataCollectionItem *item) const { return (m_serverId == item->m_serverId) && (m_id == item->m_id); }

   void updateAndSave(const DataCollectionItem *item);
   void saveToDatabase(bool newObject);
   void deleteFromDatabase();
   void setLastPollTime(time_t time);

   void startDataCollection() { m_busy = 1; incRefCount(); }
   void finishDataCollection() { m_busy = 0; decRefCount(); }

   UINT32 getTimeToNextPoll(time_t now) const
   {
      if (m_busy) // being polled now - time to next poll should not be less than full polling interval
         return m_pollingInterval;
      time_t diff = now - m_lastPollTime;
      return (diff >= m_pollingInterval) ? 0 : m_pollingInterval - (UINT32)diff;
   }
};

/**
 * Create data collection item from NXCP mesage
 */
DataCollectionItem::DataCollectionItem(UINT64 serverId, NXCPMessage *msg, UINT32 baseId) : RefCountObject()
{
   m_serverId = serverId;
   m_id = msg->getFieldAsInt32(baseId);
   m_type = (BYTE)msg->getFieldAsUInt16(baseId + 1);
   m_origin = (BYTE)msg->getFieldAsUInt16(baseId + 2);
   m_name = msg->getFieldAsString(baseId + 3);
   m_pollingInterval = msg->getFieldAsInt32(baseId + 4);
   m_lastPollTime = msg->getFieldAsTime(baseId + 5);
   m_snmpTargetGuid = msg->getFieldAsGUID(baseId + 6);
   m_snmpPort = msg->getFieldAsUInt16(baseId + 7);
   m_snmpRawValueType = (BYTE)msg->getFieldAsUInt16(baseId + 8);
   m_busy = 0;
}

/**
 * Data is selected in this order: server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid,snmp_raw_type
 */
DataCollectionItem::DataCollectionItem(DB_RESULT hResult, int row)
{
   m_serverId = DBGetFieldInt64(hResult, row, 0);
   m_id = DBGetFieldULong(hResult, row, 1);
   m_type = (BYTE)DBGetFieldULong(hResult, row, 2);
   m_origin = (BYTE)DBGetFieldULong(hResult, row, 3);
   m_name = DBGetField(hResult, row, 4, NULL, 0);
   m_pollingInterval = DBGetFieldULong(hResult, row, 5);
   m_lastPollTime = (time_t)DBGetFieldULong(hResult, row, 6);
   m_snmpPort = DBGetFieldULong(hResult, row, 7);
   m_snmpTargetGuid = DBGetFieldGUID(hResult, row, 8);
   m_snmpRawValueType = (BYTE)DBGetFieldULong(hResult, row, 9);
   m_busy = 0;
}

/**
 * Copy constructor
 */
 DataCollectionItem::DataCollectionItem(const DataCollectionItem *item)
 {
   m_serverId = item->m_serverId;
   m_id = item->m_id;
   m_type = item->m_type;
   m_origin = item->m_origin;
   m_name = _tcsdup(item->m_name);
   m_pollingInterval = item->m_pollingInterval;
   m_lastPollTime = item->m_lastPollTime;
   m_snmpTargetGuid = item->m_snmpTargetGuid;
   m_snmpPort = item->m_snmpPort;
   m_snmpRawValueType = item->m_snmpRawValueType;
   m_busy = 0;
 }

/**
 * Data collection item destructor
 */
DataCollectionItem::~DataCollectionItem()
{
   safe_free(m_name);
}

/**
 * Will check if object has changed. If at least one field is changed - all data will be updated and
 * saved to database.
 */
void DataCollectionItem::updateAndSave(const DataCollectionItem *item)
{
   // if at least one of fields changed - set all fields and save to DB
   if ((m_type != item->m_type) || (m_origin != item->m_origin) || _tcscmp(m_name, item->m_name) ||
       (m_pollingInterval != item->m_pollingInterval) || m_snmpTargetGuid.compare(item->m_snmpTargetGuid) ||
       (m_snmpPort != item->m_snmpPort) || (m_snmpRawValueType != item->m_snmpRawValueType) || (m_lastPollTime < item->m_lastPollTime))
   {
      m_type = item->m_type;
      m_origin = item->m_origin;
      m_name = _tcsdup(item->m_name);
      m_pollingInterval = item->m_pollingInterval;
      if (m_lastPollTime < item->m_lastPollTime)
         m_lastPollTime = item->m_lastPollTime;
      m_snmpTargetGuid = item->m_snmpTargetGuid;
      m_snmpPort = item->m_snmpPort;
      m_snmpRawValueType = item->m_snmpRawValueType;
      saveToDatabase(false);
   }
}

/**
 * Save configuration object to database
 */
void DataCollectionItem::saveToDatabase(bool newObject)
{
   DebugPrintf(6, _T("DataCollectionItem::saveToDatabase: %s object(serverId=") UINT64X_FMT(_T("016")) _T(",dciId=%d) saved to database"),
               newObject ? _T("new") : _T("existing"), m_serverId, m_id);
   DB_HANDLE db = GetLocalDatabaseHandle();
   DB_STATEMENT hStmt;

   if (newObject)
   {
		hStmt = DBPrepare(db,
                    _T("INSERT INTO dc_config (type,origin,name,polling_interval,")
                    _T("last_poll,snmp_port,snmp_target_guid,snmp_raw_type,server_id,dci_id)")
                    _T("VALUES (?,?,?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE dc_config SET type=?,origin=?,name=?,")
                    _T("polling_interval=?,last_poll=?,snmp_port=?,")
                    _T("snmp_target_guid=?,snmp_raw_type=? WHERE server_id=? AND dci_id=?"));
   }

	if (hStmt == NULL)
		return;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_type);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_origin);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_pollingInterval);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_lastPollTime);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_snmpPort);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_snmpTargetGuid);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_snmpRawValueType);
	DBBind(hStmt, 9, DB_SQLTYPE_BIGINT, m_serverId);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (LONG)m_id);

   DBExecute(hStmt);
   DBFreeStatement(hStmt);
}

/**
 * Remove item form database and delete not synced data if exist
 */
void DataCollectionItem::deleteFromDatabase()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM dc_config WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d"), m_serverId, m_id);
   if (DBQuery(db, query))
   {
      _sntprintf(query, 256, _T("DELETE FROM dc_queue WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d"), m_serverId, m_id);
      if (DBQuery(db, query))
      {
         DebugPrintf(6, _T("DataCollectionItem::deleteFromDatabase: object(serverId=") UINT64X_FMT(_T("016")) _T(",dciId=%d) removed from database"), m_serverId, m_id);
      }
   }
}

/**
 * Set last poll time for item
 */
void DataCollectionItem::setLastPollTime(time_t time)
{
   m_lastPollTime = time;
   s_pollTimeChanged = true;
}

/**
 * Collected data
 */
class DataElement
{
private:
   UINT64 m_serverId;
   UINT32 m_dciId;
   time_t m_timestamp;
   int m_origin;
   int m_type;
   UINT32 m_statusCode;
   uuid m_snmpNode;
   union
   {
      TCHAR *item;
      StringList *list;
      Table *table;
   } m_value;

public:
   DataElement(DataCollectionItem *dci, const TCHAR *value, UINT32 status)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_ITEM;
      m_statusCode = status;
      m_snmpNode = dci->getSnmpTargetGuid();
      m_value.item = _tcsdup(value);
   }

   DataElement(DataCollectionItem *dci, StringList *value, UINT32 status)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_LIST;
      m_statusCode = status;
      m_snmpNode = dci->getSnmpTargetGuid();
      m_value.list = value;
   }

   DataElement(DataCollectionItem *dci, Table *value, UINT32 status)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_TABLE;
      m_statusCode = status;
      m_snmpNode = dci->getSnmpTargetGuid();
      m_value.table = value;
   }

   /**
    * Create data element from database record
    * Expected field order: server_id,dci_id,dci_type,dci_origin,status_code,snmp_target_guid,timestamp,value
    */
   DataElement(DB_RESULT hResult, int row)
   {
      m_serverId = DBGetFieldUInt64(hResult, row, 0);
      m_dciId = DBGetFieldULong(hResult, row, 1);
      m_timestamp = (time_t)DBGetFieldInt64(hResult, row, 6);
      m_origin = DBGetFieldLong(hResult, row, 3);
      m_type = DBGetFieldLong(hResult, row, 2);
      m_statusCode = DBGetFieldLong(hResult, row, 4);
      m_snmpNode = DBGetFieldGUID(hResult, row, 5);
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            m_value.item = DBGetField(hResult, row, 7, NULL, 0);
            break;
         case DCO_TYPE_LIST:
            {
               m_value.list = new StringList();
               TCHAR *text = DBGetField(hResult, row, 7, NULL, 0);
               if (text != NULL)
               {
                  m_value.list->splitAndAdd(text, _T("\n"));
                  free(text);
               }
            }
            break;
         case DCO_TYPE_TABLE:
            {
               char *xml = DBGetFieldUTF8(hResult, row, 7, NULL, 0);
               if (xml != NULL)
               {
                  m_value.table = Table::createFromXML(xml);
                  free(xml);
               }
               else
               {
                  m_value.table = NULL;
               }
            }
            break;
         default:
            m_type = DCO_TYPE_ITEM;
            m_value.item = _tcsdup(_T(""));
            break;
      }
   }

   ~DataElement()
   {
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            free(m_value.item);
            break;
         case DCO_TYPE_LIST:
            delete m_value.list;
            break;
         case DCO_TYPE_TABLE:
            delete m_value.table;
            break;
      }
   }

   time_t getTimestamp() { return m_timestamp; }
   UINT64 getServerId() { return m_serverId; }
   UINT32 getDciId() { return m_dciId; }
   int getType() { return m_type; }
   UINT32 getStatusCode() { return m_statusCode; }

   void saveToDatabase(DB_STATEMENT hStmt);
   bool sendToServer(bool reconcillation);
   void fillReconciliationMessage(NXCPMessage *msg, UINT32 baseId);
};

/**
 * Save data element to database
 */
void DataElement::saveToDatabase(DB_STATEMENT hStmt)
{
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_dciId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)m_type);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_origin);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_statusCode);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_snmpNode);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_timestamp);
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_value.item, DB_BIND_STATIC);
         break;
      case DCO_TYPE_LIST:
         DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_value.list->join(_T("\n")), DB_BIND_DYNAMIC);
         break;
      case DCO_TYPE_TABLE:
         DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_value.table->createXML(), DB_BIND_DYNAMIC);
         break;
   }
   DBExecute(hStmt);
}

/**
 * Session comparator
 */
static bool SessionComparator_Sender(AbstractCommSession *session, void *data)
{
   return (session->getServerId() == *((INT64 *)data)) && session->canAcceptData();
}

/**
 * Send collected data to server
 */
bool DataElement::sendToServer(bool reconciliation)
{
   // If data in database was invalid table may not be parsed correctly
   // Consider sending a success in that case so data element can be dropped
   if ((m_type == DCO_TYPE_TABLE) && (m_value.table == NULL))
      return true;

   CommSession *session = (CommSession *)FindServerSession(SessionComparator_Sender, &m_serverId);
   if (session == NULL)
      return false;

   NXCPMessage msg;
   msg.setCode(CMD_DCI_DATA);
   msg.setId(session->generateRequestId());
   msg.setField(VID_DCI_ID, m_dciId);
   msg.setField(VID_DCI_SOURCE_TYPE, (INT16)m_origin);
   msg.setField(VID_DCOBJECT_TYPE, (INT16)m_type);
   msg.setField(VID_STATUS, m_statusCode);
   msg.setField(VID_NODE_ID, m_snmpNode);
   msg.setFieldFromTime(VID_TIMESTAMP, m_timestamp);
   msg.setField(VID_RECONCILIATION, (INT16)(reconciliation ? 1 : 0));
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         msg.setField(VID_VALUE, m_value.item);
         break;
      case DCO_TYPE_LIST:
         m_value.list->fillMessage(&msg, VID_ENUM_VALUE_BASE, VID_NUM_STRINGS);
         break;
      case DCO_TYPE_TABLE:
         m_value.table->setSource(m_origin);
         m_value.table->fillMessage(msg, 0, -1);
         break;
   }
   UINT32 rcc = session->doRequest(&msg, 2000);
   session->decRefCount();

   // consider internal error as success because it means that server
   // cannot accept data for some reason and retry is not feasible
   return (rcc == ERR_SUCCESS) || (rcc == ERR_INTERNAL_ERROR);
}

/**
 * Fill bulk reconciliation message with DCI data
 */
void DataElement::fillReconciliationMessage(NXCPMessage *msg, UINT32 baseId)
{
   msg->setField(baseId, m_dciId);
   msg->setField(baseId + 1, (INT16)m_origin);
   msg->setField(baseId + 2, (INT16)m_type);
   msg->setField(baseId + 3, m_snmpNode);
   msg->setFieldFromTime(baseId + 4, m_timestamp);
   msg->setField(baseId + 5, m_value.item);
   msg->setField(baseId + 6, m_statusCode);
}

/**
 * Server data sync status object
 */
struct ServerSyncStatus
{
   INT32 queueSize;

   ServerSyncStatus()
   {
      queueSize = 0;
   }
};

/**
 * Server sync status information
 */
static HashMap<UINT64, ServerSyncStatus> s_serverSyncStatus(true);
static MUTEX s_serverSyncStatusLock = INVALID_MUTEX_HANDLE;

/**
 * Database writer queue
 */
static Queue s_databaseWriterQueue;

/**
 * Database writer
 */
static THREAD_RESULT THREAD_CALL DatabaseWriter(void *arg)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DebugPrintf(1, _T("Database writer thread started"));

   while(true)
   {
      DataElement *e = (DataElement *)s_databaseWriterQueue.getOrBlock();
      if (e == INVALID_POINTER_VALUE)
         break;

      DB_STATEMENT hStmt= DBPrepare(hdb, _T("INSERT INTO dc_queue (server_id,dci_id,dci_type,dci_origin,status_code,snmp_target_guid,timestamp,value) VALUES (?,?,?,?,?,?,?,?)"));
      if (hStmt == NULL)
      {
         delete e;
         continue;
      }

      int count = 0;

      DBBegin(hdb);
      while((e != NULL) && (e != INVALID_POINTER_VALUE))
      {
         e->saveToDatabase(hStmt);
         delete e;

         count++;
         if (count > 200)
            break;

         e = (DataElement *)s_databaseWriterQueue.getOrBlock(500);  // Wait up to 500 ms for next data block
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      DebugPrintf(7, _T("Database writer: %d records inserted"), count);
      if (e == INVALID_POINTER_VALUE)
         break;
   }

   DebugPrintf(1, _T("Database writer thread stopped"));
   return THREAD_OK;
}

/**
 * List of all data collection items
 */
static ObjectArray<DataCollectionItem> s_items(64, 64, true);
static MUTEX s_itemLock = INVALID_MUTEX_HANDLE;

/**
 * Session comparator
 */
static bool SessionComparator_Reconciliation(AbstractCommSession *session, void *data)
{
   if ((session->getServerId() == 0) || !session->canAcceptData())
      return false;
   ServerSyncStatus *s = s_serverSyncStatus.get(session->getServerId());
   if ((s != NULL) && (s->queueSize > 0))
   {
      return true;
   }
   return false;
}

/**
 * Data reconciliation thread
 */
static THREAD_RESULT THREAD_CALL ReconciliationThread(void *arg)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   UINT32 sleepTime = 30000;
   nxlog_debug(1, _T("Data reconciliation thread started (block size %d, timeout %d ms)"), g_dcReconciliationBlockSize, g_dcReconciliationTimeout);

   bool vacuumNeeded = false;
   while(!AgentSleepAndCheckForShutdown(sleepTime))
   {
      // Check if there is something to sync
      MutexLock(s_serverSyncStatusLock);
      CommSession *session = (CommSession *)FindServerSession(SessionComparator_Reconciliation, NULL);
      MutexUnlock(s_serverSyncStatusLock);
      if (session == NULL)
      {
         // Save last poll times when reconciliation thread is idle
         MutexLock(s_itemLock);
         if (s_pollTimeChanged)
         {
            DBBegin(hdb);
            TCHAR query[256];
            for(int i = 0; i < s_items.size(); i++)
            {
               DataCollectionItem *dci = s_items.get(i);
               _sntprintf(query, 256, _T("UPDATE dc_config SET last_poll=") UINT64_FMT _T(" WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d"),
                          (UINT64)dci->getLastPollTime(), (UINT64)dci->getServerId(), dci->getId());
               DBQuery(hdb, query);
            }
            DBCommit(hdb);
            s_pollTimeChanged = false;
         }
         MutexUnlock(s_itemLock);

         if (vacuumNeeded)
         {
            DebugPrintf(4, _T("ReconciliationThread: vacuum local database"));
            DBQuery(hdb, _T("VACUUM"));
            vacuumNeeded = false;
         }
         sleepTime = 30000;
         continue;
      }

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT server_id,dci_id,dci_type,dci_origin,status_code,snmp_target_guid,timestamp,value FROM dc_queue INDEXED BY idx_dc_queue_timestamp WHERE server_id=") UINT64_FMT _T(" ORDER BY timestamp LIMIT %d"), session->getServerId(), g_dcReconciliationBlockSize);

      TCHAR sqlError[DBDRV_MAX_ERROR_TEXT];
      DB_RESULT hResult = DBSelectEx(hdb, query, sqlError);
      if (hResult == NULL)
      {
         DebugPrintf(4, _T("ReconciliationThread: database query failed: %s"), sqlError);
         sleepTime = 30000;
         session->decRefCount();
         continue;
      }

      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         ObjectArray<DataElement> bulkSendList(count, 10, true);
         ObjectArray<DataElement> deleteList(count, 10, true);
         for(int i = 0; i < count; i++)
         {
            DataElement *e = new DataElement(hResult, i);
            if ((e->getType() == DCO_TYPE_ITEM) && session->isBulkReconciliationSupported())
            {
               bulkSendList.add(e);
            }
            else
            {
               MutexLock(s_serverSyncStatusLock);
               ServerSyncStatus *status = s_serverSyncStatus.get(e->getServerId());
               if (status != NULL)
               {
                  if (e->sendToServer(true))
                  {
                     status->queueSize--;
                     deleteList.add(e);
                  }
                  else
                  {
                     delete e;
                  }
               }
               else
               {
                  DebugPrintf(5, _T("INTERNAL ERROR: cached DCI value without server sync status object"));
                  deleteList.add(e);  // record should be deleted
               }
               MutexUnlock(s_serverSyncStatusLock);
            }
         }

         if (bulkSendList.size() > 0)
         {
            DebugPrintf(6, _T("ReconciliationThread: %d records to be sent in bulk mode"), bulkSendList.size());

            NXCPMessage msg;
            msg.setCode(CMD_DCI_DATA);
            msg.setId(session->generateRequestId());
            msg.setField(VID_BULK_RECONCILIATION, (INT16)1);
            msg.setField(VID_NUM_ELEMENTS, (INT16)bulkSendList.size());

            UINT32 fieldId = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < bulkSendList.size(); i++)
            {
               bulkSendList.get(i)->fillReconciliationMessage(&msg, fieldId);
               fieldId += 10;
            }

            NXCPMessage *response = session->doRequestEx(&msg, g_dcReconciliationTimeout);
            if (response != NULL)
            {
               UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
               if (rcc == ERR_SUCCESS)
               {
                  MutexLock(s_serverSyncStatusLock);
                  ServerSyncStatus *serverSyncStatus = s_serverSyncStatus.get(session->getServerId());

                  // Check status for each data element
                  BYTE status[MAX_BULK_DATA_BLOCK_SIZE];
                  memset(status, 0, MAX_BULK_DATA_BLOCK_SIZE);
                  response->getFieldAsBinary(VID_STATUS, status, MAX_BULK_DATA_BLOCK_SIZE);
                  bulkSendList.setOwner(false);
                  for(int i = 0; i < bulkSendList.size(); i++)
                  {
                     DataElement *e = bulkSendList.get(i);
                     if (status[i] != BULK_DATA_REC_RETRY)
                     {
                        deleteList.add(e);
                        serverSyncStatus->queueSize--;
                     }
                     else
                     {
                        delete e;
                     }
                  }

                  MutexUnlock(s_serverSyncStatusLock);
               }
               else
               {
                  DebugPrintf(4, _T("ReconciliationThread: bulk send failed (%d)"), rcc);
               }
               delete response;
            }
            else
            {
               DebugPrintf(4, _T("ReconciliationThread: timeout on bulk send"));
            }
         }

         if (deleteList.size() > 0)
         {
            DBBegin(hdb);
            for(int i = 0; i < deleteList.size(); i++)
            {
               DataElement *e = deleteList.get(i);
               _sntprintf(query, 256, _T("DELETE FROM dc_queue WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d AND timestamp=") INT64_FMT,
                  e->getServerId(), e->getDciId(), (INT64)e->getTimestamp());
               DBQuery(hdb, query);
            }
            DBCommit(hdb);
            nxlog_debug(4, _T("ReconciliationThread: %d records sent"), deleteList.size());
            vacuumNeeded = true;
         }
      }
      DBFreeResult(hResult);

      session->decRefCount();
      sleepTime = (count > 0) ? 50 : 30000;
   }

   nxlog_debug(1, _T("Data reconciliation thread stopped"));
   return THREAD_OK;
}

/**
 * Data sender queue
 */
static Queue s_dataSenderQueue;

/**
 * Data sender
 */
static THREAD_RESULT THREAD_CALL DataSender(void *arg)
{
   DebugPrintf(1, _T("Data sender thread started"));
   while(true)
   {
      DataElement *e = (DataElement *)s_dataSenderQueue.getOrBlock();
      if (e == INVALID_POINTER_VALUE)
         break;

      MutexLock(s_serverSyncStatusLock);
      ServerSyncStatus *status = s_serverSyncStatus.get(e->getServerId());
      if (status == NULL)
      {
         status = new ServerSyncStatus();
         s_serverSyncStatus.set(e->getServerId(), status);
      }

      if (status->queueSize == 0)
      {
         if (!e->sendToServer(false))
         {
            status->queueSize++;
            s_databaseWriterQueue.put(e);
            e = NULL;
         }
      }
      else
      {
         status->queueSize++;
         s_databaseWriterQueue.put(e);
         e = NULL;
      }
      MutexUnlock(s_serverSyncStatusLock);

      delete e;
   }
   DebugPrintf(1, _T("Data sender thread stopped"));
   return THREAD_OK;
}

/**
 * Collect data from agent
 */
static DataElement *CollectDataFromAgent(DataCollectionItem *dci)
{
   VirtualSession session(dci->getServerId());

   DataElement *e = NULL;
   UINT32 status;
   if (dci->getType() == DCO_TYPE_ITEM)
   {
      TCHAR value[MAX_RESULT_LENGTH];
      status = GetParameterValue(dci->getName(), value, &session);
      e = new DataElement(dci, (status == ERR_SUCCESS) ? value : _T(""), status);
   }
   else if (dci->getType() == DCO_TYPE_LIST)
   {
      StringList *value = new StringList();
      status = GetListValue(dci->getName(), value, &session);
      e = new DataElement(dci, value, status);
   }
   else if (dci->getType() == DCO_TYPE_TABLE)
   {
      Table *value = new Table();
      status = GetTableValue(dci->getName(), value, &session);
      e = new DataElement(dci, value, status);
   }

   return e;
}

/**
 * Collect data from SNMP
 */
static DataElement *CollectDataFromSNMP(DataCollectionItem *dci)
{
   DataElement *e = NULL;
   if (dci->getType() == DCO_TYPE_ITEM)
   {
      DebugPrintf(8, _T("Read SNMP parameter %s"), dci->getName());

      TCHAR value[MAX_RESULT_LENGTH];
      UINT32 status = GetSnmpValue(dci->getSnmpTargetGuid(), dci->getSnmpPort(), dci->getName(), value, dci->getSnmpRawValueType());
      e = new DataElement(dci, status == ERR_SUCCESS ? value : _T(""), status);
   }
   return e;
}

/**
 * Local data collection callback
 */
static void LocalDataCollectionCallback(void *arg)
{
   DataCollectionItem *dci = (DataCollectionItem *)arg;

   DataElement *e = CollectDataFromAgent(dci);
   if (e != NULL)
   {
      s_dataSenderQueue.put(e);
   }
   else
   {
      DebugPrintf(6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
   }

   dci->setLastPollTime(time(NULL));
   dci->finishDataCollection();
}

/**
 * SNMP data collection callback
 */
static void SnmpDataCollectionCallback(void *arg)
{
   DataCollectionItem *dci = (DataCollectionItem *)arg;

   DataElement *e = CollectDataFromSNMP(dci);
   if (e != NULL)
   {
      s_dataSenderQueue.put(e);
   }
   else
   {
      DebugPrintf(6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
   }

   dci->setLastPollTime(time(NULL));
   dci->finishDataCollection();
}

/**
 * Data collectors thread pool
 */
static ThreadPool *s_dataCollectorPool = NULL;

/**
 * Single data collection scheduler run - schedule data collection if needed and calculate sleep time
 */
static UINT32 DataCollectionSchedulerRun()
{
   UINT32 sleepTime = 60;

   MutexLock(s_itemLock);
   for(int i = 0; i < s_items.size(); i++)
   {
      DataCollectionItem *dci = s_items.get(i);
      time_t now = time(NULL);
      UINT32 timeToPoll = dci->getTimeToNextPoll(now);
      if (timeToPoll == 0)
      {
         DebugPrintf(7, _T("DataCollector: polling DCI %d \"%s\""), dci->getId(), dci->getName());

         if (dci->getOrigin() == DS_NATIVE_AGENT)
         {
            dci->startDataCollection();
            ThreadPoolExecute(s_dataCollectorPool, LocalDataCollectionCallback, dci);
         }
         else if (dci->getOrigin() == DS_SNMP_AGENT)
         {
            dci->startDataCollection();
            TCHAR key[64];
            ThreadPoolExecuteSerialized(s_dataCollectorPool, dci->getSnmpTargetGuid().toString(key), SnmpDataCollectionCallback, dci);
         }
         else
         {
            DebugPrintf(7, _T("DataCollector: unsupported origin %d"), dci->getOrigin());
         }

         timeToPoll = dci->getPollingInterval();
      }

      if (sleepTime > timeToPoll)
         sleepTime = timeToPoll;
   }
   MutexUnlock(s_itemLock);
   return sleepTime;
}

/**
 * Data collection scheduler thread
 */
static THREAD_RESULT THREAD_CALL DataCollectionScheduler(void *arg)
{
   DebugPrintf(1, _T("Data collection scheduler thread started"));
   s_dataCollectorPool = ThreadPoolCreate(1, g_dcMaxCollectorPoolSize, _T("DATACOLL"));

   UINT32 sleepTime = DataCollectionSchedulerRun();
   while(!AgentSleepAndCheckForShutdown(sleepTime * 1000))
   {
      sleepTime = DataCollectionSchedulerRun();
      DebugPrintf(7, _T("DataCollector: sleeping for %d seconds"), sleepTime);
   }

   ThreadPoolDestroy(s_dataCollectorPool);
   DebugPrintf(1, _T("Data collection scheduler thread stopped"));
   return THREAD_OK;
}

/**
 * Configure data collection
 */
void ConfigureDataCollection(UINT64 serverId, NXCPMessage *msg)
{
   if (!s_dataCollectorStarted)
   {
      DebugPrintf(1, _T("Local data collector was not started, ignoring configuration received from server ") UINT64X_FMT(_T("016")), serverId);
      return;
   }

   DB_HANDLE hdb = GetLocalDatabaseHandle();

   int count = msg->getFieldAsInt32(VID_NUM_NODES);
   if (count > 0)
   {
      DBBegin(hdb);
      UINT32 fieldId = VID_NODE_INFO_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         SNMPTarget *target = new SNMPTarget(serverId, msg, fieldId);
         UpdateSnmpTarget(target);
         fieldId += 50;
      }
      DBCommit(hdb);
   }
   DebugPrintf(4, _T("%d SNMP targets received from server ") UINT64X_FMT(_T("016")), count, serverId);

   ObjectArray<DataCollectionItem> config(32, 32, true);

   count = msg->getFieldAsInt32(VID_NUM_ELEMENTS);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      config.add(new DataCollectionItem(serverId, msg, fieldId));
      fieldId += 10;
   }
   DebugPrintf(4, _T("%d data collection elements received from server ") UINT64X_FMT(_T("016")), count, serverId);

   bool txnOpen = false;

   MutexLock(s_itemLock);

   // Update and add new
   for(int j = 0; j < config.size(); j++)
   {
      DataCollectionItem *item = config.get(j);
      bool exist = false;
      for(int i = 0; i < s_items.size(); i++)
      {
         if (item->equals(s_items.get(i)))
         {
            s_items.get(i)->updateAndSave(item);
            exist = true;
         }
      }
      if (!exist)
      {
         DataCollectionItem *newItem = new DataCollectionItem(item);
         s_items.add(newItem);
         if (!txnOpen)
         {
            DBBegin(hdb);
            txnOpen = true;
         }
         newItem->saveToDatabase(true);
      }
   }

   // Remove not existing configuration and data for it
   for(int i = 0; i < s_items.size(); i++)
   {
      DataCollectionItem *item = s_items.get(i);
      //If item is from other server, then, do not search it in list of this server
      if(item->getServerId() != serverId)
         continue;
      bool exist = false;
      for(int j = 0; j < config.size(); j++)
      {
         if (item->equals(config.get(j)))
         {
            exist = true;
         }
      }
      if (!exist)
      {
         if (!txnOpen)
         {
            DBBegin(hdb);
            txnOpen = true;
         }
         item->deleteFromDatabase();
         s_items.unlink(i);
         item->decRefCount();
         i--;
      }
   }

   if (txnOpen)
      DBCommit(hdb);

   MutexUnlock(s_itemLock);

   DebugPrintf(4, _T("Data collection for server ") UINT64X_FMT(_T("016")) _T(" reconfigured"), serverId);
}

/**
 * Load saved state of local data collection
 */
static void LoadState()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid,snmp_raw_type FROM dc_config"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         s_items.add(new DataCollectionItem(hResult, i));
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT guid,server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass FROM dc_snmp_targets"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         SNMPTarget *t = new SNMPTarget(hResult, i);
         UpdateSnmpTarget(t);
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT server_id,count(*) FROM dc_queue GROUP BY server_id"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         ServerSyncStatus *s = new ServerSyncStatus;
         s->queueSize = DBGetFieldLong(hResult, i, 1);
         UINT64 serverId = DBGetFieldUInt64(hResult, i, 0);
         s_serverSyncStatus.set(serverId, s);
         DebugPrintf(2, _T("%d elements in queue for server ID ") UINT64X_FMT(_T("016")), s->queueSize, serverId);
      }
      DBFreeResult(hResult);
   }
}

/**
 * Data collector and sender thread handles
 */
static THREAD s_dataCollectionSchedulerThread = INVALID_THREAD_HANDLE;
static THREAD s_dataSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_databaseWriterThread = INVALID_THREAD_HANDLE;
static THREAD s_reconciliationThread = INVALID_THREAD_HANDLE;

/**
 * Initialize and start local data collector
 */
void StartLocalDataCollector()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == NULL)
   {
      DebugPrintf(5, _T("StartLocalDataCollector: local database unavailable"));
      return;
   }

   if (g_dcReconciliationBlockSize < 16)
   {
      nxlog_debug(1, _T("Invalid data reconciliation block size %d, resetting to 16"), g_dcReconciliationBlockSize);
      g_dcReconciliationBlockSize = 16;
   }
   else if (g_dcReconciliationBlockSize > MAX_BULK_DATA_BLOCK_SIZE)
   {
      nxlog_debug(1, _T("Invalid data reconciliation block size %d, resetting to %d"), g_dcReconciliationBlockSize, MAX_BULK_DATA_BLOCK_SIZE);
      g_dcReconciliationBlockSize = MAX_BULK_DATA_BLOCK_SIZE;
   }

   if (g_dcReconciliationTimeout < 1000)
   {
      nxlog_debug(1, _T("Invalid data reconciliation timeout %d, resetting to 1000"), g_dcReconciliationTimeout);
      g_dcReconciliationTimeout = 1000;
   }
   else if (g_dcReconciliationTimeout > 600000)
   {
      nxlog_debug(1, _T("Invalid data reconciliation timeout %d, resetting to 600000"), g_dcReconciliationTimeout);
      g_dcReconciliationTimeout = 600000;
   }

   s_itemLock = MutexCreate();
   s_serverSyncStatusLock = MutexCreate();

   LoadState();

   s_dataCollectionSchedulerThread = ThreadCreateEx(DataCollectionScheduler, 0, NULL);
   s_dataSenderThread = ThreadCreateEx(DataSender, 0, NULL);
   s_databaseWriterThread = ThreadCreateEx(DatabaseWriter, 0, NULL);
   s_reconciliationThread = ThreadCreateEx(ReconciliationThread, 0, NULL);

   s_dataCollectorStarted = true;
}

/**
 * Shutdown local data collector
 */
void ShutdownLocalDataCollector()
{
   if (!s_dataCollectorStarted)
   {
      DebugPrintf(5, _T("Local data collector was not started"));
      return;
   }

   DebugPrintf(5, _T("Waiting for data collector thread termination"));
   ThreadJoin(s_dataCollectionSchedulerThread);

   DebugPrintf(5, _T("Waiting for data sender thread termination"));
   s_dataSenderQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_dataSenderThread);

   DebugPrintf(5, _T("Waiting for database writer thread termination"));
   s_databaseWriterQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_databaseWriterThread);

   DebugPrintf(5, _T("Waiting for data reconciliation thread termination"));
   ThreadJoin(s_reconciliationThread);

   MutexDestroy(s_itemLock);
   MutexDestroy(s_serverSyncStatusLock);
}

/**
 * Clear data collection configuration
 */
void ClearDataCollectionConfiguration()
{
   MutexLock(s_itemLock);
   DB_HANDLE db = GetLocalDatabaseHandle();
   DBQuery(db, _T("DELETE FROM dc_queue"));
   DBQuery(db, _T("DELETE FROM dc_config"));
   DBQuery(db, _T("DELETE FROM dc_snmp_targets"));
   s_items.clear();
   MutexUnlock(s_itemLock);

   MutexLock(s_serverSyncStatusLock);
   s_serverSyncStatus.clear();
   MutexUnlock(s_serverSyncStatusLock);
}

/**
 * Handler for data collector queue size
 */
LONG H_DataCollectorQueueSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (!s_dataCollectorStarted)
      return SYSINFO_RC_UNSUPPORTED;

   UINT32 count = 0;
   MutexLock(s_serverSyncStatusLock);
   Iterator<ServerSyncStatus> *it = s_serverSyncStatus.iterator();
   while(it->hasNext())
      count += (UINT32)it->next()->queueSize;
   delete it;
   MutexUnlock(s_serverSyncStatusLock);

   ret_uint(value, count);
   return SYSINFO_RC_SUCCESS;
}
