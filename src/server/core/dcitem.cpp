/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Event parameter names
 */
static const TCHAR *s_paramNamesReach[] = { _T("dciName"), _T("dciDescription"), _T("thresholdValue"), _T("currentValue"), _T("dciId"), _T("instance"), _T("isRepeatedEvent") };
static const TCHAR *s_paramNamesRearm[] = { _T("dciName"), _T("dciDescription"), _T("dciId"), _T("instance"), _T("thresholdValue"), _T("currentValue") };

/**
 * DCI cache loader queue
 */
extern Queue g_dciCacheLoaderQueue;

/**
 * Default constructor for DCItem
 */
DCItem::DCItem() : DCObject()
{
   m_thresholds = NULL;
   m_dataType = DCI_DT_INT;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_instance[0] = 0;
   m_cacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_instanceDiscoveryMethod = IDM_NONE;
	m_instanceDiscoveryData = NULL;
	m_instanceFilterSource = NULL;
	m_instanceFilter = NULL;
}

/**
 * Create DCItem from another DCItem
 */
DCItem::DCItem(const DCItem *pSrc) : DCObject(pSrc)
{
   m_dataType = pSrc->m_dataType;
   m_deltaCalculation = pSrc->m_deltaCalculation;
	m_sampleCount = pSrc->m_sampleCount;
	_tcscpy(m_instance, pSrc->m_instance);
   m_cacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = pSrc->m_nBaseUnits;
	m_nMultiplier = pSrc->m_nMultiplier;
	m_customUnitName = (pSrc->m_customUnitName != NULL) ? _tcsdup(pSrc->m_customUnitName) : NULL;
	m_snmpRawValueType = pSrc->m_snmpRawValueType;
	m_instanceDiscoveryMethod = pSrc->m_instanceDiscoveryMethod;
	m_instanceDiscoveryData = (pSrc->m_instanceDiscoveryData != NULL) ? _tcsdup(pSrc->m_instanceDiscoveryData) : NULL;
	m_instanceFilterSource = NULL;
	m_instanceFilter = NULL;
   setInstanceFilter(pSrc->m_instanceFilterSource);

   // Copy thresholds
	if (pSrc->getThresholdCount() > 0)
	{
		m_thresholds = new ObjectArray<Threshold>(pSrc->m_thresholds->size(), 8, true);
		for(int i = 0; i < pSrc->m_thresholds->size(); i++)
		{
			Threshold *t = new Threshold(pSrc->m_thresholds->get(i));
			t->createId();
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
 *    instd_method,instd_data,instd_filter,samples,comments
 */
DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode) : DCObject()
{
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
   free(pszTmp);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 10, m_szDescription, MAX_DB_STRING);
   DBGetField(hResult, iRow, 11, m_instance, MAX_DB_STRING);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_thresholds = NULL;
   m_pNode = pNode;
   m_cacheSize = 0;
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
   free(pszTmp);
	m_sampleCount = DBGetFieldLong(hResult, iRow, 26);
   m_comments = DBGetField(hResult, iRow, 27, NULL, 0);

   // Load last raw value from database
	TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d"), m_id);
   DB_RESULT hTempResult = DBSelect(g_hCoreDB, szQuery);
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

	loadCustomSchedules();
}

/**
 * Constructor for creating new DCItem from scratch
 */
DCItem::DCItem(UINT32 dwId, const TCHAR *szName, int iSource, int iDataType,
               int iPollingInterval, int iRetentionTime, Template *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
	: DCObject(dwId, szName, iSource, iPollingInterval, iRetentionTime, pNode, pszDescription, systemTag)
{
   m_instance[0] = 0;
   m_dataType = iDataType;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_thresholds = NULL;
   m_cacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
	m_instanceDiscoveryMethod = IDM_NONE;
	m_instanceDiscoveryData = NULL;
	m_instanceFilterSource = NULL;
	m_instanceFilter = NULL;

   updateCacheSize();
}

/**
 * Create DCItem from import file
 */
DCItem::DCItem(ConfigEntry *config, Template *owner) : DCObject(config, owner)
{
   nx_strncpy(m_instance, config->getSubEntryValue(_T("instance"), 0, _T("")), MAX_DB_STRING);
   m_dataType = (BYTE)config->getSubEntryValueAsInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueAsInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueAsInt(_T("samples"));
   m_cacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = (WORD)config->getSubEntryValueAsInt(_T("snmpRawValueType"));
	m_instanceDiscoveryMethod = (WORD)config->getSubEntryValueAsInt(_T("instanceDiscoveryMethod"));
	const TCHAR *value = config->getSubEntryValue(_T("instanceDiscoveryData"));
	m_instanceDiscoveryData = (value != NULL) ? _tcsdup(value) : NULL;
	m_instanceFilterSource = NULL;
	m_instanceFilter = NULL;
	setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));

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

   updateCacheSize();
}

/**
 * Destructor
 */
DCItem::~DCItem()
{
	delete m_thresholds;

	safe_free(m_instanceDiscoveryData);
	safe_free(m_instanceFilterSource);
	delete m_instanceFilter;
	safe_free(m_customUnitName);
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
   UINT32 i;

   for(i = 0; i < m_cacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   m_ppValueCache = NULL;
   m_cacheSize = 0;
}

/**
 * Load data collection items thresholds from database
 */
bool DCItem::loadThresholdsFromDB()
{
   bool result = false;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB,
	           _T("SELECT threshold_id,fire_value,rearm_value,check_function,")
              _T("check_operation,sample_count,script,event_code,current_state,")
              _T("rearm_event_code,repeat_interval,current_severity,")
				  _T("last_event_timestamp,match_count FROM thresholds WHERE item_id=? ")
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
BOOL DCItem::saveToDB(DB_HANDLE hdb)
{
   // Prepare and execute query
	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("items"), _T("item_id"), m_id))
	{
		hStmt = DBPrepare(hdb,
		           _T("UPDATE items SET node_id=?,template_id=?,name=?,source=?,")
                 _T("datatype=?,polling_interval=?,retention_time=?,status=?,")
                 _T("delta_calculation=?,transformation=?,description=?,")
                 _T("instance=?,template_item_id=?,flags=?,")
                 _T("resource_id=?,proxy_node=?,base_units=?,")
		           _T("unit_multiplier=?,custom_units_name=?,perftab_settings=?,")
	              _T("system_tag=?,snmp_port=?,snmp_raw_value_type=?,")
					  _T("instd_method=?,instd_data=?,instd_filter=?,samples=?,comments=? WHERE item_id=?"));
	}
   else
	{
		hStmt = DBPrepare(hdb,
		           _T("INSERT INTO items (node_id,template_id,name,source,")
                 _T("datatype,polling_interval,retention_time,status,delta_calculation,")
                 _T("transformation,description,instance,template_item_id,flags,")
                 _T("resource_id,proxy_node,base_units,unit_multiplier,")
		           _T("custom_units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type,")
					  _T("instd_method,instd_data,instd_filter,samples,comments,item_id) VALUES ")
		           _T("(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_pNode == NULL) ? 0 : m_pNode->getId());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_source);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)m_dataType);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_iPollingInterval);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_iRetentionTime);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (INT32)m_status);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_deltaCalculation);
	DBBind(hStmt, 10, DB_SQLTYPE_TEXT, m_transformationScriptSource, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_szDescription, DB_BIND_STATIC);
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
	DBBind(hStmt, 29, DB_SQLTYPE_INTEGER, m_id);

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
   _sntprintf(query, 256, _T("SELECT item_id FROM raw_dci_values WHERE item_id=%d"), m_id);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (?,?,?)"));
         if (hStmt == NULL)
         {
            DBFreeResult(hResult);
            unlock();
            return FALSE;
         }
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_prevRawValue.getString(), DB_BIND_STATIC, 255);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT64)m_tPrevValueTimeStamp);
         bResult = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      DBFreeResult(hResult);
   }

   unlock();
	return bResult ? DCObject::saveToDB(hdb) : FALSE;
}

/**
 * Check last value for threshold violations
 */
void DCItem::checkThresholds(ItemValue &value)
{
	if (m_thresholds == NULL)
		return;

   for(int i = 0; i < m_thresholds->size(); i++)
   {
		Threshold *t = m_thresholds->get(i);
      ItemValue checkValue;
      ThresholdCheckResult result = t->check(value, m_ppValueCache, checkValue, m_pNode, this);
      switch(result)
      {
         case ACTIVATED:
            {
               PostEventWithNames(t->getEventCode(), m_pNode->getId(), "ssssisd",
					   s_paramNamesReach, m_name, m_szDescription, t->getStringValue(),
                  (const TCHAR *)checkValue, m_id, m_instance, 0);
				   EVENT_TEMPLATE *evt = FindEventTemplateByCode(t->getEventCode());
				   if (evt != NULL)
					   t->markLastEvent((int)evt->dwSeverity);
               if (!(m_flags & DCF_ALL_THRESHOLDS))
                  i = m_thresholds->size();  // Stop processing
            }
            break;
         case DEACTIVATED:
            PostEventWithNames(t->getRearmEventCode(), m_pNode->getId(), "ssisss",
					s_paramNamesRearm, m_name, m_szDescription, m_id, m_instance,
					t->getStringValue(), (const TCHAR *)checkValue);
            break;
         case ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
	            time_t now = time(NULL);
               UINT32 repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : (UINT32)t->getRepeatInterval();
				   if ((repeatInterval != 0) && (t->getLastEventTimestamp() + (time_t)repeatInterval < now))
				   {
					   PostEventWithNames(t->getEventCode(), m_pNode->getId(), "ssssisd",
						   s_paramNamesReach, m_name, m_szDescription, t->getStringValue(),
						   (const TCHAR *)checkValue, m_id, m_instance, 1);
					   EVENT_TEMPLATE *evt = FindEventTemplateByCode(t->getEventCode());
					   if (evt != NULL)
						   t->markLastEvent((int)evt->dwSeverity);
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
}

/**
 * Create NXCP message with item data
 */
void DCItem::createMessage(NXCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
   pMsg->setField(VID_INSTANCE, m_instance);
   pMsg->setField(VID_DCI_DATA_TYPE, (WORD)m_dataType);
   pMsg->setField(VID_DCI_DELTA_CALCULATION, (WORD)m_deltaCalculation);
   pMsg->setField(VID_SAMPLE_COUNT, (WORD)m_sampleCount);
	pMsg->setField(VID_BASE_UNITS, (WORD)m_nBaseUnits);
	pMsg->setField(VID_MULTIPLIER, (UINT32)m_nMultiplier);
	pMsg->setField(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	pMsg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
	if (m_instanceDiscoveryData != NULL)
		pMsg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
	if (m_instanceFilterSource != NULL)
		pMsg->setField(VID_INSTD_FILTER, m_instanceFilterSource);
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
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM idata_%d WHERE item_id=%d"), m_pNode->getId(), m_id);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM thresholds WHERE item_id=%d"), m_id);
   QueueSQLRequest(szQuery);
}

/**
 * Update item from NXCP message
 */
void DCItem::updateFromMessage(NXCPMessage *pMsg, UINT32 *pdwNumMaps, UINT32 **ppdwMapIndex, UINT32 **ppdwMapId)
{
	DCObject::updateFromMessage(pMsg);

   lock();

   pMsg->getFieldAsString(VID_INSTANCE, m_instance, MAX_DB_STRING);
   m_dataType = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_DATA_TYPE);
   m_deltaCalculation = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_DELTA_CALCULATION);
	m_sampleCount = (int)pMsg->getFieldAsUInt16(VID_SAMPLE_COUNT);
	m_nBaseUnits = pMsg->getFieldAsUInt16(VID_BASE_UNITS);
	m_nMultiplier = (int)pMsg->getFieldAsUInt32(VID_MULTIPLIER);
	safe_free(m_customUnitName);
	m_customUnitName = pMsg->getFieldAsString(VID_CUSTOM_UNITS_NAME);
	m_snmpRawValueType = pMsg->getFieldAsUInt16(VID_SNMP_RAW_VALUE_TYPE);
	m_instanceDiscoveryMethod = pMsg->getFieldAsUInt16(VID_INSTD_METHOD);

	safe_free(m_instanceDiscoveryData);
	m_instanceDiscoveryData = pMsg->getFieldAsString(VID_INSTD_DATA);

   TCHAR *pszStr = pMsg->getFieldAsString(VID_INSTD_FILTER);
	setInstanceFilter(pszStr);
   safe_free(pszStr);

   // Update thresholds
   UINT32 dwNum = pMsg->getFieldAsUInt32(VID_NUM_THRESHOLDS);
   UINT32 *newThresholds = (UINT32 *)malloc(sizeof(UINT32) * dwNum);
   *ppdwMapIndex = (UINT32 *)malloc(dwNum * sizeof(UINT32));
   *ppdwMapId = (UINT32 *)malloc(dwNum * sizeof(UINT32));
   *pdwNumMaps = 0;

   // Read all new threshold ids from message
   for(UINT32 i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      newThresholds[i] = pMsg->getFieldAsUInt32(dwId);
   }

   // Check if some thresholds was deleted, and reposition others if needed
   Threshold **ppNewList = (Threshold **)malloc(sizeof(Threshold *) * dwNum);
   memset(ppNewList, 0, sizeof(Threshold *) * dwNum);
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

	safe_free(ppNewList);
   safe_free(newThresholds);
   updateCacheSize();
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
	static int updateRawValueTypes[] = { DB_SQLTYPE_VARCHAR, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER };
	static int updateValueTypes[] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };

   ItemValue rawValue, *pValue;

   lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
   {
      unlock();
      return false;
   }

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue((const TCHAR *)originalValue, (UINT32)tmTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;

   // Cluster can have only aggregated data, and transformation
   // should not be used on aggregation
   if ((m_pNode->getObjectClass() != OBJECT_CLUSTER) || (m_flags & DCF_TRANSFORM_AGGREGATED))
   {
      if (!transform(*pValue, (tmTimeStamp > m_tPrevValueTimeStamp) ? (tmTimeStamp - m_tPrevValueTimeStamp) : 0))
      {
         unlock();
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
	   QueueIDataInsert(tmTimeStamp, m_pNode->getId(), m_id, pValue->getString());
   if (g_flags & AF_PERFDATA_STORAGE_DRIVER_LOADED)
      PerfDataStorageRequest(this, tmTimeStamp, pValue->getString());

   // Check thresholds and add value to cache
   if (m_bCacheLoaded && (tmTimeStamp >= m_tPrevValueTimeStamp))
   {
      checkThresholds(*pValue);
   }

   if ((m_cacheSize > 0) && (tmTimeStamp >= m_tPrevValueTimeStamp))
   {
      delete m_ppValueCache[m_cacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_cacheSize - 1));
      m_ppValueCache[0] = pValue;
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
void DCItem::processNewError()
{
   lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
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
         case ACTIVATED:
            {
               PostEventWithNames(t->getEventCode(), m_pNode->getId(), "ssssisd",
					   s_paramNamesReach, m_name, m_szDescription, _T(""), _T(""),
                  m_id, m_instance, 0);
				   EVENT_TEMPLATE *evt = FindEventTemplateByCode(t->getEventCode());
				   if (evt != NULL)
					   t->markLastEvent((int)evt->dwSeverity);
               if (!(m_flags & DCF_ALL_THRESHOLDS))
               {
                  i = m_thresholds->size();  // Stop processing
               }
            }
            break;
         case DEACTIVATED:
            PostEventWithNames(t->getRearmEventCode(), m_pNode->getId(), "ssisss",
					s_paramNamesRearm, m_name, m_szDescription, m_id, m_instance, _T(""), _T(""));
            break;
         case ALREADY_ACTIVE:
            {
   				// Check if we need to re-sent threshold violation event
            	time_t now = time(NULL);
               UINT32 repeatInterval = (t->getRepeatInterval() == -1) ? g_thresholdRepeatInterval : (UINT32)t->getRepeatInterval();
				   if ((repeatInterval != 0) && (t->getLastEventTimestamp() + (time_t)repeatInterval < now))
				   {
					   PostEventWithNames(t->getEventCode(), m_pNode->getId(), "ssssisd",
						   s_paramNamesReach, m_name, m_szDescription, _T(""), _T(""),
						   m_id, m_instance, 1);
					   EVENT_TEMPLATE *evt = FindEventTemplateByCode(t->getEventCode());
					   if (evt != NULL)
						   t->markLastEvent((int)evt->dwSeverity);
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
               value = (INT32)value - (INT32)m_prevRawValue;
               break;
            case DCI_DT_UINT:
               value = (UINT32)value - (UINT32)m_prevRawValue;
               break;
            case DCI_DT_INT64:
               value = (INT64)value - (INT64)m_prevRawValue;
               break;
            case DCI_DT_UINT64:
               value = (UINT64)value - (UINT64)m_prevRawValue;
               break;
            case DCI_DT_FLOAT:
               value = (double)value - (double)m_prevRawValue;
               break;
            case DCI_DT_STRING:
               value = (INT32)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      case DCM_AVERAGE_PER_MINUTE:
         nElapsedTime /= 60;  // Convert to minutes
      case DCM_AVERAGE_PER_SECOND:
         // Check elapsed time to prevent divide-by-zero exception
         if (nElapsedTime == 0)
            nElapsedTime++;

         switch(m_dataType)
         {
            case DCI_DT_INT:
               value = ((INT32)value - (INT32)m_prevRawValue) / (INT32)nElapsedTime;
               break;
            case DCI_DT_UINT:
               value = ((UINT32)value - (UINT32)m_prevRawValue) / (UINT32)nElapsedTime;
               break;
            case DCI_DT_INT64:
               value = ((INT64)value - (INT64)m_prevRawValue) / (INT64)nElapsedTime;
               break;
            case DCI_DT_UINT64:
               value = ((UINT64)value - (UINT64)m_prevRawValue) / (UINT64)nElapsedTime;
               break;
            case DCI_DT_FLOAT:
               value = ((double)value - (double)m_prevRawValue) / (double)nElapsedTime;
               break;
            case DCI_DT_STRING:
               // I don't see any meaning in _T("average delta per second (minute)") for string
               // values, so result will be 0 if there are no difference between
               // current and previous values, and 1 otherwise
               value = (INT32)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
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
      NXSL_Value *pValue = new NXSL_Value((const TCHAR *)value);
      m_transformationScript->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, m_pNode)));
      if (m_pNode->getObjectClass() == OBJECT_NODE)
      {
         m_transformationScript->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
      }
      m_transformationScript->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
      m_transformationScript->setGlobalVariable(_T("$isCluster"), new NXSL_Value((m_pNode->getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));

      if (m_transformationScript->run(1, &pValue))
      {
         pValue = m_transformationScript->getResult();
         if (pValue != NULL)
         {
            switch(m_dataType)
            {
               case DCI_DT_INT:
                  value = pValue->getValueAsInt32();
                  break;
               case DCI_DT_UINT:
                  value = pValue->getValueAsUInt32();
                  break;
               case DCI_DT_INT64:
                  value = pValue->getValueAsInt64();
                  break;
               case DCI_DT_UINT64:
                  value = pValue->getValueAsUInt64();
                  break;
               case DCI_DT_FLOAT:
                  value = pValue->getValueAsReal();
                  break;
               case DCI_DT_STRING:
                  value = CHECK_NULL_EX(pValue->getValueAsCString());
                  break;
               default:
                  break;
            }
         }
      }
      else if (m_transformationScript->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
      {
         DbgPrintf(6, _T("Transformation script for DCI \"%s\" [%d] on node %s [%d] aborted"),
            m_szDescription, m_id, (m_pNode != NULL) ? m_pNode->getName() : _T("(null)"), (m_pNode != NULL) ? m_pNode->getId() : 0);
         success = false;
      }
      else
      {
         TCHAR szBuffer[1024];

			_sntprintf(szBuffer, 1024, _T("DCI::%s::%d::TransformationScript"),
                    (m_pNode != NULL) ? m_pNode->getName() : _T("(null)"), m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer, m_transformationScript->getErrorText(), m_id);
         success = false;
      }
   }
   return success;
}

/**
 * Set new ID and node/template association
 */
void DCItem::changeBinding(UINT32 dwNewId, Template *pNewNode, BOOL doMacroExpansion)
{
	DCObject::changeBinding(dwNewId, pNewNode, doMacroExpansion);

   lock();
	if (dwNewId != 0)
	{
		for(int i = 0; i < getThresholdCount(); i++)
         m_thresholds->get(i)->bindToItem(m_id, m_pNode->getId());
	}

	if (doMacroExpansion)
		expandMacros(m_instance, m_instance, MAX_DB_STRING);

   clearCache();
   updateCacheSize();
   unlock();
}

/**
 * Update required cache size depending on thresholds
 * dwCondId is an identifier of calling condition object id. If it is not 0,
 * GetCacheSizeForDCI should be called with bNoLock == TRUE for appropriate
 * condition object
 */
void DCItem::updateCacheSize(UINT32 dwCondId)
{
   UINT32 dwSize, dwRequiredSize;

   // Sanity check
   if (m_pNode == NULL)
   {
      DbgPrintf(3, _T("DCItem::updateCacheSize() called for DCI %d when m_pNode == NULL"), m_id);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if (((m_pNode->getObjectClass() == OBJECT_NODE) || (m_pNode->getObjectClass() == OBJECT_MOBILEDEVICE) ||
        ((m_pNode->getObjectClass() == OBJECT_CLUSTER) && isAggregateOnCluster())) &&
       (m_instanceDiscoveryMethod == IDM_NONE))
   {
      dwRequiredSize = 1;

      // Calculate required cache size
      for(int i = 0; i < getThresholdCount(); i++)
         if (dwRequiredSize < m_thresholds->get(i)->getRequiredCacheSize())
            dwRequiredSize = m_thresholds->get(i)->getRequiredCacheSize();

		ObjectArray<NetObj> *conditions = g_idxConditionById.getObjects(true);
		for(int i = 0; i < conditions->size(); i++)
      {
			Condition *c = (Condition *)conditions->get(i);
			dwSize = c->getCacheSizeForDCI(m_id, dwCondId == c->getId());
         if (dwSize > dwRequiredSize)
            dwRequiredSize = dwSize;
         c->decRefCount();
      }
		delete conditions;
   }
   else
   {
      dwRequiredSize = 0;
   }

   // Update cache if needed
   if (dwRequiredSize < m_cacheSize)
   {
      // Destroy unneeded values
      if (m_cacheSize > 0)
		{
         for(UINT32 i = dwRequiredSize; i < m_cacheSize; i++)
            delete m_ppValueCache[i];
		}

      m_cacheSize = dwRequiredSize;
      m_requiredCacheSize = dwRequiredSize;
      if (m_cacheSize > 0)
      {
         m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * m_cacheSize);
      }
      else
      {
         safe_free(m_ppValueCache);
         m_ppValueCache = NULL;
      }
   }
   else if (dwRequiredSize > m_cacheSize)
   {
      // Load missing values from database
      // Skip caching for DCIs where estimated time to fill the cache is less then 5 minutes
      // to reduce load on database at server startup
      if ((m_pNode != NULL) && ((dwRequiredSize - m_cacheSize) * m_iPollingInterval > 300))
      {
         m_pNode->incRefCount();
         m_requiredCacheSize = dwRequiredSize;
         m_bCacheLoaded = false;
         g_dciCacheLoaderQueue.put(this);
      }
      else
      {
         // will not read data from database, fill cache with empty values
         m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * dwRequiredSize);
         for(UINT32 i = m_cacheSize; i < dwRequiredSize; i++)
            m_ppValueCache[i] = new ItemValue(_T(""), 1);
         DbgPrintf(7, _T("Cache load skipped for parameter %s [%d]"), m_name, (int)m_id);
         m_cacheSize = dwRequiredSize;
         m_bCacheLoaded = true;
      }
   }
}

/**
 * Reload cache from database
 */
void DCItem::reloadCache()
{
   TCHAR szBuffer[MAX_DB_STRING];

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT TOP %d idata_value,idata_timestamp FROM idata_%d ")
                           _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                 m_requiredCacheSize, m_pNode->getId(), m_id);
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT * FROM (SELECT idata_value,idata_timestamp FROM idata_%d ")
                           _T("WHERE item_id=%d ORDER BY idata_timestamp DESC) WHERE ROWNUM <= %d"),
                 m_pNode->getId(), m_id, m_requiredCacheSize);
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                           _T("WHERE item_id=%d ORDER BY idata_timestamp DESC LIMIT %d"),
                 m_pNode->getId(), m_id, m_requiredCacheSize);
         break;
      case DB_SYNTAX_DB2:
         _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
            _T("WHERE item_id=%d ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"),
            m_pNode->getId(), m_id, m_requiredCacheSize);
         break;
      default:
         _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                           _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                 m_pNode->getId(), m_id);
         break;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_ASYNC_RESULT hResult = DBAsyncSelect(hdb, szBuffer);

   lock();

	UINT32 i;
   for(i = 0; i < m_cacheSize; i++)
      delete m_ppValueCache[i];

   if (m_cacheSize != m_requiredCacheSize)
   {
      m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * m_requiredCacheSize);
   }

   if (hResult != NULL)
   {
      // Create cache entries
      bool moreData = true;
      for(i = 0; (i < m_requiredCacheSize) && moreData; i++)
      {
         moreData = DBFetch(hResult);
         if (moreData)
         {
            DBGetFieldAsync(hResult, 0, szBuffer, MAX_DB_STRING);
            m_ppValueCache[i] = new ItemValue(szBuffer, DBGetFieldAsyncULong(hResult, 1));
         }
         else
         {
            m_ppValueCache[i] = new ItemValue(_T(""), 1);   // Empty value
         }
      }

      // Fill up cache with empty values if we don't have enough values in database
      for(; i < m_requiredCacheSize; i++)
         m_ppValueCache[i] = new ItemValue(_T(""), 1);

      DBFreeAsyncResult(hResult);
   }
   else
   {
      // Error reading data from database, fill cache with empty values
      for(i = 0; i < m_requiredCacheSize; i++)
         m_ppValueCache[i] = new ItemValue(_T(""), 1);
   }

   m_cacheSize = m_requiredCacheSize;
   m_bCacheLoaded = true;
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
   pMsg->setField(dwId++, m_szDescription);
   pMsg->setField(dwId++, (UINT16)m_source);
   if (m_cacheSize > 0)
   {
      pMsg->setField(dwId++, (UINT16)m_dataType);
      pMsg->setField(dwId++, m_ppValueCache[0]->getString());
      pMsg->setField(dwId++, m_ppValueCache[0]->getTimeStamp());
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
NXSL_Value *DCItem::getRawValueForNXSL()
{
	lock();
   NXSL_Value *value = new NXSL_Value(m_prevRawValue.getString());
   unlock();
   return value;
}

/**
 * Get item's last value for use in NXSL
 */
NXSL_Value *DCItem::getValueForNXSL(int nFunction, int nPolls)
{
   NXSL_Value *pValue;

	lock();
   switch(nFunction)
   {
      case F_LAST:
         pValue = (m_cacheSize > 0) ? new NXSL_Value(m_ppValueCache[0]->getString()) : new NXSL_Value;
         break;
      case F_DIFF:
         if (m_cacheSize >= 2)
         {
            ItemValue result;

            CalculateItemValueDiff(result, m_dataType, *m_ppValueCache[0], *m_ppValueCache[1]);
            pValue = new NXSL_Value(result.getString());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_AVERAGE:
         if (m_cacheSize > 0)
         {
            ItemValue result;

            CalculateItemValueAverage(result, m_dataType,
                                      min(m_cacheSize, (UINT32)nPolls), m_ppValueCache);
            pValue = new NXSL_Value(result.getString());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_DEVIATION:
         if (m_cacheSize > 0)
         {
            ItemValue result;

            CalculateItemValueMD(result, m_dataType,
                                 min(m_cacheSize, (UINT32)nPolls), m_ppValueCache);
            pValue = new NXSL_Value(result.getString());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_ERROR:
         pValue = new NXSL_Value((INT32)((m_dwErrorCount >= (UINT32)nPolls) ? 1 : 0));
         break;
      default:
         pValue = new NXSL_Value;
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

	if (g_dbSyntax == DB_SYNTAX_ORACLE)
	{
		_sntprintf(query, 1024, _T("SELECT %s(coalesce(to_number(idata_value),0)) FROM idata_%u ")
			_T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
			functions[func], m_pNode->getId());
	}
	else if (g_dbSyntax == DB_SYNTAX_MSSQL)
	{
		_sntprintf(query, 1024, _T("SELECT %s(coalesce(cast(idata_value as float),0)) FROM idata_%u ")
			_T("WHERE item_id=? AND (idata_timestamp BETWEEN ? AND ?) AND isnumeric(idata_value)=1"),
			functions[func], m_pNode->getId());
	}
	else if (g_dbSyntax == DB_SYNTAX_PGSQL)
	{
		_sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value::double precision,0)) FROM idata_%u ")
			_T("WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"),
			functions[func], m_pNode->getId());
	}
	else
	{
		_sntprintf(query, 1024, _T("SELECT %s(coalesce(idata_value,0)) FROM idata_%u ")
			_T("WHERE item_id=? and idata_timestamp between ? and ?"),
			functions[func], m_pNode->getId());
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
 * Clean expired data
 */
void DCItem::deleteExpiredData()
{
   TCHAR szQuery[256];
   time_t now;

   now = time(NULL);
   lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%d WHERE (item_id=%d) AND (idata_timestamp<%ld)"),
              (int)m_pNode->getId(), (int)m_id, (long)(now - (time_t)m_iRetentionTime * 86400));
   unlock();
   QueueSQLRequest(szQuery);
}

/**
 * Delete all collected data
 */
bool DCItem::deleteAllData()
{
   TCHAR szQuery[256];
	bool success;

   lock();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%d WHERE item_id=%d"), m_pNode->getId(), m_id);
	success = DBQuery(hdb, szQuery) ? true : false;
   DBConnectionPoolReleaseConnection(hdb);
	clearCache();
	updateCacheSize();
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

	m_nBaseUnits = item->m_nBaseUnits;
	m_nMultiplier = item->m_nMultiplier;
	safe_free(m_customUnitName);
	m_customUnitName = (item->m_customUnitName != NULL) ? _tcsdup(item->m_customUnitName) : NULL;

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
	int count = min(getThresholdCount(), item->getThresholdCount());
	int i;
   for(i = 0; i < count; i++)
      if (!m_thresholds->get(i)->compare(item->m_thresholds->get(i)))
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
      Threshold *t = new Threshold(item->m_thresholds->get(i));
      t->createId();
      t->bindToItem(m_id, m_pNode->getId());
		m_thresholds->add(t);
   }

   if ((item->getInstanceDiscoveryMethod() != IDM_NONE) && (m_instanceDiscoveryMethod == IDM_NONE))
   {
      expandInstance();
   }
   else
   {
      expandMacros(item->m_instance, m_instance, MAX_DB_STRING);
      m_instanceDiscoveryMethod = item->m_instanceDiscoveryMethod;
      safe_free(m_instanceDiscoveryData);
	   m_instanceDiscoveryData = _tcsdup_ex(item->m_instanceDiscoveryData);
      safe_free_and_null(m_instanceFilterSource);
      delete_and_null(m_instanceFilter);
      setInstanceFilter(item->m_instanceFilterSource);
   }

   updateCacheSize();
   unlock();
}

/**
 * Set new instance discovery filter script
 */
void DCItem::setInstanceFilter(const TCHAR *pszScript)
{
	safe_free(m_instanceFilterSource);
	delete m_instanceFilter;
   if (pszScript != NULL)
   {
      m_instanceFilterSource = _tcsdup(pszScript);
      StrStrip(m_instanceFilterSource);
      if (m_instanceFilterSource[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_instanceFilter = NXSLCompileAndCreateVM(m_instanceFilterSource, NULL, 0, new NXSL_ServerEnv);
      }
      else
      {
         m_instanceFilter = NULL;
      }
   }
   else
   {
      m_instanceFilterSource = NULL;
      m_instanceFilter = NULL;
   }
}

/**
 * Get list of used events
 */
void DCItem::getEventList(UINT32 **ppdwList, UINT32 *pdwSize)
{
   lock();

   if (getThresholdCount() > 0)
   {
      *ppdwList = (UINT32 *)realloc(*ppdwList, sizeof(UINT32) * (*pdwSize + m_thresholds->size() * 2));
      UINT32 j = *pdwSize;
      *pdwSize += m_thresholds->size() * 2;
      for(int i = 0; i < m_thresholds->size(); i++)
      {
         (*ppdwList)[j++] = m_thresholds->get(i)->getEventCode();
			(*ppdwList)[j++] = m_thresholds->get(i)->getRearmEventCode();
      }
   }

   unlock();
}

/**
 * Create management pack record
 */
void DCItem::createNXMPRecord(String &str)
{
	UINT32 i;

   lock();

   str.appendFormattedString(_T("\t\t\t\t<dci id=\"%d\">\n")
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
                          _T("\t\t\t\t\t<instanceDiscoveryMethod>%d</instanceDiscoveryMethod>\n"),
								  (int)m_id, (const TCHAR *)EscapeStringForXML2(m_name),
                          (const TCHAR *)EscapeStringForXML2(m_szDescription),
                          m_dataType, m_sampleCount, (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_instance),
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (int)m_deltaCalculation, (int)m_flags,
								  (int)m_snmpRawValueType, (int)m_snmpPort, (int)m_instanceDiscoveryMethod);

	if (m_transformationScriptSource != NULL)
	{
		str += _T("\t\t\t\t\t<transformation>");
		str.appendPreallocated(EscapeStringForXML(m_transformationScriptSource, -1));
		str += _T("</transformation>\n");
	}

	if (m_dwNumSchedules > 0)
   {
      str += _T("\t\t\t\t\t<schedules>\n");
      for(i = 0; i < m_dwNumSchedules; i++)
         str.appendFormattedString(_T("\t\t\t\t\t\t<schedule>%s</schedule>\n"), (const TCHAR *)EscapeStringForXML2(m_ppScheduleList[i]));
      str += _T("\t\t\t\t\t</schedules>\n");
   }

	if (m_thresholds != NULL)
	{
	   str += _T("\t\t\t\t\t<thresholds>\n");
		for(i = 0; i < (UINT32)m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</thresholds>\n");
	}

	if (m_pszPerfTabSettings != NULL)
	{
		str += _T("\t\t\t\t\t<perfTabSettings>");
		str.appendPreallocated(EscapeStringForXML(m_pszPerfTabSettings, -1));
		str += _T("</perfTabSettings>\n");
	}

   if (m_instanceDiscoveryData != NULL)
	{
		str += _T("\t\t\t\t\t<instanceDiscoveryData>");
		str.appendPreallocated(EscapeStringForXML(m_instanceDiscoveryData, -1));
		str += _T("</instanceDiscoveryData>\n");
	}

   if (m_instanceFilterSource != NULL)
	{
		str += _T("\t\t\t\t\t<instanceFilter>");
		str.appendPreallocated(EscapeStringForXML(m_instanceFilterSource, -1));
		str += _T("</instanceFilter>\n");
	}

   unlock();
   str += _T("\t\t\t\t</dci>\n");
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
 */
bool DCItem::testTransformation(DataCollectionTarget *object, const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize)
{
	bool success = false;
	NXSL_VM *vm = NXSLCompileAndCreateVM(script, buffer, (int)bufSize, new NXSL_ServerEnv);
   if (vm != NULL)
   {
      NXSL_Value *pValue = new NXSL_Value(value);
      vm->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, object)));
      if (object->getObjectClass() == OBJECT_NODE)
      {
         vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, object)));
      }
      //vm->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
      vm->setGlobalVariable(_T("$isCluster"), new NXSL_Value((object->getObjectClass() == OBJECT_CLUSTER) ? 1 : 0));

		if (vm->run(1, &pValue))
      {
         pValue = vm->getResult();
         if (pValue != NULL)
         {
				if (pValue->isNull())
				{
					nx_strncpy(buffer, _T("(null)"), bufSize);
				}
				else if (pValue->isObject())
				{
					nx_strncpy(buffer, _T("(object)"), bufSize);
				}
				else if (pValue->isArray())
				{
					nx_strncpy(buffer, _T("(array)"), bufSize);
				}
				else
				{
					const TCHAR *strval;

					strval = pValue->getValueAsCString();
					nx_strncpy(buffer, CHECK_NULL(strval), bufSize);
				}
			}
			else
			{
				nx_strncpy(buffer, _T("(null)"), bufSize);
			}
			success = true;
      }
      else
      {
			nx_strncpy(buffer, vm->getErrorText(), bufSize);
      }
   }
   delete vm;
	return success;
}

/**
 * Fill NXCP message with thresholds
 */
void DCItem::fillMessageWithThresholds(NXCPMessage *msg)
{
	lock();

	msg->setField(VID_NUM_THRESHOLDS, (UINT32)getThresholdCount());
	UINT32 id = VID_DCI_THRESHOLD_BASE;
	for(int i = 0; i < getThresholdCount(); i++, id += 20)
	{
		m_thresholds->get(i)->createMessage(msg, id);
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
 * Should return true if object has (or can have) value
 */
bool DCItem::hasValue()
{
   if (m_pNode->getObjectClass() == OBJECT_CLUSTER)
      return isAggregateOnCluster() && (m_instanceDiscoveryMethod == IDM_NONE);
	return m_instanceDiscoveryMethod == IDM_NONE;
}

/**
 * Expand {instance} macro in name and description
 */
void DCItem::expandInstance()
{
	String temp = m_name;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
	temp.replace(_T("{instance-name}"), m_instance);
	nx_strncpy(m_name, (const TCHAR *)temp, MAX_ITEM_NAME);

	temp = m_szDescription;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
	temp.replace(_T("{instance-name}"), m_instance);
	nx_strncpy(m_szDescription, (const TCHAR *)temp, MAX_DB_STRING);
}

/**
 * Filter callback data
 */
struct FilterCallbackData
{
   StringMap *filteredInstances;
   DCItem *dci;
   NXSL_VM *instanceFilter;
};

/**
 * Callback for filtering instances
 */
static EnumerationCallbackResult FilterCallback(const TCHAR *key, const void *value, void *data)
{
   NXSL_VM *instanceFilter = ((FilterCallbackData *)data)->instanceFilter;
   DCItem *dci = ((FilterCallbackData *)data)->dci;

   instanceFilter->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, dci->getNode())));
   instanceFilter->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, dci)));

   NXSL_Value *argv[2];
   argv[0] = new NXSL_Value(key);
   argv[1] = new NXSL_Value((const TCHAR *)value);

   if (instanceFilter->run(2, argv))
   {
      bool accepted;
      const TCHAR *instance = key;
      const TCHAR *name = (const TCHAR *)value;
      NXSL_Value *result = instanceFilter->getResult();
      if (result != NULL)
      {
         if (result->isArray())
         {
            NXSL_Array *array = result->getValueAsArray();
            if (array->size() > 0)
            {
               accepted = array->get(0)->getValueAsInt32() ? true : false;
               if (accepted && (array->size() > 1))
               {
                  // transformed value
                  const TCHAR *newValue = array->get(1)->getValueAsCString();
                  if ((newValue != NULL) && (*newValue != 0))
                  {
                     DbgPrintf(5, _T("DCItem::filterInstanceList(%s [%d]): instance \"%s\" replaced by \"%s\""),
                               dci->getName(), dci->getId(), instance, newValue);
                     instance = newValue;
                  }

                  if (array->size() > 2)
                  {
                     // instance name
                     const TCHAR *newName = array->get(2)->getValueAsCString();
                     if ((newName != NULL) && (*newName != 0))
                     {
                        DbgPrintf(5, _T("DCItem::filterInstanceList(%s [%d]): instance \"%s\" name set to \"%s\""),
                                  dci->getName(), dci->getId(), instance, newName);
                        name = newName;
                     }
                  }
               }
            }
            else
            {
               accepted = true;
            }
         }
         else
         {
            accepted = result->getValueAsInt32() ? true : false;
         }
      }
      else
      {
         accepted = true;
      }
		if (accepted)
      {
         ((FilterCallbackData *)data)->filteredInstances->set(instance, name);
      }
      else
		{
			DbgPrintf(5, _T("DCItem::filterInstanceList(%s [%d]): instance \"%s\" removed by filtering script"),
                   dci->getName(), dci->getId(), key);
		}
   }
   else
   {
      TCHAR szBuffer[1024];

		_sntprintf(szBuffer, 1024, _T("DCI::%s::%d::InstanceFilter"),
                 (dci->getNode() != NULL) ? dci->getNode()->getName() : _T("(null)"), dci->getId());
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer, instanceFilter->getErrorText(), dci->getId());
      ((FilterCallbackData *)data)->filteredInstances->set(key, (const TCHAR *)value);
   }
   return _CONTINUE;
}

/**
 * Filter instance list
 */
void DCItem::filterInstanceList(StringMap *instances)
{
   if (m_instanceFilter == NULL)
		return;

   StringMap filteredInstances;
   FilterCallbackData data;
   data.filteredInstances = &filteredInstances;
   data.instanceFilter = m_instanceFilter;
   data.dci = this;
   instances->forEach(FilterCallback, &data);
   instances->clear();
   instances->addAll(&filteredInstances);
}
