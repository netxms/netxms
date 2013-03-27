/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 * Default constructor for DCItem
 */
DCItem::DCItem() : DCObject()
{
   m_thresholds = NULL;
   m_dataType = DCI_DT_INT;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_instance[0] = 0;
   m_transformerSource = NULL;
   m_transformer = NULL;
   m_dwCacheSize = 0;
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
   m_transformerSource = NULL;
   m_transformer = NULL;
   setTransformationScript(pSrc->m_transformerSource);
   m_dwCacheSize = 0;
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
 *    instd_method,instd_data,instd_filter,samples
 */
DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode) : DCObject()
{
   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   DBGetField(hResult, iRow, 1, m_szName, MAX_ITEM_NAME);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_dataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_deltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   m_transformerSource = NULL;
   m_transformer = NULL;
   TCHAR *pszTmp = DBGetField(hResult, iRow, 8, NULL, 0);
   setTransformationScript(pszTmp);
   free(pszTmp);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 10, m_szDescription, MAX_DB_STRING);
   DBGetField(hResult, iRow, 11, m_instance, MAX_DB_STRING);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_thresholds = NULL;
   m_pNode = pNode;
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 13);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 14);
	m_dwProxyNode = DBGetFieldULong(hResult, iRow, 15);
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
	m_sampleCount = DBGetFieldLong(hResult, iRow, 25);

   // Load last raw value from database
	TCHAR szQuery[256];
   _sntprintf(szQuery, 256, _T("SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d"), m_dwId);
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
DCItem::DCItem(DWORD dwId, const TCHAR *szName, int iSource, int iDataType, 
               int iPollingInterval, int iRetentionTime, Template *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
	: DCObject(dwId, szName, iSource, iPollingInterval, iRetentionTime, pNode, pszDescription, systemTag)
{
   m_instance[0] = 0;
   m_dataType = iDataType;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
	m_sampleCount = 0;
   m_transformerSource = NULL;
   m_transformer = NULL;
   m_thresholds = NULL;
   m_dwCacheSize = 0;
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
   m_dataType = (BYTE)config->getSubEntryValueInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueInt(_T("delta"));
   m_sampleCount = (BYTE)config->getSubEntryValueInt(_T("samples"));
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_customUnitName = NULL;
	m_snmpRawValueType = (WORD)config->getSubEntryValueInt(_T("snmpRawValueType"));
	m_instanceDiscoveryMethod = (WORD)config->getSubEntryValueInt(_T("instanceDiscoveryMethod"));
	const TCHAR *value = config->getSubEntryValue(_T("instanceDiscoveryData"));
	m_instanceDiscoveryData = (value != NULL) ? _tcsdup(value) : NULL;
	m_instanceFilterSource = NULL;
	m_instanceFilter = NULL;
	setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));

	if (config->getSubEntryValueInt(_T("allThresholds")))
		m_flags |= DCF_ALL_THRESHOLDS;
	if (config->getSubEntryValueInt(_T("rawValueInOctetString")))
		m_flags |= DCF_RAW_VALUE_OCTET_STRING;

	m_transformerSource = NULL;
	m_transformer = NULL;
	setTransformationScript(config->getSubEntryValue(_T("transformation")));
   
	ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
	if (thresholdsRoot != NULL)
	{
		ConfigEntryList *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
		m_thresholds = new ObjectArray<Threshold>(thresholds->getSize(), 8, true);
		for(int i = 0; i < thresholds->getSize(); i++)
		{
			m_thresholds->add(new Threshold(thresholds->getEntry(i), this));
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
   safe_free(m_transformerSource);
   delete m_transformer;
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
   DWORD i;

   for(i = 0; i < m_dwCacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   m_ppValueCache = NULL;
   m_dwCacheSize = 0;
}

/**
 * Load data collection items thresholds from database
 */
bool DCItem::loadThresholdsFromDB()
{
   bool result = false;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB,
	           _T("SELECT threshold_id,fire_value,rearm_value,check_function,")
              _T("check_operation,parameter_1,parameter_2,event_code,current_state,")
              _T("rearm_event_code,repeat_interval,current_severity,")
				  _T("last_event_timestamp FROM thresholds WHERE item_id=? ")
              _T("ORDER BY sequence_number"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
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
	if (IsDatabaseRecordExist(hdb, _T("items"), _T("item_id"), m_dwId))
	{
		hStmt = DBPrepare(hdb, 
		           _T("UPDATE items SET node_id=?,template_id=?,name=?,source=?,")
                 _T("datatype=?,polling_interval=?,retention_time=?,status=?,")
                 _T("delta_calculation=?,transformation=?,description=?,")
                 _T("instance=?,template_item_id=?,flags=?,")
                 _T("resource_id=?,proxy_node=?,base_units=?,")
		           _T("unit_multiplier=?,custom_units_name=?,perftab_settings=?,")
	              _T("system_tag=?,snmp_port=?,snmp_raw_value_type=?,")
					  _T("instd_method=?,instd_data=?,instd_filter=?,samples=? WHERE item_id=?"));
	}
   else
	{
		hStmt = DBPrepare(hdb, 
		           _T("INSERT INTO items (node_id,template_id,name,source,")
                 _T("datatype,polling_interval,retention_time,status,delta_calculation,")
                 _T("transformation,description,instance,template_item_id,flags,")
                 _T("resource_id,proxy_node,base_units,unit_multiplier,")
		           _T("custom_units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type,")
					  _T("instd_method,instd_data,instd_filter,samples,item_id) VALUES ")
		           _T("(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

   lock();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (m_pNode == NULL) ? 0 : m_pNode->Id());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwTemplateId);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_szName, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)m_source);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_dataType);
	DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_iPollingInterval);
	DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_iRetentionTime);
	DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_status);
	DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_deltaCalculation);
	DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_transformerSource, DB_BIND_STATIC);
	DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_szDescription, DB_BIND_STATIC);
	DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
	DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_dwTemplateItemId);
	DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (DWORD)m_flags);
	DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_dwResourceId);
	DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_dwProxyNode);
	DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, (LONG)m_nBaseUnits);
	DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (LONG)m_nMultiplier);
	DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, m_customUnitName, DB_BIND_STATIC);
	DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_pszPerfTabSettings, DB_BIND_STATIC);
	DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_systemTag, DB_BIND_STATIC);
	DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, (LONG)m_snmpPort);
	DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, (LONG)m_snmpRawValueType);
	DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, (LONG)m_instanceDiscoveryMethod);
	DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
	DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_instanceFilterSource, DB_BIND_STATIC);
	DBBind(hStmt, 27, DB_SQLTYPE_INTEGER, (LONG)m_sampleCount);
	DBBind(hStmt, 28, DB_SQLTYPE_INTEGER, m_dwId);

   BOOL bResult = DBExecute(hStmt);
	DBFreeStatement(hStmt);

   // Save thresholds
   if (bResult && (m_thresholds != NULL))
   {
		for(int i = 0; i < m_thresholds->size(); i++)
         m_thresholds->get(i)->saveToDB(hdb, i);
   }

   // Delete non-existing thresholds
	TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT threshold_id FROM thresholds WHERE item_id=%d"), m_dwId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      int iNumRows = DBGetNumRows(hResult);
      for(int i = 0; i < iNumRows; i++)
      {
         DWORD dwId = DBGetFieldULong(hResult, i, 0);
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
   _sntprintf(query, 256, _T("SELECT item_id FROM raw_dci_values WHERE item_id=%d"), m_dwId);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         _sntprintf(query, 256, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (%d,%s,%ld)"),
                 m_dwId, (const TCHAR *)DBPrepareString(hdb, m_prevRawValue.getString()), (long)m_tPrevValueTimeStamp);
         DBQuery(hdb, query);
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
	const TCHAR *paramNamesReach[] = { _T("dciName"), _T("dciDescription"), _T("thresholdValue"), _T("currentValue"), _T("dciId"), _T("instance"), _T("isRepeatedEvent") };
	const TCHAR *paramNamesRearm[] = { _T("dciName"), _T("dciDescription"), _T("dciId"), _T("instance"), _T("thresholdValue"), _T("currentValue") };

	if (m_thresholds == NULL)
		return;

   DWORD dwInterval;
   ItemValue checkValue;
	EVENT_TEMPLATE *evt;
	time_t now = time(NULL);

   for(int i = 0; i < m_thresholds->size(); i++)
   {
		Threshold *thr = m_thresholds->get(i);
      int iResult = thr->check(value, m_ppValueCache, checkValue);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEventWithNames(thr->getEventCode(), m_pNode->Id(), "ssssisd",
					paramNamesReach, m_szName, m_szDescription, thr->getStringValue(), 
               (const TCHAR *)checkValue, m_dwId, m_instance, 0);
				evt = FindEventTemplateByCode(thr->getEventCode());
				if (evt != NULL)
					thr->markLastEvent((int)evt->dwSeverity);
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_thresholds->size();  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEventWithNames(thr->getRearmEventCode(), m_pNode->Id(), "ssisss",
					paramNamesRearm, m_szName, m_szDescription, m_dwId, m_instance, 
					thr->getStringValue(), (const TCHAR *)checkValue);
            break;
         case NO_ACTION:
            if (thr->isReached())
				{
					// Check if we need to re-sent threshold violation event
					if (thr->getRepeatInterval() == -1)
						dwInterval = g_dwThresholdRepeatInterval;
					else
						dwInterval = (DWORD)thr->getRepeatInterval();
					if ((dwInterval != 0) && (thr->getLastEventTimestamp() + (time_t)dwInterval < now))
					{
						PostEventWithNames(thr->getEventCode(), m_pNode->Id(), "ssssisd", 
							paramNamesReach, m_szName, m_szDescription, thr->getStringValue(), 
							(const TCHAR *)checkValue, m_dwId, m_instance, 1);
						evt = FindEventTemplateByCode(thr->getEventCode());
						if (evt != NULL)
							thr->markLastEvent((int)evt->dwSeverity);
					}

					if (!(m_flags & DCF_ALL_THRESHOLDS))
					{
						i = m_thresholds->size();  // Threshold condition still true, stop processing
					}
				}
            break;
      }
   }
}

/**
 * Create NXCP message with item data
 */
void DCItem::createMessage(CSCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
   pMsg->SetVariable(VID_INSTANCE, m_instance);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_dataType);
   pMsg->SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)m_deltaCalculation);
   pMsg->SetVariable(VID_SAMPLE_COUNT, (WORD)m_sampleCount);
   pMsg->SetVariable(VID_TRANSFORMATION_SCRIPT, CHECK_NULL_EX(m_transformerSource));
	pMsg->SetVariable(VID_BASE_UNITS, (WORD)m_nBaseUnits);
	pMsg->SetVariable(VID_MULTIPLIER, (DWORD)m_nMultiplier);
	pMsg->SetVariable(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	pMsg->SetVariable(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
	if (m_instanceDiscoveryData != NULL)
		pMsg->SetVariable(VID_INSTD_DATA, m_instanceDiscoveryData);
	if (m_instanceFilterSource != NULL)
		pMsg->SetVariable(VID_INSTD_FILTER, m_instanceFilterSource);
	if (m_customUnitName != NULL)
		pMsg->SetVariable(VID_CUSTOM_UNITS_NAME, m_customUnitName);
	if (m_thresholds != NULL)
	{
		pMsg->SetVariable(VID_NUM_THRESHOLDS, (DWORD)m_thresholds->size());
		DWORD dwId = VID_DCI_THRESHOLD_BASE;
		for(int i = 0; i < m_thresholds->size(); i++, dwId += 20)
			m_thresholds->get(i)->createMessage(pMsg, dwId);
	}
	else
	{
		pMsg->SetVariable(VID_NUM_THRESHOLDS, (DWORD)0);
	}
   unlock();
}

/**
 * Delete item and collected data from database
 */
void DCItem::deleteFromDB()
{
   TCHAR szQuery[256];

	DCObject::deleteFromDB();

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM items WHERE item_id=%d"), m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM idata_%d WHERE item_id=%d"), m_pNode->Id(), m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM thresholds WHERE item_id=%d"), m_dwId);
   QueueSQLRequest(szQuery);
}

/**
 * Update item from NXCP message
 */
void DCItem::updateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
	DCObject::updateFromMessage(pMsg);

   lock();

   pMsg->GetVariableStr(VID_INSTANCE, m_instance, MAX_DB_STRING);
   m_dataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
   m_deltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
	m_sampleCount = (int)pMsg->GetVariableShort(VID_SAMPLE_COUNT);
   TCHAR *pszStr = pMsg->GetVariableStr(VID_TRANSFORMATION_SCRIPT);
   setTransformationScript(pszStr);
   safe_free(pszStr);
	m_nBaseUnits = pMsg->GetVariableShort(VID_BASE_UNITS);
	m_nMultiplier = (int)pMsg->GetVariableLong(VID_MULTIPLIER);
	safe_free(m_customUnitName);
	m_customUnitName = pMsg->GetVariableStr(VID_CUSTOM_UNITS_NAME);
	m_snmpRawValueType = pMsg->GetVariableShort(VID_SNMP_RAW_VALUE_TYPE);
	m_instanceDiscoveryMethod = pMsg->GetVariableShort(VID_INSTD_METHOD);

	safe_free(m_instanceDiscoveryData);
	m_instanceDiscoveryData = pMsg->GetVariableStr(VID_INSTD_DATA);

   pszStr = pMsg->GetVariableStr(VID_INSTD_FILTER);
	setInstanceFilter(pszStr);
   safe_free(pszStr);

   // Update thresholds
   DWORD dwNum = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
   DWORD *newThresholds = (DWORD *)malloc(sizeof(DWORD) * dwNum);
   *ppdwMapIndex = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *ppdwMapId = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *pdwNumMaps = 0;

   // Read all new threshold ids from message
   for(DWORD i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      newThresholds[i] = pMsg->GetVariableLong(dwId);
   }
   
   // Check if some thresholds was deleted, and reposition others if needed
   Threshold **ppNewList = (Threshold **)malloc(sizeof(Threshold *) * dwNum);
   for(int i = 0; i < getThresholdCount(); i++)
   {
		DWORD j;
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
   for(DWORD i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
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
		for(DWORD i = 0; i < dwNum; i++)
			m_thresholds->add(ppNewList[i]);
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
 * Process new value
 */
void DCItem::processNewValue(time_t tmTimeStamp, void *originalValue)
{
	static int updateRawValueTypes[] = { DB_SQLTYPE_VARCHAR, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER };
	static int updateValueTypes[] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };

   ItemValue rawValue, *pValue;

   lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
   {
      unlock();
      return;
   }

   m_dwErrorCount = 0;

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue((const TCHAR *)originalValue, (DWORD)tmTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;
   transform(*pValue, tmTimeStamp - m_tPrevValueTimeStamp);
   m_prevRawValue = rawValue;
   m_tPrevValueTimeStamp = tmTimeStamp;

	// Prepare SQL statement bindings
	TCHAR dciId[32], pollTime[32];
	_sntprintf(dciId, 32, _T("%d"), (int)m_dwId);
	_sntprintf(pollTime, 32, _T("%ld"), (long)tmTimeStamp);

   // Save raw value into database
	const TCHAR *values[4];
	if (_tcslen((const TCHAR *)originalValue) >= MAX_DB_STRING)
	{
		// need to be truncated
		TCHAR *temp = _tcsdup((const TCHAR *)originalValue);
		temp[MAX_DB_STRING - 1] = 0;
		values[0] = temp;
	}
	else
	{
		values[0] = (const TCHAR *)originalValue;
	}
	values[1] = pValue->getString();
	values[2] = pollTime;
	values[3] = dciId;
	QueueSQLRequest(_T("UPDATE raw_dci_values SET raw_value=?,transformed_value=?,last_poll_time=? WHERE item_id=?"),
	                4, updateRawValueTypes, values);
	if ((void *)values[0] != originalValue)
		free((void *)values[0]);

	// Save transformed value to database
	QueueIDataInsert(tmTimeStamp, m_pNode->Id(), m_dwId, pValue->getString());

   // Check thresholds and add value to cache
   checkThresholds(*pValue);

   if (m_dwCacheSize > 0)
   {
      delete m_ppValueCache[m_dwCacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_dwCacheSize - 1));
      m_ppValueCache[0] = pValue;
   }
   else
   {
      delete pValue;
   }

   unlock();
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
		Threshold *thr = m_thresholds->get(i);
      int iResult = thr->checkError(m_dwErrorCount);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEvent(thr->getEventCode(), m_pNode->Id(), "ssssis", m_szName,
                      m_szDescription, _T(""), _T(""), m_dwId, m_instance);
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_thresholds->size();  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEvent(thr->getRearmEventCode(), m_pNode->Id(), "ssis", m_szName,
                      m_szDescription, m_dwId, m_instance);
            break;
         case NO_ACTION:
            if (thr->isReached() && !(m_flags & DCF_ALL_THRESHOLDS))
               i = m_thresholds->size();  // Threshold condition still true, stop processing
            break;
      }
   }

   unlock();
}

/**
 * Transform received value
 */
void DCItem::transform(ItemValue &value, time_t nElapsedTime)
{
   switch(m_deltaCalculation)
   {
      case DCM_SIMPLE:
         switch(m_dataType)
         {
            case DCI_DT_INT:
               value = (LONG)value - (LONG)m_prevRawValue;
               break;
            case DCI_DT_UINT:
               value = (DWORD)value - (DWORD)m_prevRawValue;
               break;
            case DCI_DT_INT64:
               value = (INT64)value - (INT64)m_prevRawValue;
               break;
            case DCI_DT_UINT64:
               value = (QWORD)value - (QWORD)m_prevRawValue;
               break;
            case DCI_DT_FLOAT:
               value = (double)value - (double)m_prevRawValue;
               break;
            case DCI_DT_STRING:
               value = (LONG)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
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
               value = ((LONG)value - (LONG)m_prevRawValue) / (LONG)nElapsedTime;
               break;
            case DCI_DT_UINT:
               value = ((DWORD)value - (DWORD)m_prevRawValue) / (DWORD)nElapsedTime;
               break;
            case DCI_DT_INT64:
               value = ((INT64)value - (INT64)m_prevRawValue) / (INT64)nElapsedTime;
               break;
            case DCI_DT_UINT64:
               value = ((QWORD)value - (QWORD)m_prevRawValue) / (QWORD)nElapsedTime;
               break;
            case DCI_DT_FLOAT:
               value = ((double)value - (double)m_prevRawValue) / (double)nElapsedTime;
               break;
            case DCI_DT_STRING:
               // I don't see any meaning in _T("average delta per second (minute)") for string
               // values, so result will be 0 if there are no difference between
               // current and previous values, and 1 otherwise
               value = (LONG)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      default:    // Default is no transformation
         break;
   }

   if (m_transformer != NULL)
   {
      NXSL_Value *pValue;
      NXSL_ServerEnv *pEnv;

      pValue = new NXSL_Value((const TCHAR *)value);
      pEnv = new NXSL_ServerEnv;
      m_transformer->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
      m_transformer->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
	
      if (m_transformer->run(pEnv, 1, &pValue) == 0)
      {
         pValue = m_transformer->getResult();
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
      else
      {
         TCHAR szBuffer[1024];

			_sntprintf(szBuffer, 1024, _T("DCI::%s::%d::Transformer"),
                    (m_pNode != NULL) ? m_pNode->Name() : _T("(null)"), m_dwId);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer,
                   m_transformer->getErrorText(), m_dwId);
      }
   }
}

/**
 * Set new ID and node/template association
 */
void DCItem::changeBinding(DWORD dwNewId, Template *pNewNode, BOOL doMacroExpansion)
{
	DCObject::changeBinding(dwNewId, pNewNode, doMacroExpansion);

   lock();
	if (dwNewId != 0)
	{
		for(int i = 0; i < getThresholdCount(); i++)
			m_thresholds->get(i)->bindToItem(m_dwId);
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
void DCItem::updateCacheSize(DWORD dwCondId)
{
   DWORD dwSize, dwRequiredSize;

   // Sanity check
   if (m_pNode == NULL)
   {
      DbgPrintf(3, _T("DCItem::updateCacheSize() called for DCI %d when m_pNode == NULL"), m_dwId);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if ((m_pNode->Type() == OBJECT_NODE) || (m_pNode->Type() == OBJECT_MOBILEDEVICE))
   {
      dwRequiredSize = 1;

      // Calculate required cache size
      for(int i = 0; i < getThresholdCount(); i++)
         if (dwRequiredSize < m_thresholds->get(i)->getRequiredCacheSize())
            dwRequiredSize = m_thresholds->get(i)->getRequiredCacheSize();

		ObjectArray<NetObj> *conditions = g_idxConditionById.getObjects();
		for(int i = 0; i < conditions->size(); i++)
      {
			Condition *c = (Condition *)conditions->get(i);
			dwSize = c->getCacheSizeForDCI(m_dwId, dwCondId == c->Id());
         if (dwSize > dwRequiredSize)
            dwRequiredSize = dwSize;
      }
		delete conditions;
   }
   else
   {
      dwRequiredSize = 0;
   }

   // Update cache if needed
   if (dwRequiredSize < m_dwCacheSize)
   {
      // Destroy unneeded values
      if (m_dwCacheSize > 0)
		{
         for(DWORD i = dwRequiredSize; i < m_dwCacheSize; i++)
            delete m_ppValueCache[i];
		}

      m_dwCacheSize = dwRequiredSize;
      if (m_dwCacheSize > 0)
      {
         m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * m_dwCacheSize);
      }
      else
      {
         safe_free(m_ppValueCache);
         m_ppValueCache = NULL;
      }
   }
   else if (dwRequiredSize > m_dwCacheSize)
   {
      // Expand cache
      m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * dwRequiredSize);
      for(DWORD i = m_dwCacheSize; i < dwRequiredSize; i++)
         m_ppValueCache[i] = NULL;

      // Load missing values from database
      if (m_pNode != NULL)
      {
         TCHAR szBuffer[MAX_DB_STRING];
         BOOL bHasData;

         switch(g_nDBSyntax)
         {
            case DB_SYNTAX_MSSQL:
               _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT TOP %d idata_value,idata_timestamp FROM idata_%d ")
                                 _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                       dwRequiredSize, m_pNode->Id(), m_dwId);
               break;
            case DB_SYNTAX_ORACLE:
               _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                                 _T("WHERE item_id=%d AND ROWNUM <= %d ORDER BY idata_timestamp DESC"),
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            case DB_SYNTAX_MYSQL:
            case DB_SYNTAX_PGSQL:
            case DB_SYNTAX_SQLITE:
               _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                                 _T("WHERE item_id=%d ORDER BY idata_timestamp DESC LIMIT %d"),
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            default:
               _sntprintf(szBuffer, MAX_DB_STRING, _T("SELECT idata_value,idata_timestamp FROM idata_%d ")
                                 _T("WHERE item_id=%d ORDER BY idata_timestamp DESC"),
                       m_pNode->Id(), m_dwId);
               break;
         }
			DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         DB_ASYNC_RESULT hResult = DBAsyncSelect(hdb, szBuffer);
         if (hResult != NULL)
         {
            // Skip already cached values
				DWORD i;
            for(i = 0, bHasData = TRUE; i < m_dwCacheSize; i++)
               bHasData = DBFetch(hResult);

            // Create new cache entries
            for(; (i < dwRequiredSize) && bHasData; i++)
            {
               bHasData = DBFetch(hResult);
               if (bHasData)
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
            for(; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);

            DBFreeAsyncResult(hResult);
         }
         else
         {
            // Error reading data from database, fill cache with empty values
            for(DWORD i = m_dwCacheSize; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
			DBConnectionPoolReleaseConnection(hdb);
      }
      m_dwCacheSize = dwRequiredSize;
   }
   m_bCacheLoaded = true;
}

/**
 * Put last value into CSCP message
 */
void DCItem::getLastValue(CSCPMessage *pMsg, DWORD dwId)
{
	lock();
   pMsg->SetVariable(dwId++, m_dwId);
   pMsg->SetVariable(dwId++, m_szName);
   pMsg->SetVariable(dwId++, m_szDescription);
   pMsg->SetVariable(dwId++, (WORD)m_source);
   if (m_dwCacheSize > 0)
   {
      pMsg->SetVariable(dwId++, (WORD)m_dataType);
      pMsg->SetVariable(dwId++, (TCHAR *)m_ppValueCache[0]->getString());
      pMsg->SetVariable(dwId++, m_ppValueCache[0]->getTimeStamp());
   }
   else
   {
      pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);
      pMsg->SetVariable(dwId++, _T(""));
      pMsg->SetVariable(dwId++, (DWORD)0);
   }
   pMsg->SetVariable(dwId++, (WORD)m_status);
	pMsg->SetVariable(dwId++, (WORD)getType());
	pMsg->SetVariable(dwId++, m_dwErrorCount);
	pMsg->SetVariable(dwId++, m_dwTemplateItemId);

	int i;
   for(i = 0; i < getThresholdCount(); i++)
   {
		if (m_thresholds->get(i)->isReached())
			break;
   }
	if (i < getThresholdCount())
	{
      pMsg->SetVariable(dwId++, (WORD)1);
		m_thresholds->get(i)->createMessage(pMsg, dwId);
	}
	else
	{
      pMsg->SetVariable(dwId++, (WORD)0);
	}

	unlock();
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
         pValue = (m_dwCacheSize > 0) ? new NXSL_Value((TCHAR *)m_ppValueCache[0]->getString()) : new NXSL_Value;
         break;
      case F_DIFF:
         if (m_dwCacheSize >= 2)
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
         if (m_dwCacheSize > 0)
         {
            ItemValue result;

            CalculateItemValueAverage(result, m_dataType,
                                      min(m_dwCacheSize, (DWORD)nPolls), m_ppValueCache);
            pValue = new NXSL_Value(result.getString());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_DEVIATION:
         if (m_dwCacheSize > 0)
         {
            ItemValue result;

            CalculateItemValueMD(result, m_dataType,
                                 min(m_dwCacheSize, (DWORD)nPolls), m_ppValueCache);
            pValue = new NXSL_Value(result.getString());
         }
         else
         {
            pValue = new NXSL_Value;
         }
         break;
      case F_ERROR:
         pValue = new NXSL_Value((LONG)((m_dwErrorCount >= (DWORD)nPolls) ? 1 : 0));
         break;
      default:
         pValue = new NXSL_Value;
         break;
   }
	unlock();
   return pValue;
}


//
// Clean expired data
//

void DCItem::deleteExpiredData()
{
   TCHAR szQuery[256];
   time_t now;

   now = time(NULL);
   lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%d WHERE (item_id=%d) AND (idata_timestamp<%ld)"),
              (int)m_pNode->Id(), (int)m_dwId, (long)(now - (time_t)m_iRetentionTime * 86400));
   unlock();
   QueueSQLRequest(szQuery);
}


//
// Delete all collected data
//

bool DCItem::deleteAllData()
{
   TCHAR szQuery[256];
	bool success;

   lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%d WHERE item_id=%d"), m_pNode->Id(), m_dwId);
	success = DBQuery(g_hCoreDB, szQuery) ? true : false;
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
		DbgPrintf(2, _T("INTERNAL ERROR: DCItem::updateFromTemplate(%d, %d): source type is %d"), (int)m_dwId, (int)src->getId(), src->getType());
		return;
	}

   lock();
	DCItem *item = (DCItem *)src;

   m_dataType = item->m_dataType;
   m_deltaCalculation = item->m_deltaCalculation;
   setTransformationScript(item->m_transformerSource);

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
      t->bindToItem(m_dwId);
		m_thresholds->add(t);
   }

   expandMacros(item->m_instance, m_instance, MAX_DB_STRING);

   updateCacheSize();
   
   unlock();
}

/**
 * Set new transformation script
 */
void DCItem::setTransformationScript(const TCHAR *pszScript)
{
   safe_free(m_transformerSource);
   delete m_transformer;
   if (pszScript != NULL)
   {
      m_transformerSource = _tcsdup(pszScript);
      StrStrip(m_transformerSource);
      if (m_transformerSource[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_transformer = (NXSL_Program *)NXSLCompile(m_transformerSource, NULL, 0);
      }
      else
      {
         m_transformer = NULL;
      }
   }
   else
   {
      m_transformerSource = NULL;
      m_transformer = NULL;
   }
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
         m_instanceFilter = (NXSL_Program *)NXSLCompile(m_instanceFilterSource, NULL, 0);
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
void DCItem::getEventList(DWORD **ppdwList, DWORD *pdwSize)
{
   lock();

   if (getThresholdCount() > 0)
   {
      *ppdwList = (DWORD *)realloc(*ppdwList, sizeof(DWORD) * (*pdwSize + m_thresholds->size() * 2));
      DWORD j = *pdwSize;
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
	DWORD i;

   lock();
   
   str.addFormattedString(_T("\t\t\t\t<dci id=\"%d\">\n")
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
                          _T("\t\t\t\t\t<advancedSchedule>%d</advancedSchedule>\n")
                          _T("\t\t\t\t\t<allThresholds>%d</allThresholds>\n")
                          _T("\t\t\t\t\t<rawValueInOctetString>%d</rawValueInOctetString>\n")
                          _T("\t\t\t\t\t<snmpRawValueType>%d</snmpRawValueType>\n")
                          _T("\t\t\t\t\t<snmpPort>%d</snmpPort>\n"),
								  (int)m_dwId, (const TCHAR *)EscapeStringForXML2(m_szName),
                          (const TCHAR *)EscapeStringForXML2(m_szDescription),
                          m_dataType, m_sampleCount, (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_instance),
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (int)m_deltaCalculation, (m_flags & DCF_ADVANCED_SCHEDULE) ? 1 : 0,
                          (m_flags & DCF_ALL_THRESHOLDS) ? 1 : 0, 
								  (m_flags & DCF_RAW_VALUE_OCTET_STRING) ? 1 : 0, 
								  (int)m_snmpRawValueType, (int)m_snmpPort);

	if (m_transformerSource != NULL)
	{
		str += _T("\t\t\t\t\t<transformation>");
		str.addDynamicString(EscapeStringForXML(m_transformerSource, -1));
		str += _T("</transformation>\n");
	}

	if (m_dwNumSchedules > 0)
   {
      str += _T("\t\t\t\t\t<schedules>\n");
      for(i = 0; i < m_dwNumSchedules; i++)
         str.addFormattedString(_T("\t\t\t\t\t\t<schedule>%s</schedule>\n"), (const TCHAR *)EscapeStringForXML2(m_ppScheduleList[i]));
      str += _T("\t\t\t\t\t</schedules>\n");
   }

	if (m_thresholds != NULL)
	{
	   str += _T("\t\t\t\t\t<thresholds>\n");
		for(i = 0; i < (DWORD)m_thresholds->size(); i++)
		{
			m_thresholds->get(i)->createNXMPRecord(str, i + 1);
		}
	   str += _T("\t\t\t\t\t</thresholds>\n");
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
BOOL DCItem::enumThresholds(BOOL (* pfCallback)(Threshold *, DWORD, void *), void *pArg)
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
BOOL DCItem::testTransformation(const TCHAR *script, const TCHAR *value, TCHAR *buffer, size_t bufSize)
{
	BOOL success = FALSE;
	NXSL_Program *pScript;

	pScript = NXSLCompile(script, buffer, (int)bufSize);
   if (pScript != NULL)
   {
      NXSL_Value *pValue;
      NXSL_ServerEnv *pEnv;

      pValue = new NXSL_Value(value);
      pEnv = new NXSL_ServerEnv;
      pScript->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
	
	 	lock();
		if (pScript->run(pEnv, 1, &pValue) == 0)
      {
         pValue = pScript->getResult();
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
			success = TRUE;
      }
      else
      {
			nx_strncpy(buffer, pScript->getErrorText(), bufSize);
      }
		unlock();
   }
	return success;
}

/**
 * Fill NXCP message with thresholds
 */
void DCItem::fillMessageWithThresholds(CSCPMessage *msg)
{
	lock();

	msg->SetVariable(VID_NUM_THRESHOLDS, (DWORD)getThresholdCount());
	DWORD id = VID_DCI_THRESHOLD_BASE;
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
	return m_instanceDiscoveryMethod == IDM_NONE;
}

/**
 * Expand {instance} macro in name and description
 */
void DCItem::expandInstance()
{
	String temp = m_szName;
	temp.replace(_T("{instance}"), m_instance);
	nx_strncpy(m_szName, (const TCHAR *)temp, MAX_ITEM_NAME);

	temp = m_szDescription;
	temp.replace(_T("{instance}"), m_instance);
	nx_strncpy(m_szDescription, (const TCHAR *)temp, MAX_DB_STRING);
}

/**
 * Filter instance list
 */
void DCItem::filterInstanceList(StringList *instances)
{
   if (m_instanceFilter == NULL)
		return;

	for(int i = 0; i < instances->getSize(); i++)
	{
      NXSL_Value *pValue;
      NXSL_ServerEnv *pEnv;

		pValue = new NXSL_Value(instances->getValue(i));
      pEnv = new NXSL_ServerEnv;
      m_instanceFilter->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
      m_instanceFilter->setGlobalVariable(_T("$dci"), new NXSL_Value(new NXSL_Object(&g_nxslDciClass, this)));
	
      if (m_instanceFilter->run(pEnv, 1, &pValue) == 0)
      {
         pValue = m_instanceFilter->getResult();
         if (pValue != NULL)
         {
            bool accepted;
            if (pValue->isArray())
            {
               NXSL_Array *array = pValue->getValueAsArray();
               if (array->size() > 0)
               {
                  accepted = array->get(0)->getValueAsInt32() ? true : false;
                  if (accepted && (array->size() > 1))
                  {
                     // transformed value
                     const TCHAR *newValue = array->get(1)->getValueAsCString();
                     if (newValue != NULL)
                     {
                        DbgPrintf(5, _T("DCItem::filterInstanceList(%s [%d]): instance %d \"%s\" replaced by \"%s\""),
                                  m_szName, m_dwId, i, instances->getValue(i), newValue);
                        instances->replace(i, newValue);
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
               accepted = pValue->getValueAsInt32() ? true : false;
            }
				if (!accepted)
				{
					DbgPrintf(5, _T("DCItem::filterInstanceList(%s [%d]): instance \"%s\" removed by filtering script"),
					          m_szName, m_dwId, instances->getValue(i));
					instances->remove(i);
					i--;
				}
         }
      }
      else
      {
         TCHAR szBuffer[1024];

			_sntprintf(szBuffer, 1024, _T("DCI::%s::%d::InstanceFilter"),
                    (m_pNode != NULL) ? m_pNode->Name() : _T("(null)"), m_dwId);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer,
                   m_transformer->getErrorText(), m_dwId);
      }
   }
}
