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
** File: dcitem.cpp
**
**/

#include "nxcore.h"
#include <npe.h>

/**
 * Create DCItem from another DCItem
 */
DCItem::DCItem(const DCItem *src, bool shadowCopy) : DCObject(src, shadowCopy)
{
   m_dataType = src->m_dataType;
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
   m_tPrevValueTimeStamp = shadowCopy ? src->m_tPrevValueTimeStamp : 0;
   m_bCacheLoaded = shadowCopy ? src->m_bCacheLoaded : false;
	m_multiplier = src->m_multiplier;
	m_unitName = src->m_unitName;
	m_snmpRawValueType = src->m_snmpRawValueType;
   _tcscpy(m_predictionEngine, src->m_predictionEngine);

   // Copy thresholds
	if (src->getThresholdCount() > 0)
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
 *    retention_type,polling_interval_src,retention_time_src,snmp_version,state_flags
 */
DCItem::DCItem(DB_HANDLE hdb, DB_RESULT hResult, int row, const shared_ptr<DataCollectionOwner>& owner, bool useStartupDelay) : DCObject(owner)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_name = DBGetFieldAsSharedString(hResult, row, 1);
   m_source = (BYTE)DBGetFieldLong(hResult, row, 2);
   m_dataType = (BYTE)DBGetFieldLong(hResult, row, 3);
   m_pollingInterval = DBGetFieldLong(hResult, row, 4);
   m_retentionTime = DBGetFieldLong(hResult, row, 5);
   m_status = (BYTE)DBGetFieldLong(hResult, row, 6);
   m_deltaCalculation = (BYTE)DBGetFieldLong(hResult, row, 7);
   setTransformationScript(DBGetField(hResult, row, 8, nullptr, 0));
   m_templateId = DBGetFieldULong(hResult, row, 9);
   m_description = DBGetFieldAsSharedString(hResult, row, 10);
   m_instanceName = DBGetFieldAsSharedString(hResult, row, 11);
   m_templateItemId = DBGetFieldULong(hResult, row, 12);
   m_thresholds = nullptr;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
   m_flags = DBGetFieldLong(hResult, row, 13);
	m_resourceId = DBGetFieldULong(hResult, row, 14);
	m_sourceNode = DBGetFieldULong(hResult, row, 15);
	m_multiplier = DBGetFieldLong(hResult, row, 16);
	m_unitName = DBGetFieldAsSharedString(hResult, row, 17);
	m_pszPerfTabSettings = DBGetField(hResult, row, 18, nullptr, 0);
	m_systemTag = DBGetFieldAsSharedString(hResult, row, 19);
	m_snmpPort = static_cast<UINT16>(DBGetFieldLong(hResult, row, 20));
	m_snmpRawValueType = (WORD)DBGetFieldLong(hResult, row, 21);
	m_instanceDiscoveryMethod = (WORD)DBGetFieldLong(hResult, row, 22);
	m_instanceDiscoveryData = DBGetFieldAsSharedString(hResult, row, 23);
	m_instanceFilterSource = nullptr;
   m_instanceFilter = nullptr;
   TCHAR *tmp = DBGetField(hResult, row, 24, nullptr, 0);
	setInstanceFilter(tmp);
   MemFree(tmp);
	m_sampleCount = DBGetFieldLong(hResult, row, 25);
   m_comments = DBGetFieldAsSharedString(hResult, row, 26);
   m_guid = DBGetFieldGUID(hResult, row, 27);
   DBGetField(hResult, row, 28, m_predictionEngine, MAX_NPE_NAME_LEN);
   m_instanceRetentionTime = DBGetFieldLong(hResult, row, 29);
   m_instanceGracePeriodStart = DBGetFieldLong(hResult, row, 30);
   m_relatedObject = DBGetFieldLong(hResult, row, 31);
   m_pollingScheduleType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 32));
   m_retentionType = static_cast<BYTE>(DBGetFieldULong(hResult, row, 33));
   m_pollingIntervalSrc = (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM) ? DBGetField(hResult, row, 34, nullptr, 0) : nullptr;
   m_retentionTimeSrc = (m_retentionType == DC_RETENTION_CUSTOM) ? DBGetField(hResult, row, 35, nullptr, 0) : nullptr;
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldLong(hResult, row, 36));
   m_stateFlags = DBGetFieldLong(hResult, row, 37);

   int effectivePollingInterval = getEffectivePollingInterval();
   m_startTime = (useStartupDelay && (effectivePollingInterval >= 10)) ? time(nullptr) + rand() % (effectivePollingInterval / 2) : 0;

   // Load last raw value from database
	TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d"), m_id);
   DB_RESULT hTempResult = DBSelect(hdb, szQuery);
   if (hTempResult != nullptr)
   {
      if (DBGetNumRows(hTempResult) > 0)
      {
		   TCHAR szBuffer[MAX_DB_STRING];
         m_prevRawValue = DBGetField(hTempResult, 0, 0, szBuffer, MAX_DB_STRING);
         m_tPrevValueTimeStamp = DBGetFieldULong(hTempResult, 0, 1);
         m_lastPoll = m_lastValueTimestamp = m_tPrevValueTimeStamp;
      }
      DBFreeResult(hTempResult);
   }

   loadAccessList(hdb);
	loadCustomSchedules(hdb);

	updateTimeIntervalsInternal();
}

/**
 * Constructor for creating new DCItem from scratch
 */
DCItem::DCItem(UINT32 id, const TCHAR *name, int source, int dataType, BYTE scheduleType,
      const TCHAR *pollingInterval, BYTE retentionType, const TCHAR *retentionTime,
      const shared_ptr<DataCollectionOwner>& owner, const TCHAR *description, const TCHAR *systemTag)
	: DCObject(id, name, source, scheduleType, pollingInterval, retentionType, retentionTime, owner,
	      description, systemTag)
{
   m_dataType = dataType;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_thresholds = nullptr;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_multiplier = 0;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_predictionEngine[0] = 0;

   updateCacheSizeInternal(false);
}

/**
 * Create DCItem from import file
 */
DCItem::DCItem(ConfigEntry *config, const shared_ptr<DataCollectionOwner>& owner) : DCObject(config, owner), m_unitName(config->getSubEntryValue(_T("unitName")))
{
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = nullptr;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_multiplier = config->getSubEntryValueAsInt(_T("multiplier"));
	m_snmpRawValueType = (WORD)config->getSubEntryValueAsInt(_T("snmpRawValueType"));
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
			m_thresholds->add(new Threshold(thresholds->get(i), this));
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
bool DCItem::loadThresholdsFromDB(DB_HANDLE hdb)
{
   bool result = false;

	DB_STATEMENT hStmt = DBPrepare(hdb,
	           _T("SELECT threshold_id,fire_value,rearm_value,check_function,")
              _T("check_operation,sample_count,script,event_code,current_state,")
              _T("rearm_event_code,repeat_interval,current_severity,")
				  _T("last_event_timestamp,match_count,state_before_maint,")
				  _T("last_checked_value,last_event_message FROM thresholds WHERE item_id=? ")
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
		DBFreeStatement(hStmt);
	}
   return result;
}

/**
 * Save to database
 */
bool DCItem::saveToDatabase(DB_HANDLE hdb)
{
   static const TCHAR *columns[] = {
      _T("node_id"), _T("template_id"), _T("name"), _T("source"), _T("datatype"), _T("polling_interval"), _T("retention_time"),
      _T("status"), _T("delta_calculation"), _T("transformation"), _T("description"), _T("instance"), _T("template_item_id"),
      _T("flags"), _T("resource_id"), _T("proxy_node"), _T("multiplier"), _T("units_name"),
      _T("perftab_settings"), _T("system_tag"), _T("snmp_port"), _T("snmp_raw_value_type"), _T("instd_method"), _T("instd_data"),
      _T("instd_filter"), _T("samples"), _T("comments"), _T("guid"), _T("npe_name"), _T("instance_retention_time"),
      _T("grace_period_start"), _T("related_object"), _T("polling_interval_src"), _T("retention_time_src"),
      _T("polling_schedule_type"), _T("retention_type"), _T("snmp_version"), _T("state_flags"),
      nullptr
   };

	DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("items"), _T("item_id"), m_id, columns);
	if (hStmt == nullptr)
		return false;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_ownerId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_templateId);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC, MAX_ITEM_NAME - 1);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)m_dataType);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_pollingInterval);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_retentionTime);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_deltaCalculation);
	DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC, MAX_DB_STRING - 1);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_instanceName, DB_BIND_STATIC, MAX_INSTANCE_LEN - 1);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_templateItemId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_resourceId);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, (INT32)m_multiplier);
	DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_unitName, DB_BIND_STATIC);
	DBBind(hStmt, 19, DB_SQLTYPE_TEXT, m_pszPerfTabSettings, DB_BIND_STATIC);
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC, MAX_DB_STRING - 1);
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
   DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, (INT32)m_instanceGracePeriodStart);
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
   DBBind(hStmt, 39, DB_SQLTYPE_INTEGER, m_id);

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
         StringBuffer query(_T("DELETE FROM thresholds WHERE item_id="));
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
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM thresholds WHERE item_id=?"));
   }

   // Create record in raw_dci_values if needed
   if (success && !IsDatabaseRecordExist(hdb, _T("raw_dci_values"), _T("item_id"), m_id))
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time,cache_timestamp) VALUES (?,?,?,?)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_prevRawValue.getString(), DB_BIND_STATIC, 255);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int64_t>(m_tPrevValueTimeStamp));
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int64_t>((m_bCacheLoaded  && (m_cacheSize > 0)) ? m_ppValueCache[m_cacheSize - 1]->getTimeStamp() : 0));
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
 * Check last value for threshold violations
 */
void DCItem::checkThresholds(ItemValue &value)
{
	if (m_thresholds == nullptr)
		return;

	auto owner = m_owner.lock();

	bool thresholdDeactivated = false;
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
               shared_ptr<DCObject> sharedThis(shared_from_this());
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
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_thresholds->size();  // Stop processing
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
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
                  .param(_T("thresholdValue"), thresholdValue.getString())
                  .param(_T("currentValue"), checkValue.getString())
                  .param(_T("dciValue"), value.getString())
                  .param(_T("operation"), t->getOperation())
                  .param(_T("function"), t->getFunction())
                  .param(_T("pollCount"), t->getSampleCount())
                  .param(_T("thresholdDefinition"), t->getTextualDefinition())
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               // this flag used to re-send activation event for next active threshold
               thresholdDeactivated = true;
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
	            time_t now = time(nullptr);
	            time_t repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : static_cast<time_t>(t->getRepeatInterval());
				   if (thresholdDeactivated || ((repeatInterval != 0) && (t->getLastEventTimestamp() + repeatInterval <= now)))
				   {
	               shared_ptr<DCObject> sharedThis(shared_from_this());
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
		               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
				   }
            }
				if (!(m_flags & DCF_ALL_THRESHOLDS))
				{
					i = m_thresholds->size();  // Threshold condition still true, stop processing
				}
				thresholdDeactivated = false;
            break;
         default:
            break;
      }
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
   pMsg->setField(VID_DCI_DELTA_CALCULATION, static_cast<uint16_t>(m_deltaCalculation));
   pMsg->setField(VID_SAMPLE_COUNT, static_cast<uint16_t>(m_sampleCount));
	pMsg->setField(VID_MULTIPLIER, static_cast<uint32_t>(m_multiplier));
	pMsg->setField(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	pMsg->setField(VID_NPE_NAME, m_predictionEngine);
   pMsg->setField(VID_UNITS_NAME, m_unitName);
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
   m_deltaCalculation = (BYTE)msg.getFieldAsUInt16(VID_DCI_DELTA_CALCULATION);
	m_sampleCount = msg.getFieldAsInt16(VID_SAMPLE_COUNT);
	m_multiplier = msg.getFieldAsInt32(VID_MULTIPLIER);
	m_unitName = msg.getFieldAsSharedString(VID_UNITS_NAME);
	m_snmpRawValueType = msg.getFieldAsUInt16(VID_SNMP_RAW_VALUE_TYPE);
   msg.getFieldAsString(VID_NPE_NAME, m_predictionEngine, MAX_NPE_NAME_LEN);

   // Update thresholds
   uint32_t dwNum = msg.getFieldAsUInt32(VID_NUM_THRESHOLDS);
   uint32_t *newThresholds = MemAllocArray<uint32_t>(dwNum);
   *mapIndex = MemAllocArray<uint32_t>(dwNum);
   *mapId = MemAllocArray<uint32_t>(dwNum);
   *numMaps = 0;

   // Read all new threshold ids from message
   for(uint32_t i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      newThresholds[i] = msg.getFieldAsUInt32(dwId);
   }

   // Check if some thresholds was deleted, and reposition others if needed
   Threshold **ppNewList = MemAllocArray<Threshold*>(dwNum);
   for(int i = 0; i < getThresholdCount(); i++)
   {
      uint32_t j;
      for(j = 0; j < dwNum; j++)
         if (m_thresholds->get(i)->getId() == newThresholds[j])
            break;
      if (j == dwNum)
      {
         // No threshold with that id in new list, delete it
			m_thresholds->remove(i);
         i--;
      }
      else
      {
         // Move existing thresholds to appropriate positions in new list
         ppNewList[j] = m_thresholds->get(i);
      }
   }

   // Add or update thresholds
   for(uint32_t i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      if (newThresholds[i] == 0)    // New threshold?
      {
         ppNewList[i] = new Threshold(this);
         ppNewList[i]->createId();

         // Add index -> id mapping
         (*mapIndex)[*numMaps] = i;
         (*mapId)[*numMaps] = ppNewList[i]->getId();
         (*numMaps)++;
      }
      if (ppNewList[i] != nullptr)
         ppNewList[i]->updateFromMessage(msg, dwId);
   }

	if (dwNum > 0)
	{
		if (m_thresholds != nullptr)
		{
			m_thresholds->setOwner(Ownership::False);
			m_thresholds->clear();
			m_thresholds->setOwner(Ownership::True);
		}
		else
		{
			m_thresholds = new ObjectArray<Threshold>((int)dwNum, 8, Ownership::True);
		}
		for(uint32_t i = 0; i < dwNum; i++)
      {
         if (ppNewList[i] != nullptr)
			   m_thresholds->add(ppNewList[i]);
      }
	}
	else
	{
		delete_and_null(m_thresholds);
	}

	// Update data type in thresholds
   for(int i = 0; i < getThresholdCount(); i++)
      m_thresholds->get(i)->setDataType(m_dataType);

	MemFree(ppNewList);
   MemFree(newThresholds);
   updateCacheSizeInternal(true);
   unlock();
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 *
 * @return true on success
 */
bool DCItem::processNewValue(time_t tmTimeStamp, const TCHAR *originalValue, bool *updateStatus)
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

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue(originalValue, tmTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(*pValue, (tmTimeStamp > m_tPrevValueTimeStamp) ? (tmTimeStamp - m_tPrevValueTimeStamp) : 0))
      {
         unlock();
         delete pValue;
         return false;
      }
   }

   m_errorCount = 0;

   if (isStatusDCO() && (tmTimeStamp > m_tPrevValueTimeStamp) && ((m_cacheSize == 0) || !m_bCacheLoaded || (pValue->getUInt32() != m_ppValueCache[0]->getUInt32())))
   {
      *updateStatus = true;
   }

   if (tmTimeStamp > m_tPrevValueTimeStamp)
   {
      m_prevRawValue = rawValue;
      m_tPrevValueTimeStamp = tmTimeStamp;

      // Save raw value into database
      QueueRawDciDataUpdate(tmTimeStamp, m_id, originalValue, pValue->getString(), (m_bCacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[m_cacheSize - 1]->getTimeStamp() : 0);
   }

	// Check if user wants to collect all values or only changed values.
   if (!isStoreChangesOnly() || (m_bCacheLoaded && (m_cacheSize > 0) && _tcscmp(pValue->getString(), m_ppValueCache[0]->getString())))
   {
      //Save transformed value to database
      if (m_retentionType != DC_RETENTION_NONE)
           QueueIDataInsert(tmTimeStamp, owner->getId(), m_id, originalValue, pValue->getString(), getStorageClass());

      if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
           PerfDataStorageRequest(this, tmTimeStamp, pValue->getString());
   }

   // Update prediction engine
   if (m_predictionEngine[0] != 0)
   {
      PredictionEngine *engine = FindPredictionEngine(m_predictionEngine);
      if (engine != nullptr)
         engine->update(owner->getId(), m_id, getStorageClass(), tmTimeStamp, pValue->getDouble());
   }

   // Check thresholds and add value to cache
   if (m_bCacheLoaded && (tmTimeStamp >= m_tPrevValueTimeStamp) &&
       ((g_offlineDataRelevanceTime <= 0) || (tmTimeStamp > (time(nullptr) - g_offlineDataRelevanceTime))))
   {
      if (hasScriptThresholds())
      {
         // Run threshold check with DCI unlocked if there are script thresholds
         // to avoid possible server deadlock if script causes agent reconnect
         // Shadow copy has to be shared one because threshold check may require
         // shared pointer to DCI for additional background processing
         shared_ptr<DCItem> shadowCopy = make_shared<DCItem>(this, true);
         unlock();
         shadowCopy->checkThresholds(*pValue);
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
         checkThresholds(*pValue);
      }
   }

   // If DCI is related to interface and marked as inbound or outbound traffic indicator, update interface utilization
   if ((m_relatedObject != 0) && !m_systemTag.isEmpty() && (m_systemTag.str().startsWith(_T("iface-inbound-")) || m_systemTag.str().startsWith(_T("iface-outbound-"))))
   {
      int64_t value = pValue->getInt64();
      if (value >= 0)
      {
         shared_ptr<Interface> iface = static_pointer_cast<Interface>(FindObjectById(m_relatedObject, OBJECT_INTERFACE));
         if (iface != nullptr)
         {
            String tag(m_systemTag.str());
            ThreadPoolExecute(g_mainThreadPool,
               [iface, value, tag] () -> void {
                  int32_t u;
                  if (tag.endsWith(_T("-util")))
                  {
                     u = static_cast<int32_t>(value) * 10;
                  }
                  else
                  {
                     if (tag.endsWith(_T("-bytes")))
                        u *= 8;  // convert to bits/sec
                     u = static_cast<int32_t>(value * 1000 / iface->getSpeed());
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

   if ((m_cacheSize > 0) && (tmTimeStamp >= m_tPrevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = pValue;
      m_lastValueTimestamp = tmTimeStamp;
   }
   else if (!m_bCacheLoaded && (m_requiredCacheSize == 1))
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
      m_bCacheLoaded = true;
      m_lastValueTimestamp = tmTimeStamp;
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
void DCItem::processNewError(bool noInstance, time_t now)
{
   lock();

   m_errorCount++;

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
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
            }
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               i = m_thresholds->size();  // Stop processing
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
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
                  .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
            }
            NotifyClientsOnThresholdChange(m_ownerId, m_id, thresholdId, nullptr, result);
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
               time_t repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : static_cast<time_t>(t->getRepeatInterval());
				   if ((repeatInterval != 0) && (t->getLastEventTimestamp() + repeatInterval < now))
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
		               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, e->getSeverity(), e->getMessage()); });
				   }
            }
				if (!(m_flags & DCF_ALL_THRESHOLDS))
				{
					i = m_thresholds->size();  // Threshold condition still true, stop processing
				}
            break;
         default:
            break;
      }
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
               .param(_T("dciValue"), (m_bCacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""))
               .param(_T("operation"), t->getOperation())
               .param(_T("function"), t->getFunction())
               .param(_T("pollCount"), t->getSampleCount())
               .param(_T("thresholdDefinition"), t->getTextualDefinition())
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
               .param(_T("dciValue"), (m_bCacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""))
               .param(_T("operation"), t->getOperation())
               .param(_T("function"), t->getFunction())
               .param(_T("pollCount"), t->getSampleCount())
               .param(_T("thresholdDefinition"), t->getTextualDefinition())
               .post([sharedThis, thresholdId] (Event *e) { static_cast<DCItem&>(*sharedThis).markLastThresholdEvent(thresholdId, SEVERITY_NORMAL, nullptr); });
         }
      }
   }
   unlock();
}

/**
 * Transform received value. Assuming that DCI object is locked.
 */
bool DCItem::transform(ItemValue &value, time_t nElapsedTime)
{
   bool success = true;

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
               // assume counter reset if new value is less then previous
               value = (value.getUInt32() > m_prevRawValue.getUInt32()) ? value.getUInt32() - m_prevRawValue.getUInt32() : 0;
               break;
            case DCI_DT_INT64:
               value = value.getInt64() - m_prevRawValue.getInt64();
               break;
            case DCI_DT_UINT64:
               value = value.getUInt64() - m_prevRawValue.getUInt64();
               break;
            case DCI_DT_COUNTER64:
               // assume counter reset if new value is less then previous
               value = (value.getUInt64() > m_prevRawValue.getUInt64()) ? value.getUInt64() - m_prevRawValue.getUInt64() : 0;
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
         nElapsedTime /= 60;  // Convert to minutes
         /* no break */
      case DCM_AVERAGE_PER_SECOND:
         // Check elapsed time to prevent divide-by-zero exception
         if (nElapsedTime == 0)
            nElapsedTime++;

         switch(m_dataType)
         {
            case DCI_DT_INT:
               value = (value.getInt32() - m_prevRawValue.getInt32()) / (INT32)nElapsedTime;
               break;
            case DCI_DT_UINT:
               value = (value.getUInt32() - m_prevRawValue.getUInt32()) / (UINT32)nElapsedTime;
               break;
            case DCI_DT_COUNTER32:
               value = (value.getUInt32() > m_prevRawValue.getUInt32()) ? (value.getUInt32() - m_prevRawValue.getUInt32()) / (UINT32)nElapsedTime : 0;
               break;
            case DCI_DT_INT64:
               value = (value.getInt64() - m_prevRawValue.getInt64()) / (INT64)nElapsedTime;
               break;
            case DCI_DT_UINT64:
               value = (value.getUInt64() - m_prevRawValue.getUInt64()) / (UINT64)nElapsedTime;
               break;
            case DCI_DT_COUNTER64:
               value = (value.getUInt64() > m_prevRawValue.getUInt64()) ? (value.getUInt64() - m_prevRawValue.getUInt64()) / (UINT64)nElapsedTime : 0;
               break;
            case DCI_DT_FLOAT:
               value = (value.getDouble() - m_prevRawValue.getDouble()) / (double)nElapsedTime;
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

   if (m_transformationScript != nullptr)
   {
      ScriptVMHandle vm = CreateServerScriptVM(m_transformationScript, m_owner.lock(), createDescriptorInternal());
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
            if (success)
            {
               switch(m_dataType)
               {
                  case DCI_DT_INT:
                     value.set(nxslValue->getValueAsInt32(), nxslValue->getValueAsCString());
                     break;
                  case DCI_DT_UINT:
                  case DCI_DT_COUNTER32:
                     value.set(nxslValue->getValueAsUInt32(), nxslValue->getValueAsCString());
                     break;
                  case DCI_DT_INT64:
                     value.set(nxslValue->getValueAsInt64(), nxslValue->getValueAsCString());
                     break;
                  case DCI_DT_UINT64:
                  case DCI_DT_COUNTER64:
                     value.set(nxslValue->getValueAsUInt64(), nxslValue->getValueAsCString());
                     break;
                  case DCI_DT_FLOAT:
                     value.set(nxslValue->getValueAsReal(), nxslValue->getValueAsCString());
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
                        getOwnerName(), getOwnerId(), m_name.cstr(), m_id, vm->getErrorText());
               m_lastScriptErrorReport = now;
            }
         }
         vm.destroy();
      }
      else if (vm.failureReason() != ScriptVMFailureReason::SCRIPT_IS_EMPTY)
      {
         time_t now = time(nullptr);
         if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
         {
            ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, _T("Script load failed"), _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
            nxlog_debug_tag(DEBUG_TAG_DC_TRANSFORM, 6, _T("Failed to load transformation script for object %s [%u] DCI %s [%u]"),
                     getOwnerName(), getOwnerId(), m_name.cstr(), m_id);
            m_lastScriptErrorReport = now;
         }
         success = false;
      }
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
void DCItem::changeBinding(UINT32 newId, shared_ptr<DataCollectionOwner> newOwner, bool doMacroExpansion)
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
         m_bCacheLoaded = false;
         g_dciCacheLoaderQueue.put(createDescriptorInternal());
      }
      else
      {
         // will not read data from database, fill cache with empty values
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
         for(uint32_t i = m_cacheSize; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), 1);
         nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 7, _T("Cache load skipped for parameter %s [%u]"), m_name.cstr(), m_id);
         m_cacheSize = m_requiredCacheSize;
         m_bCacheLoaded = true;
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
   if (!forceReload && m_bCacheLoaded && (m_cacheSize == m_requiredCacheSize))
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
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,date_part('epoch',idata_timestamp)::int FROM idata_sc_%s ")
                              _T("WHERE item_id=%u AND idata_timestamp >= (SELECT to_timestamp(cache_timestamp) FROM raw_dci_values WHERE item_id=%u) ORDER BY idata_timestamp DESC LIMIT %u"),
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
   if (forceReload || !m_bCacheLoaded || (m_cacheSize != m_requiredCacheSize))
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
               m_ppValueCache[i] = new ItemValue(szBuffer, DBGetFieldULong(hResult, 1));
            }
            else
            {
               m_ppValueCache[i] = new ItemValue(_T(""), 1);   // Empty value
            }
         }

         // Fill up cache with empty values if we don't have enough values in database
         if (i < m_requiredCacheSize)
         {
            nxlog_debug_tag(DEBUG_TAG_DC_CACHE, 8, _T("DCItem::reloadCache(dci=\"%s\", node=%s [%d]): %d values missing in DB"),
                     m_name.cstr(), getOwnerName(), m_ownerId, m_requiredCacheSize - i);
            for(; i < m_requiredCacheSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
         DBFreeResult(hResult);
      }
      else
      {
         // Error reading data from database, fill cache with empty values
         for(uint32_t i = 0; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), 1);
      }

      m_cacheSize = m_requiredCacheSize;
      m_bCacheLoaded = true;
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
      msg->setField(VID_DCI_DATA_TYPE, static_cast<uint16_t>(m_dataType));
      msg->setField(VID_VALUE, m_ppValueCache[0]->getString());
      msg->setField(VID_RAW_VALUE, m_prevRawValue.getString());
      msg->setFieldFromTime(VID_TIMESTAMP, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      msg->setField(VID_DCI_DATA_TYPE, static_cast<uint16_t>(DCI_DT_NULL));
      msg->setField(VID_VALUE, _T(""));
      msg->setField(VID_RAW_VALUE, _T(""));
      msg->setField(VID_TIMESTAMP, static_cast<uint32_t>(0));
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
      msg->setField(baseId++, static_cast<uint16_t>(m_dataType));
      msg->setField(baseId++, m_ppValueCache[0]->getString());
      msg->setFieldFromTime(baseId++, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      msg->setField(baseId++, static_cast<uint16_t>(DCI_DT_NULL));
      msg->setField(baseId++, _T(""));
      msg->setField(baseId++, static_cast<uint32_t>(0));
   }
   msg->setField(baseId++, static_cast<uint16_t>(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
	msg->setField(baseId++, static_cast<uint16_t>(getType()));
	msg->setField(baseId++, m_errorCount);
	msg->setField(baseId++, m_templateItemId);
   msg->setField(baseId++, m_unitName);
   msg->setField(baseId++, m_multiplier);
   msg->setField(baseId++, !hasValue());
   msg->setField(baseId++, m_comments);

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
   if (!m_bCacheLoaded)
      reloadCache(false);

   NXSL_Value *value;

	lock();
   switch(function)
   {
      case F_LAST:
         // cache placeholders will have timestamp 1
         value = (m_bCacheLoaded && (m_cacheSize > 0) && (m_ppValueCache[0]->getTimeStamp() != 1)) ? vm->createValue(m_ppValueCache[0]->getString()) : vm->createValue();
         CastNXSLValue(value, m_dataType);
         break;
      case F_DIFF:
         if (m_bCacheLoaded && (m_cacheSize >= 2))
         {
            ItemValue result;
            CalculateItemValueDiff(&result, m_dataType, *m_ppValueCache[0], *m_ppValueCache[1]);
            value = vm->createValue(result.getString());
         }
         else
         {
            value = vm->createValue();
         }
         break;
      case F_AVERAGE:
         if (m_bCacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueAverage(&result, m_dataType, m_ppValueCache, std::min(m_cacheSize, static_cast<uint32_t>(sampleCount)));
            value = vm->createValue(result.getString());
            CastNXSLValue(value, m_dataType);
         }
         else
         {
            value = vm->createValue();
         }
         break;
      case F_MEAN_DEVIATION:
         if (m_bCacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueMeanDeviation(&result, m_dataType, m_ppValueCache, std::min(m_cacheSize, static_cast<uint32_t>(sampleCount)));
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
                  _T("SELECT %s(idata_value::double precision) FROM idata_sc_%s WHERE item_id=? AND idata_timestamp BETWEEN to_timestamp(?) AND to_timestamp(?) AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
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
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<int32_t>(periodStart));
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(periodEnd));
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
bool DCItem::deleteEntry(time_t timestamp)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   lock();

   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         _sntprintf(query, 256, _T("DELETE FROM idata_sc_%s WHERE item_id=%u AND idata_timestamp=to_timestamp(") UINT64_FMT _T(")"),
                  getStorageClassName(getStorageClass()), m_id, static_cast<uint64_t>(timestamp));
      }
      else
      {
         _sntprintf(query, 256, _T("DELETE FROM idata WHERE item_id=%d AND idata_timestamp=") UINT64_FMT, m_id, static_cast<uint64_t>(timestamp));
      }
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM idata_%d WHERE item_id=%d AND idata_timestamp=") UINT64_FMT,
               m_ownerId, m_id, static_cast<uint64_t>(timestamp));
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
      m_thresholds->get(i)->setDataType(m_dataType);

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
void DCItem::createExportRecord(StringBuffer &xml) const
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
   xml.append(_T("</dataType>\n\t\t\t\t\t<samples>"));
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
   xml.append(_T("</systemTag>\n\t\t\t\t\t<delta>"));
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
   xml.append(_T("</multiplier>\n"));

	if (m_transformationScriptSource != nullptr)
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

	if (m_pszPerfTabSettings != nullptr)
	{
		xml.append(_T("\t\t\t\t\t<perfTabSettings>"));
		xml.appendPreallocated(EscapeStringForXML(m_pszPerfTabSettings, -1));
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

   if (m_instanceFilterSource != nullptr)
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
         if (pValue != nullptr)
         {
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
			}
			else
			{
				_tcslcpy(buffer, _T("(null)"), bufSize);
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
	return m_bCacheLoaded;
}

/**
 * Create DCI from import file
 */
void DCItem::updateFromImport(ConfigEntry *config)
{
   DCObject::updateFromImport(config);

   lock();
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_snmpRawValueType = (WORD)config->getSubEntryValueAsInt(_T("snmpRawValueType"));
   m_unitName = config->getSubEntryValue(_T("unitName"));
   m_multiplier = config->getSubEntryValueAsInt(_T("multiplier"));

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
         m_thresholds->add(new Threshold(thresholds->get(i), this));
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
   json_object_set_new(root, "sampleCount", json_integer(m_sampleCount));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
   json_object_set_new(root, "prevRawValue", json_string_t(m_prevRawValue));
   json_object_set_new(root, "prevValueTimeStamp", json_integer(m_tPrevValueTimeStamp));
   json_object_set_new(root, "multiplier", json_integer(m_multiplier));
   json_object_set_new(root, "unitName", json_string_t(m_unitName));
   json_object_set_new(root, "snmpRawValueType", json_integer(m_snmpRawValueType));
   json_object_set_new(root, "predictionEngine", json_string_t(m_predictionEngine));
   unlock();
   return root;
}

/**
 * Prepare DCI object for recalculation (should be executed on DCI copy)
 */
void DCItem::prepareForRecalc()
{
   m_tPrevValueTimeStamp = 0;
   m_lastPoll = 0;
   updateCacheSizeInternal(false);
}

/**
 * Recalculate old value (should be executed on DCI copy)
 */
void DCItem::recalculateValue(ItemValue &value)
{
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = value;  // Delta should be zero for first poll
   ItemValue rawValue = value;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   auto owner = m_owner.lock();
   if ((owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(value, (value.getTimeStamp() > m_tPrevValueTimeStamp) ? (value.getTimeStamp() - m_tPrevValueTimeStamp) : 0))
      {
         return;
      }
   }

   if (value.getTimeStamp() > m_tPrevValueTimeStamp)
   {
      m_prevRawValue = rawValue;
      m_tPrevValueTimeStamp = value.getTimeStamp();
   }

   if ((m_cacheSize > 0) && (value.getTimeStamp() >= m_tPrevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = new ItemValue(value);
   }

   m_lastPoll = value.getTimeStamp();
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
