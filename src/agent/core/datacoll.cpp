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
 * Database schema version
 */
#define DATACOLL_SCHEMA_VERSION     1

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
	uuid_t m_snmpTargetGuid;
   time_t m_lastPollTime;

public:
   DataCollectionItem(UINT64 serverId, NXCPMessage *msg, UINT32 baseId);
   DataCollectionItem(DB_RESULT hResult, int row);
   DataCollectionItem(DataCollectionItem *item);
   virtual ~DataCollectionItem();

   UINT32 getId() { return m_id; }
   UINT64 getServerId() { return m_serverId; }
   const TCHAR *getName() { return m_name; }
   int getType() { return (int)m_type; }
   int getOrigin() { return (int)m_origin; }
   bool equals(DataCollectionItem *item);
   void updateAndSave(DataCollectionItem *item);
   void saveToDatabase(bool newObject);
   void deleteFromDatabase();

   UINT32 getTimeToNextPoll(time_t now)
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
   msg->getFieldAsBinary(baseId + 6, m_snmpTargetGuid, UUID_LENGTH);
   m_snmpPort = msg->getFieldAsUInt16(baseId + 7);
}

/**
 * Data is selected in this order: server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid
 */
DataCollectionItem::DataCollectionItem(DB_RESULT hResult, int row)
{
   m_serverId = DBGetFieldInt64(hResult, row, 0);
   m_id = DBGetFieldULong(hResult, row, 1);
   m_type = (BYTE)DBGetFieldULong(hResult, row, 2);
   m_origin = (BYTE)DBGetFieldULong(hResult, row, 3);
   m_name = (TCHAR *)malloc(sizeof(TCHAR) * 1025);
   DBGetField(hResult, row, 4, m_name, 1025);
   m_pollingInterval = DBGetFieldULong(hResult, row, 5);
   m_lastPollTime = (time_t)DBGetFieldULong(hResult, row, 6);
   DBGetFieldGUID(hResult, row, 7, m_snmpTargetGuid);
   m_snmpPort = DBGetFieldULong(hResult, row, 8);
}

/**
 * Copy constructor
 */
 DataCollectionItem::DataCollectionItem(DataCollectionItem *item)
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
 }

/**
 * Data collection item destructor
 */
DataCollectionItem::~DataCollectionItem()
{
   safe_free(m_name);
}

bool DataCollectionItem::equals(DataCollectionItem *item)
{
   if(m_serverId == item->m_serverId && m_id == item->m_id)
      return true;
   return false;
}

/**
 * Will check if object has changed. If at least one field is changed - all data will be updated and
 * saved to database.
 */
void DataCollectionItem::updateAndSave(DataCollectionItem *item)
{
   //if at leas one of fields changed - set all fields and save to DB
   if(m_type != item->m_type || m_origin != item->m_origin || _tcscmp(m_name, item->m_name) != 0 ||
      m_pollingInterval != item->m_pollingInterval || uuid_compare(m_snmpTargetGuid, item->m_snmpTargetGuid)!= 0 ||
      m_snmpPort != item->m_snmpPort)
   {
      m_type = item->m_type;
      m_origin = item->m_origin;
      m_name = _tcsdup(item->m_name);
      m_pollingInterval = item->m_pollingInterval;
      m_lastPollTime = item->m_lastPollTime;
      memcpy(m_snmpTargetGuid, item->m_snmpTargetGuid, UUID_LENGTH);
      m_snmpPort = item->m_snmpPort;
      saveToDatabase(false);
   }

}

void DataCollectionItem::saveToDatabase(bool newObject)
{
   DebugPrintf(INVALID_INDEX, 6, _T("DataCollectionItem::saveToDatabase: %s object(serverId=%ld,dciId=%d) saved to database"),
               newObject ? _T("new") : _T("existing"), m_serverId, m_id);
   DB_HANDLE db = GetLocalDatabaseHandle();
   DB_STATEMENT hStmt;

   if(newObject)
   {
		hStmt = DBPrepare(db,
                    _T("INSERT INTO dc_config (type,origin,name,polling_interval,")
                    _T("last_poll,snmp_port,snmp_target_guid,server_id,dci_id)")
                    _T("VALUES (?,?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(db,
                    _T("UPDATE dc_config SET type=?,origin=?,name=?")
                    _T("polling_interval=?,last_poll=?,snmp_port=?")
                    _T("snmp_target_guid=? WHERE server_id=? dci_id=?"));
   }

	if (hStmt == NULL)
	{
	   DebugPrintf(INVALID_INDEX, 2, _T("DataCollectionItem::saveToDatabase: not possible to prepare save quary for %s object(serverId=%ld,dciId=%d)"),
                  newObject ? _T("new") : _T("existing"), m_serverId, m_id);
		return;
   }

   TCHAR buffer[64];
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_type);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_origin);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_pollingInterval);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_lastPollTime);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_snmpPort);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, uuid_to_string(m_snmpTargetGuid, buffer), DB_BIND_STATIC);
	DBBind(hStmt, 8, DB_SQLTYPE_BIGINT, (LONG)m_serverId);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_id);

   if( DBExecute(hStmt))
   {
      DebugPrintf(INVALID_INDEX, 2, _T("DataCollectionItem::saveToDatabase: not possible to save %s object(serverId=%ld,dciId=%d) to database"),
                  newObject ? _T("new") : _T("existing"), m_serverId, m_id);
   }
}

/**
 * Remove item form database and delete not synced data if exist
 */
void DataCollectionItem::deleteFromDatabase()
{
   DebugPrintf(INVALID_INDEX, 6, _T("DataCollectionItem::deleteFromDatabase: object(serverId=%ld,dciId=%d) removed from database"),
   m_serverId, m_id);
   DB_HANDLE db = GetLocalDatabaseHandle();
   TCHAR query[256];
   _sntprintf(query, 256, _T("DELETE FROM dc_config WHERE server_id=%ld, dci_id=%d"), m_serverId, m_id);
   if(!DBQuery(db, query))
   {
      DebugPrintf(INVALID_INDEX, 2, _T("DataCollectionItem::deleteFromDatabase: error wile removing object(serverId=%ld,dciId=%d) from dc_config database table"),
                  m_serverId, m_id);
   }
   _sntprintf(query, 256, _T("DELETE FROM dc_queue WHERE server_id=%ld, dci_id=%d"), m_serverId, m_id);
   if(!DBQuery(db, query))
   {
      DebugPrintf(INVALID_INDEX, 2, _T("DataCollectionItem::deleteFromDatabase: error wile removing object(serverId=%ld,dciId=%d) from dc_queue database table"),
                  m_serverId, m_id);

   }
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
   int m_type;
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
      m_type = DCO_TYPE_ITEM;
      m_value.item = _tcsdup(value);
   }

   DataElement(DataCollectionItem *dci, StringList *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_type = DCO_TYPE_LIST;
      m_value.list = value;
   }

   DataElement(DataCollectionItem *dci, Table *value)
   {
      m_serverId = dci->getServerId();
      m_dciId = dci->getId();
      m_timestamp = time(NULL);
      m_type = DCO_TYPE_TABLE;
      m_value.table = value;
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
};

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
   virtual UINT32 openFile(TCHAR *fileName, UINT32 requestId) { return ERR_INTERNAL_ERROR; }
};

/**
 * Collect data from agent
 */
DataElement *CollectDataFromAgent(DataCollectionItem *dci)
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
DataElement *CollectDataFromSNMP(DataCollectionItem *dci)
{
   /* TODO: implement SNMP data collection */
   return NULL;
}

/**
 * List of all data collection items
 */
static ObjectArray<DataCollectionItem> s_items(64, 64, false);
static MUTEX s_itemLock = INVALID_MUTEX_HANDLE;

/**
 * Single data collection run - collect data if needed and calculate sleep time
 */
static UINT32 DataCollectionRun()
{
   time_t now = time(NULL);
   UINT32 sleepTime = 60;

   MutexLock(s_itemLock);
   for(int i = 0; i < s_items.size(); i++)
   {
      DataCollectionItem *dci = s_items.get(i);
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
      }
      else
      {
         if (sleepTime > timeToPoll)
            sleepTime = timeToPoll;
      }
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
   ObjectArray<DataCollectionItem> config(32, 32, true);

   int count = msg->getFieldAsInt32(VID_NUM_ELEMENTS);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      config.add(new DataCollectionItem(serverId, msg, fieldId));
      fieldId += 10;
   }
   DebugPrintf(INVALID_INDEX, 4, _T("%d data collection elements received from server ") UINT64X_FMT(_T("016")), count, serverId);

   MutexLock(s_itemLock);
   //Update and add new
   for(int j = 0; j < config.size(); j++)
   {
      DataCollectionItem *item = config.get(j);
      bool exist = false;
      for(int i = 0; i < s_items.size(); i++)
      {
         if(item->equals(s_items.get(i)))
         {
            s_items.get(i)->updateAndSave(item);
            exist = true;
         }
      }
      if(!exist)
      {
         DataCollectionItem *newItem = new DataCollectionItem(item);
         s_items.add(newItem);
         newItem->saveToDatabase(true);
      }
   }
   //Remove not existing configuration and data for it
   for(int i = 0; i < s_items.size(); i++)
   {
      DataCollectionItem *item = s_items.get(i);
      bool exist = false;
      for(int j = 0; j < config.size(); j++)
      {
         if(item->equals(config.get(j)))
         {
            exist = true;
         }
      }
      if(!exist)
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
 * Data collector and sender thread handles
 */
static THREAD s_dataCollectorThread = INVALID_THREAD_HANDLE;
static THREAD s_dataSenderThread = INVALID_THREAD_HANDLE;

/**
 * Loads configuration to for DCI
 */
static void LoadConfiguration()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   const TCHAR *query = _T("SELECT server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid FROM dc_config");
   DB_RESULT hResult = DBSelect(db, query);
   if(hResult != NULL)
   {
      for(int i = 0; i < DBGetNumRows(hResult); i++)
      {
         s_items.add(new DataCollectionItem(hResult, i));
      }
      DBFreeResult(hResult);
   }
}

/**
 * SQL script array
 */
const static TCHAR *Update [] =
{
   _T("CREATE TABLE dc_queue (")
   _T("server_id number(20) not null,")
   _T("dci_id integer not null,")
   _T("timestamp integer not null,")
   _T("value varchar not null,")
   _T("PRIMARY KEY(server_id,dci_id,timestamp));")
   _T("CREATE TABLE dc_config (")
   _T("server_id number(20) not null,")
   _T("dci_id integer not null,")
   _T("type integer not null,")
   _T("origin integer not null,")
   _T("name varchar(1023) null,")
   _T("polling_interval integer not null,")
   _T("last_poll integer not null,")
   _T("snmp_port integer not null,")
   _T("snmp_target_guid varchar(36) not null,")
   _T("PRIMARY KEY(server_id,dci_id));")
   _T("CREATE TABLE snmp_targets (")
   _T("guid varchar(36) not null,")
   _T("version integer not null,")
   _T("ip_address varchar(48) not null,")
   _T("port integer not null,")
   _T("auth_type integer not null,")
   _T("enc_type integer not null,")
   _T("auth_pass varchar(255),")
   _T("enc_pass varchar(255),")
   _T("username varchar(255)")
   _T("PRIMARY KEY(guid));")
};

/**
 * Initialize and start local data collector
 */
void StartLocalDataCollector()
{
   /* TODO: database init and configuration load */
   DB_HANDLE db = GetLocalDatabaseHandle();
   if(db == NULL)
   {
      DebugPrintf(INVALID_INDEX, 5, _T("StartLocalDataCollector: Not possible to load Data Collector. Database not initialized."));
      return;
   }
   int dbVersion = ReadMetadataAsInt(_T("DatacollSchemaVersion"));
   while(DATACOLL_SCHEMA_VERSION > dbVersion)
   {
      bool result = DBQuery(db, Update[dbVersion]);
      TCHAR query[256];
      _sntprintf(query, 256, _T("INSERT INTO metadata (attribute, value) VALUES ('DatacollSchemaVersion', '%d')"), ++dbVersion);
      if(result)
      {
         DBQuery(db, query);
      }
      else
      {
         DebugPrintf(INVALID_INDEX, 5, _T("StartLocalDataCollector: Not possible to upgdate database for Data Collector. Collection not started."));
         return;
      }
   }

   LoadConfiguration();
   /* TODO: add reading form database snmp_targets table */

   s_itemLock = MutexCreate();
   s_dataCollectorThread = ThreadCreateEx(DataCollector, 0, NULL);
   s_dataSenderThread = ThreadCreateEx(DataSender, 0, NULL);
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

   MutexDestroy(s_itemLock);
}
