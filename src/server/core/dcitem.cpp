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
** File: dcitem.cpp
**
**/

#include "nxcore.h"
#include <npe.h>

/**
 * Create DCItem from another DCItem
 */
DCItem::DCItem(const DCItem *src, bool shadowCopy, bool copyThresholds) : DCObject(src, shadowCopy)
{
   m_dataType = src->m_dataType;
   m_transformedDataType = src->m_transformedDataType;
   m_deltaCalculation = src->m_deltaCalculation;
	m_sampleCount = src->m_sampleCount;
   m_cacheSize = shadowCopy ? src->m_cacheSize : 0;
   m_requiredCacheSize = shadowCopy ? src->m_requiredCacheSize : 0;
   if (m_cacheSize > 0)
   {
      m_ppValueCache = MemAllocArray<ItemValue*>(m_cacheSize);
      for(uint32_t i = 0; i < m_cacheSize; i++)
         m_ppValueCache[i] = new ItemValue(*src->m_ppValueCache[i]);
   }
   else
   {
      m_ppValueCache = nullptr;
   }
   m_prevValueTimeStamp = shadowCopy ? src->m_prevValueTimeStamp : Timestamp::fromMilliseconds(0);
   m_prevDeltaValue = shadowCopy ? src->m_prevDeltaValue : 0;
   m_cacheLoaded = shadowCopy ? src->m_cacheLoaded : false;
   m_anomalyDetected = shadowCopy ? src->m_anomalyDetected : false;
	m_multiplier = src->m_multiplier;
	m_unitName = src->m_unitName;
	m_snmpRawValueType = src->m_snmpRawValueType;
   wcscpy(m_predictionEngine, src->m_predictionEngine);
   m_allThresholdsRearmEvent = src->m_allThresholdsRearmEvent;

   // Copy thresholds
	if (copyThresholds && (src->getThresholdCount() > 0))
	{
		m_thresholds = new ObjectArray<Threshold>(src->m_thresholds->size(), 8, Ownership::True);
		for(int i = 0; i < src->m_thresholds->size(); i++)
		{
			Threshold *t = new Threshold(*src->m_thresholds->get(i), shadowCopy);
			m_thresholds->add(t);
		}
	}
	else
	{
		m_thresholds = nullptr;
	}
}

/**
 * Constructor for creating DCItem from database
 * Assumes that fields in SELECT query are in following order:
 *    item_id,name,source,datatype,polling_interval,retention_time,status,
 *    delta_calculation,transformation,template_id,description,instance,
 *    template_item_id,flags,resource_id,proxy_node,multiplier,
 *    units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type,
 *    instd_method,instd_data,instd_filter,samples,comments,guid,npe_name,
 *    instance_retention_time,grace_period_start,related_object,polling_schedule_type,
 *    retention_type,polling_interval_src,retention_time_src,snmp_version,state_flags,
 *    all_rearmed_event,transformed_datatype,user_tag,thresholds_disable_end_time
 */
DCItem::DCItem(DB_HANDLE hdb, DB_STATEMENT *preparedStatements, DB_RESULT hResult, int row, const shared_ptr<DataCollectionOwner>& owner, bool useStartupDelay) : DCObject(owner)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_name = DBGetFieldAsSharedString(hResult, row, 1);
   m_source = (BYTE)DBGetFieldLong(hResult, row, 2);
   m_dataType = (BYTE)DBGetFieldLong(hResult, row, 3);
   m_pollingInterval = DBGetFieldInt32(hResult, row, 4);
   m_retentionTime = DBGetFieldInt32(hResult, row, 5);
   m_status = (BYTE)DBGetFieldLong(hResult, row, 6);
   m_deltaCalculation = (BYTE)DBGetFieldLong(hResult, row, 7);
   setTransformationScript(DBGetFieldAsString(hResult, row, 8));
   m_templateId = DBGetFieldULong(hResult, row, 9);
   m_description = DBGetFieldAsSharedString(hResult, row, 10);
   m_instanceName = DBGetFieldAsSharedString(hResult, row, 11);
   m_templateItemId = DBGetFieldUInt32(hResult, row, 12);
   m_thresholds = nullptr;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_prevValueTimeStamp = Timestamp::fromMilliseconds(0);
   m_prevDeltaValue = 0;
   m_cacheLoaded = false;
   m_anomalyDetected = false;
   m_flags = DBGetFieldUInt32(hResult, row, 13);
	m_resourceId = DBGetFieldUInt32(hResult, row, 14);
	m_sourceNode = DBGetFieldUInt32(hResult, row, 15);
	m_multiplier = DBGetFieldInt32(hResult, row, 16);
	m_unitName = DBGetFieldAsSharedString(hResult, row, 17);
	m_perfTabSettings = DBGetFieldAsSharedString(hResult, row, 18);
	m_systemTag = DBGetFieldAsSharedString(hResult, row, 19);
	m_snmpPort = DBGetFieldUInt16(hResult, row, 20);
	m_snmpRawValueType = DBGetFieldUInt16(hResult, row, 21);
	m_instanceDiscoveryMethod = DBGetFieldUInt16(hResult, row, 22);
	m_instanceDiscoveryData = DBGetFieldAsSharedString(hResult, row, 23);
	setInstanceFilter(DBGetFieldAsString(hResult, row, 24));
	m_sampleCount = DBGetFieldLong(hResult, row, 25);
   m_comments = DBGetFieldAsSharedString(hResult, row, 26);
   m_guid = DBGetFieldGUID(hResult, row, 27);
   DBGetField(hResult, row, 28, m_predictionEngine, MAX_NPE_NAME_LEN);
   m_instanceRetentionTime = DBGetFieldInt32(hResult, row, 29);
   m_instanceGracePeriodStart = DBGetFieldLong(hResult, row, 30);
   m_relatedObject = DBGetFieldUInt32(hResult, row, 31);
   m_pollingScheduleType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 32));
   m_retentionType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 33));
   m_pollingIntervalSrc = (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM) ? DBGetFieldAsString(hResult, row, 34) : nullptr;
   m_retentionTimeSrc = (m_retentionType == DC_RETENTION_CUSTOM) ? DBGetFieldAsString(hResult, row, 35) : nullptr;
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldInt32(hResult, row, 36));
   m_stateFlags = DBGetFieldUInt32(hResult, row, 37);
   m_allThresholdsRearmEvent = DBGetFieldUInt32(hResult, row, 38);
   m_transformedDataType = (BYTE)DBGetFieldLong(hResult, row, 39);
   m_userTag = DBGetFieldAsSharedString(hResult, row, 40);
   m_thresholdDisableEndTime = DBGetFieldTime(hResult, row, 41);

   int effectivePollingInterval = getEffectivePollingInterval();
   m_startTime = (useStartupDelay && (effectivePollingInterval >= 10)) ? time(nullptr) + rand() % (effectivePollingInterval / 2) : 0;

   // Load last raw value from database
   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DCI_RAW_VALUE,
      _T("SELECT raw_value,last_poll_time,anomaly_detected FROM raw_dci_values WHERE item_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DB_RESULT hTempResult = DBSelectPrepared(hStmt);
      if (hTempResult != nullptr)
      {
         if (DBGetNumRows(hTempResult) > 0)
         {
            TCHAR szBuffer[MAX_DB_STRING];
            m_prevRawValue = DBGetField(hTempResult, 0, 0, szBuffer, MAX_DB_STRING);
            m_prevValueTimeStamp = DBGetFieldTimestamp(hTempResult, 0, 1);
            m_anomalyDetected = (DBGetFieldInt32(hTempResult, 0, 2) != 0);
            m_lastPollTime = m_lastValueTimestamp = m_prevValueTimeStamp;
         }
         DBFreeResult(hTempResult);
      }
   }

   loadAccessList(hdb, preparedStatements);
	loadCustomSchedules(hdb, preparedStatements);

	updateTimeIntervalsInternal();
}

/**
 * Constructor for creating new DCItem from scratch
 */
DCItem::DCItem(uint32_t id, const TCHAR *name, int source, int dataType, BYTE scheduleType,
      const TCHAR *pollingInterval, BYTE retentionType, const TCHAR *retentionTime,
      const shared_ptr<DataCollectionOwner>& owner, const TCHAR *description, const TCHAR *systemTag)
	: DCObject(id, name, source, scheduleType, pollingInterval, retentionType, retentionTime, owner,
	      description, systemTag)
{
   m_dataType = dataType;
   m_transformedDataType = DCI_DT_NULL;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_thresholds = nullptr;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_prevValueTimeStamp = Timestamp::fromMilliseconds(0);
   m_prevDeltaValue = 0;
   m_cacheLoaded = false;
   m_anomalyDetected = false;
	m_multiplier = 0;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_predictionEngine[0] = 0;
	m_allThresholdsRearmEvent = 0;

   updateCacheSizeInternal(false);
}

/**
 * Create DCItem from import file
 */
DCItem::DCItem(ConfigEntry *config, const shared_ptr<DataCollectionOwner>& owner, bool nxslV5) : DCObject(config, owner, nxslV5), m_unitName(config->getSubEntryValue(_T("unitName")))
{
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_transformedDataType = (BYTE)config->getSubEntryValueAsInt(_T("transformedDataType"), 0, DCI_DT_NULL);
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_prevValueTimeStamp = Timestamp::fromMilliseconds(0);
   m_prevDeltaValue = 0;
   m_cacheLoaded = false;
   m_anomalyDetected = false;
	m_multiplier = config->getSubEntryValueAsInt(_T("multiplier"));
	m_snmpRawValueType = static_cast<uint16_t>(config->getSubEntryValueAsInt(_T("snmpRawValueType")));
   m_allThresholdsRearmEvent = config->getSubEntryValueAsUInt(_T("allThresholdsRearmEvent"));
   _tcslcpy(m_predictionEngine, config->getSubEntryValue(_T("predictionEngine"), 0, _T("")), MAX_NPE_NAME_LEN);

   // for compatibility with old format
	if (config->getSubEntryValueAsInt(_T("allThresholds")))
		m_flags |= DCF_ALL_THRESHOLDS;
	if (config->getSubEntryValueAsInt(_T("rawValueInOctetString")))
		m_flags |= DCF_RAW_VALUE_OCTET_STRING;

	ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
	if (thresholdsRoot != nullptr)
	{
      unique_ptr<ObjectArray<ConfigEntry>> thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      m_thresholds = new ObjectArray<Threshold>(thresholds->size(), 8, Ownership::True);
      for(int i = 0; i < thresholds->size(); i++)
		{
			m_thresholds->add(new Threshold(thresholds->get(i), this, nxslV5));
      }
   }
   else
	{
		m_thresholds = nullptr;
	}

	updateCacheSizeInternal(false);
}

/**
 * Destructor
 */
DCItem::~DCItem()
{
	delete m_thresholds;
   clearCache();
}

/**
 * Delete all thresholds
 */
void DCItem::deleteAllThresholds()
{
	lock();
   delete_and_null(m_thresholds);
	unlock();
}

/**
 * Clear data cache
 */
void DCItem::clearCache()
{
   for(uint32_t i = 0; i < m_cacheSize; i++)
      delete m_ppValueCache[i];
   MemFree(m_ppValueCache);
   m_ppValueCache = nullptr;
   m_cacheSize = 0;
}

/**
 * Load data collection items thresholds from database
 */
bool DCItem::loadThresholdsFromDB(DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
{
   bool result = false;

	DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DCI_THRESHOLDS,
	           _T("SELECT threshold_id,fire_value,rearm_value,check_function,")
              _T("check_operation,sample_count,script,event_code,current_state,")
              _T("rearm_event_code,repeat_interval,current_severity,")
				  _T("last_event_timestamp,match_count,state_before_maint,")
				  _T("last_checked_value,last_event_message,is_disabled FROM thresholds WHERE item_id=? ")
              _T("ORDER BY sequence_number"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			if (count > 0)
			{
				m_thresholds = new ObjectArray<Threshold>(count, 8, Ownership::True);
				for(int i = 0; i < count; i++)
					m_thresholds->add(new Threshold(hResult, i, this));
			}
			DBFreeResult(hResult);
			result = true;
		}
	}
   return result;
}

/**
 * Save to database
 */
bool DCItem::saveToDatabase(DB_HANDLE hdb)
{
   static const WCHAR *columns[] = {
      L"node_id", L"template_id", L"name", L"source", L"datatype", L"polling_interval", L"retention_time",
      L"status", L"delta_calculation", L"transformation", L"description", L"instance", L"template_item_id",
      L"flags", L"resource_id", L"proxy_node", L"multiplier", L"units_name",
      L"perftab_settings", L"system_tag", L"snmp_port", L"snmp_raw_value_type", L"instd_method", L"instd_data",
      L"instd_filter", L"samples", L"comments", L"guid", L"npe_name", L"instance_retention_time",
      L"grace_period_start", L"related_object", L"polling_interval_src", L"retention_time_src",
      L"polling_schedule_type", L"retention_type", L"snmp_version", L"state_flags", L"all_rearmed_event",
      L"transformed_datatype", L"user_tag", L"thresholds_disable_end_time", nullptr
   };

	DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"items", L"item_id", m_id, columns);
	if (hStmt == nullptr)
		return false;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_ownerId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_templateId);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, MAX_ITEM_NAME - 1);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_source));
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_dataType));
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_pollingInterval);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_retentionTime);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_status));
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_deltaCalculation));
	DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, MAX_DB_STRING - 1);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_instanceName, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_templateItemId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_resourceId);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_multiplier));
	DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_unitName, DB_BIND_STATIC);
	DBBind(hStmt, 19, DB_SQLTYPE_TEXT, m_perfTabSettings, DB_BIND_STATIC);
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC, MAX_DCI_TAG_LENGTH - 1);
	DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, (INT32)m_snmpPort);
	DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, (INT32)m_snmpRawValueType);
	DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, (INT32)m_instanceDiscoveryMethod);
	DBBind(hStmt, 24, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
	DBBind(hStmt, 25, DB_SQLTYPE_TEXT, m_instanceFilterSource, DB_BIND_STATIC);
	DBBind(hStmt, 26, DB_SQLTYPE_INTEGER, (INT32)m_sampleCount);
   DBBind(hStmt, 27, DB_SQLTYPE_TEXT, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, m_predictionEngine, DB_BIND_STATIC);
   DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_instanceRetentionTime);
   DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_instanceGracePeriodStart));
   DBBind(hStmt, 32, DB_SQLTYPE_INTEGER, m_relatedObject);
   DBBind(hStmt, 33, DB_SQLTYPE_VARCHAR, m_pollingIntervalSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   DBBind(hStmt, 34, DB_SQLTYPE_VARCHAR, m_retentionTimeSrc, DB_BIND_STATIC, MAX_DB_STRING - 1);
   TCHAR pt[2], rt[2];
   pt[0] = m_pollingScheduleType + '0';
   pt[1] = 0;
   rt[0] = m_retentionType + '0';
   rt[1] = 0;
   DBBind(hStmt, 35, DB_SQLTYPE_VARCHAR, pt, DB_BIND_STATIC);
   DBBind(hStmt, 36, DB_SQLTYPE_VARCHAR, rt, DB_BIND_STATIC);
   DBBind(hStmt, 37, DB_SQLTYPE_INTEGER, m_snmpVersion);
   DBBind(hStmt, 38, DB_SQLTYPE_INTEGER, m_stateFlags);
   DBBind(hStmt, 39, DB_SQLTYPE_INTEGER, m_allThresholdsRearmEvent);
   DBBind(hStmt, 40, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_transformedDataType));
   DBBind(hStmt, 41, DB_SQLTYPE_VARCHAR, m_userTag, DB_BIND_STATIC, MAX_DCI_TAG_LENGTH - 1);
   DBBind(hStmt, 42, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_thresholdDisableEndTime));
   DBBind(hStmt, 43, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Delete non-existing thresholds
   if (getThresholdCount() > 0)
   {
      // Save thresholds
      if (success && (m_thresholds != nullptr))
      {
         for(int i = 0; (i < m_thresholds->size()) && success; i++)
            success = m_thresholds->get(i)->saveToDB(hdb, i);
      }

      if (success)
      {
         StringBuffer query(L"DELETE FROM thresholds WHERE item_id=");
         query.append(m_id);
         query.append(_T(" AND threshold_id NOT IN ("));
         for(int i = 0; i < getThresholdCount(); i++)
         {
            query.append(m_thresholds->get(i)->getId());
            query.append(_T(','));
         }
         query.shrink();
         query.append(_T(')'));
         success = DBQuery(hdb, query);
      }
   }
   else if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM thresholds WHERE item_id=?");
   }

   // Create record in raw_dci_values if needed
   if (success && !IsDatabaseRecordExist(hdb, L"raw_dci_values", L"item_id", m_id))
   {
      hStmt = DBPrepare(hdb, L"INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time,cache_timestamp,anomaly_detected) VALUES (?,?,?,?,?)");
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_prevRawValue.getString(), DB_BIND_STATIC, 255);
         DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_prevValueTimeStamp);
         DBBind(hStmt, 4, DB_SQLTYPE_BIGINT, (m_cacheLoaded  && (m_cacheSize > 0)) ? m_ppValueCache[m_cacheSize - 1]->getTimeStamp() : Timestamp::fromMilliseconds(0));
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_anomalyDetected ? L"1" : L"0", DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   unlock();
	return success ? DCObject::saveToDatabase(hdb) : false;
}

/**
 * Check last value for threshold violations. Original DCI is a pointer to source DCI
 * if threshold check is performed on shadow copy or nullptr otherwise.
 */
void DCItem::checkThresholds(ItemValue &value, const shared_ptr<DCObject>& originalDci)
{
	if (m_thresholds == nullptr)
		return;

	auto owner = m_owner.lock();

   bool hasActiveThresholds = false;
	bool thresholdDeactivated = false;
	bool resendActivationEvent = false;
   for(int i = 0; i < m_thresholds->size(); i++)
   {
		Threshold *t = m_thresholds->get(i);
		uint32_t thresholdId = t->getId();
      ItemValue checkValue, thresholdValue;
      ThresholdCheckResult result = t->check(value, m_ppValueCache, checkValue, thresholdValue, owner, this);
      t->setLastCheckedValue(checkValue);
      switch(result)
      {
         case ThresholdCheckResult::ACTIVATED:
            {
               shared_ptr<DCObject> sharedThis((originalDci != nullptr) ? originalDci : shared_from_this());
               EventBuilder(t->getEventCode(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("thresholdValue"), thresholdValue.getString())
                  .param(_T("currentValue"), checkValue.getString())
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("instance"), m_instanceName)
                  .param(_T("isRepeatedEvent"), _T("0"))
                  .param(_T("dciValue"), value.getString())
                  .param(_T("operation"), t->getOperation())
                  .param(_T("function"), t->getFunction())
                  .param(_T("pollCount"), t->getSampleCount())
                  .param(_T("thresholdDefinition"), t->getTextualDefinition())
                  .param(_T("instanceValue"), m_instanceDiscoveryData)
                  .param(_T("instanceName"), m_instanceName)
                  .param(_T("thresholdId"), thresholdId)
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_thresholds->size();  // Stop processing
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            hasActiveThresholds = true;
            break;
         case ThresholdCheckResult::DEACTIVATED:
            {
               shared_ptr<DCObject> sharedThis((originalDci != nullptr) ? originalDci : shared_from_this());
               EventBuilder(t->getRearmEventCode(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("instance"), m_instanceName)
                  .param(_T("thresholdValue"), thresholdValue.getString())
                  .param(_T("currentValue"), checkValue.getString())
                  .param(_T("dciValue"), value.getString())
                  .param(_T("operation"), t->getOperation())
                  .param(_T("function"), t->getFunction())
                  .param(_T("pollCount"), t->getSampleCount())
                  .param(_T("thresholdDefinition"), t->getTextualDefinition())
                  .param(_T("instanceValue"), m_instanceDiscoveryData)
                  .param(_T("instanceName"), m_instanceName)
                  .param(_T("thresholdId"), thresholdId)
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               // this flag used to re-send activation event for next active threshold
               resendActivationEvent = true;
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            thresholdDeactivated = true;
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
	            time_t now = time(nullptr);
	            time_t repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : static_cast<time_t>(t->getRepeatInterval());
				   if (resendActivationEvent || ((repeatInterval != 0) && (t->getLastEventTimestamp() + repeatInterval <= now)))
				   {
	               shared_ptr<DCObject> sharedThis((originalDci != nullptr) ? originalDci : shared_from_this());
		            EventBuilder(t->getEventCode(), m_ownerId)
		               .dci(m_id)
		               .param(_T("dciName"), m_name)
		               .param(_T("dciDescription"), m_description)
		               .param(_T("thresholdValue"), thresholdValue.getString())
		               .param(_T("currentValue"), checkValue.getString())
		               .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
		               .param(_T("instance"), m_instanceName)
		               .param(_T("isRepeatedEvent"), _T("1"))
		               .param(_T("dciValue"), value.getString())
		               .param(_T("operation"), t->getOperation())
		               .param(_T("function"), t->getFunction())
		               .param(_T("pollCount"), t->getSampleCount())
		               .param(_T("thresholdDefinition"), t->getTextualDefinition())
	                  .param(_T("instanceValue"), m_instanceDiscoveryData)
	                  .param(_T("instanceName"), m_instanceName)
	                  .param(_T("thresholdId"), thresholdId)
		               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
				   }
            }
				if (!(m_flags & DCF_ALL_THRESHOLDS))
				{
					i = m_thresholds->size();  // Threshold condition still true, stop processing
				}
				resendActivationEvent = false;
            hasActiveThresholds = true;
            break;
         default:
            break;
      }
   }

   if (thresholdDeactivated && !hasActiveThresholds && (m_allThresholdsRearmEvent != 0))
   {
      EventBuilder(m_allThresholdsRearmEvent, m_ownerId)
         .dci(m_id)
         .param(_T("dciName"), m_name)
         .param(_T("dciDescription"), m_description)
         .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("instance"), m_instanceName)
         .param(_T("dciValue"), value.getString())
         .param(_T("instanceValue"), m_instanceDiscoveryData)
         .param(_T("instanceName"), m_instanceName)
         .post();
   }
}

/**
 * Create NXCP message with item data
 */
void DCItem::createMessage(NXCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
   pMsg->setField(VID_DCI_DATA_TYPE, static_cast<uint16_t>(m_dataType));
   pMsg->setField(VID_TRANSFORMED_DATA_TYPE, static_cast<uint16_t>(m_transformedDataType));
   pMsg->setField(VID_DCI_DELTA_CALCULATION, static_cast<uint16_t>(m_deltaCalculation));
   pMsg->setField(VID_SAMPLE_COUNT, static_cast<uint16_t>(m_sampleCount));
	pMsg->setField(VID_MULTIPLIER, static_cast<uint32_t>(m_multiplier));
	pMsg->setField(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	pMsg->setField(VID_NPE_NAME, m_predictionEngine);
   pMsg->setField(VID_UNITS_NAME, m_unitName);
   pMsg->setField(VID_DEACTIVATION_EVENT, m_allThresholdsRearmEvent);
	if (m_thresholds != nullptr)
	{
		pMsg->setField(VID_NUM_THRESHOLDS, static_cast<uint32_t>(m_thresholds->size()));
		uint32_t fieldId = VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < m_thresholds->size(); i++, fieldId += 20)
			m_thresholds->get(i)->fillMessage(pMsg, fieldId);
	}
	else
	{
		pMsg->setField(VID_NUM_THRESHOLDS, static_cast<uint32_t>(0));
	}
   unlock();
}

/**
 * Delete item and collected data from database
 */
void DCItem::deleteFromDatabase()
{
	DCObject::deleteFromDatabase();

   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM items WHERE item_id=%u"), m_id);
   QueueSQLRequest(query);
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM thresholds WHERE item_id=%u"), m_id);
   QueueSQLRequest(query);
   QueueRawDciDataDelete(m_id);

   auto owner = m_owner.lock();
   if ((owner != nullptr) && owner->isDataCollectionTarget() && (g_dbSyntax != DB_SYNTAX_TSDB))
      static_cast<DataCollectionTarget*>(owner.get())->scheduleItemDataCleanup(m_id);
}

/**
 * Update item from NXCP message
 */
void DCItem::updateFromMessage(const NXCPMessage& msg, uint32_t *numMaps, uint32_t **mapIndex, uint32_t **mapId)
{
	DCObject::updateFromMessage(msg);

   lock();

   m_dataType = (BYTE)msg.getFieldAsUInt16(VID_DCI_DATA_TYPE);
   m_transformedDataType = (BYTE)msg.getFieldAsUInt16(VID_TRANSFORMED_DATA_TYPE);
   m_deltaCalculation = (BYTE)msg.getFieldAsUInt16(VID_DCI_DELTA_CALCULATION);
	m_sampleCount = msg.getFieldAsInt16(VID_SAMPLE_COUNT);
	m_multiplier = msg.getFieldAsInt32(VID_MULTIPLIER);
	m_unitName = msg.getFieldAsSharedString(VID_UNITS_NAME);
	m_snmpRawValueType = msg.getFieldAsUInt16(VID_SNMP_RAW_VALUE_TYPE);
	m_allThresholdsRearmEvent = msg.getFieldAsUInt32(VID_DEACTIVATION_EVENT);
   msg.getFieldAsString(VID_NPE_NAME, m_predictionEngine, MAX_NPE_NAME_LEN);

   // Update thresholds
   uint32_t numThresholds = msg.getFieldAsUInt32(VID_NUM_THRESHOLDS);
   Buffer<uint32_t> newThresholdIds(numThresholds);
   *mapIndex = MemAllocArray<uint32_t>(numThresholds);
   *mapId = MemAllocArray<uint32_t>(numThresholds);
   *numMaps = 0;

   // Read all new threshold ids from message
   for(uint32_t i = 0, fieldId = VID_DCI_THRESHOLD_BASE; i < numThresholds; i++, fieldId += 10)
   {
      newThresholdIds[i] = msg.getFieldAsUInt32(fieldId);
   }

   // Check if some thresholds was deleted, and reposition others if needed
   Buffer<Threshold*> newThresholdObjects(numThresholds);
   for(int i = 0; i < getThresholdCount(); i++)
   {
      uint32_t j;
      for(j = 0; j < numThresholds; j++)
         if (m_thresholds->get(i)->getId() == newThresholdIds[j])
            break;
      if (j == numThresholds)
      {
         // No threshold with that id in new list, delete it
			m_thresholds->remove(i);
         i--;
      }
      else
      {
         // Move existing thresholds to appropriate positions in new list
         newThresholdObjects[j] = m_thresholds->get(i);
      }
   }

   // Add or update thresholds
   for(uint32_t i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < numThresholds; i++, dwId += 10)
   {
      if (newThresholdIds[i] == 0)    // New threshold?
      {
         newThresholdObjects[i] = new Threshold(this);
         newThresholdObjects[i]->createId();

         // Add index -> id mapping
         (*mapIndex)[*numMaps] = i;
         (*mapId)[*numMaps] = newThresholdObjects[i]->getId();
         (*numMaps)++;
      }
      if (newThresholdObjects[i] != nullptr)
         newThresholdObjects[i]->updateFromMessage(msg, dwId);
   }

	if (numThresholds > 0)
	{
		if (m_thresholds != nullptr)
		{
			m_thresholds->setOwner(Ownership::False);
			m_thresholds->clear();
			m_thresholds->setOwner(Ownership::True);
		}
		else
		{
			m_thresholds = new ObjectArray<Threshold>((int)numThresholds, 8, Ownership::True);
		}
		for(uint32_t i = 0; i < numThresholds; i++)
      {
         if (newThresholdObjects[i] != nullptr)
			   m_thresholds->add(newThresholdObjects[i]);
      }
	}
	else
	{
		delete_and_null(m_thresholds);
	}

	// Update data type in thresholds
   for(int i = 0; i < getThresholdCount(); i++)
      m_thresholds->get(i)->setDataType(getTransformedDataType());

   updateCacheSizeInternal(true);
   unlock();
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 * If allowPastDataPoints is false, data points with timestamp older than last stored one
 * will be rejected.
 *
 * @return true on success
 */
bool DCItem::processNewValue(Timestamp timestamp, const wchar_t *originalValue, bool *updateStatus, bool allowPastDataPoints)
{
   ItemValue rawValue, *pValue;

   *updateStatus = false;

   lock();

   auto owner = m_owner.lock();

   // Normally m_owner shouldn't be NULL for polled items, but who knows...
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

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue(originalValue, timestamp, false);
   if (m_prevValueTimeStamp.isNull())
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(*pValue, (timestamp > m_prevValueTimeStamp) ? (timestamp - m_prevValueTimeStamp) : 0))
      {
         unlock();
         delete pValue;
         return false;
      }
   }

   m_errorCount = 0;

   if (isStatusDCO() && (timestamp > m_prevValueTimeStamp) && ((m_cacheSize == 0) || !m_cacheLoaded || (pValue->getUInt32() != m_ppValueCache[0]->getUInt32())))
   {
      *updateStatus = true;
   }

   if (timestamp > m_prevValueTimeStamp)
   {
      if (m_flags & DCF_DETECT_ANOMALIES)
      {
         m_anomalyDetected =
                  IsAnomalousValue(static_cast<DataCollectionTarget&>(*owner), *this, pValue->getDouble(), 0.75, 1, 30, 60) &&
                  IsAnomalousValue(static_cast<DataCollectionTarget&>(*owner), *this, pValue->getDouble(), 0.75, 7, 10, 60);
         nxlog_debug_tag(DEBUG_TAG_DC_POLLER, 6, _T("Value %f is %s"), pValue->getDouble(), m_anomalyDetected ? _T("an anomaly") : _T("not an anomaly"));
      }

      m_prevRawValue = rawValue;
      m_prevValueTimeStamp = timestamp;

      // Save raw value into database
      QueueRawDciDataUpdate(timestamp, m_id, originalValue, pValue->getString(),
         (m_cacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[m_cacheSize - 1]->getTimeStamp() : Timestamp::fromMilliseconds(0), m_anomalyDetected);
   }

	// Check if user wants to collect all values or only changed values.
   if (!isStoreChangesOnly() || (m_cacheLoaded && (m_cacheSize > 0) && wcscmp(pValue->getString(), m_ppValueCache[0]->getString())))
   {
      // Save transformed value to database
      if (m_retentionType != DC_RETENTION_NONE)
           QueueIDataInsert(timestamp, owner->getId(), m_id, originalValue, pValue->getString(), getStorageClass());

      if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      {
         // Operate on copy with this DCI unlocked to prevent deadlock if fan-out driver access owning object
         DCItem copy(this, false, false);
         unlock();
         PerfDataStorageRequest(&copy, timestamp, pValue->getString());
         lock();
      }
   }

   // Update prediction engine
   if (m_predictionEngine[0] != 0)
   {
      PredictionEngine *engine = FindPredictionEngine(m_predictionEngine);
      if (engine != nullptr)
         engine->update(owner->getId(), m_id, getStorageClass(), timestamp, pValue->getDouble());
   }

   // Check thresholds
   Timestamp now = Timestamp::now();
   if (m_cacheLoaded && (timestamp >= m_prevValueTimeStamp) &&
       ((m_thresholdDisableEndTime == 0) || (m_thresholdDisableEndTime > 0 && m_thresholdDisableEndTime < timestamp.asTime())) &&
       ((g_offlineDataRelevanceTime <= 0) || (timestamp > (now - g_offlineDataRelevanceTime))))
   {
      if (hasScriptThresholds())
      {
         // Run threshold check with DCI unlocked if there are script thresholds
         // to avoid possible server deadlock if script causes agent reconnect
         // Shadow copy has to be shared one because threshold check may require
         // shared pointer to DCI for additional background processing
         shared_ptr<DCItem> shadowCopy = make_shared<DCItem>(this, true);
         unlock();
         shadowCopy->checkThresholds(*pValue, shared_from_this());
         lock();

         // Reconcile threshold updates
         for(int i = 0; i < shadowCopy->m_thresholds->size(); i++)
         {
            Threshold *src = shadowCopy->m_thresholds->get(i);
            Threshold *dst = getThresholdById(src->getId());
            if (dst != nullptr)
            {
               dst->reconcile(*src);
            }
         }
      }
      else
      {
         checkThresholds(*pValue, nullptr);
      }

      if ((m_thresholdDisableEndTime > 0) && (m_thresholdDisableEndTime < now.asTime()))
      {
         // Thresholds were disabled, reset disable end time
         m_thresholdDisableEndTime = 0;
         getOwner()->markAsModified(MODIFY_DATA_COLLECTION);
      }
   }

   // If DCI is related to interface and marked as inbound or outbound traffic indicator, update interface utilization
   if ((m_relatedObject != 0) && !m_systemTag.isEmpty() && (m_systemTag.str().startsWith(_T("iface-inbound-")) || m_systemTag.str().startsWith(_T("iface-outbound-"))))
   {
      int64_t value = pValue->getInt64();
      if (value >= 0)
      {
         shared_ptr<Interface> iface = static_pointer_cast<Interface>(FindObjectById(m_relatedObject, OBJECT_INTERFACE));
         if ((iface != nullptr) && (iface->getSpeed() > 0))
         {
            String tag(m_systemTag.str());
            ThreadPoolExecute(g_mainThreadPool,
               [iface, value, tag] () -> void
               {
                  int32_t u;
                  if (tag.endsWith(_T("-util")))
                  {
                     u = static_cast<int32_t>(value) * 10;
                  }
                  else
                  {
                     uint64_t speed = iface->getSpeed();
                     if (speed > 0)
                     {
                        u = static_cast<int32_t>((tag.endsWith(_T("-bytes")) ? value * 8 : value) * 1000 / iface->getSpeed());
                     }
                     else
                     {
                        u = 0;
                     }
                  }
                  if (u > 1000)
                     u = 1000;
                  if (tag.startsWith(_T("iface-inbound-")))
                     iface->setInboundUtilization(u);
                  else
                     iface->setOutboundUtilization(u);
               });
         }
      }
   }

   // Update cache
   if ((m_cacheSize > 0) && (timestamp >= m_prevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = pValue;
      m_lastValueTimestamp = timestamp;
   }
   else if (!m_cacheLoaded && (m_requiredCacheSize == 1))
   {
      // If required cache size is 1 and we got value before cache loader
      // loads DCI cache then update it directly
      for(uint32_t i = 0; i < m_cacheSize; i++)
         delete m_ppValueCache[i];

      if (m_cacheSize != m_requiredCacheSize)
      {
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
         m_cacheSize = m_requiredCacheSize;
      }

      m_ppValueCache[0] = pValue;
      m_cacheLoaded = true;
      m_lastValueTimestamp = timestamp;
   }
   else
   {
      delete pValue;
   }

   unlock();

   return true;
}

/**
 * Process new data collection error
 */
void DCItem::processNewError(bool noInstance, Timestamp timestamp)
{
   lock();

   m_errorCount++;

   bool hasActiveThresholds = false;
   bool thresholdDeactivated = false;
	for(int i = 0; i < getThresholdCount(); i++)
   {
		Threshold *t = m_thresholds->get(i);
		uint32_t thresholdId = t->getId();
      ThresholdCheckResult result = t->checkError(m_errorCount);
      switch(result)
      {
         case ThresholdCheckResult::ACTIVATED:
            {
               shared_ptr<DCObject> sharedThis(shared_from_this());
               EventBuilder(t->getEventCode(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("thresholdValue"), _T(""))
                  .param(_T("currentValue"), _T(""))
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("instance"), m_instanceName)
                  .param(_T("isRepeatedEvent"), _T("0"))
                  .param(_T("dciValue"), _T(""))
                  .param(_T("operation"), t->getOperation())
                  .param(_T("function"), t->getFunction())
                  .param(_T("pollCount"), t->getSampleCount())
                  .param(_T("thresholdDefinition"), t->getTextualDefinition())
                  .param(_T("instanceValue"), m_instanceDiscoveryData)
                  .param(_T("instanceName"), m_instanceName)
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               i = m_thresholds->size();  // Stop processing
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            hasActiveThresholds = true;
            break;
         case ThresholdCheckResult::DEACTIVATED:
            {
               shared_ptr<DCObject> sharedThis(shared_from_this());
               EventBuilder(t->getRearmEventCode(), m_ownerId)
                  .dci(m_id)
                  .param(_T("dciName"), m_name)
                  .param(_T("dciDescription"), m_description)
                  .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("instance"), m_instanceName)
                  .param(_T("thresholdValue"), _T(""))
                  .param(_T("currentValue"), _T(""))
                  .param(_T("dciValue"), _T(""))
                  .param(_T("operation"), t->getOperation())
                  .param(_T("function"), t->getFunction())
                  .param(_T("pollCount"), t->getSampleCount())
                  .param(_T("thresholdDefinition"), t->getTextualDefinition())
                  .param(_T("instanceValue"), m_instanceDiscoveryData)
                  .param(_T("instanceName"), m_instanceName)
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            thresholdDeactivated = true;
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
               time_t repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : static_cast<time_t>(t->getRepeatInterval());
				   if ((repeatInterval != 0) && (t->getLastEventTimestamp() + repeatInterval < timestamp.asTime()))
				   {
	               shared_ptr<DCObject> sharedThis(shared_from_this());
		            EventBuilder(t->getEventCode(), m_ownerId)
		               .dci(m_id)
		               .param(_T("dciName"), m_name)
		               .param(_T("dciDescription"), m_description)
		               .param(_T("thresholdValue"), _T(""))
		               .param(_T("currentValue"), _T(""))
		               .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
		               .param(_T("instance"), m_instanceName)
		               .param(_T("isRepeatedEvent"), _T("1"))
		               .param(_T("dciValue"), _T(""))
		               .param(_T("operation"), t->getOperation())
		               .param(_T("function"), t->getFunction())
		               .param(_T("pollCount"), t->getSampleCount())
		               .param(_T("thresholdDefinition"), t->getTextualDefinition())
	                  .param(_T("instanceValue"), m_instanceDiscoveryData)
	                  .param(_T("instanceName"), m_instanceName)
		               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
				   }
            }
				if (!(m_flags & DCF_ALL_THRESHOLDS))
				{
					i = m_thresholds->size();  // Threshold condition still true, stop processing
				}
            hasActiveThresholds = true;
            break;
         default:
            break;
      }
   }

   if (thresholdDeactivated && !hasActiveThresholds && (m_allThresholdsRearmEvent != 0))
   {
      EventBuilder(m_allThresholdsRearmEvent, m_ownerId)
         .dci(m_id)
         .param(_T("dciName"), m_name)
         .param(_T("dciDescription"), m_description)
         .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("instance"), m_instanceName)
         .param(_T("dciValue"), _T(""))
         .param(_T("instanceValue"), m_instanceDiscoveryData)
         .param(_T("instanceName"), m_instanceName)
         .post();
   }

   unlock();
}

/**
 * Save information about threshold state before maintenance
 */
void DCItem::saveStateBeforeMaintenance()
{
   lock();
   for(int i = 0; i < getThresholdCount(); i++)
   {
      m_thresholds->get(i)->saveStateBeforeMaintenance();
   }
   unlock();
}

/**
 * Generate events that persist after maintenance
 */
void DCItem::generateEventsAfterMaintenance()
{
   lock();
   int trCount = getThresholdCount() - 1;
   uint32_t ownerId = m_owner.lock()->getId();
   for(int i = trCount; i >= 0; i--)   // go backwards to generate events in correct order
   {
      Threshold *t = m_thresholds->get(i);
      if (t->isReached() != t->wasReachedBeforeMaintenance())
      {
         uint32_t thresholdId = t->getId();
         shared_ptr<DCObject> sharedThis(shared_from_this());
         if (t->isReached())
         {
            EventBuilder(t->getEventCode(), ownerId)
               .dci(m_id)
               .param(_T("dciName"), m_name)
               .param(_T("dciDescription"), m_description)
               .param(_T("thresholdValue"), t->getStringValue())
               .param(_T("currentValue"), t->getLastCheckValue().getString())
               .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("instance"), m_instanceName)
               .param(_T("isRepeatedEvent"), _T("0"))
               .param(_T("dciValue"), (m_cacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""))
               .param(_T("operation"), t->getOperation())
               .param(_T("function"), t->getFunction())
               .param(_T("pollCount"), t->getSampleCount())
               .param(_T("thresholdDefinition"), t->getTextualDefinition())
               .param(_T("instanceValue"), m_instanceDiscoveryData)
               .param(_T("instanceName"), m_instanceName)
               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
         }
         else
         {
            EventBuilder(t->getRearmEventCode(), ownerId)
               .dci(m_id)
               .param(_T("dciName"), m_name)
               .param(_T("dciDescription"), m_description)
               .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("instance"), m_instanceName)
               .param(_T("thresholdValue"), t->getStringValue())
               .param(_T("currentValue"), t->getLastCheckValue().getString())
               .param(_T("dciValue"), (m_cacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""))
               .param(_T("operation"), t->getOperation())
               .param(_T("function"), t->getFunction())
               .param(_T("pollCount"), t->getSampleCount())
               .param(_T("thresholdDefinition"), t->getTextualDefinition())
               .param(_T("instanceValue"), m_instanceDiscoveryData)
               .param(_T("instanceName"), m_instanceName)
               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
         }
      }
   }
   unlock();
}

/**
 * Convert DCI value to given data type
 */
static inline void ConvertValue(ItemValue& value, int destinationDataType, int sourceDataType)
{
   switch(destinationDataType)
   {
      case DCI_DT_INT:
         value.set(value.getInt32());
         break;
      case DCI_DT_INT64:
         // Other data types are good without conversion
         if ((sourceDataType == DCI_DT_STRING) || (sourceDataType == DCI_DT_FLOAT))
            value.set(value.getInt64());
         break;
      case DCI_DT_UINT:
      case DCI_DT_COUNTER32:
         value.set(value.getUInt32());
         break;
      case DCI_DT_UINT64:
      case DCI_DT_COUNTER64:
         value.set(value.getUInt64());
         break;
      case DCI_DT_FLOAT:
         // Other data types are good without conversion
         if (sourceDataType == DCI_DT_STRING)
            value.set(value.getDouble());
         break;
   }
}

/**
 * Transform received value. Assuming that DCI object is locked.
 */
bool DCItem::transform(ItemValue &value, time_t elapsedTime)
{
   if ((m_transformationScript == nullptr) && !m_transformationScriptSource.isNull() && !IsBlankString(m_transformationScriptSource))
      return false;  // Transformation script present but cannot be compiled

   bool success = true;
   int deltaDataType = m_dataType;

   switch(m_deltaCalculation)
   {
      case DCM_SIMPLE:
         switch(m_dataType)
         {
            case DCI_DT_INT:
               value = value.getInt32() - m_prevRawValue.getInt32();
               break;
            case DCI_DT_UINT:
               value = value.getUInt32() - m_prevRawValue.getUInt32();
               break;
            case DCI_DT_COUNTER32:
               if (value.getUInt32() >= m_prevRawValue.getUInt32())
               {
                  uint32_t delta = value.getUInt32() - m_prevRawValue.getUInt32();
                  value = delta;
                  m_prevDeltaValue = delta;
               }
               else
               {
                  // discontinuity: counter reset or wrap around
                  uint32_t delta = value.getUInt32() - m_prevRawValue.getUInt32();  // unsigned math
                  if (delta > static_cast<uint32_t>(m_prevDeltaValue))
                  {
                     // Value (probably) too big, return error
                     success = false;
                     m_prevRawValue = value;
                  }
                  else
                  {
                     // Assume counter wrap around, value looks valid
                     value = delta;
                     m_prevDeltaValue = delta;
                  }
               }
               break;
            case DCI_DT_INT64:
               value = value.getInt64() - m_prevRawValue.getInt64();
               break;
            case DCI_DT_UINT64:
               value = value.getUInt64() - m_prevRawValue.getUInt64();
               break;
            case DCI_DT_COUNTER64:
               // assume counter reset if new value is less then previous
               if (value.getUInt64() >= m_prevRawValue.getUInt64())
               {
                  m_prevDeltaValue = value.getUInt64() - m_prevRawValue.getUInt64();
                  value = m_prevDeltaValue;
               }
               else
               {
                  // discontinuity: counter reset or wrap around
                  uint64_t delta = value.getUInt64() - m_prevRawValue.getUInt64();  // unsigned math
                  if (delta > m_prevDeltaValue)
                  {
                     // Value (probably) too big, return error
                     success = false;
                     m_prevRawValue = value;
                  }
                  else
                  {
                     // Assume counter wrap around, value looks valid
                     value = delta;
                     m_prevDeltaValue = delta;
                  }
               }
               break;
            case DCI_DT_FLOAT:
               value = value.getDouble() - m_prevRawValue.getDouble();
               break;
            case DCI_DT_STRING:
               value = static_cast<int32_t>((_tcscmp(value.getString(), m_prevRawValue.getString()) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      case DCM_AVERAGE_PER_MINUTE:
         elapsedTime /= 60;  // Convert to minutes
         /* no break */
      case DCM_AVERAGE_PER_SECOND:
         // Check elapsed time to prevent divide-by-zero exception
         if (elapsedTime == 0)
            elapsedTime++;

         deltaDataType = DCI_DT_FLOAT;  // Result will be in float type
         switch(m_dataType)
         {
            case DCI_DT_INT:
               value = static_cast<double>(value.getInt32() - m_prevRawValue.getInt32()) / static_cast<double>(elapsedTime);
               break;
            case DCI_DT_UINT:
               value = static_cast<double>(value.getUInt32() - m_prevRawValue.getUInt32()) / static_cast<double>(elapsedTime);
               break;
            case DCI_DT_COUNTER32:
               if (value.getUInt32() >= m_prevRawValue.getUInt32())
               {
                  uint32_t delta = value.getUInt32() - m_prevRawValue.getUInt32();
                  value = static_cast<double>(delta) / static_cast<double>(elapsedTime);
                  m_prevDeltaValue = delta;
               }
               else
               {
                  // discontinuity: counter reset or wrap around
                  uint32_t delta = value.getUInt32() - m_prevRawValue.getUInt32();  // unsigned math
                  nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, L"32-bit counter reset or rollover detected; DCI=\"%s\" [%u]; node=\"%s\" [%u]; new value=%u old value=%u delta=%u prev delta=%u",
                            m_description.cstr(), m_id, getOwnerName(), getOwnerId(), value.getUInt32(), m_prevRawValue.getUInt32(), delta, static_cast<uint32_t>(m_prevDeltaValue));
                  if (delta > static_cast<uint32_t>(m_prevDeltaValue))
                  {
                     // Value (probably) too big, return error
                     success = false;
                     m_prevRawValue = value;
                  }
                  else
                  {
                     // Assume counter wrap around, value looks valid
                     value = static_cast<double>(delta) / static_cast<double>(elapsedTime);
                     m_prevDeltaValue = delta;
                  }
               }
               break;
            case DCI_DT_INT64:
               value = static_cast<double>(value.getInt64() - m_prevRawValue.getInt64()) / static_cast<double>(elapsedTime);
               break;
            case DCI_DT_UINT64:
               value = static_cast<double>(value.getUInt64() - m_prevRawValue.getUInt64()) / static_cast<double>(elapsedTime);
               break;
            case DCI_DT_COUNTER64:
               // assume counter reset if new value is less then previous
               if (value.getUInt64() >= m_prevRawValue.getUInt64())
               {
                  m_prevDeltaValue = value.getUInt64() - m_prevRawValue.getUInt64();
                  value = static_cast<double>(m_prevDeltaValue) / static_cast<double>(elapsedTime);
               }
               else
               {
                  // discontinuity: counter reset or wrap around
                  uint64_t delta = value.getUInt64() - m_prevRawValue.getUInt64();  // unsigned math
                  nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, L"64-bit counter reset or rollover detected; DCI=\"%s\" [%u]; node=\"%s\" [%u]; new value=" UINT64_FMT L" old value=" UINT64_FMT L" delta=" UINT64_FMT L" prev delta=" UINT64_FMT,
                            m_description.cstr(), m_id, getOwnerName(), getOwnerId(), value.getUInt64(), m_prevRawValue.getUInt64(), delta, m_prevDeltaValue);
                  if (delta > m_prevDeltaValue)
                  {
                     // Value (probably) too big, return error
                     success = false;
                     m_prevRawValue = value;
                  }
                  else
                  {
                     // Assume counter wrap around, value looks valid
                     value = static_cast<double>(delta) / static_cast<double>(elapsedTime);
                     m_prevDeltaValue = delta;
                  }
               }
               break;
            case DCI_DT_FLOAT:
               value = (value.getDouble() - m_prevRawValue.getDouble()) / static_cast<double>(elapsedTime);
               break;
            case DCI_DT_STRING:
               // I don't see any meaning in _T("average delta per second (minute)") for string
               // values, so result will be 0 if there are no difference between
               // current and previous values, and 1 otherwise
               value = static_cast<int32_t>((_tcscmp(value.getString(), m_prevRawValue.getString()) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      default:    // Default is no transformation
         break;
   }

   if (!success)
      return false;

   if (m_transformationScript != nullptr)
   {
      ScriptVMHandle vm = CreateServerScriptVM(m_transformationScript.get(), m_owner.lock(), createDescriptorInternal());
      if (vm.isValid())
      {
         NXSL_Value *nxslValue = vm->createValue(value.getString());
         if (nxslValue->isNumeric() && (m_dataType != DCI_DT_STRING))
            nxslValue->convert(getNXSLDataType()); // make sure that input NXSL variable type is the same as DCI type

         // remove lock from DCI for script execution to avoid deadlocks
         unlock();
         success = vm->run(1, &nxslValue);
         lock();
         if (success)
         {
            nxslValue = vm->getResult();
            if (nxslValue->isGuid())
            {
               // Check for special return codes
               if (NXSLExitCodeToDCE(nxslValue->getValueAsGuid()) != DCE_SUCCESS)
                  success = false;
            }
            if (!nxslValue->isNull() && success)
            {
               switch(getTransformedDataType())
               {
                  case DCI_DT_INT:
                     value.set(nxslValue->getValueAsInt32());
                     break;
                  case DCI_DT_UINT:
                  case DCI_DT_COUNTER32:
                     value.set(nxslValue->getValueAsUInt32());
                     break;
                  case DCI_DT_INT64:
                     value.set(nxslValue->getValueAsInt64());
                     break;
                  case DCI_DT_UINT64:
                  case DCI_DT_COUNTER64:
                     value.set(nxslValue->getValueAsUInt64());
                     break;
                  case DCI_DT_FLOAT:
                     value.set(nxslValue->getValueAsReal());
                     break;
                  case DCI_DT_STRING:
                     value.set(nxslValue->getValueAsCString());
                     break;
                  default:
                     break;
               }
            }
         }
         else if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, L"Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted",
                      m_description.cstr(), m_id, getOwnerName(), getOwnerId());
         }
         else
         {
            time_t now = time(nullptr);
            if (m_lastScriptErrorReport + ConfigReadInt(L"DataCollection.ScriptErrorReportInterval", 86400) < now)
            {
               ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, vm->getErrorText(), L"DCI::%s::%d::TransformationScript", getOwnerName(), m_id);
               nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, L"Failed to execute transformation script for object %s [%u] DCI %s [%u] (%s)",
                        getOwnerName(), getOwnerId(), m_name.cstr(), m_id, vm->getErrorText());
               m_lastScriptErrorReport = now;
            }
         }
         vm.destroy();
      }
      else if (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         if (getTransformedDataType() != m_dataType)
            ConvertValue(value, getTransformedDataType(), m_dataType);
      }
      else
      {
         time_t now = time(nullptr);
         if (m_lastScriptErrorReport + ConfigReadInt(L"DataCollection.ScriptErrorReportInterval", 86400) < now)
         {
            ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, L"Script load failed", L"DCI::%s::%d::TransformationScript", getOwnerName(), m_id);
            nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, L"Failed to load transformation script for object %s [%u] DCI %s [%u]",
                     getOwnerName(), getOwnerId(), m_name.cstr(), m_id);
            m_lastScriptErrorReport = now;
         }
         success = false;
      }
   }
   else if (getTransformedDataType() != deltaDataType)
   {
      ConvertValue(value, getTransformedDataType(), deltaDataType);
   }
   return success;
}

/**
 * Get DCI data type as NXSL data type
 */
int DCItem::getNXSLDataType() const
{
   static int nxslTypes[] = { NXSL_DT_INT32, NXSL_DT_UINT32, NXSL_DT_INT64, NXSL_DT_UINT64, NXSL_DT_STRING, NXSL_DT_REAL, NXSL_DT_NULL, NXSL_DT_UINT32, NXSL_DT_UINT64 };
   return (m_dataType < sizeof(nxslTypes) / sizeof(int)) ? nxslTypes[m_dataType] : NXSL_DT_STRING;
}

/**
 * Set new ID and node/template association
 */
void DCItem::changeBinding(uint32_t newId, shared_ptr<DataCollectionOwner> newOwner, bool doMacroExpansion)
{
	DCObject::changeBinding(newId, newOwner, doMacroExpansion);

   lock();
	if (newId != 0)
	{
		for(int i = 0; i < getThresholdCount(); i++)
         m_thresholds->get(i)->bindToItem(m_id, m_ownerId);
	}

   clearCache();
   updateCacheSizeInternal(true);
   unlock();
}

/**
 * Update required cache size depending on thresholds
 */
void DCItem::updateCacheSizeInternal(bool allowLoad)
{
   auto owner = m_owner.lock();

   // Sanity check
   if (owner == nullptr)
   {
      nxlog_debug(3, _T("DCItem::updateCacheSize() called for DCI %d when m_owner == nullptr"), m_id);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if (((owner->isDataCollectionTarget() && (owner->getObjectClass() != OBJECT_CLUSTER)) ||
        ((owner->getObjectClass() == OBJECT_CLUSTER) && isAggregateOnCluster())) &&
       (m_instanceDiscoveryMethod == IDM_NONE))
   {
      uint32_t requiredSize = 1;

      // Calculate required cache size
      for(int i = 0; i < getThresholdCount(); i++)
         if (requiredSize < m_thresholds->get(i)->getRequiredCacheSize())
            requiredSize = m_thresholds->get(i)->getRequiredCacheSize();

		unique_ptr<SharedObjectArray<NetObj>> conditions = g_idxConditionById.getObjects();
		for(int i = 0; i < conditions->size(); i++)
      {
		   ConditionObject *c = static_cast<ConditionObject*>(conditions->get(i));
			uint32_t size = c->getCacheSizeForDCI(m_id);
         if (size > requiredSize)
            requiredSize = size;
      }

		m_requiredCacheSize = requiredSize;
   }
   else
   {
      m_requiredCacheSize = 0;
   }

   nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 8, _T("DCItem::updateCacheSizeInternal(dci=\"%s\", node=%s [%d]): requiredSize=%d cacheSize=%d"),
            m_name.cstr(), owner->getName(), owner->getId(), m_requiredCacheSize, m_cacheSize);

   // Update cache if needed
   if (m_requiredCacheSize < m_cacheSize)
   {
      // Destroy unneeded values
      if (m_cacheSize > 0)
		{
         for(uint32_t i = m_requiredCacheSize; i < m_cacheSize; i++)
            delete m_ppValueCache[i];
		}

      m_cacheSize = m_requiredCacheSize;
      if (m_cacheSize > 0)
      {
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_cacheSize);
      }
      else
      {
         MemFree(m_ppValueCache);
         m_ppValueCache = nullptr;
      }
   }
   else if (m_requiredCacheSize > m_cacheSize)
   {
      // Load missing values from database
      // Skip caching for DCIs where estimated time to fill the cache is less then 5 minutes
      // to reduce load on database at server startup
      if (allowLoad &&
          (m_ownerId != 0) &&
          (((m_requiredCacheSize - m_cacheSize) * getEffectivePollingInterval() > 300) ||
           (m_source == DS_PUSH_AGENT) ||
           (m_pollingScheduleType == DC_POLLING_SCHEDULE_ADVANCED)))
      {
         m_cacheLoaded = false;
         g_dciCacheLoaderQueue.put(createDescriptorInternal());
      }
      else
      {
         // will not read data from database, fill cache with empty values
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
         for(uint32_t i = m_cacheSize; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), Timestamp::fromMilliseconds(1), false);
         nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 7, _T("Cache load skipped for parameter %s [%u]"), m_name.cstr(), m_id);
         m_cacheSize = m_requiredCacheSize;
         m_cacheLoaded = true;
      }
   }
}

/**
 * Load cache from database
 */
void DCItem::loadCache()
{
   updateCacheSize();
}

/**
 * Reload cache from database
 */
void DCItem::reloadCache(bool forceReload)
{
   lock();
   if (!forceReload && m_cacheLoaded && (m_cacheSize == m_requiredCacheSize))
   {
      unlock();
      return;  // Cache already fully populated
   }
   unlock();

   TCHAR szBuffer[MAX_DB_STRING];
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT TOP %d idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                    m_requiredCacheSize, m_id);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT TOP %d idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                    m_requiredCacheSize, m_ownerId, m_id);
         }
         break;
      case DB_SYNTAX_ORACLE:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT * FROM (SELECT idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC) WHERE ROWNUM <= %d"),
                    m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT * FROM (SELECT idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC) WHERE ROWNUM <= %d"),
                    m_ownerId, m_id, m_requiredCacheSize);
         }
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %u"),
                    m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %u"),
                    m_ownerId, m_id, m_requiredCacheSize);
         }
         break;
      case DB_SYNTAX_TSDB:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,timestamptz_to_ms(idata_timestamp) FROM idata_sc_%s ")
                              _T("WHERE item_id=%u AND idata_timestamp >= (SELECT ms_to_timestamptz(cache_timestamp) FROM raw_dci_values WHERE item_id=%u) ORDER BY idata_timestamp DESC LIMIT %u"),
                    getStorageClassName(getStorageClass()), m_id, m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d WHERE item_id=%u ORDER BY idata_timestamp DESC LIMIT %u"),
                    m_ownerId, m_id, m_requiredCacheSize);
         }
         break;
      case DB_SYNTAX_DB2:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
               _T("WHERE item_id=%u ORDER BY idata_timestamp DESC FETCH FIRST %u ROWS ONLY"),
               m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
               _T("WHERE item_id=%u ORDER BY idata_timestamp DESC FETCH FIRST %u ROWS ONLY"),
               m_ownerId, m_id, m_requiredCacheSize);
         }
         break;
      default:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%u ORDER BY idata_timestamp DESC"), m_id);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%u ORDER BY idata_timestamp DESC"),
                    m_ownerId, m_id);
         }
         break;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, szBuffer);

   // While reload request was in queue DCI cache may have been already filled
   lock();
   if (forceReload || !m_cacheLoaded || (m_cacheSize != m_requiredCacheSize))
   {
      for(uint32_t i = 0; i < m_cacheSize; i++)
         delete m_ppValueCache[i];

      if (m_cacheSize != m_requiredCacheSize)
      {
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
      }

      nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 8, _T("DCItem::reloadCache(dci=\"%s\", node=%s [%d]): requiredSize=%d cacheSize=%d"),
               m_name.cstr(), getOwnerName(), m_ownerId, m_requiredCacheSize, m_cacheSize);
      if (hResult != nullptr)
      {
         // Create cache entries
         bool moreData = true;
         uint32_t i;
         for(i = 0; (i < m_requiredCacheSize) && moreData; i++)
         {
            moreData = DBFetch(hResult);
            if (moreData)
            {
               DBGetField(hResult, 0, szBuffer, MAX_DB_STRING);
               m_ppValueCache[i] = new ItemValue(szBuffer, DBGetFieldTimestamp(hResult, 1), false);
            }
            else
            {
               m_ppValueCache[i] = new ItemValue(L"", Timestamp::fromMilliseconds(1), false);   // Empty value
            }
         }

         // Fill up cache with empty values if we don't have enough values in database
         if (i < m_requiredCacheSize)
         {
            nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 8, _T("DCItem::reloadCache(dci=\"%s\", node=%s [%u]): %d values missing in DB"),
                     m_name.cstr(), getOwnerName(), m_ownerId, m_requiredCacheSize - i);
            for(; i < m_requiredCacheSize; i++)
               m_ppValueCache[i] = new ItemValue(L"", Timestamp::fromMilliseconds(1), false);
         }
         DBFreeResult(hResult);
      }
      else
      {
         // Error reading data from database, fill cache with empty values
         for(uint32_t i = 0; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(L"", Timestamp::fromMilliseconds(1), false);
      }

      m_cacheSize = m_requiredCacheSize;
      m_cacheLoaded = true;
   }
   else if (hResult != nullptr)
   {
      DBFreeResult(hResult);
   }
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get cache memory usage
 */
uint64_t DCItem::getCacheMemoryUsage() const
{
   lock();
   uint64_t size = m_cacheSize * (sizeof(ItemValue) + sizeof(ItemValue*));
   unlock();
   return size;
}

/**
 * Put last value into NXCP message (single DCI per message)
 */
void DCItem::fillLastValueMessage(NXCPMessage *msg)
{
   lock();
   msg->setField(VID_DCI_SOURCE_TYPE, m_source);
   if (m_cacheSize > 0)
   {
      msg->setField(VID_DCI_DATA_TYPE, static_cast<uint16_t>(getTransformedDataType()));
      msg->setField(VID_VALUE, m_ppValueCache[0]->getString());
      msg->setField(VID_RAW_VALUE, m_prevRawValue.getString());
      msg->setField(VID_TIMESTAMP_MS, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      msg->setField(VID_DCI_DATA_TYPE, static_cast<uint16_t>(DCI_DT_NULL));
      msg->setField(VID_VALUE, L"");
      msg->setField(VID_RAW_VALUE, L"");
      msg->setField(VID_TIMESTAMP_MS, static_cast<int64_t>(0));
   }
   unlock();
}

/**
 * Put last value into NXCP message
 */
void DCItem::fillLastValueSummaryMessage(NXCPMessage *msg, uint32_t baseId, const TCHAR *column, const TCHAR *instance)
{
	lock();
   msg->setField(baseId++, m_ownerId);
   msg->setField(baseId++, m_id);
   msg->setField(baseId++, m_name);
   msg->setField(baseId++, m_flags);
   msg->setField(baseId++, m_description);
   msg->setField(baseId++, static_cast<uint16_t>(m_source));
   if (m_cacheSize > 0)
   {
      msg->setField(baseId++, static_cast<uint16_t>(getTransformedDataType()));
      msg->setField(baseId++, m_ppValueCache[0]->getString());
      msg->setField(baseId++, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      msg->setField(baseId++, static_cast<uint16_t>(DCI_DT_NULL));
      msg->setField(baseId++, L"");
      msg->setField(baseId++, static_cast<int64_t>(0));
   }
   msg->setField(baseId++, static_cast<uint16_t>(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
	msg->setField(baseId++, static_cast<uint16_t>(getType()));
	msg->setField(baseId++, m_errorCount);
	msg->setField(baseId++, m_templateItemId);
   msg->setField(baseId++, m_unitName);
   msg->setField(baseId++, m_multiplier);
   msg->setField(baseId++, !hasValue());
   msg->setField(baseId++, m_comments);
   msg->setField(baseId++, m_anomalyDetected);
   msg->setField(baseId++, m_userTag);
   msg->setFieldFromTime(baseId++, m_thresholdDisableEndTime);

	if (m_thresholds != nullptr)
	{
      int mostCritical = -1;
      int severity = STATUS_NORMAL;
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         if (m_thresholds->get(i)->isReached())
         {
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               mostCritical = i;
               break;
            }

            int currSeverity = m_thresholds->get(i)->getCurrentSeverity();
            if (currSeverity > severity)
            {
               severity = currSeverity;
               mostCritical = i;
               if (severity == STATUS_CRITICAL)
                  break;
            }
         }
      }
      if (mostCritical != -1)
      {
         msg->setField(baseId++, static_cast<uint16_t>(1));
         m_thresholds->get(mostCritical)->fillMessage(msg, baseId, this);
      }
      else
      {
         msg->setField(baseId++, static_cast<uint16_t>(0));
      }
	}
	else
	{
      msg->setField(baseId++, static_cast<uint16_t>(0));
	}

	unlock();
}

/**
 * Put last value into JSON object
 */
json_t *DCItem::lastValueToJSON()
{
   json_t *data = json_object();

   lock();
   json_object_set_new(data, "ownerId", json_integer(m_ownerId));
   json_object_set_new(data, "id", json_integer(m_id));
   json_object_set_new(data, "name", json_string_t(m_name));
   json_object_set_new(data, "flags", json_integer(m_flags));
   json_object_set_new(data, "description", json_string_t(m_description));
   json_object_set_new(data, "sourceType", json_integer(m_source));

   if (m_cacheSize > 0)
   {
      json_object_set_new(data, "dataType", json_integer(getTransformedDataType()));
      json_object_set_new(data, "value", json_string_t(m_ppValueCache[0]->getString()));
      json_object_set_new(data, "timestamp", m_ppValueCache[0]->getTimeStamp().asJson());
   }
   else
   {
      json_object_set_new(data, "dataType", json_integer(DCI_DT_NULL));
      json_object_set_new(data, "value", json_string(""));
      json_object_set_new(data, "timestamp", json_null());
   }

   // Show resource-bound DCIs as inactive if cluster resource is not on this node
   json_object_set_new(data, "status", json_integer(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED));
   json_object_set_new(data, "type", json_integer(getType()));
   json_object_set_new(data, "errorCount", json_integer(m_errorCount));
   json_object_set_new(data, "templateItemId", json_integer(m_templateItemId));
   json_object_set_new(data, "unitName", json_string_t(m_unitName));
   json_object_set_new(data, "multiplier", json_integer(m_multiplier));
   json_object_set_new(data, "noValue", json_boolean(!hasValue()));
   json_object_set_new(data, "comments", json_string_t(m_comments));
   json_object_set_new(data, "anomalyDetected", json_boolean(m_anomalyDetected));
   json_object_set_new(data, "userTag", json_string_t(m_userTag));
   json_object_set_new(data, "thresholdDisableEndTime", json_integer(m_thresholdDisableEndTime));

   if (m_thresholds != nullptr)
   {
      int mostCritical = -1;
      int severity = STATUS_NORMAL;
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         if (m_thresholds->get(i)->isReached())
         {
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               mostCritical = i;
               break;
            }

            int currSeverity = m_thresholds->get(i)->getCurrentSeverity();
            if (currSeverity > severity)
            {
               severity = currSeverity;
               mostCritical = i;
               if (severity == STATUS_CRITICAL)
                  break;
            }
         }
      }

      if (mostCritical != -1)
      {
         json_object_set_new(data, "hasActiveThreshold", json_boolean(true));
         json_object_set_new(data, "threshold", m_thresholds->get(mostCritical)->toJson());
      }
      else
      {
         json_object_set_new(data, "hasActiveThreshold", json_boolean(false));
      }
   }
   else
   {
      json_object_set_new(data, "hasActiveThreshold", json_boolean(false));
   }

   unlock();
   return data;
}

/**
 * Get item's last value for use in NXSL
 */
NXSL_Value *DCItem::getRawValueForNXSL(NXSL_VM *vm)
{
	lock();
   NXSL_Value *value = vm->createValue(m_prevRawValue.getString());
   unlock();
   return value;
}

/**
 * Cast NXSL value to match DCI data type
 */
static inline void CastNXSLValue(NXSL_Value *value, int dciDataType)
{
   switch(dciDataType)
   {
      case DCI_DT_COUNTER64:
      case DCI_DT_UINT64:
         if (value->isNumeric())
            value->convert(NXSL_DT_UINT64);
         break;
      case DCI_DT_INT64:
         if (value->isNumeric())
            value->convert(NXSL_DT_INT64);
         break;
      case DCI_DT_COUNTER32:
      case DCI_DT_UINT:
         if (value->isNumeric())
            value->convert(NXSL_DT_UINT32);
         break;
      case DCI_DT_FLOAT:
         if (value->isNumeric())
            value->convert(NXSL_DT_REAL);
         break;
   }
}

/**
 * Get item's last value for use in NXSL
 */
NXSL_Value *DCItem::getValueForNXSL(NXSL_VM *vm, int function, int sampleCount)
{
   if (!m_cacheLoaded)
      reloadCache(false);

   NXSL_Value *value;

	lock();
   switch(function)
   {
      case F_LAST:
         // cache placeholders will have timestamp 1
         value = (m_cacheLoaded && (m_cacheSize > 0) && (m_ppValueCache[0]->getTimeStamp().asMilliseconds() != 1)) ? vm->createValue(m_ppValueCache[0]->getString()) : vm->createValue();
         CastNXSLValue(value, getTransformedDataType());
         break;
      case F_DIFF:
         if (m_cacheLoaded && (m_cacheSize >= 2))
         {
            ItemValue result;
            CalculateItemValueDiff(&result, getTransformedDataType(), *m_ppValueCache[0], *m_ppValueCache[1]);
            value = vm->createValue(result.getString());
         }
         else
         {
            value = vm->createValue();
         }
         break;
      case F_AVERAGE:
         if (m_cacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueAverage(&result, getTransformedDataType(), m_ppValueCache, std::min(m_cacheSize, static_cast<uint32_t>(sampleCount)));
            value = vm->createValue(result.getString());
            CastNXSLValue(value, getTransformedDataType());
         }
         else
         {
            value = vm->createValue();
         }
         break;
      case F_MEAN_DEVIATION:
         if (m_cacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueMeanDeviation(&result, getTransformedDataType(), m_ppValueCache, std::min(m_cacheSize, static_cast<uint32_t>(sampleCount)));
            value = vm->createValue(result.getString());
         }
         else
         {
            value = vm->createValue();
         }
         break;
      case F_ERROR:
         value = vm->createValue(static_cast<int32_t>((m_errorCount >= static_cast<uint32_t>(sampleCount)) ? 1 : 0));
         break;
      default:
         value = vm->createValue();
         break;
   }
	unlock();
   return value;
}

/**
 * Get last value
 */
const TCHAR *DCItem::getLastValue()
{
   lock();
   const TCHAR *v = (m_cacheSize > 0) ? (const TCHAR *)m_ppValueCache[0]->getString() : nullptr;
   unlock();
   return v;
}

/**
 * Get copy of internal last value object. Caller is responsible for destroying returned object.
 */
ItemValue *DCItem::getInternalLastValue()
{
   lock();
   ItemValue *v = (m_cacheSize > 0) ? new ItemValue(*m_ppValueCache[0]) : nullptr;
   unlock();
   return v;
}

/**
 * Get aggregate value. Returned value must be deallocated by caller.
 *
 * @return dynamically allocated value or nullptr on error
 */
TCHAR *DCItem::getAggregateValue(AggregationFunction func, time_t periodStart, time_t periodEnd)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	TCHAR query[1024];
   TCHAR *result = nullptr;

   static const TCHAR *functions[] = { _T(""), _T("min"), _T("max"), _T("avg"), _T("sum") };

   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func]);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"),
                  functions[func]);
            break;
         case DB_SYNTAX_PGSQL:
            _sntprintf(query, 1024,
                  _T("SELECT %s(idata_value::double precision) FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
                  functions[func]);
            break;
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 1024,
                  _T("SELECT %s(idata_value::double precision) FROM idata_sc_%s WHERE item_id=? AND idata_timestamp BETWEEN ms_to_timestamptz(?) AND ms_to_timestamptz(?) AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
                  functions[func], getStorageClassName(getStorageClass()));
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as decimal(30,10)),0)) FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func]);
            break;
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as double),0)) FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func]);
            break;
         default:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(idata_value,0)) FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func]);
      }
   }
   else
   {
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func], m_ownerId);
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata_%u WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"),
                  functions[func], m_ownerId);
            break;
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 1024,
                  _T("SELECT %s(idata_value::double precision) FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
                  functions[func], m_ownerId);
            break;
         case DB_SYNTAX_MYSQL:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as decimal(30,10)),0)) FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func], m_ownerId);
            break;
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(cast(idata_value as double),0)) FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func], m_ownerId);
            break;
         default:
            _sntprintf(query, 1024,
                  _T("SELECT %s(coalesce(idata_value,0)) FROM idata_%u WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
                  functions[func], m_ownerId);
            break;
      }
   }

	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(periodStart) * 1000L);
		DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, static_cast<int64_t>(periodEnd) * 1000L);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			if (DBGetNumRows(hResult) == 1)
			{
				result = DBGetField(hResult, 0, 0, nullptr, 0);
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
   return result;
}

/**
 * Delete all collected data
 */
bool DCItem::deleteAllData()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   lock();
   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         _sntprintf(query, 256, _T("DELETE FROM idata_sc_%s WHERE item_id=%u"), getStorageClassName(getStorageClass()), m_id);
      else
         _sntprintf(query, 256, _T("DELETE FROM idata WHERE item_id=%u"), m_id);
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM idata_%d WHERE item_id=%u"), m_ownerId, m_id);
   }
	bool success = DBQuery(hdb, query);
	clearCache();
	updateCacheSizeInternal(true);
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Delete single collected data entry
 */
bool DCItem::deleteEntry(Timestamp timestamp)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   lock();

   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         _sntprintf(query, 256, _T("DELETE FROM idata_sc_%s WHERE item_id=%u AND idata_timestamp=ms_to_timestamptz(") INT64_FMT _T(")"),
                  getStorageClassName(getStorageClass()), m_id, timestamp.asMilliseconds());
      }
      else
      {
         _sntprintf(query, 256, _T("DELETE FROM idata WHERE item_id=%d AND idata_timestamp=") INT64_FMT, m_id, timestamp.asMilliseconds());
      }
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM idata_%d WHERE item_id=%d AND idata_timestamp=") INT64_FMT, m_ownerId, m_id, timestamp.asMilliseconds());
   }
   unlock();

   bool success = DBQuery(hdb, query);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
      return false;

   lock();
   for(uint32_t i = 0; i < m_cacheSize; i++)
   {
      if (m_ppValueCache[i]->getTimeStamp() == timestamp)
      {
         delete m_ppValueCache[i];
         memmove(&m_ppValueCache[i], &m_ppValueCache[i + 1], sizeof(ItemValue *) * (m_cacheSize - (i + 1)));
         m_cacheSize--;
         updateCacheSizeInternal(true);
         break;
      }
   }
   unlock();

   return success;
}

/**
 * Update from template item
 */
void DCItem::updateFromTemplate(DCObject *src)
{
	DCObject::updateFromTemplate(src);

	if (src->getType() != DCO_TYPE_ITEM)
	{
	   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_DC_TEMPLATES, _T("INTERNAL ERROR: DCItem::updateFromTemplate(%u, %u): source type is %d"), m_id, src->getId(), src->getType());
		return;
	}

   lock();
	DCItem *item = static_cast<DCItem*>(src);

   m_dataType = item->m_dataType;
   m_transformedDataType = item->m_transformedDataType;
   m_deltaCalculation = item->m_deltaCalculation;
   m_sampleCount = item->m_sampleCount;
   m_snmpRawValueType = item->m_snmpRawValueType;

	m_multiplier = item->m_multiplier;
	m_unitName = item->m_unitName;

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
	int count = std::min(getThresholdCount(), item->getThresholdCount());
	int i;
   for(i = 0; i < count; i++)
      if (!m_thresholds->get(i)->equals(*item->m_thresholds->get(i)))
         break;
   count = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
	while(count < getThresholdCount())
		m_thresholds->remove(count);

   // (Re)create thresholds starting from first unmatched
	if ((m_thresholds == nullptr) && (item->getThresholdCount() > 0))
		m_thresholds = new ObjectArray<Threshold>(item->getThresholdCount(), 8, Ownership::True);
	for(i = count; i < item->getThresholdCount(); i++)
   {
      Threshold *t = new Threshold(*item->m_thresholds->get(i), false);
      t->bindToItem(m_id, m_ownerId);
		m_thresholds->add(t);
   }

   // Update data type in thresholds
   for(i = 0; i < getThresholdCount(); i++)
      m_thresholds->get(i)->setDataType(getTransformedDataType());

   updateCacheSizeInternal(true);
   unlock();
}

/**
 * Get list of used events
 */
void DCItem::getEventList(HashSet<uint32_t> *eventList) const
{
   lock();
   if (m_thresholds != nullptr)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         eventList->put(m_thresholds->get(i)->getEventCode());
         eventList->put(m_thresholds->get(i)->getRearmEventCode());
      }
   }
   unlock();
}

/**
 * Check if given event is used
 */
bool DCItem::isUsingEvent(uint32_t eventCode) const
{
   if (m_thresholds == nullptr)
      return false;
   for (int i = 0; i < m_thresholds->size(); i++)
      if (m_thresholds->get(i)->isUsingEvent(eventCode))
         return true;
   return false;
}

/**
 * Create management pack record
 */
void DCItem::createExportRecord(TextFileWriter& xml) const
{
   lock();

   xml.append(_T("\t\t\t\t<dci id=\""));
   xml.append(m_id);
   xml.append(_T("\">\n\t\t\t\t\t<guid>"));
   xml.append(m_guid);
   xml.append(_T("</guid>\n\t\t\t\t\t<name>"));
   xml.append(EscapeStringForXML2(m_name));
   xml.append(_T("</name>\n\t\t\t\t\t<description>"));
   xml.append(EscapeStringForXML2(m_description));
   xml.append(_T("</description>\n\t\t\t\t\t<dataType>"));
   xml.append(static_cast<int32_t>(m_dataType));
   xml.append(_T("</dataType>\n\t\t\t\t\t<transformedDataType>"));
   xml.append(static_cast<int32_t>(m_transformedDataType));
   xml.append(_T("</transformedDataType>\n\t\t\t\t\t<samples>"));
   xml.append(m_sampleCount);
   xml.append(_T("</samples>\n\t\t\t\t\t<origin>"));
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
   xml.append(_T("</userTag>\n\t\t\t\t\t<delta>"));
   xml.append(static_cast<int32_t>(m_deltaCalculation));
   xml.append(_T("</delta>\n\t\t\t\t\t<flags>"));
   xml.append(m_flags);
   xml.append(_T("</flags>\n\t\t\t\t\t<snmpRawValueType>"));
   xml.append(m_snmpRawValueType);
   xml.append(_T("</snmpRawValueType>\n\t\t\t\t\t<snmpPort>"));
   xml.append(m_snmpPort);
   xml.append(_T("</snmpPort>\n\t\t\t\t\t<snmpVersion>"));
   xml.append(static_cast<int32_t>(m_snmpVersion));
   xml.append(_T("</snmpVersion>\n\t\t\t\t\t<instanceDiscoveryMethod>"));
   xml.append(m_instanceDiscoveryMethod);
   xml.append(_T("</instanceDiscoveryMethod>\n\t\t\t\t\t<instanceRetentionTime>"));
   xml.append(m_instanceRetentionTime);
   xml.append(_T("</instanceRetentionTime>\n\t\t\t\t\t<comments>"));
   xml.append(EscapeStringForXML2(m_comments));
   xml.append(_T("</comments>\n\t\t\t\t\t<isDisabled>"));
   xml.append(BooleanToString(m_status == ITEM_STATUS_DISABLED));
   xml.append(_T("</isDisabled>\n\t\t\t\t\t<unitName>"));
   xml.append(EscapeStringForXML2(m_unitName));
   xml.append(_T("</unitName>\n\t\t\t\t\t<multiplier>"));
   xml.append(m_multiplier);
   xml.append(_T("</multiplier>\n\t\t\t\t\t<allThresholdsRearmEvent>"));
   xml.append(m_allThresholdsRearmEvent);
   xml.append(_T("</allThresholdsRearmEvent>\n"));

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
         xml.appendPreallocated(EscapeStringForXML(m_schedules->get(i), -1));
         xml.append(_T("</schedule>\n"));
      }
      xml.append(_T("\t\t\t\t\t</schedules>\n"));
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

   if (m_instanceName != nullptr)
   {
      xml.append(_T("\t\t\t\t\t<instance>"));
      xml.appendPreallocated(EscapeStringForXML(m_instanceName, -1));
      xml.append(_T("</instance>\n"));
   }

   if (m_instanceDiscoveryData != nullptr)
	{
		xml.append(_T("\t\t\t\t\t<instanceDiscoveryData>"));
		xml.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
		xml.append(_T("</instanceDiscoveryData>\n"));
	}

   if (!m_instanceFilterSource.isBlank())
	{
		xml.append(_T("\t\t\t\t\t<instanceFilter>"));
		xml.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
		xml.append(_T("</instanceFilter>\n"));
	}

   unlock();
   xml.append(_T("\t\t\t\t</dci>\n"));
}

/**
 * Add threshold to the list
 */
void DCItem::addThreshold(Threshold *pThreshold)
{
	if (m_thresholds == nullptr)
		m_thresholds = new ObjectArray<Threshold>(8, 8, Ownership::True);
	m_thresholds->add(pThreshold);
}

/**
 * Test DCI's transformation script
 * If dcObjectInfo is not nullptr it will be destroyed by this method
 */
bool DCItem::testTransformation(DataCollectionTarget *object, const shared_ptr<DCObjectInfo>& dcObjectInfo,
         const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize)
{
	bool success = false;
	NXSL_CompilationDiagnostic diag;
	NXSL_VM *vm = NXSLCompileAndCreateVM(script, new NXSL_ServerEnv, &diag);
   if (vm != nullptr)
   {
      NXSL_Value *pValue = vm->createValue(value);
      vm->setGlobalVariable("$object", object->createNXSLObject(vm));
      if (object->getObjectClass() == OBJECT_NODE)
      {
         vm->setGlobalVariable("$node", object->createNXSLObject(vm));
      }
      if (dcObjectInfo != nullptr)
      {
         vm->setGlobalVariable(_T("$dci"), vm->createValue(vm->createObject(&g_nxslDciClass, new shared_ptr<DCObjectInfo>(dcObjectInfo))));
      }
      vm->setGlobalVariable("$isCluster", vm->createValue(object->getObjectClass() == OBJECT_CLUSTER));

		if (vm->run(1, &pValue))
      {
         pValue = vm->getResult();
         if (pValue->isNull())
         {
            _tcslcpy(buffer, _T("(null)"), bufSize);
         }
         else if (pValue->isObject())
         {
            _tcslcpy(buffer, _T("(object)"), bufSize);
         }
         else if (pValue->isArray())
         {
            _tcslcpy(buffer, _T("(array)"), bufSize);
         }
         else
         {
            const TCHAR *strval = pValue->getValueAsCString();
            _tcslcpy(buffer, CHECK_NULL(strval), bufSize);
         }
			success = true;
      }
      else
      {
			_tcslcpy(buffer, vm->getErrorText(), bufSize);
      }
   }
   else
   {
      _tcslcpy(buffer, diag.errorText, bufSize);
   }
   delete vm;
	return success;
}

/**
 * Fill NXCP message with thresholds
 */
void DCItem::fillMessageWithThresholds(NXCPMessage *msg, bool activeOnly)
{
	lock();

	msg->setField(VID_NUM_THRESHOLDS, (UINT32)getThresholdCount());
	uint32_t fieldId = VID_DCI_THRESHOLD_BASE;
	for(int i = 0; i < getThresholdCount(); i++, fieldId += 20)
	{
	   Threshold *threshold = m_thresholds->get(i);
	   if (!activeOnly || threshold->isReached())
         threshold->fillMessage(msg, fieldId, this);
	}

	unlock();
}

/**
 * Check if DCI has active threshold
 */
bool DCItem::hasActiveThreshold() const
{
	bool result = false;
	lock();
	int count = getThresholdCount();
	for(int i = 0; i < count; i++)
	{
		if (m_thresholds->get(i)->isReached())
		{
			result = true;
			break;
		}
	}
	unlock();
	return result;
}

/**
 * Get severity of active threshold. If no active threshold exist, returns SEVERITY_NORMAL.
 */
int DCItem::getThresholdSeverity() const
{
   int result = SEVERITY_NORMAL;
	lock();
	for(int i = 0; i < getThresholdCount(); i++)
	{
      Threshold *t = m_thresholds->get(i);
		if (t->isReached())
		{
		   result = MAX(result, t->getCurrentSeverity());
			if(!(m_flags & DCF_ALL_THRESHOLDS) || result == SEVERITY_CRITICAL)
			   break;
		}
	}
	unlock();
	return result;
}

/**
 * Returns true if internal cache is loaded. If data collection object
 * does not have cache should return true
 */
bool DCItem::isCacheLoaded()
{
	return m_cacheLoaded;
}

/**
 * Create DCI from import file
 */
void DCItem::updateFromImport(ConfigEntry *config, bool nxslV5)
{
   DCObject::updateFromImport(config, nxslV5);

   lock();
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_transformedDataType = (BYTE)config->getSubEntryValueAsInt(_T("transformedDataType"), 0, DCI_DT_NULL);
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_snmpRawValueType = static_cast<uint16_t>(config->getSubEntryValueAsInt(_T("snmpRawValueType")));
   m_unitName = config->getSubEntryValue(_T("unitName"));
   m_multiplier = config->getSubEntryValueAsInt(_T("multiplier"));
   m_allThresholdsRearmEvent = config->getSubEntryValueAsUInt(_T("allThresholdsRearmEvent"));

   ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
   if (thresholdsRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      if (m_thresholds != nullptr)
         m_thresholds->clear();
      else
         m_thresholds = new ObjectArray<Threshold>(thresholds->size(), 8, Ownership::True);
      for(int i = 0; i < thresholds->size(); i++)
      {
         m_thresholds->add(new Threshold(thresholds->get(i), this, nxslV5));
      }
   }
   else
   {
      delete_and_null(m_thresholds);
   }

   updateCacheSizeInternal(true);
   unlock();
}

/*
 * Clone DCI
 */
DCObject *DCItem::clone() const
{
   return new DCItem(this, false);
}

/**
 * Serialize object to JSON
 */
json_t *DCItem::toJson()
{
   json_t *root = DCObject::toJson();
   lock();
   json_object_set_new(root, "deltaCalculation", json_integer(m_deltaCalculation));
   json_object_set_new(root, "dataType", json_integer(m_dataType));
   json_object_set_new(root, "transformedDataType", json_integer(m_transformedDataType));
   json_object_set_new(root, "sampleCount", json_integer(m_sampleCount));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
   json_object_set_new(root, "prevRawValue", json_string_t(m_prevRawValue));
   json_object_set_new(root, "prevValueTimeStamp", m_prevValueTimeStamp.asJson());
   json_object_set_new(root, "multiplier", json_integer(m_multiplier));
   json_object_set_new(root, "unitName", json_string_t(m_unitName));
   json_object_set_new(root, "snmpRawValueType", json_integer(m_snmpRawValueType));
   json_object_set_new(root, "predictionEngine", json_string_t(m_predictionEngine));
   json_object_set_new(root, "allThresholdsRearmEvent", json_integer(m_allThresholdsRearmEvent));
   unlock();
   return root;
}

/**
 * Prepare DCI object for recalculation (should be executed on DCI copy)
 */
void DCItem::prepareForRecalc()
{
   m_prevValueTimeStamp = Timestamp::fromMilliseconds(0);
   m_lastPollTime = Timestamp::fromMilliseconds(0);
   updateCacheSizeInternal(false);
}

/**
 * Recalculate old value (should be executed on DCI copy)
 */
void DCItem::recalculateValue(ItemValue &value)
{
   if (m_prevValueTimeStamp.isNull())
      m_prevRawValue = value;  // Delta should be zero for first poll
   ItemValue rawValue = value;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   auto owner = m_owner.lock();
   if ((owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(value, (value.getTimeStamp() > m_prevValueTimeStamp) ? (value.getTimeStamp() - m_prevValueTimeStamp) : 0))
      {
         return;
      }
   }

   if (value.getTimeStamp() > m_prevValueTimeStamp)
   {
      m_prevRawValue = rawValue;
      m_prevValueTimeStamp = value.getTimeStamp();
   }

   if ((m_cacheSize > 0) && (value.getTimeStamp() >= m_prevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = new ItemValue(value);
   }

   m_lastPollTime = value.getTimeStamp();
}

/**
 * Check if this DCI has script thresholds
 */
bool DCItem::hasScriptThresholds() const
{
   if (m_thresholds == nullptr)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
      Threshold *t = m_thresholds->get(i);
      if ((t->getFunction() == F_SCRIPT) || t->needValueExpansion()) // Macro expansion can invoke script
         return true;
   }
   return false;
}

/**
 * Get threshold object by it's ID
 */
Threshold *DCItem::getThresholdById(uint32_t id) const
{
   if (m_thresholds == nullptr)
      return nullptr;

   for(int i = 0; i < m_thresholds->size(); i++)
      if (m_thresholds->get(i)->getId() == id)
         return m_thresholds->get(i);

   return nullptr;
}

/**
 * Create descriptor for this object
 */
shared_ptr<DCObjectInfo> DCItem::createDescriptorInternal() const
{
   shared_ptr<DCObjectInfo> info = DCObject::createDescriptorInternal();

   for(int i = 0; i < getThresholdCount(); i++)
   {
      Threshold *t = m_thresholds->get(i);
      if (t->isReached())
      {
         info->m_hasActiveThreshold = true;
         info->m_thresholdSeverity = std::max(info->m_thresholdSeverity, t->getCurrentSeverity());
         if (!(m_flags & DCF_ALL_THRESHOLDS) || (info->m_thresholdSeverity == SEVERITY_CRITICAL))
            break;
      }
   }

   return info;
}

/**
 * Get all script dependencies for this object
 */
void DCItem::getScriptDependencies(StringSet *dependencies) const
{
   DCObject::getScriptDependencies(dependencies);

   if (m_thresholds != nullptr)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         m_thresholds->get(i)->getScriptDependencies(dependencies);
      }
   }
}
