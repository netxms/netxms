/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Raden Solutions
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

#define DEBUG_TAG _T("dc")

/**
 * Interval in milliseconds between stalled data checks
 */
#define STALLED_DATA_CHECK_INTERVAL    3600000

/**
 * Externals
 */
void UpdateSnmpTarget(shared_ptr<SNMPTarget> target);
uint32_t GetSnmpValue(const uuid& target, uint16_t port, SNMP_Version version, const TCHAR *oid,
         TCHAR *value, int interpretRawValue);
uint32_t GetSnmpTable(const uuid& target, uint16_t port, SNMP_Version version, const TCHAR *oid,
         const ObjectArray<SNMPTableColumnDefinition> &columns, Table *value);

void LoadProxyConfiguration();
void UpdateProxyConfiguration(uint64_t serverId, HashMap<ServerObjectKey, DataCollectionProxy> *proxyList, const ZoneConfiguration& zone);
void ProxyConnectionChecker();
void ProxyListenerThread();

extern HashMap<ServerObjectKey, DataCollectionProxy> g_proxyList;
extern Mutex g_proxyListLock;

extern uint32_t g_dcReconciliationBlockSize;
extern uint32_t g_dcReconciliationTimeout;
extern uint32_t g_dcWriterFlushInterval;
extern uint32_t g_dcWriterMaxTransactionSize;
extern uint32_t g_dcMinCollectorPoolSize;
extern uint32_t g_dcMaxCollectorPoolSize;
extern uint32_t g_dcOfflineExpirationTime;

/**
 * Data collector start indicator
 */
static bool s_dataCollectorStarted = false;

/**
 * Unsaved poll time indicator
 */
static bool s_pollTimeChanged = false;

/**
 * Create column object from NXCP message
 */
SNMPTableColumnDefinition::SNMPTableColumnDefinition(const NXCPMessage& msg, uint32_t baseId)
{
   msg.getFieldAsString(baseId, m_name, MAX_COLUMN_NAME);
   m_flags = msg.getFieldAsUInt16(baseId + 1);
   m_displayName = msg.getFieldAsString(baseId + 3);

   if (msg.isFieldExist(baseId + 2))
   {
      uint32_t oid[256];
      size_t len = msg.getFieldAsInt32Array(baseId + 2, 256, oid);
      if (len > 0)
      {
         m_snmpOid = SNMP_ObjectId(oid, len);
      }
   }
}

/**
 * Create column object from DB record
 */
SNMPTableColumnDefinition::SNMPTableColumnDefinition(DB_RESULT hResult, int row)
{
   DBGetField(hResult, row, 0, m_name, MAX_COLUMN_NAME);
   m_displayName = DBGetField(hResult, row, 1, nullptr, 0);
   m_flags = static_cast<uint16_t>(DBGetFieldULong(hResult, row, 2));

   TCHAR oid[1024];
   oid[0] = 0;
   DBGetField(hResult, row, 3, oid, 1024);
   Trim(oid);
   m_snmpOid = SNMP_ObjectId::parse(oid);
}

/**
 * Create copy of column object
 */
SNMPTableColumnDefinition::SNMPTableColumnDefinition(const SNMPTableColumnDefinition *src) : m_snmpOid(src->m_snmpOid)
{
   memcpy(m_name, src->m_name, sizeof(m_name));
   m_displayName = MemCopyString(src->m_displayName);
   m_flags = src->m_flags;
}

/**
 * Statement set
 */
struct DataCollectionStatementSet
{
   DB_STATEMENT insertItem;
   DB_STATEMENT updateItem;
   DB_STATEMENT deleteItem;
   DB_STATEMENT insertColumn;
   DB_STATEMENT deleteColumns;
   DB_STATEMENT deleteSchedules;
   DB_STATEMENT insertSchedules;
};

enum class ScheduleType
{
   NONE = 0,
   MINUTES = 1,
   SECONDS = 2
};

/**
 * Data collection item
 */
class DataCollectionItem
{
private:
   uint64_t m_serverId;
   uint32_t m_id;
   int32_t m_pollingInterval;
   TCHAR *m_name;
   uint8_t m_type;
   uint8_t m_origin;
   uint8_t m_snmpRawValueType;
   bool m_busy;
   bool m_disabled;
   uint16_t m_snmpPort;
   SNMP_Version m_snmpVersion;
	uuid m_snmpTargetGuid;
	Timestamp m_lastPollTime;
   uint32_t m_backupProxyId;
   ObjectArray<SNMPTableColumnDefinition> *m_tableColumns;
   StringList m_schedules;
   ScheduleType m_scheduleType;
   time_t m_tLastCheck;

public:
   DataCollectionItem(uint64_t serverId, const NXCPMessage& msg, uint32_t baseId, uint32_t extBaseId, bool hasExtraData);
   DataCollectionItem(DB_RESULT hResult, int row);
   DataCollectionItem(const DataCollectionItem *item);
   ~DataCollectionItem();

   uint32_t getId() const { return m_id; }
   uint64_t getServerId() const { return m_serverId; }
   ServerObjectKey getKey() const { return ServerObjectKey(m_serverId, m_id); }
   const TCHAR *getName() const { return m_name; }
   int getType() const { return (int)m_type; }
   int getOrigin() const { return (int)m_origin; }
   const uuid& getSnmpTargetGuid() const { return m_snmpTargetGuid; }
   uint16_t getSnmpPort() const { return m_snmpPort; }
   SNMP_Version getSnmpVersion() const { return m_snmpVersion; }
   int getSnmpRawValueType() const { return (int)m_snmpRawValueType; }
   uint32_t getPollingInterval() const { return static_cast<uint32_t>(m_pollingInterval); }
   Timestamp getLastPollTime() { return m_lastPollTime; }
   uint32_t getBackupProxyId() const { return m_backupProxyId; }
   const ObjectArray<SNMPTableColumnDefinition> *getColumns() const { return m_tableColumns; }

   bool updateAndSave(const shared_ptr<DataCollectionItem>& item, bool txnOpen, DB_HANDLE hdb, DataCollectionStatementSet *statements);
   void saveToDatabase(bool newObject, DB_HANDLE hdb, DataCollectionStatementSet *statements);
   void deleteFromDatabase(DB_HANDLE hdb, DataCollectionStatementSet *statements);
   void setLastPollTime(Timestamp time)
   {
      m_lastPollTime = time;
      s_pollTimeChanged = true;
   }

   void startDataCollection() { m_busy = true; }
   void finishDataCollection() { m_busy = false; }

   bool isDisabled() const { return m_disabled; }
   void setDisabled() { m_disabled = true; }

   uint32_t getTimeToNextPoll(time_t now)
   {
      if (m_busy) // being polled now - time to next poll should not be less than full polling interval
         return m_pollingInterval;

      if (m_scheduleType == ScheduleType::NONE)
      {
         time_t diff = now - m_lastPollTime.asTime();
         return (diff >= m_pollingInterval) ? 0 : m_pollingInterval - static_cast<uint32_t>(diff);
      }
      else
      {
         bool scheduleMatched = false;
         struct tm tmCurrLocal, tmLastLocal;
#if HAVE_LOCALTIME_R
         localtime_r(&now, &tmCurrLocal);
         localtime_r(&m_tLastCheck, &tmLastLocal);
#else
         memcpy(&tmCurrLocal, localtime(&now), sizeof(struct tm));
         memcpy(&tmLastLocal, localtime(&m_tLastCheck), sizeof(struct tm));
#endif

         for (int i = 0; i < m_schedules.size(); i++)
         {
            bool withSeconds = false;
            if (MatchSchedule(m_schedules.get(i), &withSeconds, &tmCurrLocal, now))
            {
               if (withSeconds || (now - m_tLastCheck >= 60) || (tmCurrLocal.tm_min != tmLastLocal.tm_min))
               {
                  scheduleMatched = true;
                  break;
               }
            }
         }
         m_tLastCheck = now;

         if (scheduleMatched)
            return 0;
         if (m_scheduleType == ScheduleType::SECONDS)
            return 1;
         else
            return 60 - (now % 60);

      }
   }
};

static bool UsesSeconds(const TCHAR *schedule)
{
   TCHAR szValue[256];

   const TCHAR *pszCurr = ExtractWord(schedule, szValue);
   for (int i = 0; i < 5; i++)
   {
      szValue[0] = _T('\0');
      pszCurr = ExtractWord(pszCurr, szValue);
      if (szValue[0] == _T('\0'))
      {
         return false;
      }
   }

   return true;
}

/**
 * Create data collection item from NXCP mesage
 */
DataCollectionItem::DataCollectionItem(uint64_t serverId, const NXCPMessage& msg, uint32_t baseId, uint32_t extBaseId, bool hasExtraData)
{
   m_serverId = serverId;
   m_id = msg.getFieldAsInt32(baseId);
   m_type = static_cast<uint8_t>(msg.getFieldAsUInt16(baseId + 1));
   m_origin = static_cast<uint8_t>(msg.getFieldAsUInt16(baseId + 2));
   m_name = msg.getFieldAsString(baseId + 3);
   m_pollingInterval = msg.getFieldAsInt32(baseId + 4);
   m_snmpTargetGuid = msg.getFieldAsGUID(baseId + 6);
   m_snmpPort = msg.getFieldAsUInt16(baseId + 7);
   m_snmpRawValueType = static_cast<uint8_t>(msg.getFieldAsUInt16(baseId + 8));
   m_backupProxyId = msg.getFieldAsInt32(baseId + 9);
   m_busy = false;
   m_disabled = false;
   m_tLastCheck = 0;

   // Starting with version 6.0, last poll time in milliseconds will be stored at extBaseId + 1
   if (hasExtraData)
   {
      m_lastPollTime = msg.getFieldAsTimestamp(extBaseId + 1);
      if (m_lastPollTime.isNull())
      {
         // For backward compatibility, if last poll time in milliseconds is not set, use time in seconds
         m_lastPollTime = Timestamp::fromTime(msg.getFieldAsTime(baseId + 5));
      }
   }
   else
   {
      m_lastPollTime = Timestamp::fromTime(msg.getFieldAsTime(baseId + 5));
   }

   if (hasExtraData && (m_origin == DS_SNMP_AGENT))
   {
      m_snmpVersion = SNMP_VersionFromInt(msg.getFieldAsInt16(extBaseId));
      if (m_type == DCO_TYPE_TABLE)
      {
         int count = msg.getFieldAsInt32(extBaseId + 9);
         uint32_t fieldId = extBaseId + 10;
         m_tableColumns = new ObjectArray<SNMPTableColumnDefinition>(count, 16, Ownership::True);
         for(int i = 0; i < count; i++)
         {
            m_tableColumns->add(new SNMPTableColumnDefinition(msg, fieldId));
            fieldId += 10;
         }
      }
      else
      {
         m_tableColumns = nullptr;
      }
   }
   else
   {
      m_snmpVersion = SNMP_VERSION_DEFAULT;
      m_tableColumns = nullptr;
   }
   extBaseId += 1000;

   int32_t size = msg.getFieldAsInt32(extBaseId++);
   if (size > 0)
   {
      m_scheduleType = ScheduleType::MINUTES;
      for(int i = 0; i < size; i++)
      {
         TCHAR *tmp = msg.getFieldAsString(extBaseId++);
         if (UsesSeconds(tmp))
            m_scheduleType = ScheduleType::SECONDS;
         m_schedules.addPreallocated(tmp);
      }
   }
   else
   {
      m_scheduleType = ScheduleType::NONE;
   }
}

/**
 * Data is selected in this order: server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid,snmp_raw_type,backup_proxy_id,snmp_version,schedule_type
 */
DataCollectionItem::DataCollectionItem(DB_RESULT hResult, int row)
{
   m_serverId = DBGetFieldUInt64(hResult, row, 0);
   m_id = DBGetFieldUInt32(hResult, row, 1);
   m_type = static_cast<uint8_t>(DBGetFieldUInt32(hResult, row, 2));
   m_origin = static_cast<uint8_t>(DBGetFieldUInt32(hResult, row, 3));
   m_name = DBGetField(hResult, row, 4, nullptr, 0);
   m_pollingInterval = DBGetFieldULong(hResult, row, 5);
   m_lastPollTime = DBGetFieldTimestamp(hResult, row, 6);
   m_snmpPort = DBGetFieldUInt32(hResult, row, 7);
   m_snmpTargetGuid = DBGetFieldGUID(hResult, row, 8);
   m_snmpRawValueType = static_cast<uint8_t>(DBGetFieldULong(hResult, row, 9));
   m_backupProxyId = DBGetFieldUInt32(hResult, row, 10);
   m_snmpVersion = SNMP_VersionFromInt(DBGetFieldLong(hResult, row, 11));
   m_scheduleType = static_cast<ScheduleType>(DBGetFieldInt32(hResult, row, 12));
   m_busy = false;
   m_disabled = false;
   m_tLastCheck = 0;

   if ((m_origin == DS_SNMP_AGENT) && (m_type == DCO_TYPE_TABLE))
   {
      DB_HANDLE hdb = GetLocalDatabaseHandle();

      TCHAR query[256];
      _sntprintf(query, 256,
               _T("SELECT name,display_name,flags,snmp_oid FROM dc_snmp_table_columns WHERE server_id=") UINT64_FMT _T(" AND dci_id=%u ORDER BY column_id"),
               m_serverId, m_id);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         if (count > 0)
         {
            m_tableColumns = new ObjectArray<SNMPTableColumnDefinition>(count, 16, Ownership::True);
            for(int i = 0; i < count; i++)
            {
               m_tableColumns->add(new SNMPTableColumnDefinition(hResult, i));
            }
         }
         else
         {
            m_tableColumns = nullptr;
         }
         DBFreeResult(hResult);
      }
      else
      {
         m_tableColumns = nullptr;
      }
   }
   else
   {
      m_tableColumns = nullptr;
   }

   if (m_scheduleType != ScheduleType::NONE)
   {
      DB_HANDLE hdb = GetLocalDatabaseHandle();

      TCHAR query[256];
      _sntprintf(query, 256,
               _T("SELECT schedule FROM dc_schedules WHERE server_id=") UINT64_FMT _T(" AND dci_id=%u"),
               m_serverId, m_id);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
        int count = DBGetNumRows(hResult);
        for(int i = 0; i < count; i++)
        {
           m_schedules.addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
        }
      }
   }
}

/**
 * Copy constructor
 */
 DataCollectionItem::DataCollectionItem(const DataCollectionItem *item) : m_schedules(item->m_schedules)
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
   m_snmpVersion = item->m_snmpVersion;
   m_snmpRawValueType = item->m_snmpRawValueType;
   m_backupProxyId = item->m_backupProxyId;
   if (item->m_tableColumns != nullptr)
   {
      m_tableColumns = new ObjectArray<SNMPTableColumnDefinition>(item->m_tableColumns->size(), 16, Ownership::True);
      for(int i = 0; i < item->m_tableColumns->size(); i++)
         m_tableColumns->add(new SNMPTableColumnDefinition(item->m_tableColumns->get(i)));
   }
   else
   {
      m_tableColumns = nullptr;
   }
   m_busy = false;
   m_disabled = false;
   m_tLastCheck = 0;
   m_scheduleType = item->m_scheduleType;
 }

/**
 * Data collection item destructor
 */
DataCollectionItem::~DataCollectionItem()
{
   MemFree(m_name);
   delete m_tableColumns;
}

/**
 * Will check if object has changed. If at least one field is changed - all data will be updated and
 * saved to database.
 */
bool DataCollectionItem::updateAndSave(const shared_ptr<DataCollectionItem>& item, bool txnOpen, DB_HANDLE hdb, DataCollectionStatementSet *statements)
{
   // if at least one of fields changed - set all fields and save to DB
   if ((m_type != item->m_type) || (m_origin != item->m_origin) || _tcscmp(m_name, item->m_name) ||
       (m_pollingInterval != item->m_pollingInterval) || m_snmpTargetGuid.compare(item->m_snmpTargetGuid) ||
       (m_snmpPort != item->m_snmpPort) || (m_snmpRawValueType != item->m_snmpRawValueType) ||
       (m_lastPollTime < item->m_lastPollTime) || (m_backupProxyId != item->m_backupProxyId) ||
       (item->m_tableColumns != nullptr) || (m_tableColumns != nullptr) ||
       (item->m_scheduleType != m_scheduleType) || (item->m_schedules.size() > 0) ||
       (m_schedules.size() > 0))   // Do not do actual compare for table columns
   {
      m_type = item->m_type;
      m_origin = item->m_origin;
      MemFree(m_name);
      m_name = MemCopyString(item->m_name);
      m_pollingInterval = item->m_pollingInterval;
      if (m_lastPollTime < item->m_lastPollTime)
         m_lastPollTime = item->m_lastPollTime;
      m_snmpTargetGuid = item->m_snmpTargetGuid;
      m_snmpPort = item->m_snmpPort;
      m_snmpRawValueType = item->m_snmpRawValueType;
      m_backupProxyId = item->m_backupProxyId;

      delete m_tableColumns;
      if (item->m_tableColumns != nullptr)
      {
         m_tableColumns = new ObjectArray<SNMPTableColumnDefinition>(item->m_tableColumns->size(), 16, Ownership::True);
         for(int i = 0; i < item->m_tableColumns->size(); i++)
            m_tableColumns->add(new SNMPTableColumnDefinition(item->m_tableColumns->get(i)));
      }
      else
      {
         m_tableColumns = nullptr;
      }
      m_scheduleType = item->m_scheduleType;
      m_schedules.clear();
      m_schedules.addAll(&item->m_schedules);

      if (!txnOpen)
      {
         DBBegin(hdb);
         txnOpen = true;
      }
      saveToDatabase(false, hdb, statements);
   }
   return txnOpen;
}

/**
 * Save configuration object to database
 */
void DataCollectionItem::saveToDatabase(bool newObject, DB_HANDLE hdb, DataCollectionStatementSet *statements)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("DataCollectionItem::saveToDatabase: %s object(serverId=") UINT64X_FMT(_T("016")) _T(",dciId=%d) saved to database"),
               newObject ? _T("new") : _T("existing"), m_serverId, m_id);

   DB_STATEMENT hStmt;
   if (newObject)
   {
      if (statements->insertItem == nullptr)
      {
         statements->insertItem = DBPrepare(hdb,
                  _T("INSERT INTO dc_config (type,origin,name,polling_interval,")
                  _T("last_poll,snmp_port,snmp_target_guid,snmp_raw_type,backup_proxy_id,")
                  _T("snmp_version,schedule_type,server_id,dci_id)")
                  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)"));
      }
      hStmt = statements->insertItem;
   }
   else
   {
      if (statements->updateItem == nullptr)
      {
         statements->updateItem = DBPrepare(hdb,
                    _T("UPDATE dc_config SET type=?,origin=?,name=?,")
                    _T("polling_interval=?,last_poll=?,snmp_port=?,")
                    _T("snmp_target_guid=?,snmp_raw_type=?,backup_proxy_id=?,")
                    _T("snmp_version=?,schedule_type=? WHERE server_id=? AND dci_id=?"));
      }
      hStmt = statements->updateItem;
   }

	if (hStmt == nullptr)
		return;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (int32_t)m_type);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (int32_t)m_origin);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_pollingInterval);
	DBBind(hStmt, 5, DB_SQLTYPE_BIGINT, m_lastPollTime);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (int32_t)m_snmpPort);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_snmpTargetGuid);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (int32_t)m_snmpRawValueType);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_backupProxyId);
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (int32_t)m_snmpVersion);
   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (int32_t)m_scheduleType);
	DBBind(hStmt, 12, DB_SQLTYPE_BIGINT, m_serverId);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_id);

   if (!DBExecute(hStmt))
      return;

   if ((m_origin == DS_SNMP_AGENT) && (m_type == DCO_TYPE_TABLE))
   {
      if (statements->deleteColumns == nullptr)
      {
         statements->deleteColumns = DBPrepare(hdb, _T("DELETE FROM dc_snmp_table_columns WHERE server_id=? AND dci_id=?"), true);
         if (statements->deleteColumns == nullptr)
            return;
      }
      DBBind(statements->deleteColumns, 1, DB_SQLTYPE_BIGINT, m_serverId);
      DBBind(statements->deleteColumns, 2, DB_SQLTYPE_INTEGER, m_id);
      if (!DBExecute(statements->deleteColumns))
         return;

      if (m_tableColumns != nullptr)
      {
         if (statements->insertColumn == nullptr)
         {
            statements->insertColumn = DBPrepare(hdb, _T("INSERT INTO dc_snmp_table_columns (server_id,dci_id,column_id,name,display_name,flags,snmp_oid) VALUES (?,?,?,?,?,?,?)"), true);
            if (statements->insertColumn == nullptr)
               return;
         }
         DBBind(statements->insertColumn, 1, DB_SQLTYPE_BIGINT, m_serverId);
         DBBind(statements->insertColumn, 2, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; i < m_tableColumns->size(); i++)
         {
            SNMPTableColumnDefinition *c = m_tableColumns->get(i);
            DBBind(statements->insertColumn, 3, DB_SQLTYPE_INTEGER, i);
            DBBind(statements->insertColumn, 4, DB_SQLTYPE_VARCHAR, c->getName(), DB_BIND_STATIC);
            DBBind(statements->insertColumn, 5, DB_SQLTYPE_VARCHAR, c->getDisplayName(), DB_BIND_STATIC);
            DBBind(statements->insertColumn, 6, DB_SQLTYPE_INTEGER, c->getFlags());
            const SNMP_ObjectId& oid = c->getSnmpOid();
            TCHAR text[1024];
            DBBind(statements->insertColumn, 7, DB_SQLTYPE_VARCHAR, oid.isValid() ? oid.toString(text, 1024) : nullptr, DB_BIND_STATIC);
            DBExecute(statements->insertColumn);
         }
      }
   }

   if (m_scheduleType != ScheduleType::NONE)
   {
      if (statements->deleteSchedules == nullptr)
      {
         statements->deleteSchedules = DBPrepare(hdb, _T("DELETE FROM dc_schedules WHERE server_id=? AND dci_id=?"), true);
         if (statements->deleteSchedules == nullptr)
            return;
      }
      DBBind(statements->deleteSchedules, 1, DB_SQLTYPE_BIGINT, m_serverId);
      DBBind(statements->deleteSchedules, 2, DB_SQLTYPE_INTEGER, m_id);
      if (!DBExecute(statements->deleteSchedules))
         return;

      if (statements->insertSchedules == nullptr)
      {
         statements->insertSchedules = DBPrepare(hdb, _T("INSERT INTO dc_schedules (server_id,dci_id,schedule) VALUES (?,?,?)"), true);
         if (statements->insertSchedules == nullptr)
            return;
      }

      DBBind(statements->insertSchedules, 1, DB_SQLTYPE_BIGINT, m_serverId);
      DBBind(statements->insertSchedules, 2, DB_SQLTYPE_INTEGER, m_id);
      for(int i = 0; i < m_schedules.size(); i++)
      {
         DBBind(statements->insertSchedules, 3, DB_SQLTYPE_VARCHAR, m_schedules.get(i), DB_BIND_STATIC);
         DBExecute(statements->insertSchedules);
      }
   }
}

/**
 * Remove item form database
 */
void DataCollectionItem::deleteFromDatabase(DB_HANDLE hdb, DataCollectionStatementSet *statements)
{
   if (statements->deleteItem == nullptr)
   {
      statements->deleteItem = DBPrepare(hdb, _T("DELETE FROM dc_config WHERE server_id=? AND dci_id=?"), true);
      if (statements->deleteItem == nullptr)
         return;
   }
   DBBind(statements->deleteItem, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(statements->deleteItem, 2, DB_SQLTYPE_INTEGER, m_id);

   if (statements->deleteColumns == nullptr)
   {
      statements->deleteColumns = DBPrepare(hdb, _T("DELETE FROM dc_snmp_table_columns WHERE server_id=? AND dci_id=?"), true);
      if (statements->deleteColumns == nullptr)
         return;
   }
   DBBind(statements->deleteColumns, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(statements->deleteColumns, 2, DB_SQLTYPE_INTEGER, m_id);

   if (statements->deleteSchedules == nullptr)
   {
      statements->deleteSchedules = DBPrepare(hdb, _T("DELETE FROM dc_schedules WHERE server_id=? AND dci_id=?"), true);
      if (statements->deleteSchedules == nullptr)
         return;
   }
   DBBind(statements->deleteSchedules, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(statements->deleteSchedules, 2, DB_SQLTYPE_INTEGER, m_id);

   if (DBExecute(statements->deleteItem) && DBExecute(statements->deleteColumns) && DBExecute(statements->deleteSchedules))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("DataCollectionItem::deleteFromDatabase: object(serverId=") UINT64X_FMT(_T("016")) _T(",dciId=%d) removed from database"), m_serverId, m_id);
   }
}

/**
 * Collected data
 */
class DataElement
{
private:
   uint64_t m_serverId;
   uint32_t m_dciId;
   int64_t m_timestamp;
   int m_origin;
   int m_type;
   uint32_t m_statusCode;
   uuid m_snmpNode;
   union
   {
      TCHAR *item;
      Table *table;
   } m_value;

public:
   DataElement(const DataCollectionItem& dci, const TCHAR *value, uint32_t status)
   {
      m_serverId = dci.getServerId();
      m_dciId = dci.getId();
      m_timestamp = GetCurrentTimeMs();
      m_origin = dci.getOrigin();
      m_type = DCO_TYPE_ITEM;
      m_statusCode = status;
      m_snmpNode = dci.getSnmpTargetGuid();
      m_value.item = MemCopyString(value);
   }

   DataElement(const DataCollectionItem& dci, Table *value, uint32_t status)
   {
      m_serverId = dci.getServerId();
      m_dciId = dci.getId();
      m_timestamp = GetCurrentTimeMs();
      m_origin = dci.getOrigin();
      m_type = DCO_TYPE_TABLE;
      m_statusCode = status;
      m_snmpNode = dci.getSnmpTargetGuid();
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
      m_timestamp = DBGetFieldInt64(hResult, row, 6);
      m_origin = DBGetFieldLong(hResult, row, 3);
      m_type = DBGetFieldLong(hResult, row, 2);
      m_statusCode = DBGetFieldLong(hResult, row, 4);
      m_snmpNode = DBGetFieldGUID(hResult, row, 5);
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            m_value.item = DBGetField(hResult, row, 7, nullptr, 0);
            break;
         case DCO_TYPE_TABLE:
            {
               char *xml = DBGetFieldUTF8(hResult, row, 7, nullptr, 0);
               if (xml != nullptr)
               {
                  m_value.table = Table::createFromXML(xml);
                  MemFree(xml);
               }
               else
               {
                  m_value.table = nullptr;
               }
            }
            break;
         default:
            m_type = DCO_TYPE_ITEM;
            m_value.item = MemCopyString(_T(""));
            break;
      }
   }

   ~DataElement()
   {
      switch(m_type)
      {
         case DCO_TYPE_ITEM:
            MemFree(m_value.item);
            break;
         case DCO_TYPE_TABLE:
            delete m_value.table;
            break;
      }
   }

   int64_t getTimestamp() const { return m_timestamp; }
   uint64_t getServerId() const { return m_serverId; }
   uint32_t getDciId() const { return m_dciId; }
   int getType() const { return m_type; }
   uint32_t getStatusCode() const { return m_statusCode; }

   void saveToDatabase(DB_STATEMENT hStmt) const;
   bool sendToServer(bool reconcillation) const;
   void fillReconciliationMessage(NXCPMessage *msg, uint32_t baseId) const;
};

/**
 * Save data element to database
 */
void DataElement::saveToDatabase(DB_STATEMENT hStmt) const
{
   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dciId);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)m_type);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_origin);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_statusCode);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_snmpNode);
   DBBind(hStmt, 7, DB_SQLTYPE_BIGINT, m_timestamp);
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_value.item, DB_BIND_STATIC);
         break;
      case DCO_TYPE_TABLE:
         DBBind(hStmt, 8, DB_SQLTYPE_TEXT, m_value.table->toXML(), DB_BIND_DYNAMIC);
         break;
   }
   DBExecute(hStmt);
}

/**
 * Session comparator
 */
static bool SessionComparator_Sender(AbstractCommSession *session, void *data)
{
   return (session->getServerId() == *static_cast<UINT64*>(data)) && session->canAcceptData();
}

/**
 * Send collected data to server
 */
bool DataElement::sendToServer(bool reconciliation) const
{
   // If data in database was invalid table may not be parsed correctly
   // Consider sending a success in that case so data element can be dropped
   if ((m_type == DCO_TYPE_TABLE) && (m_value.table == nullptr))
      return true;

   shared_ptr<CommSession> session = static_pointer_cast<CommSession>(FindServerSession(SessionComparator_Sender, (void*)&m_serverId));
   if (session == nullptr)
      return false;

   NXCPMessage msg(CMD_DCI_DATA, session->generateRequestId(), session->getProtocolVersion());
   msg.setField(VID_DCI_ID, m_dciId);
   msg.setField(VID_DCI_SOURCE_TYPE, (INT16)m_origin);
   msg.setField(VID_DCOBJECT_TYPE, (INT16)m_type);
   msg.setField(VID_STATUS, m_statusCode);
   msg.setField(VID_NODE_ID, m_snmpNode);
   msg.setField(VID_TIMESTAMP_MS, m_timestamp);
   msg.setField(VID_TIMESTAMP, m_timestamp / 1000); // for backward compatibility
   msg.setField(VID_RECONCILIATION, (INT16)(reconciliation ? 1 : 0));
   switch(m_type)
   {
      case DCO_TYPE_ITEM:
         msg.setField(VID_VALUE, m_value.item);
         break;
      case DCO_TYPE_TABLE:
         m_value.table->setSource(m_origin);
         m_value.table->fillMessage(&msg, 0, -1);
         break;
   }
   uint32_t rcc = session->doRequest(&msg, 2000);

   // consider internal error as success because it means that server
   // cannot accept data for some reason and retry is not feasible
   return (rcc == ERR_SUCCESS) || (rcc == ERR_INTERNAL_ERROR);
}

/**
 * Fill bulk reconciliation message with DCI data
 */
void DataElement::fillReconciliationMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_dciId);
   msg->setField(baseId + 1, (INT16)m_origin);
   msg->setField(baseId + 2, (INT16)m_type);
   msg->setField(baseId + 3, m_snmpNode);
   msg->setField(baseId + 4, m_timestamp / 1000); // for backward compatibility
   msg->setField(baseId + 5, m_value.item);
   msg->setField(baseId + 6, m_statusCode);
   msg->setField(baseId + 7, m_timestamp);
}

/**
 * Server data sync status object
 */
struct ServerSyncStatus
{
   uint64_t serverId;
   int32_t queueSize;
   int64_t lastSync;

   ServerSyncStatus(uint64_t sid)
   {
      serverId = sid;
      queueSize = 0;
      lastSync = GetCurrentTimeMs();
   }
};

/**
 * Server sync status information
 */
static HashMap<uint64_t, ServerSyncStatus> s_serverSyncStatus(Ownership::True);
static Mutex s_serverSyncStatusLock;

/**
 * Database writer queue
 */
static ObjectQueue<DataElement> s_databaseWriterQueue;

/**
 * Database writer
 */
static void DatabaseWriter()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Database writer thread started"));

   while(true)
   {
      DataElement *e = s_databaseWriterQueue.getOrBlock();
      if (e == INVALID_POINTER_VALUE)
         break;

      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dc_queue (server_id,dci_id,dci_type,dci_origin,status_code,snmp_target_guid,timestamp,value) VALUES (?,?,?,?,?,?,?,?)"));
      if (hStmt == nullptr)
      {
         delete e;
         continue;
      }

      uint32_t count = 0;
      DBBegin(hdb);
      while((e != nullptr) && (e != INVALID_POINTER_VALUE))
      {
         e->saveToDatabase(hStmt);
         delete e;

         count++;
         if (count == g_dcWriterMaxTransactionSize)
            break;

         e = s_databaseWriterQueue.get();
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Database writer: %u records inserted"), count);
      if (e == INVALID_POINTER_VALUE)
         break;

      // Sleep only if queue was emptied
      if (count < g_dcWriterMaxTransactionSize)
         ThreadSleepMs(g_dcWriterFlushInterval);
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Database writer thread stopped"));
}

/**
 * List of all data collection items
 */
static SharedHashMap<ServerObjectKey, DataCollectionItem> s_items;
static Mutex s_itemLock;

/**
 * Session comparator
 */
static bool SessionComparator_Reconciliation(AbstractCommSession *session, void *data)
{
   if ((session->getServerId() == 0) || !session->canAcceptData())
      return false;
   ServerSyncStatus *s = s_serverSyncStatus.get(session->getServerId());
   if ((s != nullptr) && (s->queueSize > 0))
   {
      return true;
   }
   return false;
}

/**
 * Calculate next delay value
 */
static inline uint32_t NextDelayValue(uint32_t curr)
{
   if (curr >= 60000)
      return curr;
   if (curr == 0)
      return 100;
   return curr + rand() % (curr / 2) + (curr / 2);
}

/**
 * Data reconciliation thread
 */
static void ReconciliationThread()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   uint32_t sleepTime = 30000;
   uint32_t sendDelay = 0;
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data reconciliation thread started (block size %d, timeout %d ms)"), g_dcReconciliationBlockSize, g_dcReconciliationTimeout);

   bool vacuumNeeded = false;
   while(!AgentSleepAndCheckForShutdown(sleepTime + sendDelay))
   {
      // Check if there is something to sync
      s_serverSyncStatusLock.lock();
      shared_ptr<CommSession> session = static_pointer_cast<CommSession>(FindServerSession(SessionComparator_Reconciliation, nullptr));
      s_serverSyncStatusLock.unlock();
      if (session == nullptr)
      {
         // Save last poll times when reconciliation thread is idle
         s_itemLock.lock();
         if (s_pollTimeChanged)
         {
            DBBegin(hdb);
            TCHAR query[256];
            for(shared_ptr<DataCollectionItem> dci : s_items)
            {
               _sntprintf(query, 256, _T("UPDATE dc_config SET last_poll=") INT64_FMT _T(" WHERE server_id=") UINT64_FMT _T(" AND dci_id=%d"),
                          dci->getLastPollTime().asMilliseconds(), dci->getServerId(), dci->getId());
               DBQuery(hdb, query);
            }
            DBCommit(hdb);
            s_pollTimeChanged = false;
         }
         s_itemLock.unlock();

         if (vacuumNeeded)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: vacuum local database"));
            DBQuery(hdb, _T("VACUUM"));
            vacuumNeeded = false;
         }
         sleepTime = 30000;
         sendDelay = 0;
         continue;
      }

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT server_id,dci_id,dci_type,dci_origin,status_code,snmp_target_guid,timestamp,value FROM dc_queue INDEXED BY idx_dc_queue_timestamp WHERE server_id=") UINT64_FMT _T(" ORDER BY timestamp LIMIT %d"), session->getServerId(), g_dcReconciliationBlockSize);

      TCHAR sqlError[DBDRV_MAX_ERROR_TEXT];
      DB_RESULT hResult = DBSelectEx(hdb, query, sqlError);
      if (hResult == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: database query failed: %s"), sqlError);
         sleepTime = 30000;
         sendDelay = 0;
         continue;
      }

      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         ObjectArray<DataElement> bulkSendList(count, 10, Ownership::True);
         ObjectArray<DataElement> deleteList(count, 10, Ownership::True);
         for(int i = 0; i < count; i++)
         {
            DataElement *e = new DataElement(hResult, i);
            if ((e->getType() == DCO_TYPE_ITEM) && session->isBulkReconciliationSupported())
            {
               bulkSendList.add(e);
            }
            else
            {
               s_serverSyncStatusLock.lock();
               ServerSyncStatus *status = s_serverSyncStatus.get(e->getServerId());
               if (status != nullptr)
               {
                  if (e->sendToServer(true))
                  {
                     status->queueSize--;
                     status->lastSync = GetCurrentTimeMs();
                     deleteList.add(e);
                  }
                  else
                  {
                     delete e;
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("INTERNAL ERROR: cached DCI value without server sync status object"));
                  deleteList.add(e);  // record should be deleted
               }
               s_serverSyncStatusLock.unlock();
            }
         }

         if (bulkSendList.size() > 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("ReconciliationThread: %d records to be sent in bulk mode"), bulkSendList.size());

            NXCPMessage msg(CMD_DCI_DATA, session->generateRequestId(), session->getProtocolVersion());
            msg.setField(VID_BULK_RECONCILIATION, true);
            msg.setField(VID_NUM_ELEMENTS, static_cast<int16_t>(bulkSendList.size()));
            msg.setField(VID_TIMEOUT, g_dcReconciliationTimeout);

            uint32_t fieldId = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < bulkSendList.size(); i++)
            {
               bulkSendList.get(i)->fillReconciliationMessage(&msg, fieldId);
               fieldId += 10;
            }

            if (session->sendMessage(&msg))
            {
               uint32_t rcc;
               do
               {
                  NXCPMessage *response = session->waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), g_dcReconciliationTimeout);
                  if (response != nullptr)
                  {
                     rcc = response->getFieldAsUInt32(VID_RCC);
                     if (rcc == ERR_SUCCESS)
                     {
                        s_serverSyncStatusLock.lock();
                        ServerSyncStatus *serverSyncStatus = s_serverSyncStatus.get(session->getServerId());

                        // Check status for each data element
                        BYTE status[MAX_BULK_DATA_BLOCK_SIZE];
                        memset(status, 0, MAX_BULK_DATA_BLOCK_SIZE);
                        response->getFieldAsBinary(VID_STATUS, status, MAX_BULK_DATA_BLOCK_SIZE);
                        bulkSendList.setOwner(Ownership::False);
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
                        serverSyncStatus->lastSync = GetCurrentTimeMs();

                        s_serverSyncStatusLock.unlock();
                     }
                     else if (rcc == ERR_PROCESSING)
                     {
                        nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: server is processing data (%d%% completed)"), response->getFieldAsInt32(VID_PROGRESS));
                     }
                     else if (rcc == ERR_RESOURCE_BUSY)
                     {
                        nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: server is busy"));
                     }
                     else
                     {
                        nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: bulk send failed (%u)"), rcc);
                     }
                     delete response;
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: timeout on bulk send"));
                     rcc = ERR_REQUEST_TIMEOUT;
                  }
               } while(rcc == ERR_PROCESSING);
               sendDelay = (rcc == ERR_SUCCESS) ? 0 : NextDelayValue(sendDelay);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: communication error"));
               sendDelay = NextDelayValue(sendDelay);
            }
         }

         if (deleteList.size() > 0)
         {
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM dc_queue WHERE server_id=? AND dci_id=? AND timestamp=?"), true);
            if (hStmt != nullptr)
            {
               DBBegin(hdb);
               for(int i = 0; i < deleteList.size(); i++)
               {
                  DataElement *e = deleteList.get(i);
                  DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, e->getServerId());
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e->getDciId());
                  DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, e->getTimestamp());
                  DBExecute(hStmt);
               }
               DBCommit(hdb);
               DBFreeStatement(hStmt);
               vacuumNeeded = true;
            }
            nxlog_debug_tag(DEBUG_TAG, 4, _T("ReconciliationThread: %d records sent"), deleteList.size());
         }
      }
      DBFreeResult(hResult);

      sleepTime = (count > 0) ? 50 : 30000;
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data reconciliation thread stopped"));
}

/**
 * Data sender queue
 */
static Queue s_dataSenderQueue;

/**
 * Data sender
 */
static void DataSender()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data sender thread started"));
   while(true)
   {
      DataElement *e = static_cast<DataElement*>(s_dataSenderQueue.getOrBlock());
      if (e == INVALID_POINTER_VALUE)
         break;

      s_serverSyncStatusLock.lock();
      ServerSyncStatus *status = s_serverSyncStatus.get(e->getServerId());
      if (status == nullptr)
      {
         status = new ServerSyncStatus(e->getServerId());
         s_serverSyncStatus.set(e->getServerId(), status);
      }

      if (status->queueSize == 0)
      {
         if (!e->sendToServer(false))
         {
            status->queueSize++;
            s_databaseWriterQueue.put(e);
            e = nullptr;
         }
      }
      else
      {
         status->queueSize++;
         s_databaseWriterQueue.put(e);
         e = nullptr;
      }
      s_serverSyncStatusLock.unlock();

      delete e;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data sender thread stopped"));
}

/**
 * Collect data from agent
 */
static DataElement *CollectDataFromAgent(const DataCollectionItem& dci)
{
   VirtualSession session(dci.getServerId());

   DataElement *e = nullptr;
   uint32_t status;
   if (dci.getType() == DCO_TYPE_ITEM)
   {
      TCHAR value[MAX_RESULT_LENGTH];
      status = GetMetricValue(dci.getName(), value, &session);
      e = new DataElement(dci, (status == ERR_SUCCESS) ? value : _T(""), status);
   }
   else if (dci.getType() == DCO_TYPE_TABLE)
   {
      Table *value = new Table();
      status = GetTableValue(dci.getName(), value, &session);
      e = new DataElement(dci, value, status);
   }

   return e;
}

/**
 * Collect data from SNMP
 */
static DataElement *CollectDataFromSNMP(const DataCollectionItem& dci)
{
   DataElement *e = nullptr;
   if (dci.getType() == DCO_TYPE_ITEM)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("Read SNMP parameter %s"), dci.getName());

      TCHAR value[MAX_RESULT_LENGTH];
      uint32_t status = GetSnmpValue(dci.getSnmpTargetGuid(), dci.getSnmpPort(), dci.getSnmpVersion(), dci.getName(), value, dci.getSnmpRawValueType());
      e = new DataElement(dci, status == ERR_SUCCESS ? value : _T(""), status);
   }
   else if (dci.getType() == DCO_TYPE_TABLE)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("Read SNMP table %s"), dci.getName());
      const ObjectArray<SNMPTableColumnDefinition> *columns = dci.getColumns();
      if (columns != nullptr)
      {
         Table *value = new Table();
         uint32_t status = GetSnmpTable(dci.getSnmpTargetGuid(), dci.getSnmpPort(), dci.getSnmpVersion(), dci.getName(), *columns, value);
         e = new DataElement(dci, value, status);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("Missing column definition for SNMP table %s"), dci.getName());
      }
   }
   return e;
}

/**
 * Local data collection callback
 */
static void LocalDataCollectionCallback(const shared_ptr<DataCollectionItem>& dci)
{
   if (!dci->isDisabled())
   {
      DataElement *e = CollectDataFromAgent(*dci);
      if (e != nullptr)
      {
         s_dataSenderQueue.put(e);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
      }
   }
   dci->setLastPollTime(Timestamp::now());
   dci->finishDataCollection();
}

/**
 * SNMP data collection callback
 */
static void SnmpDataCollectionCallback(const shared_ptr<DataCollectionItem>& dci)
{
   if (!dci->isDisabled())
   {
      DataElement *e = CollectDataFromSNMP(*dci);
      if (e != nullptr)
      {
         s_dataSenderQueue.put(e);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("DataCollector: collection error for DCI %d \"%s\""), dci->getId(), dci->getName());
      }
   }
   dci->setLastPollTime(Timestamp::now());
   dci->finishDataCollection();
}

/**
 * Data collectors thread pool
 */
ThreadPool *g_dataCollectorPool = nullptr;

/**
 * Single data collection scheduler run - schedule data collection if needed and calculate sleep time
 */
static uint32_t DataCollectionSchedulerRun()
{
   uint32_t sleepTime = 60;

   s_itemLock.lock();
   time_t now = time(nullptr);
   auto it = s_items.begin();
   while(it.hasNext())
   {
      const shared_ptr<DataCollectionItem>& dci = it.next();
      uint32_t timeToPoll = dci->getTimeToNextPoll(now);
      if (timeToPoll == 0)
      {
         bool schedule;
         if (dci->getBackupProxyId() == 0)
         {
            schedule = true;
         }
         else
         {
            LockGuard lockGuard(g_proxyListLock);
            DataCollectionProxy *proxy = g_proxyList.get(ServerObjectKey(dci->getServerId(), dci->getBackupProxyId()));
            schedule = ((proxy != nullptr) && !proxy->isConnected());
         }

         if (schedule)
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("DataCollector: polling DCI %d \"%s\""), dci->getId(), dci->getName());

            if ((dci->getOrigin() == DS_NATIVE_AGENT) || (dci->getOrigin() == DS_MODBUS))
            {
               dci->startDataCollection();
               ThreadPoolExecute(g_dataCollectorPool, LocalDataCollectionCallback, dci);
            }
            else if (dci->getOrigin() == DS_SNMP_AGENT)
            {
               dci->startDataCollection();
               TCHAR key[64];
               ThreadPoolExecuteSerialized(g_dataCollectorPool, dci->getSnmpTargetGuid().toString(key), SnmpDataCollectionCallback, dci);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 7, _T("DataCollector: unsupported origin %d"), dci->getOrigin());
               dci->setLastPollTime(Timestamp::now());
            }
         }

         timeToPoll = dci->getPollingInterval();
      }
      if (sleepTime > timeToPoll)
         sleepTime = timeToPoll;
   }
   s_itemLock.unlock();
   return sleepTime;
}

/**
 * Data collection scheduler thread
 */
static void DataCollectionScheduler()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data collection scheduler thread started"));

   uint32_t sleepTime = DataCollectionSchedulerRun();
   while(!AgentSleepAndCheckForShutdown(sleepTime * 1000))
   {
      sleepTime = DataCollectionSchedulerRun();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("DataCollector: sleeping for %d seconds"), sleepTime);
   }

   ThreadPoolDestroy(g_dataCollectorPool);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Data collection scheduler thread stopped"));
}

/**
 * Configure data collection
 */
void ConfigureDataCollection(uint64_t serverId, const NXCPMessage& request)
{
   s_itemLock.lock();
   if (!s_dataCollectorStarted)
   {
      s_itemLock.unlock();
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Local data collector was not started, ignoring configuration received from server ") UINT64X_FMT(_T("016")), serverId);
      return;
   }
   s_itemLock.unlock();

   DB_HANDLE hdb = GetLocalDatabaseHandle();

   int count = request.getFieldAsInt32(VID_NUM_NODES);
   if (count > 0)
   {
      DBBegin(hdb);
      uint32_t fieldId = VID_NODE_INFO_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         shared_ptr<SNMPTarget> target = make_shared<SNMPTarget>(serverId, request, fieldId);
         UpdateSnmpTarget(target);
         target->saveToDatabase(hdb);
         fieldId += 50;
      }
      DBCommit(hdb);
   }
   nxlog_debug_tag(DEBUG_TAG, 4, _T("%d SNMP targets received from server ") UINT64X_FMT(_T("016")), count, serverId);

   // Read proxy list
   HashMap<ServerObjectKey, DataCollectionProxy> *proxyList = new HashMap<ServerObjectKey, DataCollectionProxy>(Ownership::True);
   count = request.getFieldAsInt32(VID_ZONE_PROXY_COUNT);
   if (count > 0)
   {
      uint32_t fieldId = VID_ZONE_PROXY_BASE;
      for(int i = 0; i < count; i++)
      {
         uint32_t proxyId = request.getFieldAsInt32(fieldId);
         proxyList->set(ServerObjectKey(serverId, proxyId),
               new DataCollectionProxy(serverId, proxyId, request.getFieldAsInetAddress(fieldId + 1)));
         fieldId += 10;
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 4, _T("%d proxies received from server ") UINT64X_FMT(_T("016")), count, serverId);

   // Read data collection items
   SharedHashMap<ServerObjectKey, DataCollectionItem> config;
   bool hasExtraData = request.getFieldAsBoolean(VID_EXTENDED_DCI_DATA);
   count = request.getFieldAsInt32(VID_NUM_ELEMENTS);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   uint32_t extFieldId = VID_EXTRA_DCI_INFO_BASE;
   for(int i = 0; i < count; i++)
   {
      shared_ptr<DataCollectionItem> dci = make_shared<DataCollectionItem>(serverId, request, fieldId, extFieldId, hasExtraData);
      config.set(dci->getKey(), dci);
      fieldId += 10;
      extFieldId += 1100;
   }
   nxlog_debug_tag(DEBUG_TAG, 4, _T("%d data collection elements received from server ") UINT64X_FMT(_T("016")) _T(" (extended data: %s)"),
            count, serverId, hasExtraData ? _T("YES") : _T("NO"));

   bool txnOpen = false;
   DataCollectionStatementSet statements;
   memset(&statements, 0, sizeof(statements));

   s_itemLock.lock();

   // Update and add new
   for(shared_ptr<DataCollectionItem> item : config)
   {
      shared_ptr<DataCollectionItem> existingItem = s_items.getShared(item->getKey());
      if (existingItem != nullptr)
      {
         txnOpen = existingItem->updateAndSave(item, txnOpen, hdb, &statements);
      }
      else
      {
         s_items.set(item->getKey(), item);
         if (!txnOpen)
         {
            DBBegin(hdb);
            txnOpen = true;
         }
         item->saveToDatabase(true, hdb, &statements);
      }

      if (item->getBackupProxyId() != 0)
      {
         DataCollectionProxy *proxy = proxyList->get(ServerObjectKey(serverId, item->getBackupProxyId()));
         if (proxy != nullptr)
         {
            proxy->setInUse(true);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("No proxy found for ServerID=") UINT64X_FMT(_T("016")) _T(", ProxyID=%u, ItemID=%u"),
                  serverId, item->getBackupProxyId(), item->getId());
         }
      }
   }

   // Remove non-existing configuration
   auto it = s_items.begin();
   while(it.hasNext())
   {
      shared_ptr<DataCollectionItem> item = it.next();
      if (item->getServerId() != serverId)
         continue;

      if (!config.contains(item->getKey()))
      {
         if (!txnOpen)
         {
            DBBegin(hdb);
            txnOpen = true;
         }
         item->deleteFromDatabase(hdb, &statements);
         it.remove();
      }
   }

   if (txnOpen)
      DBCommit(hdb);

   if (statements.insertItem != nullptr)
      DBFreeStatement(statements.insertItem);
   if (statements.updateItem != nullptr)
      DBFreeStatement(statements.updateItem);
   if (statements.deleteItem != nullptr)
      DBFreeStatement(statements.deleteItem);
   if (statements.insertColumn != nullptr)
      DBFreeStatement(statements.insertColumn);
   if (statements.deleteColumns != nullptr)
      DBFreeStatement(statements.deleteColumns);
   if (statements.insertSchedules != nullptr)
      DBFreeStatement(statements.insertSchedules);
   if (statements.deleteSchedules != nullptr)
      DBFreeStatement(statements.deleteSchedules);

   s_itemLock.unlock();

   if (request.isFieldExist(VID_THIS_PROXY_ID))
   {
      // FIXME: delete configuration if not set?
      BYTE sharedSecret[ZONE_PROXY_KEY_LENGTH];
      request.getFieldAsBinary(VID_SHARED_SECRET, sharedSecret, ZONE_PROXY_KEY_LENGTH);
      ZoneConfiguration zone(serverId, request.getFieldAsUInt32(VID_THIS_PROXY_ID), request.getFieldAsUInt32(VID_ZONE_UIN), sharedSecret);
      UpdateProxyConfiguration(serverId, proxyList, zone);
   }
   delete proxyList;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Data collection for server ") UINT64X_FMT(_T("016")) _T(" reconfigured"), serverId);
}

/**
 * Load saved state of local data collection
 */
static void LoadState()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT server_id,dci_id,type,origin,name,polling_interval,last_poll,snmp_port,snmp_target_guid,snmp_raw_type,backup_proxy_id,snmp_version,schedule_type FROM dc_config"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         shared_ptr<DataCollectionItem> dci = make_shared<DataCollectionItem>(hResult, i);
         s_items.set(dci->getKey(), dci);
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT guid,server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass FROM dc_snmp_targets"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UpdateSnmpTarget(make_shared<SNMPTarget>(hResult, i));
      }
      DBFreeResult(hResult);
   }

   hResult = DBSelect(hdb, _T("SELECT server_id,count(*),coalesce(min(timestamp),0) FROM dc_queue GROUP BY server_id"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint64_t serverId = DBGetFieldUInt64(hResult, i, 0);
         ServerSyncStatus *s = new ServerSyncStatus(serverId);
         s->queueSize = DBGetFieldLong(hResult, i, 1);
         s->lastSync = DBGetFieldInt64(hResult, i, 2);
         s_serverSyncStatus.set(serverId, s);
         nxlog_debug_tag(DEBUG_TAG, 2, _T("%d elements in queue for server ID ") UINT64X_FMT(_T("016")), s->queueSize, serverId);

         TCHAR ts[64];
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Oldest timestamp is %s for server ID ") UINT64X_FMT(_T("016")), FormatTimestampMs(s->lastSync, ts), serverId);
      }
      DBFreeResult(hResult);
   }

   LoadProxyConfiguration();
}

/**
 * Clear stalled offline data
 */
static void ClearStalledOfflineData()
{
   if (g_dwFlags & AF_SHUTDOWN)
      return;

   IntegerArray<uint64_t> deleteList;

   s_serverSyncStatusLock.lock();
   int64_t expirationTime = GetCurrentTimeMs() - g_dcOfflineExpirationTime * _LL(86400000);
   Iterator<ServerSyncStatus> it = s_serverSyncStatus.begin();
   while(it.hasNext())
   {
      ServerSyncStatus *status = it.next();
      if ((status->queueSize > 0) && (status->lastSync < expirationTime))
      {
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Offline data for server ID ") UINT64X_FMT(_T("016")) _T(" expired"), status->serverId);
         deleteList.add(status->serverId);
         it.remove();
      }
   }
   s_serverSyncStatusLock.unlock();

   if (deleteList.size() > 0)
   {
      TCHAR query[256];
      DB_HANDLE hdb = GetLocalDatabaseHandle();

      for (int i = 0; i < deleteList.size(); i++)
      {
         uint64_t serverId = deleteList.get(i);

         s_itemLock.lock();
         auto it = s_items.begin();
         while(it.hasNext())
         {
            shared_ptr<DataCollectionItem> item = it.next();
            if (item->getServerId() == serverId)
            {
               item->setDisabled();    // In case it is currently scheduled for collection
               it.remove();
            }
         }
         s_itemLock.unlock();

         DBBegin(hdb);

         _sntprintf(query, 256, _T("DELETE FROM dc_queue WHERE server_id=") UINT64_FMT, serverId);
         DBQuery(hdb, query);

         _sntprintf(query, 256, _T("DELETE FROM dc_snmp_targets WHERE server_id=") UINT64_FMT, serverId);
         DBQuery(hdb, query);

         _sntprintf(query, 256, _T("DELETE FROM dc_config WHERE server_id=") UINT64_FMT, serverId);
         DBQuery(hdb, query);

         DBCommit(hdb);

         nxlog_debug_tag(DEBUG_TAG, 2, _T("Offline data collection configuration for server ID ") UINT64X_FMT(_T("016")) _T(" deleted"), serverId);
      }
   }

   ThreadPoolScheduleRelative(g_dataCollectorPool, STALLED_DATA_CHECK_INTERVAL, ClearStalledOfflineData);
}

/**
 * Data collector and sender thread handles
 */
static THREAD s_dataCollectionSchedulerThread = INVALID_THREAD_HANDLE;
static THREAD s_dataSenderThread = INVALID_THREAD_HANDLE;
static THREAD s_databaseWriterThread = INVALID_THREAD_HANDLE;
static THREAD s_reconciliationThread = INVALID_THREAD_HANDLE;
static THREAD s_proxyListennerThread = INVALID_THREAD_HANDLE;

/**
 * Initialize and start local data collector
 */
void StartLocalDataCollector()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("StartLocalDataCollector: local database unavailable"));
      return;
   }

   if (g_dcReconciliationBlockSize < 16)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Invalid data reconciliation block size %d, resetting to 16"), g_dcReconciliationBlockSize);
      g_dcReconciliationBlockSize = 16;
   }
   else if (g_dcReconciliationBlockSize > MAX_BULK_DATA_BLOCK_SIZE)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Invalid data reconciliation block size %d, resetting to %d"), g_dcReconciliationBlockSize, MAX_BULK_DATA_BLOCK_SIZE);
      g_dcReconciliationBlockSize = MAX_BULK_DATA_BLOCK_SIZE;
   }

   if (g_dcReconciliationTimeout < 1000)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Invalid data reconciliation timeout %d, resetting to 1000"), g_dcReconciliationTimeout);
      g_dcReconciliationTimeout = 1000;
   }
   else if (g_dcReconciliationTimeout > 900000)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Invalid data reconciliation timeout %d, resetting to 900000"), g_dcReconciliationTimeout);
      g_dcReconciliationTimeout = 900000;
   }

   LoadState();

   g_dataCollectorPool = ThreadPoolCreate(_T("DATACOLL"), g_dcMinCollectorPoolSize, g_dcMaxCollectorPoolSize);
   s_dataCollectionSchedulerThread = ThreadCreateEx(DataCollectionScheduler);
   s_dataSenderThread = ThreadCreateEx(DataSender);
   s_databaseWriterThread = ThreadCreateEx(DatabaseWriter);
   s_reconciliationThread = ThreadCreateEx(ReconciliationThread);
   if (g_dwFlags & AF_DISABLE_HEARTBEAT)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG_COMM, _T("Heartbeat listener is disabled"));
   }
   else
   {
      s_proxyListennerThread = ThreadCreateEx(ProxyListenerThread);
   }
   ThreadPoolScheduleRelative(g_dataCollectorPool, STALLED_DATA_CHECK_INTERVAL, ClearStalledOfflineData);
   ThreadPoolScheduleRelative(g_dataCollectorPool, 1000, ProxyConnectionChecker);

   s_dataCollectorStarted = true;
}

/**
 * Shutdown local data collector
 */
void ShutdownLocalDataCollector()
{
   if (!s_dataCollectorStarted)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Local data collector was not started"));
      return;
   }

   // Prevent configuration attempts by incoming sessions during shutdown
   s_itemLock.lock();
   s_dataCollectorStarted = false;
   s_itemLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for data collector thread termination"));
   ThreadJoin(s_dataCollectionSchedulerThread);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for data sender thread termination"));
   s_dataSenderQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_dataSenderThread);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for database writer thread termination"));
   s_databaseWriterQueue.put(INVALID_POINTER_VALUE);
   ThreadJoin(s_databaseWriterThread);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for data reconciliation thread termination"));
   ThreadJoin(s_reconciliationThread);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for proxy heartbeat listening thread"));
   ThreadJoin(s_proxyListennerThread);
}

/**
 * Clear data collection configuration
 */
void ClearDataCollectionConfiguration()
{
   DB_HANDLE db = GetLocalDatabaseHandle();
   if (db == nullptr)
      return;
   s_itemLock.lock();
   DBQuery(db, _T("DELETE FROM dc_queue"));
   DBQuery(db, _T("DELETE FROM dc_config"));
   DBQuery(db, _T("DELETE FROM dc_snmp_targets"));
   s_items.clear();
   s_itemLock.unlock();

   s_serverSyncStatusLock.lock();
   s_serverSyncStatus.clear();
   s_serverSyncStatusLock.unlock();
}

/**
 * Handler for data collector queue size
 */
LONG H_DataCollectorQueueSize(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (!s_dataCollectorStarted)
      return SYSINFO_RC_UNSUPPORTED;

   UINT32 count = 0;
   s_serverSyncStatusLock.lock();
   Iterator<ServerSyncStatus> it = s_serverSyncStatus.begin();
   while(it.hasNext())
      count += (UINT32)it.next()->queueSize;
   s_serverSyncStatusLock.unlock();

   ret_uint(value, count);
   return SYSINFO_RC_SUCCESS;
}

