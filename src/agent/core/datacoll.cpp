/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
bool GetSnmpValue(const uuid_t& target, UINT16 port, const TCHAR *oid, TCHAR *value, int interpretRawValue);

/**
 * Database schema version
 */
#define DATACOLL_SCHEMA_VERSION     3

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
	uuid_t m_snmpTargetGuid;
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
   const uuid_t& getSnmpTargetGuid() const { return m_snmpTargetGuid; }
   UINT16 getSnmpPort() const { return m_snmpPort; }
   int getSnmpRawValueType() const { return (int)m_snmpRawValueType; }
   UINT32 getPollingInterval() { return (UINT32)m_pollingInterval; }

   bool equals(const DataCollectionItem *item) const { return (m_serverId == item->m_serverId) && (m_id == item->m_id); }

   void updateAndSave(const DataCollectionItem *item);
   void saveToDatabase(bool newObject);
   void deleteFromDatabase();
   void setLastPollTime(time_t time);

   UINT32 getTimeToNextPoll(time_t now) const
   {
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
   memset(m_snmpTargetGuid, 0, UUID_LENGTH);
   msg->getFieldAsBinary(baseId + 6, m_snmpTargetGuid, UUID_LENGTH);
   m_snmpPort = msg->getFieldAsUInt16(baseId + 7);
   m_snmpRawValueType = (BYTE)msg->getFieldAsUInt16(baseId + 8);
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
   DBGetFieldGUID(hResult, row, 7, m_snmpTargetGuid);
   m_snmpPort = DBGetFieldULong(hResult, row, 8);
   m_snmpRawValueType = (BYTE)DBGetFieldULong(hResult, row, 9);
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
   memcpy(m_snmpTargetGuid, item->m_snmpTargetGuid, UUID_LENGTH);
   m_snmpPort = item->m_snmpPort;
   m_snmpRawValueType = item->m_snmpRawValueType;
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
   //if at leas one of fields changed - set all fields and save to DB
   if ((m_type != item->m_type) || (m_origin != item->m_origin) || _tcscmp(m_name, item->m_name) ||
       (m_pollingInterval != item->m_pollingInterval) || uuid_compare(m_snmpTargetGuid, item->m_snmpTargetGuid) ||
       (m_snmpPort != item->m_snmpPort) || (m_snmpRawValueType != item->m_snmpRawValueType) || (m_lastPollTime < item->m_lastPollTime))
   {
      m_type = item->m_type;
      m_origin = item->m_origin;
      m_name = _tcsdup(item->m_name);
      m_pollingInterval = item->m_pollingInterval;
      if (m_lastPollTime < item->m_lastPollTime)
         m_lastPollTime = item->m_lastPollTime;
      memcpy(m_snmpTargetGuid, item->m_snmpTargetGuid, UUID_LENGTH);
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
   DebugPrintf(INVALID_INDEX, 6, _T("DataCollectionItem::saveToDatabase: %s object(serverId=%ld,dciId=%d) saved to database"),
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

   TCHAR buffer[64];
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_type);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_origin);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_pollingInterval);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_lastPollTime);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_snmpPort);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, uuid_to_string(m_snmpTargetGuid, buffer), DB_BIND_STATIC);
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
         DebugPrintf(INVALID_INDEX, 6, _T("DataCollectionItem::deleteFromDatabase: object(serverId=") UINT64X_FMT(_T("016")) _T(",dciId=%d) removed from database"), m_serverId, m_id);
      }
   }
}

/**
 * Set last poll time for item
 */
void DataCollectionItem::setLastPollTime(time_t time)
{
   m_lastPollTime = time;
   TCHAR query[256];
   _sntprintf(query, 256, _T("UPDATE dc_config SET last_poll=") UINT64_FMT _T(" WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d"),
      (UINT64)m_lastPollTime, m_serverId, m_id);
   DBQuery(GetLocalDatabaseHandle(), query);
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
   uuid_t m_snmpNode;
   union
   {
      TCHAR *item;
      StringList *list;
      Table *table;
   } m_value;

public:
   DataElement(DataCollectionItem *dci, const TCHAR *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_ITEM;
      memcpy(m_snmpNode, dci->getSnmpTargetGuid(), UUID_LENGTH);
      m_value.item = _tcsdup(value);
   }

   DataElement(DataCollectionItem *dci, StringList *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_LIST;
      memcpy(m_snmpNode, dci->getSnmpTargetGuid(), UUID_LENGTH);
      m_value.list = value;
   }

   DataElement(DataCollectionItem *dci, Table *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_origin = dci->getOrigin();
      m_type = DCO_TYPE_TABLE;
      memcpy(m_snmpNode, dci->getSnmpTargetGuid(), UUID_LENGTH);
      m_value.table = value;
   }

   /**
    * Create data element from database record
    * Expected field order: server_id,dci_id,dci_type,dci_origin,snmp_target_guid,timestamp,value
    */
   DataElement(DB_RESULT hResult, int row)
   {
      m_serverId = DBGetFieldUInt64(hResult, row, 0);
      m_dciId = DBGetFieldULong(hResult, row, 1);
      m_timestamp = (time_t)DBGetFieldInt64(hResult, row, 5);
      m_origin = DBGetFieldLong(hResult, row, 3);
      m_type = DBGetFieldLong(hResult, row, 2);
      DBGetFieldGUID(hResult, row, 4, m_snmpNode);
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            m_value.item = DBGetField(hResult, row, 6, NULL, 0);
            break;
         case DCO_TYPE_LIST:
            {
               m_value.list = new StringList();
               TCHAR *text = DBGetField(hResult, row, 6, NULL, 0);
               if (text != NULL)
               {
                  m_value.list->splitAndAdd(text, _T("\n"));
                  free(text);
               }
            }
            break;
         case DCO_TYPE_TABLE:
            {
               char *xml = DBGetFieldUTF8(hResult, row, 6, NULL, 0);
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

   void saveToDatabase();
   bool sendToServer(bool reconcillation);
};

/**
 * Save data element to database
 */
void DataElement::saveToDatabase()
{
    DB_HANDLE db = GetLocalDatabaseHandle();
    DB_STATEMENT hStmt= DBPrepare(db, _T("INSERT INTO dc_queue (server_id,dci_id,dci_type,dci_origin,snmp_target_guid,timestamp,value) VALUES (?,?,?,?,?,?,?)"));
    if (hStmt == NULL)
       return;

   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_dciId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)m_type);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_origin);
   TCHAR buffer[64];
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, uuid_to_string(m_snmpNode, buffer), DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_timestamp);
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_value.item, DB_BIND_STATIC);
         break;
      case DCO_TYPE_LIST:
         DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_value.list->join(_T("\n")), DB_BIND_DYNAMIC);
         break;
      case DCO_TYPE_TABLE:
         DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_value.table->createXML(), DB_BIND_DYNAMIC);
         break;
   }
   DBExecute(hStmt);
   DBFreeStatement(hStmt);
}

/**
 * Send collected data to server
 */
bool DataElement::sendToServer(bool reconcillation)
{
   // If data in database was invalid table may not be parsed correctly
   // Consider sending a success in that case so data element can be dropped
   if ((m_type == DCO_TYPE_TABLE) && (m_value.table == NULL))
      return true;

   CommSession *session = (CommSession *)FindServerSession(m_serverId);
   if (session == NULL)
      return false;

   NXCPMessage msg;
   msg.setCode(CMD_DCI_DATA);
   msg.setId(session->generateRequestId());
   msg.setField(VID_DCI_ID, m_dciId);
   msg.setField(VID_DCI_SOURCE_TYPE, (INT16)m_origin);
   msg.setField(VID_DCOBJECT_TYPE, (INT16)m_type);
   msg.setField(VID_NODE_ID, m_snmpNode, UUID_LENGTH);
   msg.setFieldFromTime(VID_TIMESTAMP, m_timestamp);
   msg.setField(VID_RECONCILLATION, (INT16)(reconcillation ? 1 : 0));
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         msg.setField(VID_VALUE, m_value.item);
         break;
      case DCO_TYPE_LIST:
         m_value.list->fillMessage(&msg, VID_ENUM_VALUE_BASE, VID_NUM_STRINGS);
         break;
      case DCO_TYPE_TABLE:
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
 * Data reconcillation thread
 */
static THREAD_RESULT THREAD_CALL ReconcillationThread(void *arg)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   UINT32 sleepTime = 30000;
   DebugPrintf(INVALID_INDEX, 1, _T("Data reconcillation thread started"));

   while(!AgentSleepAndCheckForShutdown(sleepTime))
   {
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT server_id,dci_id,dci_type,dci_origin,snmp_target_guid,timestamp,value FROM dc_queue ORDER BY timestamp LIMIT 100"));
      if (hResult == NULL)
         continue;

      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         ObjectArray<DataElement> deleteList(count, 10, true);
         for(int i = 0; i < count; i++)
         {
            DataElement *e = new DataElement(hResult, i);
            bool sent = false;
            MutexLock(s_serverSyncStatusLock);
            ServerSyncStatus *status = s_serverSyncStatus.get(e->getServerId());
            if (status != NULL)
            {
               sent = e->sendToServer(true);
            }
            else
            {
               DebugPrintf(INVALID_INDEX, 5, _T("INTERNAL ERROR: cached DCI value without server sync status object"));
            }

            if (sent)
            {
               status->queueSize--;
               deleteList.add(e);
            }
            else
            {
               delete e;
            }
            MutexUnlock(s_serverSyncStatusLock);
         }

         if (deleteList.size() > 0)
         {
            TCHAR query[256];

            DBBegin(hdb);
            for(int i = 0; i < deleteList.size(); i++)
            {
               DataElement *e = deleteList.get(i);
               _sntprintf(query, 256, _T("DELETE FROM dc_queue WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d AND timestamp=") INT64_FMT,
                  e->getServerId(), e->getDciId(), (INT64)e->getTimestamp());
               DBQuery(hdb, query);
            }
            DBCommit(hdb);
         }
      }
      DBFreeResult(hResult);

      sleepTime = (count == 100) ? 100 : 30000;
   }

   DebugPrintf(INVALID_INDEX, 1, _T("Data reconcillation thread stopped"));
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
   DebugPrintf(INVALID_INDEX, 1, _T("Data sender thread started"));
   while(true)
   {
      DataElement *e = (DataElement *)s_dataSenderQueue.GetOrBlock();
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
            e->saveToDatabase();
            status->queueSize++;
         }
      }
      else
      {
         e->saveToDatabase();
         status->queueSize++;
      }
      MutexUnlock(s_serverSyncStatusLock);

      delete e;
   }
   DebugPrintf(INVALID_INDEX, 1, _T("Data sender thread stopped"));
   return THREAD_OK;
}

/**
 * Pseudo-session for cached data collection
 */
class VirtualSession : public AbstractCommSession
{
private:
   UINT64 m_serverId;

public:
   VirtualSession(UINT64 serverId) { m_serverId = serverId; }

   virtual bool isMasterServer() { return false; }
   virtual bool isControlServer() { return false; }
   virtual bool canAcceptTraps() { return true; }
   virtual UINT64 getServerId() { return m_serverId; };
   virtual const InetAddress& getServerAddress() { return InetAddress::LOOPBACK; }

   virtual bool isIPv6Aware() { return true; }

   virtual void sendMessage(NXCPMessage *pMsg) { }
   virtual void sendRawMessage(NXCP_MESSAGE *pMsg) { }
   virtual bool sendFile(UINT32 requestId, const TCHAR *file, long offset) { return false; }
   virtual UINT32 doRequest(NXCPMessage *msg, UINT32 timeout) { return RCC_NOT_IMPLEMENTED; }
   virtual UINT32 generateRequestId() { return 0; }
   virtual UINT32 openFile(TCHAR *fileName, UINT32 requestId) { return ERR_INTERNAL_ERROR; }
};

/**
 * Collect data from agent
 */
static DataElement *CollectDataFromAgent(DataCollectionItem *dci)
{
   VirtualSession session(dci->getServerId());

   DataElement *e = NULL;
   if (dci->getType() == DCO_TYPE_ITEM)
   {
      TCHAR value[MAX_RESULT_LENGTH];
      if (GetParameterValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
   }
   else if (dci->getType() == DCO_TYPE_LIST)
   {
      StringList *value = new StringList;
      if (GetListValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
   }
   else if (dci->getType() == DCO_TYPE_TABLE)
   {
      Table *value = new Table;
      if (GetTableValue(INVALID_INDEX, dci->getName(), value, &session) == ERR_SUCCESS)
         e = new DataElement(dci, value);
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
      TCHAR value[MAX_RESULT_LENGTH];
      if (GetSnmpValue(dci->getSnmpTargetGuid(), dci->getSnmpPort(), dci->getName(), value, dci->getSnmpRawValueType()))
         e = new DataElement(dci, value);
   }
   return e;
}

/**
 * List of all data collection items
 */
static ObjectArray<DataCollectionItem> s_items(64, 64, true);
static MUTEX s_itemLock = INVALID_MUTEX_HANDLE;

/**
 * Single data collection run - collect data if needed and calculate sleep time
 */
static UINT32 DataCollectionRun()
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
         DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: polling DCI %d \"%s\""), dci->getId(), dci->getName());
         DataElement *e;
         if (dci->getOrigin() == DS_NATIVE_AGENT)
         {
            e = CollectDataFromAgent(dci);
         }
         else if (dci->getOrigin() == DS_SNMP_AGENT)
         {
            e = CollectDataFromSNMP(dci);
         }
         else
         {
            DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: unsupported origin %d"), dci->getOrigin());
            e = NULL;
         }

         if (e != NULL)
         {
            s_dataSenderQueue.Put(e);
         }
         else
         {
            DebugPrintf(INVALID_INDEX, 6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
         }

         dci->setLastPollTime(now);
         timeToPoll = dci->getPollingInterval();
      }

      if (sleepTime > timeToPoll)
         sleepTime = timeToPoll;
   }
   MutexUnlock(s_itemLock);
   return sleepTime;
}

/**
 * Data collector thread
 */
static THREAD_RESULT THREAD_CALL DataCollector(void *arg)
{
   DebugPrintf(INVALID_INDEX, 1, _T("Data collector thread started"));

   UINT32 sleepTime = DataCollectionRun();
   while(!AgentSleepAndCheckForShutdown(sleepTime * 1000))
   {
      sleepTime = DataCollectionRun();
      DebugPrintf(INVALID_INDEX, 7, _T("DataCollector: sleeping for %d seconds"), sleepTime);
   }

   DebugPrintf(INVALID_INDEX, 1, _T("Data collector thread stopped"));
   return THREAD_OK;
}

/**
 * Configure data collection
 */
void ConfigureDataCollection(UINT64 serverId, NXCPMessage *msg)
{
   int count = msg->getFieldAsInt32(VID_NUM_NODES);
   UINT32 fieldId = VID_NODE_INFO_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      SNMPTarget *target = new SNMPTarget(serverId, msg, fieldId);
      UpdateSnmpTarget(target);
      fieldId += 50;
   }
   DebugPrintf(INVALID_INDEX, 4, _T("%d SNMP targets received from server ") UINT64X_FMT(_T("016")), count, serverId);

   ObjectArray<DataCollectionItem> config(32, 32, true);

   count = msg->getFieldAsInt32(VID_NUM_ELEMENTS);
   fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      config.add(new DataCollectionItem(serverId, msg, fieldId));
      fieldId += 10;
   }
   DebugPrintf(INVALID_INDEX, 4, _T("%d data collection elements received from server ") UINT64X_FMT(_T("016")), count, serverId);

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
         if(item->equals(config.get(j)))
         {
            exist = true;
         }
      }
      if (!exist)
      {
         item->deleteFromDatabase();
         s_items.remove(i);
         i--;
         delete item;
      }
   }
   MutexUnlock(s_itemLock);
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
         DebugPrintf(INVALID_INDEX, 2, _T("%d elements in queue for server ID ") UINT64X_FMT(_T("016")), s->queueSize, serverId);
      }
      DBFreeResult(hResult);
   }
}

/**
 * SQL script array
 */
static const TCHAR *s_upgradeQueries[] =
{
   _T("CREATE TABLE dc_queue (")
   _T("  server_id number(20) not null,")
   _T("  dci_id integer not null,")
   _T("  dci_type integer not null,")
   _T("  dci_origin integer not null,")
   _T("  snmp_target_guid varchar(36) not null,")
   _T("  timestamp integer not null,")
   _T("  value varchar not null,")
   _T("  PRIMARY KEY(server_id,dci_id,timestamp))"),

   _T("CREATE TABLE dc_config (")
   _T("  server_id number(20) not null,")
   _T("  dci_id integer not null,")
   _T("  type integer not null,")
   _T("  origin integer not null,")
   _T("  name varchar(1023) null,")
   _T("  polling_interval integer not null,")
   _T("  last_poll integer not null,")
   _T("  snmp_port integer not null,")
   _T("  snmp_target_guid varchar(36) not null,")
   _T("  snmp_raw_type integer not null,")
   _T("  PRIMARY KEY(server_id,dci_id))"),

   _T("CREATE TABLE dc_snmp_targets (")
   _T("  guid varchar(36) not null,")
   _T("  server_id number(20) not null,")
   _T("  ip_address varchar(48) not null,")
   _T("  snmp_version integer not null,")
   _T("  port integer not null,")
   _T("  auth_type integer not null,")
   _T("  enc_type integer not null,")
   _T("  auth_name varchar(63),")
   _T("  auth_pass varchar(63),")
   _T("  enc_pass varchar(63),")
   _T("  PRIMARY KEY(guid))")
};

/**
 * Data collector and sender thread handles
 */
static THREAD s_dataCollectorThread = INVALID_THREAD_HANDLE;
static THREAD s_dataSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_reconcillationThread = INVALID_THREAD_HANDLE;

/**
 * Initialize and start local data collector
 */
void StartLocalDataCollector()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == NULL)
   {
      DebugPrintf(INVALID_INDEX, 5, _T("StartLocalDataCollector: local database unavailable"));
      return;
   }

   INT32 dbVersion = ReadMetadataAsInt(_T("DataCollectionSchemaVersion"));
   while(dbVersion < DATACOLL_SCHEMA_VERSION)
   {
      if (!DBQuery(db, s_upgradeQueries[dbVersion]))
      {
         nxlog_write(MSG_DC_DBSCHEMA_UPGRADE_FAILED, NXLOG_ERROR, NULL);
         return;
      }
      dbVersion++;
      WriteMetadata(_T("DataCollectionSchemaVersion"), dbVersion);
   }

   s_itemLock = MutexCreate();
   s_serverSyncStatusLock = MutexCreate();

   LoadState();

   s_dataCollectorThread = ThreadCreateEx(DataCollector, 0, NULL);
   s_dataSenderThread = ThreadCreateEx(DataSender, 0, NULL);
   s_reconcillationThread = ThreadCreateEx(ReconcillationThread, 0, NULL);
}

/**
 * Shutdown local data collector
 */
void ShutdownLocalDataCollector()
{
   DebugPrintf(INVALID_INDEX, 5, _T("Waiting for data collector thread termination"));
   ThreadJoin(s_dataCollectorThread);

   DebugPrintf(INVALID_INDEX, 5, _T("Waiting for data sender thread termination"));
   s_dataSenderQueue.Put(INVALID_POINTER_VALUE);
   ThreadJoin(s_dataSenderThread);

   DebugPrintf(INVALID_INDEX, 5, _T("Waiting for data reconcillation thread termination"));
   ThreadJoin(s_reconcillationThread);

   MutexDestroy(s_itemLock);
   MutexDestroy(s_serverSyncStatusLock);
}
