/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

#ifdef WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Event parameter names
 */
static const TCHAR *s_paramNamesReach[] = { _T("dciName"), _T("dciDescription"), _T("thresholdValue"), _T("currentValue"), _T("dciId"), _T("instance"), _T("isRepeatedEvent"), _T("dciValue") };
static const TCHAR *s_paramNamesRearm[] = { _T("dciName"), _T("dciDescription"), _T("dciId"), _T("instance"), _T("thresholdValue"), _T("currentValue"), _T("dciValue") };

/**
 * Default constructor for DCItem
 */
DCItem::DCItem() : DCObject()
{
   m_thresholds = NULL;
   m_dataType = DCI_DT_INT;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_predictionEngine[0] = 0;
}

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
      for(UINT32 i = 0; i < m_cacheSize; i++)
         m_ppValueCache[i] = new ItemValue(src->m_ppValueCache[i]);
   }
   else
   {
      m_ppValueCache = NULL;
   }
   m_tPrevValueTimeStamp = shadowCopy ? src->m_tPrevValueTimeStamp : 0;
   m_bCacheLoaded = shadowCopy ? src->m_bCacheLoaded : false;
	m_nBaseUnits = src->m_nBaseUnits;
	m_nMultiplier = src->m_nMultiplier;
	m_customUnitName = MemCopyString(src->m_customUnitName);
	m_snmpRawValueType = src->m_snmpRawValueType;
   _tcscpy(m_predictionEngine, src->m_predictionEngine);

   // Copy thresholds
	if (src->getThresholdCount() > 0)
	{
		m_thresholds = new ObjectArray<Threshold>(src->m_thresholds->size(), 8, true);
		for(int i = 0; i < src->m_thresholds->size(); i++)
		{
			Threshold *t = new Threshold(src->m_thresholds->get(i), shadowCopy);
			m_thresholds->add(t);
		}
	}
	else
	{
		m_thresholds = NULL;
	}
}

/**
 * Constructor for creating DCItem from database
 * Assumes that fields in SELECT query are in following order:
 *    item_id,name,source,datatype,polling_interval,retention_time,status,
 *    delta_calculation,transformation,template_id,description,instance,
 *    template_item_id,flags,resource_id,proxy_node,base_units,unit_multiplier,
 *    custom_units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type,
 *    instd_method,instd_data,instd_filter,samples,comments,guid,npe_name,
 *    instance_retention_time, grace_period_start
 */
DCItem::DCItem(DB_HANDLE hdb, DB_RESULT hResult, int iRow, DataCollectionOwner *pNode, bool useStartupDelay) : DCObject()
{
   m_owner = pNode;
   m_id = DBGetFieldULong(hResult, iRow, 0);
   DBGetField(hResult, iRow, 1, m_name, MAX_ITEM_NAME);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_dataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_deltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   TCHAR *pszTmp = DBGetField(hResult, iRow, 8, NULL, 0);
   setTransformationScript(pszTmp);
   MemFree(pszTmp);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 10, m_description, MAX_DB_STRING);
   m_instance = DBGetField(hResult, iRow, 11, NULL, 0);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_thresholds = NULL;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 13);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 14);
	m_sourceNode = DBGetFieldULong(hResult, iRow, 15);
	m_nBaseUnits = DBGetFieldLong(hResult, iRow, 16);
	m_nMultiplier = DBGetFieldLong(hResult, iRow, 17);
	m_customUnitName = DBGetField(hResult, iRow, 18, NULL, 0);
	m_pszPerfTabSettings = DBGetField(hResult, iRow, 19, NULL, 0);
	DBGetField(hResult, iRow, 20, m_systemTag, MAX_DB_STRING);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, iRow, 21);
	m_snmpRawValueType = (WORD)DBGetFieldLong(hResult, iRow, 22);
	m_instanceDiscoveryMethod = (WORD)DBGetFieldLong(hResult, iRow, 23);
	m_instanceDiscoveryData = DBGetField(hResult, iRow, 24, NULL, 0);
	m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   pszTmp = DBGetField(hResult, iRow, 25, NULL, 0);
	setInstanceFilter(pszTmp);
   MemFree(pszTmp);
	m_sampleCount = DBGetFieldLong(hResult, iRow, 26);
   m_comments = DBGetField(hResult, iRow, 27, NULL, 0);
   m_guid = DBGetFieldGUID(hResult, iRow, 28);
   DBGetField(hResult, iRow, 29, m_predictionEngine, MAX_NPE_NAME_LEN);
   m_instanceRetentionTime = DBGetFieldLong(hResult, iRow, 30);
   m_instanceGracePeriodStart = DBGetFieldLong(hResult, iRow, 31);

   int effectivePollingInterval = getEffectivePollingInterval();
   m_startTime = (useStartupDelay && (effectivePollingInterval > 0)) ? time(NULL) + rand() % (effectivePollingInterval / 2) : 0;

   // Load last raw value from database
	TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d"), m_id);
   DB_RESULT hTempResult = DBSelect(hdb, szQuery);
   if (hTempResult != NULL)
   {
      if (DBGetNumRows(hTempResult) > 0)
      {
		   TCHAR szBuffer[MAX_DB_STRING];
         m_prevRawValue = DBGetField(hTempResult, 0, 0, szBuffer, MAX_DB_STRING);
         m_tPrevValueTimeStamp = DBGetFieldULong(hTempResult, 0, 1);
         m_tLastPoll = m_tPrevValueTimeStamp;
      }
      DBFreeResult(hTempResult);
   }

   loadAccessList(hdb);
	loadCustomSchedules(hdb);
}

/**
 * Constructor for creating new DCItem from scratch
 */
DCItem::DCItem(UINT32 dwId, const TCHAR *szName, int iSource, int iDataType,
               int iPollingInterval, int iRetentionTime, DataCollectionOwner *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
	: DCObject(dwId, szName, iSource, iPollingInterval, iRetentionTime, pNode, pszDescription, systemTag)
{
   m_dataType = iDataType;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_thresholds = NULL;
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_predictionEngine[0] = 0;

   updateCacheSizeInternal(false);
}

/**
 * Create DCItem from import file
 */
DCItem::DCItem(ConfigEntry *config, DataCollectionOwner *owner) : DCObject(config, owner)
{
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_cacheSize = 0;
   m_requiredCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = (WORD)config->getSubEntryValueAsInt(_T("snmpRawValueType"));
   _tcslcpy(m_predictionEngine, config->getSubEntryValue(_T("predictionEngine"), 0, _T("")), MAX_NPE_NAME_LEN);

   // for compatibility with old format
	if (config->getSubEntryValueAsInt(_T("allThresholds")))
		m_flags |= DCF_ALL_THRESHOLDS;
	if (config->getSubEntryValueAsInt(_T("rawValueInOctetString")))
		m_flags |= DCF_RAW_VALUE_OCTET_STRING;

	ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
	if (thresholdsRoot != NULL)
	{
		ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
		m_thresholds = new ObjectArray<Threshold>(thresholds->size(), 8, true);
		for(int i = 0; i < thresholds->size(); i++)
		{
			m_thresholds->add(new Threshold(thresholds->get(i), this));
		}
		delete thresholds;
	}
	else
	{
		m_thresholds = NULL;
	}

	updateCacheSizeInternal(false);
}

/**
 * Destructor
 */
DCItem::~DCItem()
{
	delete m_thresholds;
	MemFree(m_customUnitName);
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
   for(UINT32 i = 0; i < m_cacheSize; i++)
      delete m_ppValueCache[i];
   MemFree(m_ppValueCache);
   m_ppValueCache = NULL;
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
				  _T("last_checked_value FROM thresholds WHERE item_id=? ")
              _T("ORDER BY sequence_number"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			if (count > 0)
			{
				m_thresholds = new ObjectArray<Threshold>(count, 8, true);
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
      _T("flags"), _T("resource_id"), _T("proxy_node"), _T("base_units"), _T("unit_multiplier"), _T("custom_units_name"),
      _T("perftab_settings"), _T("system_tag"), _T("snmp_port"), _T("snmp_raw_value_type"), _T("instd_method"), _T("instd_data"),
      _T("instd_filter"), _T("samples"), _T("comments"), _T("guid"), _T("npe_name"), _T("instance_retention_time"),
      _T("grace_period_start"),
      NULL
   };

	DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("items"), _T("item_id"), m_id, columns);
	if (hStmt == NULL)
		return false;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_owner == NULL) ? 0 : m_owner->getId());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)m_dataType);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_iPollingInterval);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_iRetentionTime);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_deltaCalculation);
	DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (UINT32)m_flags);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_sourceNode);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, (INT32)m_nBaseUnits);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (INT32)m_nMultiplier);
	DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, m_customUnitName, DB_BIND_STATIC);
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_pszPerfTabSettings, DB_BIND_STATIC);
	DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC);
	DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, (INT32)m_snmpPort);
	DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, (INT32)m_snmpRawValueType);
	DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, (INT32)m_instanceDiscoveryMethod);
	DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
	DBBind(hStmt, 26, DB_SQLTYPE_TEXT, m_instanceFilterSource, DB_BIND_STATIC);
	DBBind(hStmt, 27, DB_SQLTYPE_INTEGER, (INT32)m_sampleCount);
   DBBind(hStmt, 28, DB_SQLTYPE_TEXT, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 30, DB_SQLTYPE_VARCHAR, m_predictionEngine, DB_BIND_STATIC);
   DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, m_instanceRetentionTime);
   DBBind(hStmt, 32, DB_SQLTYPE_INTEGER, (INT32)m_instanceGracePeriodStart);
   DBBind(hStmt, 33, DB_SQLTYPE_INTEGER, m_id);

   bool bResult = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save thresholds
   if (bResult && (m_thresholds != NULL))
   {
		for(int i = 0; i < m_thresholds->size(); i++)
         m_thresholds->get(i)->saveToDB(hdb, i);
   }

   // Delete non-existing thresholds
	TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT threshold_id FROM thresholds WHERE item_id=%d"), m_id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      int iNumRows = DBGetNumRows(hResult);
      for(int i = 0; i < iNumRows; i++)
      {
         UINT32 dwId = DBGetFieldULong(hResult, i, 0);
			int j;
			for(j = 0; j < getThresholdCount(); j++)
				if (m_thresholds->get(j)->getId() == dwId)
					break;
         if (j == getThresholdCount())
         {
            _sntprintf(query, 256, _T("DELETE FROM thresholds WHERE threshold_id=%d"), dwId);
            DBQuery(hdb, query);
         }
      }
      DBFreeResult(hResult);
   }

   // Create record in raw_dci_values if needed
   if (!IsDatabaseRecordExist(hdb, _T("raw_dci_values"), _T("item_id"), m_id))
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (?,?,?)"));
      if (hStmt == NULL)
      {
         unlock();
         return false;
      }
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_prevRawValue.getString(), DB_BIND_STATIC, 255);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT64)m_tPrevValueTimeStamp);
      bResult = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   unlock();
	return bResult ? DCObject::saveToDatabase(hdb) : false;
}

/**
 * Check last value for threshold violations
 */
void DCItem::checkThresholds(ItemValue &value)
{
	if (m_thresholds == NULL)
		return;

	bool thresholdDeactivated = false;
   for(int i = 0; i < m_thresholds->size(); i++)
   {
		Threshold *t = m_thresholds->get(i);
      ItemValue checkValue;
      ThresholdCheckResult result = t->check(value, m_ppValueCache, checkValue, m_owner, this);
      t->setLastCheckedValue(checkValue);
      switch(result)
      {
         case ThresholdCheckResult::ACTIVATED:
            {
               PostDciEventWithNames(t->getEventCode(), m_owner->getId(), m_id, "ssssisds",
					   s_paramNamesReach, m_name, m_description, t->getStringValue(),
                  (const TCHAR *)checkValue, m_id, m_instance, 0, (const TCHAR *)value);
				   EventTemplate *evt = FindEventTemplateByCode(t->getEventCode());
				   if (evt != NULL)
				   {
					   t->markLastEvent(evt->getSeverity());
					   evt->decRefCount();
				   }
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing
            }
            NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), NULL, result);
            break;
         case ThresholdCheckResult::DEACTIVATED:
            PostDciEventWithNames(t->getRearmEventCode(), m_owner->getId(), m_id, "ssissss",
					s_paramNamesRearm, m_name, m_description, m_id, m_instance,
					t->getStringValue(), (const TCHAR *)checkValue, (const TCHAR *)value);
            if (!(m_flags & DCF_ALL_THRESHOLDS))
            {
               // this flag used to re-send activation event for next active threshold
               thresholdDeactivated = true;
            }
            NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), NULL, result);
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
	            time_t now = time(NULL);
               UINT32 repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : (UINT32)t->getRepeatInterval();
				   if (thresholdDeactivated || ((repeatInterval != 0) && (t->getLastEventTimestamp() + (time_t)repeatInterval < now)))
				   {
                  PostDciEventWithNames(t->getEventCode(), m_owner->getId(), m_id, "ssssisds",
                     s_paramNamesReach, m_name, m_description, t->getStringValue(),
                           (const TCHAR *)checkValue, m_id, m_instance, 1, (const TCHAR *)value);
                  EventTemplate *evt = FindEventTemplateByCode(t->getEventCode());
					   if (evt != NULL)
					   {
						   t->markLastEvent(evt->getSeverity());
						   evt->decRefCount();
					   }
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
   pMsg->setField(VID_DCI_DATA_TYPE, (WORD)m_dataType);
   pMsg->setField(VID_DCI_DELTA_CALCULATION, (WORD)m_deltaCalculation);
   pMsg->setField(VID_SAMPLE_COUNT, (WORD)m_sampleCount);
	pMsg->setField(VID_BASE_UNITS, (WORD)m_nBaseUnits);
	pMsg->setField(VID_MULTIPLIER, (UINT32)m_nMultiplier);
	pMsg->setField(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	pMsg->setField(VID_NPE_NAME, m_predictionEngine);
	if (m_customUnitName != NULL)
		pMsg->setField(VID_CUSTOM_UNITS_NAME, m_customUnitName);
	if (m_thresholds != NULL)
	{
		pMsg->setField(VID_NUM_THRESHOLDS, (UINT32)m_thresholds->size());
		UINT32 dwId = VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < m_thresholds->size(); i++, dwId += 20)
			m_thresholds->get(i)->createMessage(pMsg, dwId);
	}
	else
	{
		pMsg->setField(VID_NUM_THRESHOLDS, (UINT32)0);
	}
   unlock();
}

/**
 * Delete item and collected data from database
 */
void DCItem::deleteFromDatabase()
{
   TCHAR szQuery[256];

	DCObject::deleteFromDatabase();

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM items WHERE item_id=%d"), m_id);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM thresholds WHERE item_id=%d"), m_id);
   QueueSQLRequest(szQuery);
   QueueRawDciDataDelete(m_id);

   if (m_owner->isDataCollectionTarget())
      static_cast<DataCollectionTarget*>(m_owner)->scheduleItemDataCleanup(m_id);
}

/**
 * Update item from NXCP message
 */
void DCItem::updateFromMessage(NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId)
{
	DCObject::updateFromMessage(pMsg);

   lock();

   m_dataType = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_DATA_TYPE);
   m_deltaCalculation = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_DELTA_CALCULATION);
	m_sampleCount = (int)pMsg->getFieldAsUInt16(VID_SAMPLE_COUNT);
	m_nBaseUnits = pMsg->getFieldAsUInt16(VID_BASE_UNITS);
	m_nMultiplier = (int)pMsg->getFieldAsUInt32(VID_MULTIPLIER);
	MemFree(m_customUnitName);
	m_customUnitName = pMsg->getFieldAsString(VID_CUSTOM_UNITS_NAME);
	m_snmpRawValueType = pMsg->getFieldAsUInt16(VID_SNMP_RAW_VALUE_TYPE);
   pMsg->getFieldAsString(VID_NPE_NAME, m_predictionEngine, MAX_NPE_NAME_LEN);

   // Update thresholds
   UINT32 dwNum = pMsg->getFieldAsUInt32(VID_NUM_THRESHOLDS);
   UINT32 *newThresholds = MemAllocArray<UINT32>(dwNum);
   *ppdwMapIndex = MemAllocArray<UINT32>(dwNum);
   *ppdwMapId = MemAllocArray<UINT32>(dwNum);
   *pdwNumMaps = 0;

   // Read all new threshold ids from message
   for(UINT32 i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      newThresholds[i] = pMsg->getFieldAsUInt32(dwId);
   }

   // Check if some thresholds was deleted, and reposition others if needed
   Threshold **ppNewList = MemAllocArray<Threshold*>(dwNum);
   for(int i = 0; i < getThresholdCount(); i++)
   {
		UINT32 j;
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
   for(UINT32 i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      if (newThresholds[i] == 0)    // New threshold?
      {
         ppNewList[i] = new Threshold(this);
         ppNewList[i]->createId();

         // Add index -> id mapping
         (*ppdwMapIndex)[*pdwNumMaps] = i;
         (*ppdwMapId)[*pdwNumMaps] = ppNewList[i]->getId();
         (*pdwNumMaps)++;
      }
      if (ppNewList[i] != NULL)
         ppNewList[i]->updateFromMessage(pMsg, dwId);
   }

	if (dwNum > 0)
	{
		if (m_thresholds != NULL)
		{
			m_thresholds->setOwner(false);
			m_thresholds->clear();
			m_thresholds->setOwner(true);
		}
		else
		{
			m_thresholds = new ObjectArray<Threshold>((int)dwNum, 8, true);
		}
		for(UINT32 i = 0; i < dwNum; i++)
      {
         if (ppNewList[i] != NULL)
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
bool DCItem::processNewValue(time_t tmTimeStamp, const void *originalValue, bool *updateStatus)
{
   ItemValue rawValue, *pValue;

   lock();

   // Normally m_owner shouldn't be NULL for polled items, but who knows...
   if (m_owner == NULL)
   {
      unlock();
      return false;
   }

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue((const TCHAR *)originalValue, tmTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((m_owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(*pValue, (tmTimeStamp > m_tPrevValueTimeStamp) ? (tmTimeStamp - m_tPrevValueTimeStamp) : 0))
      {
         unlock();
         delete pValue;
         return false;
      }
   }

   m_dwErrorCount = 0;

   if (isStatusDCO() && (tmTimeStamp > m_tPrevValueTimeStamp) && ((m_cacheSize == 0) || !m_bCacheLoaded || ((UINT32)*pValue != (UINT32)*m_ppValueCache[0])))
   {
      *updateStatus = true;
   }
   else
   {
      *updateStatus = false;
   }

   if (tmTimeStamp > m_tPrevValueTimeStamp)
   {
      m_prevRawValue = rawValue;
      m_tPrevValueTimeStamp = tmTimeStamp;

      // Save raw value into database
      QueueRawDciDataUpdate(tmTimeStamp, m_id, (const TCHAR *)originalValue, pValue->getString());
   }

	// Save transformed value to database
   if ((m_flags & DCF_NO_STORAGE) == 0)
	   QueueIDataInsert(tmTimeStamp, m_owner->getId(), m_id, (const TCHAR *)originalValue, pValue->getString(), getStorageClass());
   if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      PerfDataStorageRequest(this, tmTimeStamp, pValue->getString());

   // Update prediction engine
   if (m_predictionEngine[0] != 0)
   {
      PredictionEngine *engine = FindPredictionEngine(m_predictionEngine);
      if (engine != NULL)
         engine->update(m_owner->getId(), m_id, tmTimeStamp, pValue->getDouble());
   }

   // Check thresholds and add value to cache
   if (m_bCacheLoaded && (tmTimeStamp >= m_tPrevValueTimeStamp) &&
       ((g_offlineDataRelevanceTime <= 0) || (tmTimeStamp > (time(NULL) - g_offlineDataRelevanceTime))))
   {
      if (hasScriptThresholds())
      {
         // Run threshold check with DCI unlocked if there are script thresholds
         // to avoid possible server deadlock if script causes agent reconnect
         DCItem *shadowCopy = new DCItem(this, true);
         unlock();
         shadowCopy->checkThresholds(*pValue);
         lock();

         // Reconcile threshold updates
         for(int i = 0; i < shadowCopy->m_thresholds->size(); i++)
         {
            Threshold *src = shadowCopy->m_thresholds->get(i);
            Threshold *dst = getThresholdById(src->getId());
            if (dst != NULL)
            {
               dst->reconcile(src);
            }
         }

         delete shadowCopy;
      }
      else
      {
         checkThresholds(*pValue);
      }
   }

   if ((m_cacheSize > 0) && (tmTimeStamp >= m_tPrevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = pValue;
   }
   else if (!m_bCacheLoaded && (m_requiredCacheSize == 1))
   {
      // If required cache size is 1 and we got value before cache loader
      // loads DCI cache then update it directly
      for(int i = 0; i < m_cacheSize; i++)
         delete m_ppValueCache[i];

      if (m_cacheSize != m_requiredCacheSize)
      {
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
         m_cacheSize = m_requiredCacheSize;
      }

      m_ppValueCache[0] = pValue;
      m_bCacheLoaded = true;
   }
   else
   {
      delete pValue;
   }

   unlock();

#ifdef WITH_ZMQ
   ZmqPublishData(m_owner->getId(), m_id, m_name, pValue->getString());
#endif

   return true;
}

/**
 * Process new data collection error
 */
void DCItem::processNewError(bool noInstance, time_t now)
{
   lock();

   // Normally m_owner shouldn't be NULL for polled items, but who knows...
   if (m_owner == NULL)
   {
      unlock();
      return;
   }

   m_dwErrorCount++;

	for(int i = 0; i < getThresholdCount(); i++)
   {
		Threshold *t = m_thresholds->get(i);
      ThresholdCheckResult result = t->checkError(m_dwErrorCount);
      switch(result)
      {
         case ThresholdCheckResult::ACTIVATED:
            {
               PostDciEventWithNames(t->getEventCode(), m_owner->getId(), m_id, "ssssisds",
					   s_paramNamesReach, m_name, m_description, _T(""), _T(""),
                  m_id, m_instance, 0, _T(""));
               EventTemplate *evt = FindEventTemplateByCode(t->getEventCode());
				   if (evt != NULL)
				   {
					   t->markLastEvent(evt->getSeverity());
					   evt->decRefCount();
				   }
               if (!(m_flags & DCF_ALL_THRESHOLDS))
               {
                  i = m_thresholds->size();  // Stop processing
               }
            }
            NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), NULL, result);
            break;
         case ThresholdCheckResult::DEACTIVATED:
            PostDciEventWithNames(t->getRearmEventCode(), m_owner->getId(), m_id, "ssissss",
					s_paramNamesRearm, m_name, m_description, m_id, m_instance, _T(""), _T(""), _T(""));
            NotifyClientsOnThresholdChange(m_owner->getId(), m_id, t->getId(), NULL, result);
            break;
         case ThresholdCheckResult::ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
               UINT32 repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : (UINT32)t->getRepeatInterval();
				   if ((repeatInterval != 0) && (t->getLastEventTimestamp() + (time_t)repeatInterval < now))
				   {
					   PostDciEventWithNames(t->getEventCode(), m_owner->getId(), m_id, "ssssisds",
						   s_paramNamesReach, m_name, m_description, _T(""), _T(""),
						   m_id, m_instance, 1, _T(""));
					   EventTemplate *evt = FindEventTemplateByCode(t->getEventCode());
					   if (evt != NULL)
					   {
						   t->markLastEvent(evt->getSeverity());
						   evt->decRefCount();
					   }
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
void DCItem::updateThresholdsBeforeMaintenanceState()
{
   lock();
   for(int i = 0; i < getThresholdCount(); i++)
   {
      m_thresholds->get(i)->updateBeforeMaintenanceState();
   }
   unlock();
}

/**
 * Generate events that persist after maintenance
 */
void DCItem::generateEventsBasedOnThrDiff()
{
   lock();
   int trCount = getThresholdCount() - 1;
   for(int i = trCount; i >= 0; i--)//go backwards to generate events in correct order
   {
      Threshold *t = m_thresholds->get(i);
      if(t->isReached() != t->wasReachedBeforeMaintenance())
      {
         if(t->isReached())
         {
            PostDciEventWithNames(t->getEventCode(), m_owner->getId(), m_id, "ssssisds",
                              s_paramNamesReach, m_name, m_description, t->getStringValue(),
                              t->getLastCheckValue().getString(), m_id, m_instance, 0,
                              (m_bCacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""));
         }
         else
         {
            PostDciEventWithNames(t->getRearmEventCode(), m_owner->getId(), m_id, "ssissss",
                              s_paramNamesRearm, m_name, m_description, m_id, m_instance, t->getStringValue(),
                              t->getLastCheckValue().getString(),
                              (m_bCacheLoaded && (m_cacheSize > 0)) ? m_ppValueCache[0]->getString() : _T(""));
         }
      }
   }
   unlock();
}

/**
 * Transform received value
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
               value = (INT32)((_tcscmp(value.getString(), m_prevRawValue.getString()) == 0) ? 0 : 1);
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
               value = (INT32)((_tcscmp(value.getString(), m_prevRawValue.getString()) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      default:    // Default is no transformation
         break;
   }

   if (m_transformationScript != NULL)
   {
      NXSL_VM *vm = CreateServerScriptVM(m_transformationScript, m_owner, this);
      if (vm != NULL)
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
            if (nxslValue != NULL)
            {
               switch(m_dataType)
               {
                  case DCI_DT_INT:
                     value = nxslValue->getValueAsInt32();
                     break;
                  case DCI_DT_UINT:
                  case DCI_DT_COUNTER32:
                     value = nxslValue->getValueAsUInt32();
                     break;
                  case DCI_DT_INT64:
                     value = nxslValue->getValueAsInt64();
                     break;
                  case DCI_DT_UINT64:
                  case DCI_DT_COUNTER64:
                     value = nxslValue->getValueAsUInt64();
                     break;
                  case DCI_DT_FLOAT:
                     value = nxslValue->getValueAsReal();
                     break;
                  case DCI_DT_STRING:
                     value = CHECK_NULL_EX(nxslValue->getValueAsCString());
                     break;
                  default:
                     break;
               }
            }
         }
         else if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
         {
            DbgPrintf(6, _T("Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted"),
                      m_description, m_id, getOwnerName(), getOwnerId());
         }
         else
         {
            time_t now = time(NULL);
            if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
            {
               TCHAR buffer[1024];
               _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
               PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, vm->getErrorText(), m_id);
               nxlog_write(NXLOG_WARNING, _T("Failed to execute transformation script for object %s [%u] DCI %s [%u] (%s)"),
                        getOwnerName(), getOwnerId(), m_name, m_id, vm->getErrorText());
               m_lastScriptErrorReport = now;
            }
         }
         delete vm;
      }
      else
      {
         time_t now = time(NULL);
         if (m_lastScriptErrorReport + ConfigReadInt(_T("DataCollection.ScriptErrorReportInterval"), 86400) < now)
         {
            TCHAR buffer[1024];
            _sntprintf(buffer, 1024, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id);
            PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", buffer, _T("Script load failed"), m_id);
            nxlog_write(NXLOG_WARNING, _T("Failed to load transformation script for object %s [%u] DCI %s [%u]"),
                     getOwnerName(), getOwnerId(), m_name, m_id);
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
void DCItem::changeBinding(UINT32 dwNewId, DataCollectionOwner *pNewNode, BOOL doMacroExpansion)
{
	DCObject::changeBinding(dwNewId, pNewNode, doMacroExpansion);

   lock();
	if (dwNewId != 0)
	{
		for(int i = 0; i < getThresholdCount(); i++)
         m_thresholds->get(i)->bindToItem(m_id, m_owner->getId());
	}

   clearCache();
   updateCacheSizeInternal(true);
   unlock();
}

/**
 * Update required cache size depending on thresholds
 * dwCondId is an identifier of calling condition object id. If it is not 0,
 * GetCacheSizeForDCI should be called with bNoLock == TRUE for appropriate
 * condition object
 */
void DCItem::updateCacheSizeInternal(bool allowLoad, UINT32 conditionId)
{
   // Sanity check
   if (m_owner == NULL)
   {
      nxlog_debug(3, _T("DCItem::updateCacheSize() called for DCI %d when m_owner == NULL"), m_id);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if (((m_owner->isDataCollectionTarget() && (m_owner->getObjectClass() != OBJECT_CLUSTER)) ||
        ((m_owner->getObjectClass() == OBJECT_CLUSTER) && isAggregateOnCluster())) &&
       (m_instanceDiscoveryMethod == IDM_NONE))
   {
      UINT32 requiredSize = 1;

      // Calculate required cache size
      for(int i = 0; i < getThresholdCount(); i++)
         if (requiredSize < m_thresholds->get(i)->getRequiredCacheSize())
            requiredSize = m_thresholds->get(i)->getRequiredCacheSize();

		ObjectArray<NetObj> *conditions = g_idxConditionById.getObjects(true);
		for(int i = 0; i < conditions->size(); i++)
      {
		   ConditionObject *c = (ConditionObject *)conditions->get(i);
			UINT32 size = c->getCacheSizeForDCI(m_id, conditionId == c->getId());
         if (size > requiredSize)
            requiredSize = size;
         c->decRefCount();
      }
		delete conditions;

		m_requiredCacheSize = requiredSize;
   }
   else
   {
      m_requiredCacheSize = 0;
   }

   nxlog_debug_tag(_T("obj.dc.cache"), 8, _T("DCItem::updateCacheSizeInternal(dci=\"%s\", node=%s [%d]): requiredSize=%d cacheSize=%d"),
            m_name, m_owner->getName(), m_owner->getId(), m_requiredCacheSize, m_cacheSize);

   // Update cache if needed
   if (m_requiredCacheSize < m_cacheSize)
   {
      // Destroy unneeded values
      if (m_cacheSize > 0)
		{
         for(UINT32 i = m_requiredCacheSize; i < m_cacheSize; i++)
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
         m_ppValueCache = NULL;
      }
   }
   else if (m_requiredCacheSize > m_cacheSize)
   {
      // Load missing values from database
      // Skip caching for DCIs where estimated time to fill the cache is less then 5 minutes
      // to reduce load on database at server startup
      if (allowLoad && (m_owner != NULL) && (((m_requiredCacheSize - m_cacheSize) * m_iPollingInterval > 300) || (m_source == DS_PUSH_AGENT)))
      {
         m_bCacheLoaded = false;
         g_dciCacheLoaderQueue.put(new DCObjectInfo(this));
      }
      else
      {
         // will not read data from database, fill cache with empty values
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
         for(UINT32 i = m_cacheSize; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), 1);
         DbgPrintf(7, _T("Cache load skipped for parameter %s [%d]"), m_name, (int)m_id);
         m_cacheSize = m_requiredCacheSize;
         m_bCacheLoaded = true;
      }
   }
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
                    m_requiredCacheSize, m_owner->getId(), m_id);
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
                    m_owner->getId(), m_id, m_requiredCacheSize);
         }
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC LIMIT %d"),
                    m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC LIMIT %d"),
                    m_owner->getId(), m_id, m_requiredCacheSize);
         }
         break;
      case DB_SYNTAX_DB2:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
               _T("WHERE item_id=%d ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"),
               m_id, m_requiredCacheSize);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
               _T("WHERE item_id=%d ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"),
               m_owner->getId(), m_id, m_requiredCacheSize);
         }
         break;
      default:
         if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"), m_id);
         }
         else
         {
            _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                              _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                    m_owner->getId(), m_id);
         }
         break;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, szBuffer);

   // While reload request was in queue DCI cache may have been already filled
   lock();
   if (forceReload || !m_bCacheLoaded || (m_cacheSize != m_requiredCacheSize))
   {
      UINT32 i;
      for(i = 0; i < m_cacheSize; i++)
         delete m_ppValueCache[i];

      if (m_cacheSize != m_requiredCacheSize)
      {
         m_ppValueCache = MemReallocArray(m_ppValueCache, m_requiredCacheSize);
      }

      nxlog_debug_tag(_T("obj.dc.cache"), 8, _T("DCItem::reloadCache(dci=\"%s\", node=%s [%d]): requiredSize=%d cacheSize=%d"),
               m_name, m_owner->getName(), m_owner->getId(), m_requiredCacheSize, m_cacheSize);
      if (hResult != NULL)
      {
         // Create cache entries
         bool moreData = true;
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
            nxlog_debug_tag(_T("obj.dc.cache"), 8, _T("DCItem::reloadCache(dci=\"%s\", node=%s [%d]): %d values missing in DB"),
                     m_name, m_owner->getName(), m_owner->getId(), m_requiredCacheSize - i);
            for(; i < m_requiredCacheSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
         DBFreeResult(hResult);
      }
      else
      {
         // Error reading data from database, fill cache with empty values
         for(i = 0; i < m_requiredCacheSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), 1);
      }

      m_cacheSize = m_requiredCacheSize;
      m_bCacheLoaded = true;
   }
   unlock();

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Put last value into CSCP message
 */
void DCItem::fillLastValueMessage(NXCPMessage *pMsg, UINT32 dwId)
{
	lock();
   pMsg->setField(dwId++, m_id);
   pMsg->setField(dwId++, m_name);
   pMsg->setField(dwId++, m_flags);
   pMsg->setField(dwId++, m_description);
   pMsg->setField(dwId++, (UINT16)m_source);
   if (m_cacheSize > 0)
   {
      pMsg->setField(dwId++, (UINT16)m_dataType);
      pMsg->setField(dwId++, m_ppValueCache[0]->getString());
      pMsg->setFieldFromTime(dwId++, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      pMsg->setField(dwId++, (UINT16)DCI_DT_NULL);
      pMsg->setField(dwId++, _T(""));
      pMsg->setField(dwId++, (UINT32)0);
   }
   pMsg->setField(dwId++, (WORD)(matchClusterResource() ? m_status : ITEM_STATUS_DISABLED)); // show resource-bound DCIs as inactive if cluster resource is not on this node
	pMsg->setField(dwId++, (WORD)getType());
	pMsg->setField(dwId++, m_dwErrorCount);
	pMsg->setField(dwId++, m_dwTemplateItemId);

	int i;
   for(i = 0; i < getThresholdCount(); i++)
   {
		if (m_thresholds->get(i)->isReached())
			break;
   }
	if (i < getThresholdCount())
	{
      pMsg->setField(dwId++, (WORD)1);
		m_thresholds->get(i)->createMessage(pMsg, dwId);
	}
	else
	{
      pMsg->setField(dwId++, (WORD)0);
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
 * Get item's last value for use in NXSL
 */
NXSL_Value *DCItem::getValueForNXSL(NXSL_VM *vm, int nFunction, int nPolls)
{
   NXSL_Value *pValue;

	lock();
   switch(nFunction)
   {
      case F_LAST:
         // cache placeholders will have timestamp 1
         pValue = (m_bCacheLoaded && (m_cacheSize > 0) && (m_ppValueCache[0]->getTimeStamp() != 1)) ? vm->createValue(m_ppValueCache[0]->getString()) : vm->createValue();
         break;
      case F_DIFF:
         if (m_bCacheLoaded && (m_cacheSize >= 2))
         {
            ItemValue result;
            CalculateItemValueDiff(result, m_dataType, *m_ppValueCache[0], *m_ppValueCache[1]);
            pValue = vm->createValue(result.getString());
         }
         else
         {
            pValue = vm->createValue();
         }
         break;
      case F_AVERAGE:
         if (m_bCacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueAverage(result, m_dataType, m_ppValueCache, std::min(m_cacheSize, (UINT32)nPolls));
            pValue = vm->createValue(result.getString());
         }
         else
         {
            pValue = vm->createValue();
         }
         break;
      case F_DEVIATION:
         if (m_bCacheLoaded && (m_cacheSize > 0))
         {
            ItemValue result;
            CalculateItemValueMD(result, m_dataType, m_ppValueCache, std::min(m_cacheSize, (UINT32)nPolls));
            pValue = vm->createValue(result.getString());
         }
         else
         {
            pValue = vm->createValue();
         }
         break;
      case F_ERROR:
         pValue = vm->createValue((INT32)((m_dwErrorCount >= (UINT32)nPolls) ? 1 : 0));
         break;
      default:
         pValue = vm->createValue();
         break;
   }
	unlock();
   return pValue;
}

/**
 * Get last value
 */
const TCHAR *DCItem::getLastValue()
{
   lock();
   const TCHAR *v = (m_cacheSize > 0) ? (const TCHAR *)m_ppValueCache[0]->getString() : NULL;
   unlock();
   return v;
}

/**
 * Get copy of internal last value object. Caller is responsible for destroying returned object.
 */
ItemValue *DCItem::getInternalLastValue()
{
   lock();
   ItemValue *v = (m_cacheSize > 0) ? new ItemValue(m_ppValueCache[0]) : NULL;
   unlock();
   return v;
}

/**
 * Get aggregate value. Returned value must be deallocated by caller.
 *
 * @return dynamically allocated value or NULL on error
 */
TCHAR *DCItem::getAggregateValue(AggregationFunction func, time_t periodStart, time_t periodEnd)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	TCHAR query[1024];
   TCHAR *result = NULL;

   static const TCHAR *functions[] = { _T(""), _T("min"), _T("max"), _T("avg"), _T("sum") };

   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_ORACLE)
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata ")
            _T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
            functions[func]);
      }
      else if (g_dbSyntax == DB_SYNTAX_MSSQL)
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata ")
            _T("WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"),
            functions[func]);
      }
      else if ((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
      {
         _sntprintf(query, 1024, _T("SELECT %s(idata_value::double precision) FROM idata ")
            _T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
            functions[func]);
      }
      else
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value,0)) FROM idata ")
            _T("WHERE item_id=? and idata_timestamp between ? and ?"),
            functions[func]);
      }
   }
   else
   {
      if (g_dbSyntax == DB_SYNTAX_ORACLE)
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata_%u ")
            _T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
            functions[func], m_owner->getId());
      }
      else if (g_dbSyntax == DB_SYNTAX_MSSQL)
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata_%u ")
            _T("WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"),
            functions[func], m_owner->getId());
      }
      else if ((g_dbSyntax == DB_SYNTAX_PGSQL) || (g_dbSyntax == DB_SYNTAX_TSDB))
      {
         _sntprintf(query, 1024, _T("SELECT %s(idata_value::double precision) FROM idata_%u ")
            _T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ? AND idata_value~E'^\\\\d+(\\\\.\\\\d+)*$'"),
            functions[func], m_owner->getId());
      }
      else
      {
         _sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value,0)) FROM idata_%u ")
            _T("WHERE item_id=? and idata_timestamp between ? and ?"),
            functions[func], m_owner->getId());
      }
   }

	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)periodStart);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)periodEnd);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) == 1)
			{
				result = DBGetField(hResult, 0, 0, NULL, 0);
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
         _sntprintf(query, 256, _T("DELETE FROM idata_sc_%s WHERE item_id=%d"), getStorageClassName(getStorageClass()), m_id);
      else
         _sntprintf(query, 256, _T("DELETE FROM idata WHERE item_id=%d"), m_id);
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM idata_%d WHERE item_id=%d"), m_owner->getId(), m_id);
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
         _sntprintf(query, 256, _T("DELETE FROM idata_sc_%s WHERE item_id=%d AND idata_timestamp=%d"),
                  getStorageClassName(getStorageClass()), m_id, (int)timestamp);
      }
      else
      {
         _sntprintf(query, 256, _T("DELETE FROM idata WHERE item_id=%d AND idata_timestamp=%d"), m_id, (int)timestamp);
      }
   }
   else
   {
      _sntprintf(query, 256, _T("DELETE FROM idata_%d WHERE item_id=%d AND idata_timestamp=%d"), m_owner->getId(), m_id, (int)timestamp);
   }
   unlock();

   bool success = DBQuery(hdb, query);
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
      return false;

   lock();
   for(UINT32 i = 0; i < m_cacheSize; i++)
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
		DbgPrintf(2, _T("INTERNAL ERROR: DCItem::updateFromTemplate(%d, %d): source type is %d"), (int)m_id, (int)src->getId(), src->getType());
		return;
	}

   lock();
	DCItem *item = (DCItem *)src;

   m_dataType = item->m_dataType;
   m_deltaCalculation = item->m_deltaCalculation;
   m_sampleCount = item->m_sampleCount;
   m_snmpRawValueType = item->m_snmpRawValueType;

	m_nBaseUnits = item->m_nBaseUnits;
	m_nMultiplier = item->m_nMultiplier;
	MemFree(m_customUnitName);
	m_customUnitName = (item->m_customUnitName != NULL) ? _tcsdup(item->m_customUnitName) : NULL;

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
	int count = std::min(getThresholdCount(), item->getThresholdCount());
	int i;
   for(i = 0; i < count; i++)
      if (!m_thresholds->get(i)->equals(item->m_thresholds->get(i)))
         break;
   count = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
	while(count < getThresholdCount())
		m_thresholds->remove(count);

   // (Re)create thresholds starting from first unmatched
	if ((m_thresholds == NULL) && (item->getThresholdCount() > 0))
		m_thresholds = new ObjectArray<Threshold>(item->getThresholdCount(), 8, true);
	for(i = count; i < item->getThresholdCount(); i++)
   {
      Threshold *t = new Threshold(item->m_thresholds->get(i), false);
      t->bindToItem(m_id, m_owner->getId());
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
void DCItem::getEventList(IntegerArray<UINT32> *eventList)
{
   lock();
   if (m_thresholds != NULL)
   {
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         eventList->add(m_thresholds->get(i)->getEventCode());
         eventList->add(m_thresholds->get(i)->getRearmEventCode());
      }
   }
   unlock();
}

/**
 * Create management pack record
 */
void DCItem::createExportRecord(String &str)
{
   lock();

   str.appendFormattedString(_T("\t\t\t\t<dci id=\"%d\">\n")
                             _T("\t\t\t\t\t<guid>%s</guid>\n")
                          _T("\t\t\t\t\t<name>%s</name>\n")
                          _T("\t\t\t\t\t<description>%s</description>\n")
                          _T("\t\t\t\t\t<dataType>%d</dataType>\n")
                          _T("\t\t\t\t\t<samples>%d</samples>\n")
                          _T("\t\t\t\t\t<origin>%d</origin>\n")
                          _T("\t\t\t\t\t<interval>%d</interval>\n")
                          _T("\t\t\t\t\t<retention>%d</retention>\n")
                          _T("\t\t\t\t\t<instance>%s</instance>\n")
                          _T("\t\t\t\t\t<systemTag>%s</systemTag>\n")
                          _T("\t\t\t\t\t<delta>%d</delta>\n")
                          _T("\t\t\t\t\t<flags>%d</flags>\n")
                          _T("\t\t\t\t\t<snmpRawValueType>%d</snmpRawValueType>\n")
                          _T("\t\t\t\t\t<snmpPort>%d</snmpPort>\n")
                          _T("\t\t\t\t\t<instanceDiscoveryMethod>%d</instanceDiscoveryMethod>\n")
                          _T("\t\t\t\t\t<instanceRetentionTime>%d</instanceRetentionTime>\n")
                          _T("\t\t\t\t\t<comments>%s</comments>\n"),
								  (int)m_id, (const TCHAR *)m_guid.toString(),
								  (const TCHAR *)EscapeStringForXML2(m_name),
                          (const TCHAR *)EscapeStringForXML2(m_description),
                          m_dataType, m_sampleCount, (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_instance),
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (int)m_deltaCalculation, (int)m_flags,
								  (int)m_snmpRawValueType, (int)m_snmpPort,
								  (int)m_instanceDiscoveryMethod, m_instanceRetentionTime,
								  (const TCHAR *)EscapeStringForXML2(m_comments));

	if (m_transformationScriptSource != NULL)
	{
		str.append(_T("\t\t\t\t\t<transformation>"));
		str.appendPreallocated(EscapeStringForXML(m_transformationScriptSource, -1));
		str.append(_T("</transformation>\n"));
	}

	if ((m_schedules != NULL) && (m_schedules->size() > 0))
   {
      str.append(_T("\t\t\t\t\t<schedules>\n"));
      for(int i = 0; i < m_schedules->size(); i++)
      {
         str.append(_T("\t\t\t\t\t\t<schedule>"));
         str.appendPreallocated(EscapeStringForXML(m_schedules->get(i), -1));
         str.append(_T("</schedule>\n"));
      }
      str.append(_T("\t\t\t\t\t</schedules>\n"));
   }

	if (m_thresholds != NULL)
	{
	   str.append(_T("\t\t\t\t\t<thresholds>\n"));
		for(int i = 0; i < m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createExportRecord(str, i + 1);
		}
	   str.append(_T("\t\t\t\t\t</thresholds>\n"));
	}

	if (m_pszPerfTabSettings != NULL)
	{
		str.append(_T("\t\t\t\t\t<perfTabSettings>"));
		str.appendPreallocated(EscapeStringForXML(m_pszPerfTabSettings, -1));
		str.append(_T("</perfTabSettings>\n"));
	}

   if (m_instanceDiscoveryData != NULL)
	{
		str.append(_T("\t\t\t\t\t<instanceDiscoveryData>"));
		str.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
		str.append(_T("</instanceDiscoveryData>\n"));
	}

   if (m_instanceFilterSource != NULL)
	{
		str.append(_T("\t\t\t\t\t<instanceFilter>"));
		str.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
		str.append(_T("</instanceFilter>\n"));
	}

   unlock();
   str.append(_T("\t\t\t\t</dci>\n"));
}

/**
 * Add threshold to the list
 */
void DCItem::addThreshold(Threshold *pThreshold)
{
	if (m_thresholds == NULL)
		m_thresholds = new ObjectArray<Threshold>(8, 8, true);
	m_thresholds->add(pThreshold);
}

/**
 * Enumerate all thresholds
 */
BOOL DCItem::enumThresholds(BOOL (* pfCallback)(Threshold *, UINT32, void *), void *pArg)
{
	BOOL bRet = TRUE;

	lock();
	if (m_thresholds != NULL)
	{
		for(int i = 0; i < m_thresholds->size(); i++)
		{
			if (!pfCallback(m_thresholds->get(i), i, pArg))
			{
				bRet = FALSE;
				break;
			}
		}
	}
	unlock();
	return bRet;
}

/**
 * Test DCI's transformation script
 * If dcObjectInfo is not NULL it will be destroyed by this method
 */
bool DCItem::testTransformation(DataCollectionTarget *object, DCObjectInfo *dcObjectInfo, const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize)
{
	bool success = false;
	NXSL_VM *vm = NXSLCompileAndCreateVM(script, buffer, (int)bufSize, new NXSL_ServerEnv);
   if (vm != NULL)
   {
      NXSL_Value *pValue = vm->createValue(value);
      vm->setGlobalVariable("$object", object->createNXSLObject(vm));
      if (object->getObjectClass() == OBJECT_NODE)
      {
         vm->setGlobalVariable("$node", object->createNXSLObject(vm));
      }
      if (dcObjectInfo != NULL)
      {
         vm->setGlobalVariable(_T("$dci"), vm->createValue(new NXSL_Object(vm, &g_nxslDciClass, dcObjectInfo)));
      }
      vm->setGlobalVariable("$isCluster", vm->createValue((object->getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));

		if (vm->run(1, &pValue))
      {
         pValue = vm->getResult();
         if (pValue != NULL)
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
					const TCHAR *strval;

					strval = pValue->getValueAsCString();
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
      delete dcObjectInfo;
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
	UINT32 id = VID_DCI_THRESHOLD_BASE;
	for(int i = 0; i < getThresholdCount(); i++, id += 20)
	{
	   Threshold *threshold = m_thresholds->get(i);
	   if (activeOnly && threshold->isReached())
         threshold->createMessage(msg, id);
	   else
	      threshold->createMessage(msg, id);
	}

	unlock();
}

/**
 * Check if DCI has active threshold
 */
bool DCItem::hasActiveThreshold()
{
	bool result = false;
	lock();
	for(int i = 0; i < getThresholdCount(); i++)
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
int DCItem::getThresholdSeverity()
{
   int result = SEVERITY_NORMAL;
	lock();
	for(int i = 0; i < getThresholdCount(); i++)
	{
      Threshold *t = m_thresholds->get(i);
		if (t->isReached())
		{
			result = t->getCurrentSeverity();
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

   ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
   if (thresholdsRoot != NULL)
   {
      ObjectArray<ConfigEntry> *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
      if (m_thresholds != NULL)
         m_thresholds->clear();
      else
         m_thresholds = new ObjectArray<Threshold>(thresholds->size(), 8, true);
      for(int i = 0; i < thresholds->size(); i++)
      {
         m_thresholds->add(new Threshold(thresholds->get(i), this));
      }
      delete thresholds;
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
   json_object_set_new(root, "deltaCalculation", json_integer(m_deltaCalculation));
   json_object_set_new(root, "dataType", json_integer(m_dataType));
   json_object_set_new(root, "sampleCount", json_integer(m_sampleCount));
   json_object_set_new(root, "thresholds", json_object_array(m_thresholds));
   json_object_set_new(root, "prevRawValue", json_string_t(m_prevRawValue));
   json_object_set_new(root, "prevValueTimeStamp", json_integer(m_tPrevValueTimeStamp));
   json_object_set_new(root, "baseUnits", json_integer(m_nBaseUnits));
   json_object_set_new(root, "multiplier", json_integer(m_nMultiplier));
   json_object_set_new(root, "customUnitName", json_string_t(m_customUnitName));
   json_object_set_new(root, "snmpRawValueType", json_integer(m_snmpRawValueType));
   json_object_set_new(root, "predictionEngine", json_string_t(m_predictionEngine));
   return root;
}

/**
 * Prepare DCI object for recalculation (should be executed on DCI copy)
 */
void DCItem::prepareForRecalc()
{
   m_tPrevValueTimeStamp = 0;
   m_tLastPoll = 0;
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
   if ((m_owner->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
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
      m_ppValueCache[0] = new ItemValue(&value);
   }

   m_tLastPoll = value.getTimeStamp();
}

/**
 * Check if this DCI has script thresholds
 */
bool DCItem::hasScriptThresholds() const
{
   if (m_thresholds == NULL)
      return false;

   for(int i = 0; i < m_thresholds->size(); i++)
      if (m_thresholds->get(i)->getFunction() == F_SCRIPT)
         return true;

   return false;
}

/**
 * Get threshold object by it's ID
 */
Threshold *DCItem::getThresholdById(UINT32 id) const
{
   if (m_thresholds == NULL)
      return NULL;

   for(int i = 0; i < m_thresholds->size(); i++)
      if (m_thresholds->get(i)->getId() == id)
         return m_thresholds->get(i);

   return NULL;
}
