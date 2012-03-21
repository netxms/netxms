/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Get DCI object
// First argument is a node object (usually passed to script via $node variable),
// and second is DCI ID
//

static int F_GetDCIObject(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		*ppResult = new NXSL_Value(new NXSL_Object(&g_nxslDciClass, dci));
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
	}

	return 0;
}


//
// Get DCI value from within transformation script
// First argument is a node object (passed to script via $node variable),
// and second is DCI ID
//

static int F_GetDCIValue(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isInteger())
		return NXSL_ERR_NOT_INTEGER;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectById(argv[1]->getValueAsUInt32());
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		*ppResult = ((DCItem *)dci)->getValueForNXSL(F_LAST, 1);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
	}

	return 0;
}


//
// Internal implementation of GetDCIValueByName and GetDCIValueByDescription
//

static int GetDciValueExImpl(bool byName, int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = byName ? node->getDCObjectByName(argv[1]->getValueAsCString()) : node->getDCObjectByDescription(argv[1]->getValueAsCString());
	if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
	{
		*ppResult = ((DCItem *)dci)->getValueForNXSL(F_LAST, 1);
	}
	else
	{
		*ppResult = new NXSL_Value;	// Return NULL if DCI not found
	}

	return 0;
}


//
// Get DCI value from within transformation script
// First argument is a node object (passed to script via $node variable),
// and second is DCI name
//

static int F_GetDCIValueByName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	return GetDciValueExImpl(true, argc, argv, ppResult);
}


//
// Get DCI value from within transformation script
// First argument is a node object (passed to script via $node variable),
// and second is DCI description
//

static int F_GetDCIValueByDescription(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	return GetDciValueExImpl(false, argc, argv, ppResult);
}


//
// Find DCI by name
//

static int F_FindDCIByName(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectByName(argv[1]->getValueAsCString());
	*ppResult = ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM)) ? new NXSL_Value(dci->getId()) : new NXSL_Value((DWORD)0);
	return 0;
}


//
// Find DCI by description
//

static int F_FindDCIByDescription(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program)
{
	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	NXSL_Object *object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), g_nxslNodeClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	Node *node = (Node *)object->getData();
	DCObject *dci = node->getDCObjectByDescription(argv[1]->getValueAsCString());
	*ppResult = ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM)) ? new NXSL_Value(dci->getId()) : new NXSL_Value((DWORD)0);
	return 0;
}


//
// Additional functions or use within transformation scripts
//

static NXSL_ExtFunction m_nxslDCIFunctions[] =
{
   { _T("FindDCIByName"), F_FindDCIByName, 2 },
   { _T("FindDCIByDescription"), F_FindDCIByDescription, 2 },
   { _T("GetDCIObject"), F_GetDCIObject, 2 },
   { _T("GetDCIValue"), F_GetDCIValue, 2 },
   { _T("GetDCIValueByName"), F_GetDCIValueByName, 2 },
   { _T("GetDCIValueByDescription"), F_GetDCIValueByDescription, 2 }
};

void RegisterDCIFunctions(NXSL_Environment *pEnv)
{
	pEnv->registerFunctionSet(sizeof(m_nxslDCIFunctions) / sizeof(NXSL_ExtFunction), m_nxslDCIFunctions);
}


//
// Default constructor for DCItem
//

DCItem::DCItem() : DCObject()
{
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_dataType = DCI_DT_INT;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
   m_szInstance[0] = 0;
   m_pszScript = NULL;
   m_pScript = NULL;
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_pszCustomUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;
}


//
// Create DCItem from another DCItem
//

DCItem::DCItem(const DCItem *pSrc) : DCObject(pSrc)
{
   DWORD i;

   m_dataType = pSrc->m_dataType;
   m_deltaCalculation = pSrc->m_deltaCalculation;
	_tcscpy(m_szInstance, pSrc->m_szInstance);
   m_pszScript = NULL;
   m_pScript = NULL;
   setTransformationScript(pSrc->m_pszScript);
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = pSrc->m_nBaseUnits;
	m_nMultiplier = pSrc->m_nMultiplier;
	m_pszCustomUnitName = (pSrc->m_pszCustomUnitName != NULL) ? _tcsdup(pSrc->m_pszCustomUnitName) : NULL;
	m_snmpRawValueType = pSrc->m_snmpRawValueType;

   // Copy thresholds
   m_dwNumThresholds = pSrc->m_dwNumThresholds;
   m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i] = new Threshold(pSrc->m_ppThresholdList[i]);
      m_ppThresholdList[i]->createId();
   }
}


//
// Constructor for creating DCItem from database
// Assumes that fields in SELECT query are in following order:
// item_id,name,source,datatype,polling_interval,retention_time,status,
// delta_calculation,transformation,template_id,description,instance,
// template_item_id,flags,resource_id,proxy_node,base_units,unit_multiplier,
// custom_units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type
//

DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode) : DCObject()
{
   TCHAR *pszTmp, szQuery[256], szBuffer[MAX_DB_STRING];
   DB_RESULT hTempResult;
   DWORD i;

   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   DBGetField(hResult, iRow, 1, m_szName, MAX_ITEM_NAME);
   m_source = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_dataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_status = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_deltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   m_pszScript = NULL;
   m_pScript = NULL;
   pszTmp = DBGetField(hResult, iRow, 8, NULL, 0);
   setTransformationScript(pszTmp);
   free(pszTmp);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   DBGetField(hResult, iRow, 10, m_szDescription, MAX_DB_STRING);
   DBGetField(hResult, iRow, 11, m_szInstance, MAX_DB_STRING);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreateRecursive();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
   m_flags = (WORD)DBGetFieldLong(hResult, iRow, 13);
	m_dwResourceId = DBGetFieldULong(hResult, iRow, 14);
	m_dwProxyNode = DBGetFieldULong(hResult, iRow, 15);
	m_nBaseUnits = DBGetFieldLong(hResult, iRow, 16);
	m_nMultiplier = DBGetFieldLong(hResult, iRow, 17);
	m_pszCustomUnitName = DBGetField(hResult, iRow, 18, NULL, 0);
	m_pszPerfTabSettings = DBGetField(hResult, iRow, 19, NULL, 0);
	m_systemTag[0] = 0;
	DBGetField(hResult, iRow, 20, m_systemTag, MAX_DB_STRING);
	m_snmpPort = (WORD)DBGetFieldLong(hResult, iRow, 21);
	m_snmpRawValueType = (WORD)DBGetFieldLong(hResult, iRow, 22);

   if (m_flags & DCF_ADVANCED_SCHEDULE)
   {
      _sntprintf(szQuery, 256, _T("SELECT schedule FROM dci_schedules WHERE item_id=%d"), m_dwId);
      hTempResult = DBSelect(g_hCoreDB, szQuery);
      if (hTempResult != NULL)
      {
         m_dwNumSchedules = DBGetNumRows(hTempResult);
         m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
         for(i = 0; i < m_dwNumSchedules; i++)
         {
            m_ppScheduleList[i] = DBGetField(hTempResult, i, 0, NULL, 0);
            DecodeSQLString(m_ppScheduleList[i]);
         }
         DBFreeResult(hTempResult);
      }
      else
      {
         m_dwNumSchedules = 0;
         m_ppScheduleList = NULL;
      }
   }
   else
   {
      m_dwNumSchedules = 0;
      m_ppScheduleList = NULL;
   }

   // Load last raw value from database
   _sntprintf(szQuery, 256, _T("SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%d"), m_dwId);
   hTempResult = DBSelect(g_hCoreDB, szQuery);
   if (hTempResult != NULL)
   {
      if (DBGetNumRows(hTempResult) > 0)
      {
         m_prevRawValue = DBGetField(hTempResult, 0, 0, szBuffer, MAX_DB_STRING);
         m_tPrevValueTimeStamp = DBGetFieldULong(hTempResult, 0, 1);
         m_tLastPoll = m_tPrevValueTimeStamp;
      }
      DBFreeResult(hTempResult);
   }
}


//
// Constructor for creating new DCItem from scratch
//

DCItem::DCItem(DWORD dwId, const TCHAR *szName, int iSource, int iDataType, 
               int iPollingInterval, int iRetentionTime, Template *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
	: DCObject(dwId, szName, iSource, iPollingInterval, iRetentionTime, pNode, pszDescription, systemTag)
{
   m_szInstance[0] = 0;
   m_dataType = iDataType;
   m_deltaCalculation = DCM_ORIGINAL_VALUE;
   m_pszScript = NULL;
   m_pScript = NULL;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_pszCustomUnitName = NULL;
	m_snmpRawValueType = SNMP_RAWTYPE_NONE;

   updateCacheSize();
}


//
// Create DCItem from import file
//

DCItem::DCItem(ConfigEntry *config, Template *owner) : DCObject(config, owner)
{
   nx_strncpy(m_szInstance, config->getSubEntryValue(_T("instance"), 0, _T("")), MAX_DB_STRING);
   m_dataType = (BYTE)config->getSubEntryValueInt(_T("dataType"));
   m_deltaCalculation = (BYTE)config->getSubEntryValueInt(_T("delta"));
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = false;
	m_nBaseUnits = DCI_BASEUNITS_OTHER;
	m_nMultiplier = 1;
	m_pszCustomUnitName = NULL;
	m_snmpRawValueType = (WORD)config->getSubEntryValueInt(_T("snmpRawValueType"));

	if (config->getSubEntryValueInt(_T("allThresholds")))
		m_flags |= DCF_ALL_THRESHOLDS;
	if (config->getSubEntryValueInt(_T("rawValueInOctetString")))
		m_flags |= DCF_RAW_VALUE_OCTET_STRING;

	m_pszScript = NULL;
	m_pScript = NULL;
	setTransformationScript(config->getSubEntryValue(_T("transformation")));
   
	ConfigEntry *thresholdsRoot = config->findEntry(_T("thresholds"));
	if (thresholdsRoot != NULL)
	{
		ConfigEntryList *thresholds = thresholdsRoot->getSubEntries(_T("threshold#*"));
		m_dwNumThresholds = (DWORD)thresholds->getSize();
		m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
		for(int i = 0; i < thresholds->getSize(); i++)
		{
			m_ppThresholdList[i] = new Threshold(thresholds->getEntry(i), this);
		}
		delete thresholds;
	}
	else
	{
		m_dwNumThresholds = 0;
		m_ppThresholdList = NULL;
	}

   updateCacheSize();
}


//
// Destructor for DCItem
//

DCItem::~DCItem()
{
   for(DWORD i = 0; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];
   safe_free(m_ppThresholdList);
   safe_free(m_pszScript);
   delete m_pScript;
	safe_free(m_pszCustomUnitName);
   clearCache();
}


//
// Delete all thresholds
//

void DCItem::deleteAllThresholds()
{
	lock();
   for(DWORD i = 0; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];
   safe_free_and_null(m_ppThresholdList);
	m_dwNumThresholds = 0;
	unlock();
}


//
// Clear data cache
//

void DCItem::clearCache()
{
   DWORD i;

   for(i = 0; i < m_dwCacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   m_ppValueCache = NULL;
   m_dwCacheSize = 0;
}


//
// Load data collection items thresholds from database
//

bool DCItem::loadThresholdsFromDB()
{
   DWORD i;
   TCHAR szQuery[256];
   DB_RESULT hResult;
   bool result = false;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR),
	           _T("SELECT threshold_id,fire_value,rearm_value,check_function,")
              _T("check_operation,parameter_1,parameter_2,event_code,current_state,")
              _T("rearm_event_code,repeat_interval FROM thresholds WHERE item_id=%d ")
              _T("ORDER BY sequence_number"), m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumThresholds = DBGetNumRows(hResult);
      if (m_dwNumThresholds > 0)
      {
         m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
         for(i = 0; i < m_dwNumThresholds; i++)
            m_ppThresholdList[i] = new Threshold(hResult, i, this);
      }
      DBFreeResult(hResult);
      result = true;
   }

   //updateCacheSize();

   return result;
}


//
// Save to database
//

BOOL DCItem::saveToDB(DB_HANDLE hdb)
{
	TCHAR *pszQuery;
   DB_RESULT hResult;
   BOOL bNewObject = TRUE, bResult;

   lock();

   String escName = DBPrepareString(g_hCoreDB, m_szName);
   String escScript = DBPrepareString(g_hCoreDB, m_pszScript);
   String escDescr = DBPrepareString(g_hCoreDB, m_szDescription);
   String escInstance = DBPrepareString(g_hCoreDB, m_szInstance);
	String escCustomUnitName = DBPrepareString(g_hCoreDB, m_pszCustomUnitName);
	String escPerfTabSettings = DBPrepareString(g_hCoreDB, m_pszPerfTabSettings);
	int qlen = escScript.getSize() + escPerfTabSettings.getSize() + 2048;
	pszQuery = (TCHAR *)malloc(sizeof(TCHAR) * qlen);

   // Check for object's existence in database
   _sntprintf(pszQuery, qlen, _T("SELECT item_id FROM items WHERE item_id=%d"), m_dwId);
   hResult = DBSelect(hdb, pszQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
	if (bNewObject)
	{
      _sntprintf(pszQuery, qlen, 
		           _T("INSERT INTO items (item_id,node_id,template_id,name,description,source,")
                 _T("datatype,polling_interval,retention_time,status,delta_calculation,")
                 _T("transformation,instance,template_item_id,flags,")
                 _T("resource_id,proxy_node,base_units,unit_multiplier,")
		           _T("custom_units_name,perftab_settings,system_tag,snmp_port,snmp_raw_value_type) VALUES ")
		           _T("(%d,%d,%d,%s,%s,%d,%d,%d,%d,%d,%d,%s,%s,%d,%d,%d,%d,%d,%d,%s,%s,%s,%d,%d)"),
                 m_dwId, (m_pNode == NULL) ? (DWORD)0 : m_pNode->Id(), m_dwTemplateId,
                 (const TCHAR *)escName, (const TCHAR *)escDescr, m_source, m_dataType, m_iPollingInterval,
                 m_iRetentionTime, m_status, m_deltaCalculation,
                 (const TCHAR *)escScript, (const TCHAR *)escInstance, m_dwTemplateItemId,
                 (int)m_flags, m_dwResourceId, m_dwProxyNode, m_nBaseUnits, m_nMultiplier, (const TCHAR *)escCustomUnitName,
		           (const TCHAR *)escPerfTabSettings, (const TCHAR *)DBPrepareString(g_hCoreDB, m_systemTag), 
					  (int)m_snmpPort, (int)m_snmpRawValueType);
	}
   else
	{
      _sntprintf(pszQuery, qlen,
		           _T("UPDATE items SET node_id=%d,template_id=%d,name=%s,source=%d,")
                 _T("datatype=%d,polling_interval=%d,retention_time=%d,status=%d,")
                 _T("delta_calculation=%d,transformation=%s,description=%s,")
                 _T("instance=%s,template_item_id=%d,flags=%d,")
                 _T("resource_id=%d,proxy_node=%d,base_units=%d,")
		           _T("unit_multiplier=%d,custom_units_name=%s,perftab_settings=%s,")
	              _T("system_tag=%s,snmp_port=%d,snmp_raw_value_type=%d WHERE item_id=%d"),
                 (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                 (const TCHAR *)escName, m_source, m_dataType, m_iPollingInterval,
                 m_iRetentionTime, m_status, m_deltaCalculation, (const TCHAR *)escScript,
                 (const TCHAR *)escDescr, (const TCHAR *)escInstance, m_dwTemplateItemId,
                 (int)m_flags, m_dwResourceId, m_dwProxyNode, m_nBaseUnits, m_nMultiplier, (const TCHAR *)escCustomUnitName,
		           (const TCHAR *)escPerfTabSettings, (const TCHAR *)DBPrepareString(g_hCoreDB, m_systemTag),
					  (int)m_snmpPort, (int)m_snmpRawValueType, m_dwId);
	}
   bResult = DBQuery(hdb, pszQuery);

   // Save thresholds
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumThresholds; i++)
         m_ppThresholdList[i]->saveToDB(hdb, i);
   }

   // Delete non-existing thresholds
   _sntprintf(pszQuery, qlen, _T("SELECT threshold_id FROM thresholds WHERE item_id=%d"), m_dwId);
   hResult = DBSelect(hdb, pszQuery);
   if (hResult != NULL)
   {
      int i, iNumRows;
      DWORD j, dwId;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         for(j = 0; j < m_dwNumThresholds; j++)
            if (m_ppThresholdList[j]->getId() == dwId)
               break;
         if (j == m_dwNumThresholds)
         {
            _sntprintf(pszQuery, qlen, _T("DELETE FROM thresholds WHERE threshold_id=%d"), dwId);
            DBQuery(hdb, pszQuery);
         }
      }
      DBFreeResult(hResult);
   }

   // Create record in raw_dci_values if needed
   _sntprintf(pszQuery, qlen, _T("SELECT item_id FROM raw_dci_values WHERE item_id=%d"), m_dwId);
   hResult = DBSelect(hdb, pszQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         _sntprintf(pszQuery, qlen, _T("INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time) VALUES (%d,%s,%ld)"),
                 m_dwId, (const TCHAR *)DBPrepareString(hdb, m_prevRawValue.String()), (long)m_tPrevValueTimeStamp);
         DBQuery(hdb, pszQuery);
      }
      DBFreeResult(hResult);
   }

   // Save schedules
   _sntprintf(pszQuery, qlen, _T("DELETE FROM dci_schedules WHERE item_id=%d"), m_dwId);
   DBQuery(hdb, pszQuery);
   if (m_flags & DCF_ADVANCED_SCHEDULE)
   {
      TCHAR *pszEscSchedule;
      DWORD i;

      for(i = 0; i < m_dwNumSchedules; i++)
      {
         pszEscSchedule = EncodeSQLString(m_ppScheduleList[i]);
         _sntprintf(pszQuery, qlen, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES (%d,%d,'%s')"),
                    m_dwId, i + 1, pszEscSchedule);
         free(pszEscSchedule);
         DBQuery(hdb, pszQuery);
      }
   }

   unlock();
   free(pszQuery);
   return bResult;
}


//
// Check last value for threshold breaches
//

void DCItem::checkThresholds(ItemValue &value)
{
	const TCHAR *paramNamesReach[] = { _T("dciName"), _T("dciDescription"), _T("thresholdValue"), _T("currentValue"), _T("dciId"), _T("instance"), _T("isRepeatedEvent") };
	const TCHAR *paramNamesRearm[] = { _T("dciName"), _T("dciDescription"), _T("dciId"), _T("instance"), _T("thresholdValue"), _T("currentValue") };

   DWORD i, iResult, dwInterval;
   ItemValue checkValue;
	time_t now = time(NULL);

   for(i = 0; i < m_dwNumThresholds; i++)
   {
      iResult = m_ppThresholdList[i]->check(value, m_ppValueCache, checkValue);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEventWithNames(m_ppThresholdList[i]->getEventCode(), m_pNode->Id(), "ssssisd",
					paramNamesReach, m_szName, m_szDescription, m_ppThresholdList[i]->getStringValue(), 
               (const TCHAR *)checkValue, m_dwId, m_szInstance, 0);
				m_ppThresholdList[i]->setLastEventTimestamp();
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_dwNumThresholds;  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEventWithNames(m_ppThresholdList[i]->getRearmEventCode(), m_pNode->Id(), "ssisss",
					paramNamesRearm, m_szName, m_szDescription, m_dwId, m_szInstance, 
					m_ppThresholdList[i]->getStringValue(), (const TCHAR *)checkValue);
            break;
         case NO_ACTION:
            if (m_ppThresholdList[i]->isReached())
				{
					// Check if we need to re-sent threshold violation event
					if (m_ppThresholdList[i]->getRepeatInterval() == -1)
						dwInterval = g_dwThresholdRepeatInterval;
					else
						dwInterval = (DWORD)m_ppThresholdList[i]->getRepeatInterval();
					if ((dwInterval != 0) && (m_ppThresholdList[i]->getLastEventTimestamp() + (time_t)dwInterval < now))
					{
						PostEventWithNames(m_ppThresholdList[i]->getEventCode(), m_pNode->Id(), "ssssisd", 
							paramNamesReach, m_szName, m_szDescription, m_ppThresholdList[i]->getStringValue(), 
							(const TCHAR *)checkValue, m_dwId, m_szInstance, 1);
						m_ppThresholdList[i]->setLastEventTimestamp();
					}

					if (!(m_flags & DCF_ALL_THRESHOLDS))
					{
						i = m_dwNumThresholds;  // Threshold condition still true, stop processing
					}
				}
            break;
      }
   }
}


//
// Create NXCP message with item data
//

void DCItem::createMessage(CSCPMessage *pMsg)
{
	DCObject::createMessage(pMsg);

   lock();
   pMsg->SetVariable(VID_INSTANCE, m_szInstance);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_dataType);
   pMsg->SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)m_deltaCalculation);
   pMsg->SetVariable(VID_DCI_FORMULA, CHECK_NULL_EX(m_pszScript));
	pMsg->SetVariable(VID_BASE_UNITS, (WORD)m_nBaseUnits);
	pMsg->SetVariable(VID_MULTIPLIER, (DWORD)m_nMultiplier);
	pMsg->SetVariable(VID_SNMP_RAW_VALUE_TYPE, m_snmpRawValueType);
	if (m_pszCustomUnitName != NULL)
		pMsg->SetVariable(VID_CUSTOM_UNITS_NAME, m_pszCustomUnitName);
   pMsg->SetVariable(VID_NUM_THRESHOLDS, m_dwNumThresholds);
   for(DWORD i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < m_dwNumThresholds; i++, dwId += 10)
      m_ppThresholdList[i]->createMessage(pMsg, dwId);
   unlock();
}


//
// Delete item and collected data from database
//

void DCItem::deleteFromDB()
{
   TCHAR szQuery[256];

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM items WHERE item_id=%d"), m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM idata_%d WHERE item_id=%d"), m_pNode->Id(), m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM thresholds WHERE item_id=%d"), m_dwId);
   QueueSQLRequest(szQuery);
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM dci_schedules WHERE item_id=%d"), m_dwId);
   QueueSQLRequest(szQuery);
}


//
// Update item from NXCP message
//

void DCItem::updateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                               DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
	DCObject::updateFromMessage(pMsg);

   lock();

   pMsg->GetVariableStr(VID_INSTANCE, m_szInstance, MAX_DB_STRING);
   m_dataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
   m_deltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
   TCHAR *pszStr = pMsg->GetVariableStr(VID_DCI_FORMULA);
   setTransformationScript(pszStr);
   free(pszStr);
	m_nBaseUnits = pMsg->GetVariableShort(VID_BASE_UNITS);
	m_nMultiplier = (int)pMsg->GetVariableLong(VID_MULTIPLIER);
	safe_free(m_pszCustomUnitName);
	m_pszCustomUnitName = pMsg->GetVariableStr(VID_CUSTOM_UNITS_NAME);
	m_snmpRawValueType = pMsg->GetVariableShort(VID_SNMP_RAW_VALUE_TYPE);

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
   for(DWORD i = 0; i < m_dwNumThresholds; i++)
   {
		DWORD j;
      for(j = 0; j < dwNum; j++)
         if (m_ppThresholdList[i]->getId() == newThresholds[j])
            break;
      if (j == dwNum)
      {
         // No threshold with that id in new list, delete it
         delete m_ppThresholdList[i];
         m_dwNumThresholds--;
         memmove(&m_ppThresholdList[i], &m_ppThresholdList[i + 1], sizeof(Threshold *) * (m_dwNumThresholds - i));
         i--;
      }
      else
      {
         // Move existing thresholds to appropriate positions in new list
         ppNewList[j] = m_ppThresholdList[i];
      }
   }
   safe_free(m_ppThresholdList);
   m_ppThresholdList = ppNewList;
   m_dwNumThresholds = dwNum;

   // Add or update thresholds
   for(DWORD i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId += 10)
   {
      if (newThresholds[i] == 0)    // New threshold?
      {
         m_ppThresholdList[i] = new Threshold(this);
         m_ppThresholdList[i]->createId();

         // Add index -> id mapping
         (*ppdwMapIndex)[*pdwNumMaps] = i;
         (*ppdwMapId)[*pdwNumMaps] = m_ppThresholdList[i]->getId();
         (*pdwNumMaps)++;
      }
      m_ppThresholdList[i]->updateFromMessage(pMsg, dwId);
   }
      
   safe_free(newThresholds);
   updateCacheSize();
   unlock();
}


//
// Process new value
//

void DCItem::processNewValue(time_t tmTimeStamp, const TCHAR *pszOriginalValue)
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
   pValue = new ItemValue(pszOriginalValue, (DWORD)tmTimeStamp);
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
	values[0] = pszOriginalValue;
	values[1] = pValue->String();
	values[2] = pollTime;
	values[3] = dciId;
	QueueSQLRequest(_T("UPDATE raw_dci_values SET raw_value=?,transformed_value=?,last_poll_time=? WHERE item_id=?"),
	                4, updateRawValueTypes, values);

	// Save transformed value to database
	QueueIDataInsert(tmTimeStamp, m_pNode->Id(), m_dwId, pValue->String());
	/*
	TCHAR query[128];
	_sntprintf(query, 128, _T("INSERT INTO idata_%d (item_id,idata_timestamp,idata_value) VALUES (?,?,?)"), m_pNode->Id());
	values[0] = dciId;
	values[1] = pollTime;
	values[2] = pValue->String();
	QueueSQLRequest(query, 3, updateValueTypes, values);
	*/

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


//
// Process new data collection error
//

void DCItem::processNewError()
{
   DWORD i, iResult;

   lock();

   // Normally m_pNode shouldn't be NULL for polled items, but who knows...
   if (m_pNode == NULL)
   {
      unlock();
      return;
   }

   m_dwErrorCount++;

   for(i = 0; i < m_dwNumThresholds; i++)
   {
      iResult = m_ppThresholdList[i]->checkError(m_dwErrorCount);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEvent(m_ppThresholdList[i]->getEventCode(), m_pNode->Id(), "ssssis", m_szName,
                      m_szDescription, _T(""), _T(""), m_dwId, m_szInstance);
            if (!(m_flags & DCF_ALL_THRESHOLDS))
               i = m_dwNumThresholds;  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEvent(m_ppThresholdList[i]->getRearmEventCode(), m_pNode->Id(), "ssis", m_szName,
                      m_szDescription, m_dwId, m_szInstance);
            break;
         case NO_ACTION:
            if ((m_ppThresholdList[i]->isReached()) &&
					(!(m_flags & DCF_ALL_THRESHOLDS)))
               i = m_dwNumThresholds;  // Threshold condition still true, stop processing
            break;
      }
   }

   unlock();
}


//
// Transform received value
//

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

   if (m_pScript != NULL)
   {
      NXSL_Value *pValue;
      NXSL_ServerEnv *pEnv;

      pValue = new NXSL_Value((const TCHAR *)value);
      pEnv = new NXSL_ServerEnv;
      m_pScript->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_pNode)));
	
      if (m_pScript->run(pEnv, 1, &pValue) == 0)
      {
         pValue = m_pScript->getResult();
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

         _sntprintf(szBuffer, 1024, _T("DCI::%s::%d"),
                    (m_pNode != NULL) ? m_pNode->Name() : _T("(null)"), m_dwId);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer,
                   m_pScript->getErrorText(), m_dwId);
      }
   }
}


//
// Set new ID and node/template association
//

void DCItem::changeBinding(DWORD dwNewId, Template *pNewNode, BOOL doMacroExpansion)
{
	DCObject::changeBinding(dwNewId, pNewNode, doMacroExpansion);

   lock();
	if (dwNewId != 0)
	{
		for(DWORD i = 0; i < m_dwNumThresholds; i++)
			m_ppThresholdList[i]->bindToItem(m_dwId);
	}

	if (doMacroExpansion)
		expandMacros(m_szInstance, m_szInstance, MAX_DB_STRING);

   clearCache();
   updateCacheSize();
   unlock();
}


//
// Update required cache size depending on thresholds
// dwCondId is an identifier of calling condition object id. If it is not 0,
// GetCacheSizeForDCI should be called with bNoLock == TRUE for appropriate
// condition object
//

void DCItem::updateCacheSize(DWORD dwCondId)
{
   DWORD i, dwSize, dwRequiredSize;

   // Sanity check
   if (m_pNode == NULL)
   {
      DbgPrintf(3, _T("DCItem::updateCacheSize() called for DCI %d when m_pNode == NULL"), m_dwId);
      return;
   }

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and it is always 0 for templates
   if (m_pNode->Type() == OBJECT_NODE)
   {
      dwRequiredSize = 1;

      // Calculate required cache size
      for(i = 0; i < m_dwNumThresholds; i++)
         if (dwRequiredSize < m_ppThresholdList[i]->getRequiredCacheSize())
            dwRequiredSize = m_ppThresholdList[i]->getRequiredCacheSize();

		ObjectArray<NetObj> *conditions = g_idxConditionById.getObjects();
		for(i = 0; i < (DWORD)conditions->size(); i++)
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
         for(i = dwRequiredSize; i < m_dwCacheSize; i++)
            delete m_ppValueCache[i];

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
      for(i = m_dwCacheSize; i < dwRequiredSize; i++)
         m_ppValueCache[i] = NULL;

      // Load missing values from database
      if (m_pNode != NULL)
      {
         DB_ASYNC_RESULT hResult;
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
         hResult = DBAsyncSelect(g_hCoreDB, szBuffer);
         if (hResult != NULL)
         {
            // Skip already cached values
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
            for(i = m_dwCacheSize; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
      }
      m_dwCacheSize = dwRequiredSize;
   }
   m_bCacheLoaded = true;
}


//
// Put last value into CSCP message
//

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
      pMsg->SetVariable(dwId++, (TCHAR *)m_ppValueCache[0]->String());
      pMsg->SetVariable(dwId++, m_ppValueCache[0]->GetTimeStamp());
   }
   else
   {
      pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);
      pMsg->SetVariable(dwId++, _T(""));
      pMsg->SetVariable(dwId++, (DWORD)0);
   }
   pMsg->SetVariable(dwId++, (WORD)m_status);

	DWORD i;
   for(i = 0; i < m_dwNumThresholds; i++)
   {
		if (m_ppThresholdList[i]->isReached())
			break;
   }
	if (i < m_dwNumThresholds)
	{
      pMsg->SetVariable(dwId++, (WORD)1);
		m_ppThresholdList[i]->createMessage(pMsg, dwId);
	}
	else
	{
      pMsg->SetVariable(dwId++, (WORD)0);
	}

	unlock();
}


//
// Get item's last value for use in NXSL
//

NXSL_Value *DCItem::getValueForNXSL(int nFunction, int nPolls)
{
   NXSL_Value *pValue;

	lock();
   switch(nFunction)
   {
      case F_LAST:
         pValue = (m_dwCacheSize > 0) ? new NXSL_Value((TCHAR *)m_ppValueCache[0]->String()) : new NXSL_Value;
         break;
      case F_DIFF:
         if (m_dwCacheSize >= 2)
         {
            ItemValue result;

            CalculateItemValueDiff(result, m_dataType, *m_ppValueCache[0], *m_ppValueCache[1]);
            pValue = new NXSL_Value((TCHAR *)result.String());
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
            pValue = new NXSL_Value((TCHAR *)result.String());
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
            pValue = new NXSL_Value((TCHAR *)result.String());
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


//
// Update from template item
//

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
   setTransformationScript(item->m_pszScript);

	m_nBaseUnits = item->m_nBaseUnits;
	m_nMultiplier = item->m_nMultiplier;
	safe_free(m_pszCustomUnitName);
	m_pszCustomUnitName = (item->m_pszCustomUnitName != NULL) ? _tcsdup(item->m_pszCustomUnitName) : NULL;

   // Copy thresholds
   // ***************************
   // First, skip matching thresholds
   DWORD dwCount = min(m_dwNumThresholds, item->m_dwNumThresholds);
	DWORD i;
   for(i = 0; i < dwCount; i++)
      if (!m_ppThresholdList[i]->compare(item->m_ppThresholdList[i]))
         break;
   dwCount = i;   // First unmatched threshold's position

   // Delete all original thresholds starting from first unmatched
   for(; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];

   // (Re)create thresholds starting from first unmatched
   m_dwNumThresholds = item->m_dwNumThresholds;
   m_ppThresholdList = (Threshold **)realloc(m_ppThresholdList, sizeof(Threshold *) * m_dwNumThresholds);
   for(i = dwCount; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i] = new Threshold(item->m_ppThresholdList[i]);
      m_ppThresholdList[i]->createId();
      m_ppThresholdList[i]->bindToItem(m_dwId);
   }

   expandMacros(item->m_szInstance, m_szInstance, MAX_DB_STRING);

   updateCacheSize();
   
   unlock();
}


//
// Set new formula
//

void DCItem::setTransformationScript(const TCHAR *pszScript)
{
   safe_free(m_pszScript);
   delete m_pScript;
   if (pszScript != NULL)
   {
      m_pszScript = _tcsdup(pszScript);
      StrStrip(m_pszScript);
      if (m_pszScript[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_pScript = (NXSL_Program *)NXSLCompile(m_pszScript, NULL, 0);
      }
      else
      {
         m_pScript = NULL;
      }
   }
   else
   {
      m_pszScript = NULL;
      m_pScript = NULL;
   }
}


//
// Get list of used events
//

void DCItem::getEventList(DWORD **ppdwList, DWORD *pdwSize)
{
   DWORD i, j;

   lock();

   if (m_dwNumThresholds > 0)
   {
      *ppdwList = (DWORD *)realloc(*ppdwList, sizeof(DWORD) * (*pdwSize + m_dwNumThresholds * 2));
      j = *pdwSize;
      *pdwSize += m_dwNumThresholds * 2;
      for(i = 0; i < m_dwNumThresholds; i++)
      {
         (*ppdwList)[j++] = m_ppThresholdList[i]->getEventCode();
			(*ppdwList)[j++] = m_ppThresholdList[i]->getRearmEventCode();
      }
   }

   unlock();
}


//
// Create management pack record
//

void DCItem::createNXMPRecord(String &str)
{
	DWORD i;

   lock();
   
   str.addFormattedString(_T("\t\t\t\t<dci id=\"%d\">\n")
                          _T("\t\t\t\t\t<name>%s</name>\n")
                          _T("\t\t\t\t\t<description>%s</description>\n")
                          _T("\t\t\t\t\t<dataType>%d</dataType>\n")
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
                          m_dataType, (int)m_source, m_iPollingInterval, m_iRetentionTime,
                          (const TCHAR *)EscapeStringForXML2(m_szInstance),
                          (const TCHAR *)EscapeStringForXML2(m_systemTag),
								  (int)m_deltaCalculation, (m_flags & DCF_ADVANCED_SCHEDULE) ? 1 : 0,
                          (m_flags & DCF_ALL_THRESHOLDS) ? 1 : 0, 
								  (m_flags & DCF_RAW_VALUE_OCTET_STRING) ? 1 : 0, 
								  (int)m_snmpRawValueType, (int)m_snmpPort);

	if (m_pszScript != NULL)
	{
		str += _T("\t\t\t\t\t<transformation>");
		str.addDynamicString(EscapeStringForXML(m_pszScript, -1));
		str += _T("</transformation>\n");
	}

	if (m_flags & DCF_ADVANCED_SCHEDULE)
   {
      str += _T("\t\t\t\t\t<schedules>\n");
      for(i = 0; i < m_dwNumSchedules; i++)
         str.addFormattedString(_T("\t\t\t\t\t\t<schedule>%s</schedule>\n"), (const TCHAR *)EscapeStringForXML2(m_ppScheduleList[i]));
      str += _T("\t\t\t\t\t</schedules>\n");
   }

   str += _T("\t\t\t\t\t<thresholds>\n");
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i]->createNXMPRecord(str, i + 1);
   }
   str += _T("\t\t\t\t\t</thresholds>\n");

   unlock();
   str += _T("\t\t\t\t</dci>\n");
}


//
// Modify item - intended for updating items in system templates
//

void DCItem::systemModify(const TCHAR *pszName, int nOrigin, int nRetention, int nInterval, int nDataType)
{
   m_dataType = nDataType;
   m_iPollingInterval = nInterval;
   m_iRetentionTime = nRetention;
   m_source = nOrigin;
	nx_strncpy(m_szName, pszName, MAX_ITEM_NAME);
}


//
// Add threshold to the list
//

void DCItem::addThreshold(Threshold *pThreshold)
{
	m_dwNumThresholds++;
	m_ppThresholdList = (Threshold **)realloc(m_ppThresholdList, sizeof(Threshold *) * m_dwNumThresholds);
   m_ppThresholdList[m_dwNumThresholds - 1] = pThreshold;
}


//
// Enumerate all thresholds
//

BOOL DCItem::enumThresholds(BOOL (* pfCallback)(Threshold *, DWORD, void *), void *pArg)
{
	DWORD i;
	BOOL bRet = TRUE;

	lock();
	for(i = 0; i < m_dwNumThresholds; i++)
	{
		if (!pfCallback(m_ppThresholdList[i], i, pArg))
		{
			bRet = FALSE;
			break;
		}
	}
	unlock();
	return bRet;
}


//
// Test DCI's transformation script
// Runs 
//

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


//
// Fill NXCP message with thresholds
//

void DCItem::fillMessageWithThresholds(CSCPMessage *msg)
{
	DWORD i, id;

	lock();

	msg->SetVariable(VID_NUM_THRESHOLDS, m_dwNumThresholds);
	for(i = 0, id = VID_DCI_THRESHOLD_BASE; i < m_dwNumThresholds; i++, id += 10)
	{
		m_ppThresholdList[i]->createMessage(msg, id);
	}

	unlock();
}


//
// Returns true if internal cache is loaded. If data collection object
// does not have cache should return true
//

bool DCItem::isCacheLoaded()
{
	return m_bCacheLoaded;
}
