/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: dctable.cpp
**
**/

#include "nxcore.h"

/**
 * Copy constructor
 */
DCTable::DCTable(const DCTable *src, bool shadowCopy) : DCObject(src, shadowCopy)
{
	m_columns = new ObjectArray<DCTableColumn>(src->m_columns->size(), 8, Ownership::True);
	for(int i = 0; i < src->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(src->m_columns->get(i)));
   m_thresholds = new ObjectArray<DCTableThreshold>(src->m_thresholds->size(), 4, Ownership::True);
	for(int i = 0; i < src->m_thresholds->size(); i++)
		m_thresholds->add(new DCTableThreshold(src->m_thresholds->get(i), shadowCopy));
	if (shadowCopy && (src->m_lastValue != nullptr))
	{
	   m_lastValue = make_shared<Table>(*src->m_lastValue);
	}
}

/**
 * Constructor for creating new DCTable from scratch
 */
DCTable::DCTable(uint32_t id, const TCHAR *name, int source, BYTE scheduleType, const TCHAR *pollingInterval,
      BYTE retentionType, const TCHAR *retentionTime, const shared_ptr<DataCollectionOwner>& owner,
      const TCHAR *description, const TCHAR *systemTag)
        : DCObject(id, name, source, scheduleType, pollingInterval, retentionType, retentionTime,
              owner, description, systemTag)
{
	m_columns = new ObjectArray<DCTableColumn>(8, 8, Ownership::True);
   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, Ownership::True);
}

/**
 * Constructor for creating DCTable from database
 * Assumes that fields in SELECT query are in following order:
 *    item_id,template_id,template_item_id,name,
 *    description,flags,source,snmp_port,polling_interval,retention_time,
 *    status,system_tag,resource_id,proxy_node,perftab_settings,
 *    transformation_script,comments,guid,instd_method,instd_data,
 *    instd_filter,instance,instance_retention_time,grace_period_start,
 *    related_object,polling_schedule_type,retention_type,polling_interval_src,
 *    retention_time_src,snmp_version,state_flags,user_tag,thresholds_disable_end_time
 */
DCTable::DCTable(DB_HANDLE hdb, DB_STATEMENT *preparedStatements, DB_RESULT hResult, int row, const shared_ptr<DataCollectionOwner>& owner, bool useStartupDelay) : DCObject(owner)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_templateId = DBGetFieldUInt32(hResult, row, 1);
   m_templateItemId = DBGetFieldUInt32(hResult, row, 2);
	m_name = DBGetFieldAsSharedString(hResult, row, 3);
   m_description = DBGetFieldAsSharedString(hResult, row, 4);
   m_flags = DBGetFieldUInt32(hResult, row, 5);
   m_source = (BYTE)DBGetFieldLong(hResult, row, 6);
	m_snmpPort = DBGetFieldUInt16(hResult, row, 7);
   m_pollingInterval = DBGetFieldInt32(hResult, row, 8);
   m_retentionTime = DBGetFieldInt32(hResult, row, 9);
   m_status = (BYTE)DBGetFieldLong(hResult, row, 10);
	m_systemTag = DBGetFieldAsSharedString(hResult, row, 11);
	m_resourceId = DBGetFieldUInt32(hResult, row, 12);
	m_sourceNode = DBGetFieldUInt32(hResult, row, 13);
	m_perfTabSettings = DBGetFieldAsSharedString(hResult, row, 14);
   setTransformationScript(DBGetFieldAsString(hResult, row, 15));
   m_comments = DBGetFieldAsSharedString(hResult, row, 16);
   m_guid = DBGetFieldGUID(hResult, row, 17);
   m_instanceDiscoveryMethod = DBGetFieldUInt16(hResult, row, 18);
   m_instanceDiscoveryData = DBGetFieldAsSharedString(hResult, row, 19);
   setInstanceFilter(DBGetFieldAsString(hResult, row, 20));
   m_instanceName = DBGetFieldAsSharedString(hResult, row, 21);
   m_instanceRetentionTime = DBGetFieldInt32(hResult, row, 22);
   m_instanceGracePeriodStart = DBGetFieldLong(hResult, row, 23);
   m_relatedObject = DBGetFieldUInt32(hResult, row, 24);
   m_pollingScheduleType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 25));
   m_retentionType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 26));
   m_pollingIntervalSrc = (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM) ? DBGetFieldAsString(hResult, row, 27) : nullptr;
   m_retentionTimeSrc = (m_retentionType == DC_RETENTION_CUSTOM) ? DBGetFieldAsString(hResult, row, 28) : nullptr;
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldInt32(hResult, row, 29));
   m_stateFlags = DBGetFieldUInt32(hResult, row, 30);
   m_userTag = DBGetFieldAsSharedString(hResult, row, 31);
   m_thresholdDisableEndTime = DBGetFieldTime(hResult, row, 32);

   int effectivePollingInterval = getEffectivePollingInterval();
   m_startTime = (useStartupDelay && (effectivePollingInterval >= 10)) ? time(nullptr) + rand() % (effectivePollingInterval / 2) : 0;

	m_columns = new ObjectArray<DCTableColumn>(8, 8, Ownership::True);
	DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DCI_TABLE_COLUMNS, _T("SELECT column_name,flags,snmp_oid,display_name FROM dc_table_columns WHERE table_id=? ORDER BY sequence_number"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hColumnList = DBSelectPrepared(hStmt);
		if (hColumnList != nullptr)
		{
			int count = DBGetNumRows(hColumnList);
			for(int i = 0; i < count; i++)
				m_columns->add(new DCTableColumn(hColumnList, i));
			DBFreeResult(hColumnList);
		}
	}

   loadAccessList(hdb, preparedStatements);
   loadCustomSchedules(hdb, preparedStatements);

   m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, Ownership::True);
   loadThresholds(hdb);

   updateTimeIntervalsInternal();
}

/**
 * Create DCTable from import file
 */
DCTable::DCTable(ConfigEntry *config, const shared_ptr<DataCollectionOwner>& owner, bool nxslV5) : DCObject(config, owner, nxslV5)
{
	ConfigEntry *columnsRoot = config->findEntry(_T("columns"));
	if (columnsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> columns = columnsRoot->getSubEntries(_T("column#*"));
      m_columns = new ObjectArray<DCTableColumn>(columns->size(), 8, Ownership::True);
      for (int i = 0; i < columns->size(); i++)
      {
         m_columns->add(new DCTableColumn(columns->get(i)));
      }
   }
   else
   {
      m_columns = new ObjectArray<DCTableColumn>(8, 8, Ownership::True);
   }

   ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
   if (thresholdsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      m_thresholds = new ObjectArray<DCTableThreshold>(thresholds->size(), 8, Ownership::True);
      for (int i = 0; i < thresholds->size(); i++)
      {
         m_thresholds->add(new DCTableThreshold(thresholds->get(i)));
      }
   }
   else
   {
      m_thresholds = new ObjectArray<DCTableThreshold>(0, 4, Ownership::True);
   }
}

/**
 * Destructor
 */
DCTable::~DCTable()
{
	delete m_columns;
   delete m_thresholds;
}

/**
 * Delete all collected data
 */
bool DCTable::deleteAllData()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   lock();
   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         _sntprintf(query, 256, _T("DELETE FROM tdata_sc_%s WHERE item_id=%u"), getStorageClassName(getStorageClass()), m_id);
      else
         _sntprintf(query, 256, _T("DELETE FROM tdata WHERE item_id=%u"), m_id);
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM tdata_%u WHERE item_id=%u"), m_ownerId, m_id);
   }
	bool success = DBQuery(hdb, query);
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Delete single DCI entry
 */
bool DCTable::deleteEntry(Timestamp timestamp)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   lock();
   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         _sntprintf(query, 256, _T("DELETE FROM tdata_sc_%s WHERE item_id=%u AND tdata_timestamp=ms_to_timestamptz(")  INT64_FMT _T(")"),
                  getStorageClassName(getStorageClass()), m_id, timestamp.asMilliseconds());
      }
      else
      {
         _sntprintf(query, 256, _T("DELETE FROM tdata WHERE item_id=%u AND tdata_timestamp=") INT64_FMT, m_id, timestamp.asMilliseconds());
      }
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM tdata_%u WHERE item_id=%u AND tdata_timestamp=") INT64_FMT, m_ownerId, m_id, timestamp.asMilliseconds());
   }
   bool success = DBQuery(hdb, query);
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 * If allowPastDataPoints is false, data points with timestamp older than last stored one
 * will be rejected.
 *
 * @return true on success
 */
bool DCTable::processNewValue(Timestamp timestamp, const shared_ptr<Table>& value, bool *updateStatus, bool allowPastDataPoints)
{
   *updateStatus = false;
   lock();

   auto owner = m_owner.lock();

   // Normally m_owner shouldn't be nullptr for polled items, but who knows...
   if (owner == nullptr)
   {
      unlock();
      return false;
   }

   if (!allowPastDataPoints && (timestamp < m_lastValueTimestamp))
   {
      // Old value, ignore
      unlock();
      return false;
   }

   // Transform input value
   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(value))
      {
         unlock();
         return false;
      }
   }

   m_errorCount = 0;
	m_lastValue = value;
	m_lastValue->setTitle(m_description);
   m_lastValue->setSource(m_source);
   m_lastValueTimestamp = timestamp;

	// Copy required fields into local variables
	uint32_t tableId = m_id;
	uint32_t nodeId = owner->getId();
   bool save = (m_retentionType != DC_RETENTION_NONE);

   unlock();

	// Save data to database
	// Object is unlocked, so only local variables can be used
   if (save)
   {
	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      if (!DBBegin(hdb))
      {
   	   DBConnectionPoolReleaseConnection(hdb);
         return true;
      }

      bool success = false;

	   DB_STATEMENT hStmt;
	   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	   {
	      if (g_dbSyntax == DB_SYNTAX_TSDB)
	      {
	         wchar_t query[256];
	         _sntprintf(query, 256, _T("INSERT INTO tdata_sc_%s (item_id,tdata_timestamp,tdata_value) VALUES (?,to_timestamp(?),?)"),
	                  getStorageClassName(getStorageClass()));
            hStmt = DBPrepare(hdb, query);
	      }
	      else
	      {
	         hStmt = DBPrepare(hdb, _T("INSERT INTO tdata (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"));
	      }
	   }
	   else
	   {
	      wchar_t query[256];
	      _sntprintf(query, 256, _T("INSERT INTO tdata_%u (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), nodeId);
	      hStmt = DBPrepare(hdb, query);
	   }
	   if (hStmt != nullptr)
	   {
		   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
		   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, timestamp);
		   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, value->toPackedXML(), DB_BIND_DYNAMIC);
	      success = DBExecute(hStmt);
		   DBFreeStatement(hStmt);
	   }

      if (success)
         DBCommit(hdb);
      else
         DBRollback(hdb);

	   DBConnectionPoolReleaseConnection(hdb);
   }

   Timestamp now = Timestamp::now();
   if (((g_offlineDataRelevanceTime <= 0) || (timestamp > (now - g_offlineDataRelevanceTime))) &&
       ((m_thresholdDisableEndTime == 0) || (m_thresholdDisableEndTime > 0 && m_thresholdDisableEndTime < timestamp.asTime())))
   {
      checkThresholds(value.get());

      if ((m_thresholdDisableEndTime > 0) && (m_thresholdDisableEndTime < now.asTime()))
      {
         // Thresholds were disabled, reset disable end time
         m_thresholdDisableEndTime = 0;
         getOwner()->markAsModified(MODIFY_DATA_COLLECTION);
      }
   }

   if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      PerfDataStorageRequest(this, timestamp, value.get());

   return true;
}

/**
 * Transform received value. Expected to be called while object is locked.
 */
bool DCTable::transform(const shared_ptr<Table>& value)
{
   if (m_transformationScript == nullptr)
   {
      // Return error if transformation script is present but cannot be compiled
      return m_transformationScriptSource.isNull() || IsBlankString(m_transformationScriptSource);
   }

   bool success = false;
   ScriptVMHandle vm = CreateServerScriptVM(m_transformationScript.get(), m_owner.lock(), createDescriptorInternal());
   if (vm.isValid())
   {
      NXSL_Value *nxslValue = vm->createValue(vm->createObject(&g_nxslTableClass, new shared_ptr<Table>(value)));

      // remove lock from DCI for script execution to avoid deadlocks
      unlock();
      success = vm->run(1, &nxslValue);
      lock();
      if (success)
      {
         if (vm->getResult()->isGuid() && (NXSLExitCodeToDCE(vm->getResult()->getValueAsGuid()) != DCE_SUCCESS))
            success = false;
      }
      else
      {
         if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, _T("Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted"),
                            m_description.cstr(), m_id, getOwnerName(), getOwnerId());
         }
         else
         {
            time_t now = time(nullptr);
            if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
            {
               ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, vm->getErrorText(), _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
               nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, _T("Failed to execute transformation script for object %s [%u] DCI %s [%u] (%s)"),
                        getOwnerName(), m_ownerId, m_name.cstr(), m_id, vm->getErrorText());
               m_lastScriptErrorReport = now;
            }
         }
      }
      vm.destroy();
   }
   else if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
   {
      time_t now = time(nullptr);
      if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
      {
         ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, _T("Script load error"), _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
         nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, _T("Failed to load transformation script for object %s [%u] DCI %s [%u]"),
                  getOwnerName(), m_ownerId, m_name.cstr(), m_id);
         m_lastScriptErrorReport = now;
      }
   }
   return success;
}

/**
 * Check thresholds
 */
void DCTable::checkThresholds(Table *value)
{
   lock();
   StringList instanceList;
   for(int row = 0; row < value->getNumRows(); row++)
   {
      TCHAR instance[MAX_RESULT_LENGTH];
      value->buildInstanceString(row, instance, MAX_RESULT_LENGTH);
      instanceList.add(instance);
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         DCTableThreshold *t = m_thresholds->get(i);
         ThresholdCheckResult result = t->check(value, row, instance);
         switch(result)
         {
            case ThresholdCheckResult::ACTIVATED:
               EventBuilder(t->getActivationEvent(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("row"), row)
                  .param(_T("instance"), instance)
                  .post();
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing (for current row)
               NotifyClientsOnThresholdChange(m_ownerId, m_id, t->getId(), instance, result);
               break;
            case ThresholdCheckResult::DEACTIVATED:
               EventBuilder(t->getDeactivationEvent(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("row"), row)
                  .param(_T("instance"), instance)
                  .param(_T("instanceMissing"), false)
                  .post();
               NotifyClientsOnThresholdChange(m_ownerId, m_id, t->getId(), instance, result);
               break;
            case ThresholdCheckResult::ALREADY_ACTIVE:
               i = m_thresholds->size();  // Threshold condition still true, stop processing
               break;
            default:
               break;
         }
      }
   }

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      DCTableThreshold *t = m_thresholds->get(i);
      StringList missingInstances = t->removeMissingInstances(instanceList);
      for (int i = 0; i < missingInstances.size(); i++)
      {
         EventBuilder(t->getDeactivationEvent(), m_ownerId)
                          .dci(m_id)
                          .param(_T("dciName"), m_name)
                          .param(_T("dciDescription"), m_description)
                          .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                          .param(_T("row"), -1)
                          .param(_T("instance"), missingInstances.get(i))
                          .param(_T("instanceMissing"), true)
                          .post();
         NotifyClientsOnThresholdChange(m_ownerId, m_id, t->getId(), missingInstances.get(i), ThresholdCheckResult::DEACTIVATED);
      }
   }
   unlock();
}

/**
 * Process new data collection error
 */
void DCTable::processNewError(bool noInstance, Timestamp timestamp)
{
	m_errorCount++;
}

/**
 * Save information about threshold state before maintenance
 */
void DCTable::saveStateBeforeMaintenance()
{
   lock();
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      m_thresholds->get(i)->saveStateBeforeMaintenance();
   }
   unlock();
}

/**
 * Generate events based on saved state before maintenance
 */
void DCTable::generateEventsAfterMaintenance()
{
   lock();
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      m_thresholds->get(i)->generateEventsAfterMaintenance(this);
   }
   unlock();
}

/**
 * Save to database
 */
bool DCTable::saveToDatabase(DB_HANDLE hdb)
{
   static const WCHAR *columns[] = {
      L"node_id", L"template_id", L"template_item_id", L"name", L"description", L"flags", L"source",
      L"snmp_port", L"polling_interval", L"retention_time", L"status", L"system_tag", L"resource_id",
      L"proxy_node", L"perftab_settings", L"transformation_script", L"comments", L"guid",
      L"instd_method", L"instd_data", L"instd_filter", L"instance", L"instance_retention_time",
      L"grace_period_start", L"related_object", L"polling_interval_src", L"retention_time_src",
      L"polling_schedule_type", L"retention_type", L"snmp_version", L"state_flags", L"user_tag",
      L"thresholds_disable_end_time", nullptr
   };

	DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"dc_tables", L"item_id", m_id, columns);
	if (hStmt == nullptr)
		return false;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_ownerId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_templateId);
	DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_templateItemId);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, MAX_ITEM_NAME - 1);
	DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, MAX_DB_STRING - 1);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (UINT32)m_snmpPort);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_pollingInterval);
	DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)m_retentionTime);
	DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC, MAX_DCI_TAG_LENGTH - 1);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_resourceId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_perfTabSettings, DB_BIND_STATIC);
   DBBind(hStmt, 16, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
   DBBind(hStmt, 17, DB_SQLTYPE_TEXT, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (INT32)m_instanceDiscoveryMethod);
   DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
   DBBind(hStmt, 21, DB_SQLTYPE_TEXT, m_instanceFilterSource, DB_BIND_STATIC);
   DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_instanceName, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
   DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_instanceRetentionTime);
   DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, (INT32)m_instanceGracePeriodStart);
   DBBind(hStmt, 25, DB_SQLTYPE_INTEGER, m_relatedObject);
   DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_pollingIntervalSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   DBBind(hStmt, 27, DB_SQLTYPE_VARCHAR, m_retentionTimeSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   TCHAR pt[2], rt[2];
   pt[0] = m_pollingScheduleType + '0';
   pt[1] = 0;
   rt[0] = m_retentionType + '0';
   rt[1] = 0;
   DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, pt, DB_BIND_STATIC);
   DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, rt, DB_BIND_STATIC);
   DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_snmpVersion);
   DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, m_stateFlags);
   DBBind(hStmt, 32, DB_SQLTYPE_VARCHAR, m_userTag, DB_BIND_STATIC, MAX_DCI_TAG_LENGTH - 1);
   DBBind(hStmt, 33, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_thresholdDisableEndTime));
   DBBind(hStmt, 34, DB_SQLTYPE_INTEGER, m_id);

	bool result = DBExecute(hStmt);
	DBFreeStatement(hStmt);

	if (result)
	{
		// Save column configuration
		hStmt = DBPrepare(hdb, L"DELETE FROM dc_table_columns WHERE table_id=?");
		if (hStmt != nullptr)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
			result = DBExecute(hStmt);
			DBFreeStatement(hStmt);
		}
		else
		{
			result = false;
		}

		if (result && (m_columns->size() > 0))
		{
			hStmt = DBPrepare(hdb, L"INSERT INTO dc_table_columns (table_id,sequence_number,column_name,snmp_oid,flags,display_name) VALUES (?,?,?,?,?,?)");
			if (hStmt != nullptr)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
				for(int i = 0; i < m_columns->size(); i++)
				{
					DCTableColumn *column = m_columns->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
					DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, column->getName(), DB_BIND_STATIC);
					const SNMP_ObjectId& oid = column->getSnmpOid();
					DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, oid.isValid() ? oid.toString().cstr() : nullptr, DB_BIND_TRANSIENT);
					DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)column->getFlags());
					DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, column->getDisplayName(), DB_BIND_STATIC);

					result = DBExecute(hStmt);
					if (!result)
						break;
				}
				DBFreeStatement(hStmt);
			}
			else
			{
				result = false;
			}
		}
	}

   saveThresholds(hdb);

   unlock();
	return result ? DCObject::saveToDatabase(hdb) : false;
}

/**
 * Load thresholds from database
 */
bool DCTable::loadThresholds(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT id,activation_event,deactivation_event,sample_count FROM dct_thresholds WHERE table_id=? ORDER BY sequence_number");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         DCTableThreshold *t = new DCTableThreshold(hdb, hResult, i);
         m_thresholds->add(t);
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Save thresholds to database
 */
bool DCTable::saveThresholds(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, L"DELETE FROM dct_threshold_conditions WHERE threshold_id=?");
   if (hStmt == nullptr)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_thresholds->get(i)->getId());
      DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, L"DELETE FROM dct_threshold_instances WHERE threshold_id=?");
   if (hStmt == nullptr)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_thresholds->get(i)->getId());
      DBExecute(hStmt);
   }
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, L"DELETE FROM dct_thresholds WHERE table_id=?");
   if (hStmt == nullptr)
      return false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBExecute(hStmt);
   DBFreeStatement(hStmt);

   for(int i = 0; i < m_thresholds->size(); i++)
      m_thresholds->get(i)->saveToDatabase(hdb, m_id, i);
   return true;
}

/**
 * Delete table object and collected data from database
 */
void DCTable::deleteFromDatabase()
{
	DCObject::deleteFromDatabase();

   TCHAR szQuery[256];
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_tables WHERE item_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dc_table_columns WHERE table_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      _sntprintf(szQuery, 256, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=%d"), (int)m_thresholds->get(i)->getId());
      QueueSQLRequest(szQuery);
   }

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dct_thresholds WHERE table_id=%d"), (int)m_id);
   QueueSQLRequest(szQuery);

   auto owner = m_owner.lock();
   if (owner->isDataCollectionTarget() && g_dbSyntax != DB_SYNTAX_TSDB)
      static_cast<DataCollectionTarget*>(owner.get())->scheduleTableDataCleanup(m_id);
}

/**
 * Create NXCP message with item data
 */
void DCTable::createMessage(NXCPMessage *msg)
{
	DCObject::createMessage(msg);

   lock();
   msg->setField(VID_NUM_COLUMNS, (UINT32)m_columns->size());
	uint32_t fieldId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < m_columns->size(); i++)
	{
		m_columns->get(i)->fillMessage(msg, fieldId);
		fieldId += 10;
	}

	msg->setField(VID_NUM_THRESHOLDS, (UINT32)m_thresholds->size());
   fieldId = VID_DCI_THRESHOLD_BASE;
   for(int i = 0; i < m_thresholds->size(); i++)
   {
      fieldId = m_thresholds->get(i)->fillMessage(msg, fieldId);
   }

   unlock();
}

/**
 * Update data collection object from NXCP message
 */
void DCTable::updateFromMessage(const NXCPMessage& msg)
{
	DCObject::updateFromMessage(msg);

   lock();

	m_columns->clear();
	int count = msg.getFieldAsInt32(VID_NUM_COLUMNS);
	uint32_t fieldId = VID_DCI_COLUMN_BASE;
	for(int i = 0; i < count; i++)
	{
		m_columns->add(new DCTableColumn(msg, fieldId));
		fieldId += 10;
	}

	count = msg.getFieldAsInt32(VID_NUM_THRESHOLDS);
   ObjectArray<DCTableThreshold> *newThresholds = new ObjectArray<DCTableThreshold>(count, 8, Ownership::True);
	fieldId = VID_DCI_THRESHOLD_BASE;
	for(int i = 0; i < count; i++)
	{
      DCTableThreshold *t = new DCTableThreshold(msg, &fieldId);
		newThresholds->add(t);
      for(int j = 0; j < m_thresholds->size(); j++)
      {
         DCTableThreshold *old = m_thresholds->get(j);
         if (old->getId() == t->getId())
         {
            t->copyState(old);
            break;
         }
      }
	}
   delete m_thresholds;
   m_thresholds = newThresholds;

	unlock();
}

/**
 * Get last collected value
 */
void DCTable::fillLastValueMessage(NXCPMessage *msg)
{
   lock();
	if (m_lastValue != nullptr)
	{
      msg->setField(VID_TIMESTAMP_MS, m_lastValueTimestamp);
      m_lastValue->fillMessage(msg, 0, -1);
	}
	else
	{
      msg->setField(VID_TIMESTAMP_MS, static_cast<int64_t>(0));
	}
   unlock();
}

/**
 * Put last value into JSON object
 */
json_t *DCTable::lastValueToJSON()
{
   json_t *value;
   lock();
   if (m_lastValue != nullptr)
   {
      value = m_lastValue->toJson();
   }
   else
   {
      value = json_null();
   }
   unlock();
   return value;
}

/**
 * Get summary of last collected value (to show along simple DCI values)
 */
void DCTable::fillLastValueSummaryMessage(NXCPMessage *msg, uint32_t fieldId, const TCHAR *column, const TCHAR *instance)
{
   lock();
   msg->setField(fieldId++, m_ownerId);
   msg->setField(fieldId++, m_id);
   msg->setField(fieldId++, m_name);
   msg->setField(fieldId++, m_flags);
   msg->setField(fieldId++, m_description);
   msg->setField(fieldId++, static_cast<uint16_t>(m_source));
   if ((instance != nullptr) && (column != nullptr))
   {
      shared_ptr<Table> t = getLastValue();
      int columnIndex =  t->getColumnIndex(column);
      int rowIndex = t->findRowByInstance(instance);
      msg->setField(fieldId++, t->getColumnDataType(columnIndex));
      msg->setField(fieldId++, t->getAsString(rowIndex, columnIndex));
   }
   else
   {
      msg->setField(fieldId++, static_cast<uint16_t>(DCI_DT_NULL));  // compatibility: data type
      msg->setField(fieldId++, _T(""));             // compatibility: value
   }
   msg->setField(fieldId++, m_lastPollTime);
   msg->setField(fieldId++, static_cast<uint16_t>(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
   msg->setField(fieldId++, static_cast<uint16_t>(getType()));
   msg->setField(fieldId++, m_errorCount);
   msg->setField(fieldId++, m_templateItemId);
   msg->setField(fieldId++, _T(""));
   msg->setField(fieldId++, 0);
   msg->setField(fieldId++, !hasValue());
   msg->setField(fieldId++, m_comments);
   msg->setField(fieldId++, false); // Anomaly detected
   msg->setField(fieldId++, m_userTag);
   msg->setFieldFromTime(fieldId++, m_thresholdDisableEndTime);

   if (m_thresholds != nullptr)
   {
      int severity = -1;
      DCTableThreshold *threshold = nullptr;
      for(int i = 0; (i < m_thresholds->size()) && (severity != STATUS_CRITICAL); i++)
      {
         DCTableThreshold *t = m_thresholds->get(i);
         if (t->isActive())
         {
            shared_ptr<EventTemplate> event = FindEventTemplateByCode(t->getActivationEvent());
            if ((event != nullptr) && (event->getSeverity() > severity))
            {
               threshold = t;
               severity = event->getSeverity();
            }
         }
      }
      if (severity != -1)
      {
         msg->setField(fieldId++, static_cast<uint16_t>(1));   // number of thresholds
         msg->setField(fieldId++, threshold->getId());
         msg->setField(fieldId++, threshold->getActivationEvent());
         msg->setField(fieldId++, threshold->getDeactivationEvent());
         msg->setField(fieldId++, static_cast<uint16_t>(0));   // function
         msg->setField(fieldId++, static_cast<uint16_t>(0));   // operation
         msg->setField(fieldId++, static_cast<uint32_t>(threshold->getSampleCount()));
         msg->setField(fieldId++, _T(""));   // script source
         msg->setField(fieldId++, static_cast<uint32_t>(0));   // compatibility: repeat interval
         msg->setField(fieldId++, threshold->getConditionAsText());
         msg->setField(fieldId++, true);  // Is active
         msg->setField(fieldId++, static_cast<uint16_t>(severity));
         msg->setFieldFromTime(fieldId++, static_cast<uint32_t>(0)); // compatibility: last activation timestamp
      }
      else
      {
         msg->setField(fieldId++, static_cast<uint16_t>(0));   // number of thresholds
      }
   }
   else
   {
      msg->setField(fieldId++, static_cast<uint16_t>(0));   // number of thresholds
   }

   unlock();
}

/**
 * Get data type of given column
 */
int DCTable::getColumnDataType(const TCHAR *name) const
{
   int dt = DCI_DT_STRING;
   bool found = false;

   lock();

   // look in column definition first
	for(int i = 0; i < m_columns->size(); i++)
	{
		DCTableColumn *column = m_columns->get(i);
      if (!_tcsicmp(column->getName(), name))
      {
         dt = column->getDataType();
         break;
      }
   }

   // use last values if not found in definitions
   if (!found && (m_lastValue != nullptr))
   {
      int index = m_lastValue->getColumnIndex(name);
      if (index != -1)
         dt = m_lastValue->getColumnDataType(index);
   }

   unlock();
   return dt;
}

/**
 * Get last collected value
 */
shared_ptr<Table> DCTable::getLastValue()
{
   lock();
   shared_ptr<Table> value = m_lastValue;
   unlock();
   return value;
}

/**
 * Update destination value from source value
 */
#define RECALCULATE_VALUE(dst, src, func, count) \
   { \
      switch(func) \
      { \
         case DCF_FUNCTION_MIN: \
            if (src < dst) dst = src; \
            break; \
         case DCF_FUNCTION_MAX: \
            if (src > dst) dst = src; \
            break; \
         case DCF_FUNCTION_SUM: \
            dst += src; \
            break; \
         case DCF_FUNCTION_AVG: \
            dst = (dst * count + src) / (count + 1); \
            break; \
      } \
   }

/**
 * Merge values
 */
void DCTable::mergeValues(Table *dest, Table *src, int count) const
{
   for(int sRow = 0; sRow < src->getNumRows(); sRow++)
   {
      wchar_t instance[MAX_RESULT_LENGTH];

      src->buildInstanceString(sRow, instance, MAX_RESULT_LENGTH);
      int dRow = dest->findRowByInstance(instance);
      if (dRow >= 0)
      {
         for(int j = 0; j < m_columns->size(); j++)
         {
            DCTableColumn *cd = m_columns->get(j);
            if ((cd == nullptr) || cd->isInstanceColumn() || (cd->getDataType() == DCI_DT_STRING))
               continue;
            int column = dest->getColumnIndex(cd->getName());
            if (column == -1)
               continue;

            if (cd->getDataType() == DCI_DT_FLOAT)
            {
               double sval = src->getAsDouble(sRow, column);
               double dval = dest->getAsDouble(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
            else if ((cd->getDataType() == DCI_DT_UINT) || (cd->getDataType() == DCI_DT_UINT64))
            {
               uint64_t sval = src->getAsUInt64(sRow, column);
               uint64_t dval = dest->getAsUInt64(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
            else
            {
               int64_t sval = src->getAsInt64(sRow, column);
               int64_t dval = dest->getAsInt64(dRow, column);

               RECALCULATE_VALUE(dval, sval, cd->getAggregationFunction(), count);

               dest->setAt(dRow, column, dval);
            }
         }
      }
      else
      {
         // no such instance
         dest->copyRow(src, sRow);
      }
   }
}

/**
 * Update columns in resulting table according to definition
 */
void DCTable::updateResultColumns(const shared_ptr<Table>& t) const
{
   lock();
   for(int i = 0; i < m_columns->size(); i++)
   {
      DCTableColumn *col = m_columns->get(i);
      int index = t->getColumnIndex(col->getName());
      if (index == -1)
      {
         t->insertColumn(i, col->getName(), col->getDataType(), col->getDisplayName(), col->isInstanceColumn());
      }
      else if (_tcscmp(t->getColumnName(i), col->getName()) != 0)
      {
         t->swapColumns(i, index);
         TableColumnDefinition *cd = t->getColumnDefinitions().get(i);
         if (cd != nullptr)
         {
            cd->setDataType(col->getDataType());
            cd->setInstanceColumn(col->isInstanceColumn());
            cd->setDisplayName(col->getDisplayName());
         }
      }
      else
      {
         TableColumnDefinition *cd = t->getColumnDefinitions().get(i);
         if (cd != nullptr)
         {
            cd->setDataType(col->getDataType());
            cd->setInstanceColumn(col->isInstanceColumn());
            cd->setDisplayName(col->getDisplayName());
         }
      }
   }
   unlock();
}

/**
 * Update from template item
 */
void DCTable::updateFromTemplate(DCObject *src)
{
	DCObject::updateFromTemplate(src);

	if (src->getType() != DCO_TYPE_TABLE)
	{
	   nxlog_debug_tag(DEBUG_TAG_DC_TEMPLATES, 2, _T("INTERNAL ERROR: DCTable::updateFromTemplate(%d, %d): source type is %d"), (int)m_id, (int)src->getId(), src->getType());
		return;
	}

   lock();
	DCTable *table = (DCTable *)src;

   m_columns->clear();
	for(int i = 0; i < table->m_columns->size(); i++)
		m_columns->add(new DCTableColumn(table->m_columns->get(i)));

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
   int count = std::min(m_thresholds->size(), table->m_thresholds->size());
   int i;
   for(i = 0; i < count; i++)
      if (!m_thresholds->get(i)->equals(table->m_thresholds->get(i)))
         break;
   count = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
   while(count < m_thresholds->size())
      m_thresholds->remove(count);

   // (Re)create thresholds starting from first unmatched
   for(i = count; i < table->m_thresholds->size(); i++)
      m_thresholds->add(new DCTableThreshold(table->m_thresholds->get(i), false));

   unlock();
}

/**
 * Create management pack record
 */
void DCTable::createExportRecord(TextFileWriter& xml) const
{
   lock();

   xml.append(_T("\t\t\t\t<dctable id=\""));
   xml.append(m_id);
   xml.append(_T("\">\n\t\t\t\t\t<guid>"));
   xml.append(m_guid);
   xml.append(_T("</guid>\n\t\t\t\t\t<name>"));
   xml.append(EscapeStringForXML2(m_name));
   xml.append(_T("</name>\n\t\t\t\t\t<description>"));
   xml.append(EscapeStringForXML2(m_description));
   xml.append(_T("</description>\n\t\t\t\t\t<origin>"));
   xml.append(static_cast<int32_t>(m_source));
   xml.append(_T("</origin>\n\t\t\t\t\t<scheduleType>"));
   xml.append(static_cast<int32_t>(m_pollingScheduleType));
   xml.append(_T("</scheduleType>\n\t\t\t\t\t<interval>"));
   xml.append(EscapeStringForXML2(m_pollingIntervalSrc));
   xml.append(_T("</interval>\n\t\t\t\t\t<retentionType>"));
   xml.append(static_cast<int32_t>(m_retentionType));
   xml.append(_T("</retentionType>\n\t\t\t\t\t<retention>"));
   xml.append(EscapeStringForXML2(m_retentionTimeSrc));
   xml.append(_T("</retention>\n\t\t\t\t\t<systemTag>"));
   xml.append(EscapeStringForXML2(m_systemTag));
   xml.append(_T("</systemTag>\n\t\t\t\t\t<userTag>"));
   xml.append(EscapeStringForXML2(m_userTag));
   xml.append(_T("</userTag>\n\t\t\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t\t\t<snmpPort>"));
   xml.append(m_snmpPort);
   xml.append(_T("</snmpPort>\n\t\t\t\t\t<snmpVersion>"));
   xml.append(m_snmpVersion);
   xml.append(_T("</snmpVersion>\n\t\t\t\t\t<instanceDiscoveryMethod>"));
   xml.append(m_instanceDiscoveryMethod);
   xml.append(_T("</instanceDiscoveryMethod>\n\t\t\t\t\t<instance>"));
   xml.append(EscapeStringForXML2(m_instanceName));
   xml.append(_T("</instance>\n\t\t\t\t\t<instanceRetentionTime>"));
   xml.append(m_instanceRetentionTime);
   xml.append(_T("</instanceRetentionTime>\n\t\t\t\t\t<comments>"));
   xml.append(EscapeStringForXML2(m_comments));
   xml.append(_T("</comments>\n\t\t\t\t\t<isDisabled>"));
   xml.append(BooleanToString(m_status == ITEM_STATUS_DISABLED));
   xml.append(_T("</isDisabled>\n"));

	if (!m_transformationScriptSource.isBlank())
	{
		xml.append(_T("\t\t\t\t\t<transformation>"));
		xml.appendPreallocated(EscapeStringForXML(m_transformationScriptSource, -1));
		xml.append(_T("</transformation>\n"));
	}

   if ((m_schedules != nullptr) && !m_schedules->isEmpty())
   {
      xml.append(_T("\t\t\t\t\t<schedules>\n"));
      for(int i = 0; i < m_schedules->size(); i++)
      {
         xml.append(_T("\t\t\t\t\t\t<schedule>"));
         xml.append(EscapeStringForXML2(m_schedules->get(i)));
         xml.append(_T("</schedule>\n"));
      }
      xml.append(_T("\t\t\t\t\t</schedules>\n"));
   }

	if (m_columns != nullptr)
	{
	   xml.append(_T("\t\t\t\t\t<columns>\n"));
		for(int i = 0; i < m_columns->size(); i++)
		{
			m_columns->get(i)->createExportRecord(xml, i + 1);
		}
	   xml.append(_T("\t\t\t\t\t</columns>\n"));
	}

	if (m_thresholds != nullptr)
	{
	   xml.append(_T("\t\t\t\t\t<thresholds>\n"));
		for(int i = 0; i < m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createExportRecord(xml, i + 1);
		}
	   xml.append(_T("\t\t\t\t\t</thresholds>\n"));
	}

	if (!m_perfTabSettings.isEmpty())
	{
		xml.append(_T("\t\t\t\t\t<perfTabSettings>"));
		xml.appendPreallocated(EscapeStringForXML(m_perfTabSettings, -1));
		xml.append(_T("</perfTabSettings>\n"));
	}

	if (m_instanceDiscoveryData != nullptr)
   {
      xml += _T("\t\t\t\t\t<instanceDiscoveryData>");
      xml.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
      xml += _T("</instanceDiscoveryData>\n");
   }

   if (!m_instanceFilterSource.isBlank())
   {
      xml.append(_T("\t\t\t\t\t<instanceFilter>"));
      xml.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
      xml.append(_T("</instanceFilter>\n"));
   }

   unlock();
   xml.append(_T("\t\t\t\t</dctable>\n"));
}

/**
 * Create DCObject from import file
 */
void DCTable::updateFromImport(ConfigEntry *config, bool nxslV5)
{
   DCObject::updateFromImport(config, nxslV5);

   lock();
   m_columns->clear();
   ConfigEntry *columnsRoot = config->findEntry(_T("columns"));
   if (columnsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> columns = columnsRoot->getSubEntries(_T("column#*"));
      for(int i = 0; i < columns->size(); i++)
      {
         m_columns->add(new DCTableColumn(columns->get(i)));
      }
   }

   m_thresholds->clear();
   ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
   if (thresholdsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      for(int i = 0; i < thresholds->size(); i++)
      {
         m_thresholds->add(new DCTableThreshold(thresholds->get(i)));
      }
   }
   unlock();
}

/**
 * Get list of used events
 */
void DCTable::getEventList(HashSet<uint32_t> *eventList) const
{
   lock();
   if (m_thresholds != nullptr)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         eventList->put(m_thresholds->get(i)->getActivationEvent());
         eventList->put(m_thresholds->get(i)->getDeactivationEvent());
      }
   }
   unlock();
}

/**
 * Check if given event is used
 */
bool DCTable::isUsingEvent(uint32_t eventCode) const
{
   for (int i = 0; i < m_thresholds->size(); i++)
      if (m_thresholds->get(i)->isUsingEvent(eventCode))
         return true;
   return false;
}

/*
 * Clone DCTable
 */
DCObject *DCTable::clone() const
{
   return new DCTable(this, false);
}

/**
 * Serialize object to JSON
 */
json_t *DCTable::toJson()
{
   json_t *root = DCObject::toJson();
   lock();
   json_object_set_new(root, "columns", json_object_array(m_columns));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
   unlock();
   return root;
}

/**
 * Get list of all threshold IDs
 */
void DCTable::getThresholdIdList(IntegerArray<uint32_t> *idList) const
{
   lock();
   if (m_thresholds != nullptr)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         idList->add(m_thresholds->get(i)->getId());
      }
   }
   unlock();
}

/**
 * Loads DCTable last value
 */
void DCTable::loadCache()
{
   if (m_lastValue != nullptr)
      return;  // Value already collected

   TCHAR query[512];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP 1 tdata_value,tdata_timestamp FROM tdata WHERE item_id=%u ORDER BY tdata_timestamp DESC"), m_id);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT tdata_value,tdata_timestamp FROM tdata WHERE item_id=%u ORDER BY tdata_timestamp DESC) WHERE ROWNUM<=1"), m_id);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 512, _T("SELECT tdata_value,tdata_timestamp FROM tdata WHERE item_id=%u ORDER BY tdata_timestamp DESC LIMIT 1"), m_id);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT tdata_value,tdata_timestamp FROM tdata WHERE item_id=%u ORDER BY tdata_timestamp DESC FETCH FIRST 1 ROWS ONLY"), m_id);
            break;
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT tdata_value,tdata_timestamp FROM tdata_sc_%s WHERE item_id=%u ORDER BY tdata_timestamp DESC LIMIT 1"), getStorageClassName(getStorageClass()), m_id);
            break;
         default:
            nxlog_debug_tag(_T("dc"), 2, _T("INTERNAL ERROR: unsupported database in DCTable::loadCache"));
            query[0] = 0;   // Unsupported database
            break;
      }
   }
   else
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP 1 tdata_value,tdata_timestamp FROM tdata_%u WHERE item_id=%u ORDER BY tdata_timestamp DESC"), m_ownerId, m_id);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT tdata_value,tdata_timestamp FROM tdata_%u WHERE item_id=%u ORDER BY tdata_timestamp DESC) WHERE ROWNUM<=1"), m_ownerId, m_id);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT tdata_value,tdata_timestamp FROM tdata_%u WHERE item_id=%u ORDER BY tdata_timestamp DESC LIMIT 1"), m_ownerId, m_id);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT tdata_value,tdata_timestamp FROM tdata_%u WHERE item_id=%u ORDER BY tdata_timestamp DESC FETCH FIRST 1 ROWS ONLY"), m_ownerId, m_id);
            break;
         default:
            nxlog_debug_tag(_T("dc"), 2, _T("INTERNAL ERROR: unsupported database in DCTable::loadCache"));
            query[0] = 0;   // Unsupported database
            break;
      }
   }

   char *encodedTable = nullptr;
   Timestamp timestamp = Timestamp::fromMilliseconds(0);
   if (query[0] != 0)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            encodedTable = DBGetFieldUTF8(hResult, 0, 0, nullptr, 0);
            timestamp = DBGetFieldTimestamp(hResult, 0, 1);
         }
         DBFreeResult(hResult);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   lock();
   if (encodedTable != nullptr && m_lastValue == nullptr) //m_lastValue can be changed while query is executed
   {
      m_lastValue = shared_ptr<Table>(Table::createFromPackedXML(encodedTable));
      m_lastValueTimestamp = timestamp;
   }
   unlock();
   MemFree(encodedTable);
}
