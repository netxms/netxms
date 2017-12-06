/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: dcobject.cpp
**
**/

#include "nxcore.h"

/**
 * Default retention time for collected data
 */
int DCObject::m_defaultRetentionTime = 30;

/**
 * Default data collection polling interval
 */
int DCObject::m_defaultPollingInterval = 60;

/**
 * Default constructor for DCObject
 */
DCObject::DCObject()
{
   m_id = 0;
   m_guid = uuid::generate();
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_iPollingInterval = 3600;
   m_iRetentionTime = 0;
   m_source = DS_INTERNAL;
   m_status = ITEM_STATUS_NOT_SUPPORTED;
   m_name[0] = 0;
   m_description[0] = 0;
	m_systemTag[0] = 0;
   m_tLastPoll = 0;
   m_owner = NULL;
   m_hMutex = MutexCreateRecursive();
   m_schedules = NULL;
   m_tLastCheck = 0;
	m_flags = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_sourceNode = 0;
	m_pszPerfTabSettings = NULL;
	m_snmpPort = 0;	// use default
   m_transformationScriptSource = NULL;
   m_transformationScript = NULL;
   m_comments = NULL;
   m_pollingSession = NULL;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceDiscoveryData = NULL;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   m_instance[0] = 0;
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
}

/**
 * Create DCObject from another DCObject
 */
DCObject::DCObject(const DCObject *pSrc)
{
   m_id = pSrc->m_id;
   m_guid = pSrc->m_guid;
   m_dwTemplateId = pSrc->m_dwTemplateId;
   m_dwTemplateItemId = pSrc->m_dwTemplateItemId;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_iPollingInterval = pSrc->m_iPollingInterval;
   m_iRetentionTime = pSrc->m_iRetentionTime;
   m_source = pSrc->m_source;
   m_status = pSrc->m_status;
   m_tLastPoll = 0;
	_tcscpy(m_name, pSrc->m_name);
	_tcscpy(m_description, pSrc->m_description);
	_tcscpy(m_systemTag, pSrc->m_systemTag);
   m_owner = NULL;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_flags = pSrc->m_flags;
	m_dwResourceId = pSrc->m_dwResourceId;
	m_sourceNode = pSrc->m_sourceNode;
	m_pszPerfTabSettings = (pSrc->m_pszPerfTabSettings != NULL) ? _tcsdup(pSrc->m_pszPerfTabSettings) : NULL;
	m_snmpPort = pSrc->m_snmpPort;
	m_comments = (pSrc->m_comments != NULL) ? _tcsdup(pSrc->m_comments) : NULL;
	m_pollingSession = pSrc->m_pollingSession;

   m_transformationScriptSource = NULL;
   m_transformationScript = NULL;
   setTransformationScript(pSrc->m_transformationScriptSource);

   m_schedules = (pSrc->m_schedules != NULL) ? new StringList(pSrc->m_schedules) : NULL;

   m_instanceDiscoveryMethod = pSrc->m_instanceDiscoveryMethod;
   m_instanceDiscoveryData = (pSrc->m_instanceDiscoveryData != NULL) ? _tcsdup(pSrc->m_instanceDiscoveryData) : NULL;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   setInstanceFilter(pSrc->m_instanceFilterSource);
   _tcscpy(m_instance, pSrc->m_instance);
   m_accessList = new IntegerArray<UINT32>(pSrc->m_accessList);
   m_instanceRetentionTime = pSrc->m_instanceRetentionTime;
   m_instanceGracePeriodStart = pSrc->m_instanceGracePeriodStart;
}

/**
 * Constructor for creating new DCObject from scratch
 */
DCObject::DCObject(UINT32 dwId, const TCHAR *szName, int iSource,
               int iPollingInterval, int iRetentionTime, Template *pNode,
               const TCHAR *pszDescription, const TCHAR *systemTag)
{
   m_id = dwId;
   m_guid = uuid::generate();
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   nx_strncpy(m_name, szName, MAX_ITEM_NAME);
   if (pszDescription != NULL)
      nx_strncpy(m_description, pszDescription, MAX_DB_STRING);
   else
      _tcscpy(m_description, m_name);
	nx_strncpy(m_systemTag, CHECK_NULL_EX(systemTag), MAX_DB_STRING);
   m_source = iSource;
   m_iPollingInterval = iPollingInterval;
   m_iRetentionTime = iRetentionTime;
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_tLastPoll = 0;
   m_owner = pNode;
   m_hMutex = MutexCreateRecursive();
   m_flags = 0;
   m_schedules = NULL;
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_sourceNode = 0;
	m_pszPerfTabSettings = NULL;
	m_snmpPort = 0;	// use default
   m_transformationScriptSource = NULL;
   m_transformationScript = NULL;
   m_comments = NULL;
   m_pollingSession = NULL;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceDiscoveryData = NULL;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   m_instance[0] = 0;
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
}

/**
 * Create DCObject from import file
 */
DCObject::DCObject(ConfigEntry *config, Template *owner)
{
   m_id = CreateUniqueId(IDG_ITEM);
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
	nx_strncpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("unnamed")), MAX_ITEM_NAME);
   nx_strncpy(m_description, config->getSubEntryValue(_T("description"), 0, m_name), MAX_DB_STRING);
	nx_strncpy(m_systemTag, config->getSubEntryValue(_T("systemTag"), 0, _T("")), MAX_DB_STRING);
	m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_iPollingInterval = config->getSubEntryValueAsInt(_T("interval"));
   m_iRetentionTime = config->getSubEntryValueAsInt(_T("retention"));
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
	m_flags = (UINT16)config->getSubEntryValueAsInt(_T("flags"));
   m_tLastPoll = 0;
   m_owner = owner;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_sourceNode = 0;
   const TCHAR *perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   m_pszPerfTabSettings = (perfTabSettings != NULL) ? _tcsdup(perfTabSettings) : NULL;
	m_snmpPort = (WORD)config->getSubEntryValueAsInt(_T("snmpPort"));
   m_schedules = NULL;

	m_transformationScriptSource = NULL;
	m_transformationScript = NULL;
	m_comments = NULL;
   m_pollingSession = NULL;
	setTransformationScript(config->getSubEntryValue(_T("transformation")));

   // for compatibility with old format
	if (config->getSubEntryValueAsInt(_T("advancedSchedule")))
		m_flags |= DCF_ADVANCED_SCHEDULE;

	ConfigEntry *schedules = config->findEntry(_T("schedules"));
	if (schedules != NULL)
		schedules = schedules->findEntry(_T("schedule"));
	if ((schedules != NULL) && (schedules->getValueCount() > 0))
	{
	   m_schedules = new StringList();
	   int count = schedules->getValueCount();
      for(int i = 0; i < count; i++)
      {
         m_schedules->add(schedules->getValue(i));
      }
	}

   m_instanceDiscoveryMethod = (WORD)config->getSubEntryValueAsInt(_T("instanceDiscoveryMethod"));
   const TCHAR *value = config->getSubEntryValue(_T("instanceDiscoveryData"));
   m_instanceDiscoveryData = (value != NULL) ? _tcsdup(value) : NULL;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   nx_strncpy(m_instance, config->getSubEntryValue(_T("instance"), 0, _T("")), MAX_DB_STRING);
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = config->getSubEntryValueAsInt(_T("instanceRetentionTime"), 0, -1);
   m_instanceGracePeriodStart = 0;
}

/**
 * Destructor
 */
DCObject::~DCObject()
{
   free(m_transformationScriptSource);
   delete m_transformationScript;
   delete m_schedules;
	free(m_pszPerfTabSettings);
	free(m_comments);
   MutexDestroy(m_hMutex);
   free(m_instanceDiscoveryData);
   free(m_instanceFilterSource);
   delete m_instanceFilter;
   delete m_accessList;
}

/**
 * Load access list
 */
bool DCObject::loadAccessList(DB_HANDLE hdb)
{
   m_accessList->clear();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT user_id FROM dci_access WHERE dci_id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         m_accessList->add(DBGetFieldULong(hResult, i, 0));
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   return hResult != NULL;
}

/**
 * Load custom schedules from database
 * (assumes that no schedules was created before this call)
 */
bool DCObject::loadCustomSchedules(DB_HANDLE hdb)
{
   if (!(m_flags & DCF_ADVANCED_SCHEDULE))
		return true;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT schedule FROM dci_schedules WHERE item_id=?"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      if (count > 0)
      {
         m_schedules = new StringList();
         for(int i = 0; i < count; i++)
         {
            m_schedules->addPreallocated(DBGetField(hResult, i, 0, NULL, 0));
         }
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
	return hResult != NULL;
}

/**
 * Check if associated cluster resource is active. Returns true also if
 * DCI has no resource association
 */
bool DCObject::matchClusterResource()
{
	Cluster *pCluster;

   if ((m_dwResourceId == 0) || (m_owner->getObjectClass() != OBJECT_NODE))
		return true;

	pCluster = ((Node *)m_owner)->getMyCluster();
	if (pCluster == NULL)
		return false;	// Has association, but cluster object cannot be found

	return pCluster->isResourceOnNode(m_dwResourceId, m_owner->getId());
}

/**
 * Expand macros in text
 */
void DCObject::expandMacros(const TCHAR *src, TCHAR *dst, size_t dstLen)
{
	int index = 0, index2;
	String temp = src;
	while((index = temp.find(_T("%{"), index)) != String::npos)
	{
		String head = temp.substring(0, index);
		index2 = temp.find(_T("}"), index);
		if (index2 == String::npos)
			break;	// Missing closing }

		String rest = temp.substring(index2 + 1, -1);
		String macro = temp.substring(index + 2, index2 - index - 2);
		macro.trim();

		temp = head;
		if (!_tcscmp(macro, _T("node_id")))
		{
			if (m_owner != NULL)
			{
				temp.appendFormattedString(_T("%d"), m_owner->getId());
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_name")))
		{
			if (m_owner != NULL)
			{
				temp += m_owner->getName();
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_primary_ip")))
		{
			if ((m_owner != NULL) && (m_owner->getObjectClass() == OBJECT_NODE))
			{
				TCHAR ipAddr[64];
				temp += ((Node *)m_owner)->getIpAddress().toString(ipAddr);
			}
			else
			{
				temp += _T("(error)");
			}
		}
		else if (!_tcsncmp(macro, _T("script:"), 7))
		{
			NXSL_VM *vm = CreateServerScriptVM(&macro[7]);
			if (vm != NULL)
			{
				if (m_owner != NULL)
					vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_owner)));

				if (vm->run(0, NULL))
				{
					NXSL_Value *result = vm->getResult();
					if (result != NULL)
						temp += CHECK_NULL_EX(result->getValueAsCString());
		         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s executed successfully"), m_id, src, &macro[7]);
				}
				else
				{
		         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s execution error: %s"),
					          m_id, src, &macro[7], vm->getErrorText());
					PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", &macro[7], vm->getErrorText(), m_id);
				}
            delete vm;
			}
			else
			{
	         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Cannot find script %s"), m_id, src, &macro[7]);
			}
		}
		temp.append(rest);
	}
	nx_strncpy(dst, temp, dstLen);
}

/**
 * Delete all collected data
 */
bool DCObject::deleteAllData()
{
	return false;
}

/**
 * Add schedule
 */
void DCObject::addSchedule(const TCHAR *pszSchedule)
{
   if (m_schedules == NULL)
      m_schedules = new StringList();
   m_schedules->add(pszSchedule);
}

/**
 * Set new ID and node/template association
 */
void DCObject::changeBinding(UINT32 dwNewId, Template *newOwner, BOOL doMacroExpansion)
{
   lock();
   m_owner = newOwner;
	if (dwNewId != 0)
	{
		m_id = dwNewId;
		m_guid = uuid::generate();
	}

	if (doMacroExpansion)
	{
		expandMacros(m_name, m_name, MAX_ITEM_NAME);
		expandMacros(m_description, m_description, MAX_DB_STRING);
      expandMacros(m_instance, m_instance, MAX_DB_STRING);
	}

   unlock();
}

/**
 * Set DCI status
 */
void DCObject::setStatus(int status, bool generateEvent)
{
	if (generateEvent && (m_owner != NULL) && (m_status != (BYTE)status) && IsEventSource(m_owner->getObjectClass()))
	{
		static UINT32 eventCode[3] = { EVENT_DCI_ACTIVE, EVENT_DCI_DISABLED, EVENT_DCI_UNSUPPORTED };
		static const TCHAR *originName[8] = { _T("Internal"), _T("NetXMS Agent"), _T("SNMP"), _T("CheckPoint SNMP"), _T("Push"), _T("WinPerf"), _T("iLO"), _T("Script") };
		PostEvent(eventCode[status], m_owner->getId(), "dssds", m_id, m_name, m_description, m_source, originName[m_source]);
	}
	m_status = (BYTE)status;
}

/**
 * Match schedule to current time
 */
bool DCObject::matchSchedule(const TCHAR *schedule, bool *withSeconds, struct tm *currLocalTime, time_t currTimestamp)
{
   TCHAR szValue[256], expandedSchedule[1024];
   const TCHAR *realSchedule = schedule;

   if (_tcslen(schedule) > 4 && !_tcsncmp(schedule, _T("%["), 2))
   {
      TCHAR *scriptName = _tcsdup(schedule + 2);
      if (scriptName != NULL)
      {
         bool success = false;
         TCHAR *closingBracker = _tcschr(scriptName, _T(']'));
         if (closingBracker != NULL)
         {
            *closingBracker = 0;

            NXSL_VM *vm = CreateServerScriptVM(scriptName);
            if (vm != NULL)
            {
               vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, m_owner)));
               vm->setGlobalVariable(_T("$dci"), createNXSLObject());
               if (vm->run(0, NULL))
               {
                  NXSL_Value *result = vm->getResult();
                  if (result != NULL)
                  {
                     const TCHAR *temp = result->getValueAsCString();
                     if (temp != NULL)
                     {
                        DbgPrintf(7, _T("DCObject::matchSchedule(%%[%s]) expanded to \"%s\""), scriptName, temp);
                        nx_strncpy(expandedSchedule, temp, 1024);
                        realSchedule = expandedSchedule;
                        success = true;
                     }
                  }
               }
               else
               {
                  DbgPrintf(4, _T("DCObject::matchSchedule(%%[%s]) script execution failed (%s)"), scriptName, vm->getErrorText());
               }
               delete vm;
            }
         }
         else
         {
            DbgPrintf(4, _T("DCObject::matchSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name);
         }
         free(scriptName);
         if (!success)
            return false;
      }
      else
      {
         DbgPrintf(4, _T("DCObject::matchSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name);
         return false;
      }
   }

   // Minute
   const TCHAR *pszCurr = ExtractWord(realSchedule, szValue);
   if (!MatchScheduleElement(szValue, currLocalTime->tm_min, 59, currLocalTime, currTimestamp))
      return false;

   // Hour
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currLocalTime->tm_hour, 23, currLocalTime, currTimestamp))
      return false;

   // Day of month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currLocalTime->tm_mday, GetLastMonthDay(currLocalTime), currLocalTime, currTimestamp))
      return false;

   // Month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currLocalTime->tm_mon + 1, 12, currLocalTime, currTimestamp))
      return false;

   // Day of week
   pszCurr = ExtractWord(pszCurr, szValue);
   for(int i = 0; szValue[i] != 0; i++)
      if (szValue[i] == _T('7'))
         szValue[i] = _T('0');
   if (!MatchScheduleElement(szValue, currLocalTime->tm_wday, 7, currLocalTime, currTimestamp))
      return false;

   // Seconds
   szValue[0] = _T('\0');
   ExtractWord(pszCurr, szValue);
   if (szValue[0] != _T('\0'))
   {
      if (withSeconds != NULL)
         *withSeconds = true;
      return MatchScheduleElement(szValue, currLocalTime->tm_sec, 59, currLocalTime, currTimestamp);
   }

   return true;
}

/**
 * Check if data collection object have to be polled
 */
bool DCObject::isReadyForPolling(time_t currTime)
{
   // Normally data collection object will be locked when it is being
   // changed or when it is processing new data
   // In both cases there is no point to block item poller and wait, it
   // is more effective to try schedule on next run
   if (!tryLock())
   {
      nxlog_debug(3, _T("DCObject::isReadyForPolling: cannot obtain lock for data collection object %d"), m_id);
      return false;
   }

   if ((m_pollingSession != NULL) && !m_busy)
   {
      if ((m_status != ITEM_STATUS_DISABLED) &&
          isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
          matchClusterResource() && hasValue() && (getAgentCacheMode() == AGENT_CACHE_OFF))
      {
         unlock();
         return true;
      }
      else
      {
         // DCI cannot be force polled at the moment, clear force poll request
         nxlog_debug(6, _T("Forced poll of DC object %s [%d] on node %s [%d] cancelled"), m_name, m_id, m_owner->getName(), m_owner->getId());
         m_pollingSession->decRefCount();
         m_pollingSession = NULL;
         unlock();
         return false;
      }
   }

   bool result;
   if ((m_status != ITEM_STATUS_DISABLED) && (!m_busy) &&
       isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
       matchClusterResource() && hasValue() && (getAgentCacheMode() == AGENT_CACHE_OFF))
   {
      if (m_flags & DCF_ADVANCED_SCHEDULE)
      {
         if (m_schedules != NULL)
         {
            struct tm tmCurrLocal, tmLastLocal;
#if HAVE_LOCALTIME_R
            localtime_r(&currTime, &tmCurrLocal);
            localtime_r(&m_tLastCheck, &tmLastLocal);
#else
            memcpy(&tmCurrLocal, localtime(&currTime), sizeof(struct tm));
            memcpy(&tmLastLocal, localtime(&m_tLastCheck), sizeof(struct tm));
#endif
            result = false;
            for(int i = 0; i < m_schedules->size(); i++)
            {
               bool withSeconds = false;
               if (matchSchedule(m_schedules->get(i), &withSeconds, &tmCurrLocal, currTime))
               {
                  // TODO: do we have to take care about the schedules with seconds
                  // that trigger polling too often?
                  if (withSeconds || (currTime - m_tLastCheck >= 60) || (tmCurrLocal.tm_min != tmLastLocal.tm_min))
                  {
                     result = true;
                     break;
                  }
               }
            }
         }
         else
         {
            result = false;
         }
         m_tLastCheck = currTime;
      }
      else
      {
			if (m_status == ITEM_STATUS_NOT_SUPPORTED)
		      result = (m_tLastPoll + getEffectivePollingInterval() * 10 <= currTime);
			else
		      result = (m_tLastPoll + getEffectivePollingInterval() <= currTime);
      }
   }
   else
   {
      result = false;
   }
   unlock();
   return result;
}

/**
 * Returns true if internal cache is loaded. If data collection object
 * does not have cache should return true
 */
bool DCObject::isCacheLoaded()
{
	return true;
}

/**
 * Prepare object for deletion
 */
bool DCObject::prepareForDeletion()
{
	DbgPrintf(9, _T("DCObject::prepareForDeletion for DCO %d"), m_id);

	lock();
   m_status = ITEM_STATUS_DISABLED;   // Prevent future polls
	m_scheduledForDeletion = 1;
	bool canDelete = (m_busy ? false : true);
   unlock();
	DbgPrintf(9, _T("DCObject::prepareForDeletion: completed for DCO %d, canDelete=%d"), m_id, (int)canDelete);

	return canDelete;
}

/**
 * Create NXCP message with object data
 */
void DCObject::createMessage(NXCPMessage *pMsg)
{
	lock();
   pMsg->setField(VID_DCI_ID, m_id);
	pMsg->setField(VID_DCOBJECT_TYPE, (WORD)getType());
   pMsg->setField(VID_TEMPLATE_ID, m_dwTemplateId);
   pMsg->setField(VID_NAME, m_name);
   pMsg->setField(VID_DESCRIPTION, m_description);
   pMsg->setField(VID_TRANSFORMATION_SCRIPT, CHECK_NULL_EX(m_transformationScriptSource));
   pMsg->setField(VID_FLAGS, m_flags);
   pMsg->setField(VID_SYSTEM_TAG, m_systemTag);
   pMsg->setField(VID_POLLING_INTERVAL, (UINT32)m_iPollingInterval);
   pMsg->setField(VID_RETENTION_TIME, (UINT32)m_iRetentionTime);
   pMsg->setField(VID_DCI_SOURCE_TYPE, (WORD)m_source);
   pMsg->setField(VID_DCI_STATUS, (WORD)m_status);
	pMsg->setField(VID_RESOURCE_ID, m_dwResourceId);
	pMsg->setField(VID_AGENT_PROXY, m_sourceNode);
	pMsg->setField(VID_SNMP_PORT, m_snmpPort);
	if (m_comments != NULL)
		pMsg->setField(VID_COMMENTS, m_comments);
	if (m_pszPerfTabSettings != NULL)
		pMsg->setField(VID_PERFTAB_SETTINGS, m_pszPerfTabSettings);
	if (m_schedules != NULL)
	{
      pMsg->setField(VID_NUM_SCHEDULES, (UINT32)m_schedules->size());
      UINT32 fieldId = VID_DCI_SCHEDULE_BASE;
      for(int i = 0; i < m_schedules->size(); i++, fieldId++)
         pMsg->setField(fieldId, m_schedules->get(i));
	}
	else
	{
      pMsg->setField(VID_NUM_SCHEDULES, (UINT32)0);
	}
   pMsg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   if (m_instanceDiscoveryData != NULL)
      pMsg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   if (m_instanceFilterSource != NULL)
      pMsg->setField(VID_INSTD_FILTER, m_instanceFilterSource);
   pMsg->setField(VID_INSTANCE, m_instance);
   pMsg->setFieldFromInt32Array(VID_ACL, m_accessList);
   pMsg->setField(VID_INSTANCE_RETENTION, m_instanceRetentionTime);
   unlock();
}

/**
 * Update data collection object from NXCP message
 */
void DCObject::updateFromMessage(NXCPMessage *pMsg)
{
   lock();

   pMsg->getFieldAsString(VID_NAME, m_name, MAX_ITEM_NAME);
   pMsg->getFieldAsString(VID_DESCRIPTION, m_description, MAX_DB_STRING);
   pMsg->getFieldAsString(VID_SYSTEM_TAG, m_systemTag, MAX_DB_STRING);
	m_flags = pMsg->getFieldAsUInt16(VID_FLAGS);
   m_source = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_SOURCE_TYPE);
   m_iPollingInterval = pMsg->getFieldAsUInt32(VID_POLLING_INTERVAL);
   m_iRetentionTime = pMsg->getFieldAsUInt32(VID_RETENTION_TIME);
   setStatus(pMsg->getFieldAsUInt16(VID_DCI_STATUS), true);
	m_dwResourceId = pMsg->getFieldAsUInt32(VID_RESOURCE_ID);
	m_sourceNode = pMsg->getFieldAsUInt32(VID_AGENT_PROXY);
   m_snmpPort = pMsg->getFieldAsUInt16(VID_SNMP_PORT);

	free(m_pszPerfTabSettings);
	m_pszPerfTabSettings = pMsg->getFieldAsString(VID_PERFTAB_SETTINGS);

	free(m_comments);
   m_comments = pMsg->getFieldAsString(VID_COMMENTS);

   TCHAR *pszStr = pMsg->getFieldAsString(VID_TRANSFORMATION_SCRIPT);
   setTransformationScript(pszStr);
   free(pszStr);

   // Update schedules
   int count = pMsg->getFieldAsInt32(VID_NUM_SCHEDULES);
   if (count > 0)
   {
      if (m_schedules != NULL)
         m_schedules->clear();
      else
         m_schedules = new StringList();

      UINT32 fieldId = VID_DCI_SCHEDULE_BASE;
      for(int i = 0; i < count; i++, fieldId++)
      {
         TCHAR *s = pMsg->getFieldAsString(fieldId);
         if (s != NULL)
         {
            m_schedules->addPreallocated(s);
         }
      }
   }
   else
   {
      delete_and_null(m_schedules);
   }

   m_instanceDiscoveryMethod = pMsg->getFieldAsUInt16(VID_INSTD_METHOD);

   free(m_instanceDiscoveryData);
   m_instanceDiscoveryData = pMsg->getFieldAsString(VID_INSTD_DATA);

   pszStr = pMsg->getFieldAsString(VID_INSTD_FILTER);
   setInstanceFilter(pszStr);
   free(pszStr);
   pMsg->getFieldAsString(VID_INSTANCE, m_instance, MAX_DB_STRING);

   m_accessList->clear();
   pMsg->getFieldAsInt32Array(VID_ACL, m_accessList);

   m_instanceRetentionTime = pMsg->getFieldAsInt32(VID_INSTANCE_RETENTION);

	unlock();
}

/**
 * Save to database
 */
bool DCObject::saveToDatabase(DB_HANDLE hdb)
{
	lock();

   // Save schedules
   bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_schedules WHERE item_id=?"));
	if (success && (m_schedules != NULL) && !m_schedules->isEmpty())
   {
	   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES (?,?,?)"));
	   if (hStmt != NULL)
	   {
	      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_schedules->size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_schedules->get(i), DB_BIND_STATIC);
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
	   }
	   else
	   {
	      success = false;
	   }
   }

   // Save access list
   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE dci_id=?"));
   if (success && !m_accessList->isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dci_access (dci_id,user_id) VALUES (?,?)"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_accessList->size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_accessList->get(i));
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

	unlock();

	return success;
}

/**
 * Delete object and collected data from database
 */
void DCObject::deleteFromDatabase()
{
	TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM dci_schedules WHERE item_id=%d"), (int)m_id);
   QueueSQLRequest(query);

   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM dci_access WHERE dci_id=%d"), (int)m_id);
   QueueSQLRequest(query);
}

/**
 * Load data collection object thresholds from database
 */
bool DCObject::loadThresholdsFromDB(DB_HANDLE hdb)
{
	return true;
}

/**
 * Expand {instance} macro in name and description
 */
void DCObject::expandInstance()
{
   String temp = m_name;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instance);
   nx_strncpy(m_name, (const TCHAR *)temp, MAX_ITEM_NAME);

   temp = m_description;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instance);
   nx_strncpy(m_description, (const TCHAR *)temp, MAX_DB_STRING);
}

/**
 * Update DC object from template object
 */
void DCObject::updateFromTemplate(DCObject *src)
{
	lock();

   expandMacros(src->m_name, m_name, MAX_ITEM_NAME);
   expandMacros(src->m_description, m_description, MAX_DB_STRING);
	expandMacros(src->m_systemTag, m_systemTag, MAX_DB_STRING);

   m_iPollingInterval = src->m_iPollingInterval;
   m_iRetentionTime = src->m_iRetentionTime;
   m_source = src->m_source;
   setStatus(src->m_status, true);
	m_flags = src->m_flags;
	m_sourceNode = src->m_sourceNode;
	m_dwResourceId = src->m_dwResourceId;
	m_snmpPort = src->m_snmpPort;

	free(m_pszPerfTabSettings);
	m_pszPerfTabSettings = _tcsdup_ex(src->m_pszPerfTabSettings);

   setTransformationScript(src->m_transformationScriptSource);

   // Copy schedules
   delete m_schedules;
   m_schedules = (src->m_schedules != NULL) ? new StringList(src->m_schedules) : NULL;

   if ((src->getInstanceDiscoveryMethod() != IDM_NONE) && (m_instanceDiscoveryMethod == IDM_NONE))
   {
      expandInstance();
   }
   else
   {
      expandMacros(src->m_instance, m_instance, MAX_DB_STRING);
      m_instanceDiscoveryMethod = src->m_instanceDiscoveryMethod;
      free(m_instanceDiscoveryData);
      m_instanceDiscoveryData = _tcsdup_ex(src->m_instanceDiscoveryData);
      safe_free_and_null(m_instanceFilterSource);
      delete_and_null(m_instanceFilter);
      setInstanceFilter(src->m_instanceFilterSource);
   }
   m_instanceRetentionTime = src->m_instanceRetentionTime;

	unlock();
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 *
 * @return true on success
 */
bool DCObject::processNewValue(time_t nTimeStamp, const void *value, bool *updateStatus)
{
   *updateStatus = false;
   return false;
}

/**
 * Process new data collection error
 */
void DCObject::processNewError(bool noInstance)
{
   time_t now = time(NULL);
   processNewError(noInstance, now);
}

/**
 * Process new data collection error
 */
void DCObject::processNewError(bool noInstance, time_t now)
{
}

/**
 * Should return true if object has (or can have) value
 */
bool DCObject::hasValue()
{
   if ((m_owner != NULL) && (m_owner->getObjectClass() == OBJECT_CLUSTER))
      return isAggregateOnCluster() && (m_instanceDiscoveryMethod == IDM_NONE);
   return m_instanceDiscoveryMethod == IDM_NONE;
}

/**
 * Set new transformation script
 */
void DCObject::setTransformationScript(const TCHAR *source)
{
   free(m_transformationScriptSource);
   delete m_transformationScript;
   if (source != NULL)
   {
      m_transformationScriptSource = _tcsdup(source);
      StrStrip(m_transformationScriptSource);
      if (m_transformationScriptSource[0] != 0)
      {
         TCHAR errorText[1024];
         m_transformationScript = NXSLCompile(m_transformationScriptSource, errorText, 1024, NULL);
         if (m_transformationScript == NULL)
         {
            nxlog_write(MSG_TRANSFORMATION_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dsdss",
                        getOwnerId(), getOwnerName(), m_id, m_name, errorText);
         }
      }
      else
      {
         m_transformationScript = NULL;
      }
   }
   else
   {
      m_transformationScriptSource = NULL;
      m_transformationScript = NULL;
   }
}

/**
 * Get actual agent cache mode
 */
INT16 DCObject::getAgentCacheMode()
{
   if ((m_source != DS_NATIVE_AGENT) && (m_source != DS_SNMP_AGENT))
      return AGENT_CACHE_OFF;

   Node *node = NULL;
   if (m_sourceNode != 0)
   {
      node = (Node *)FindObjectById(m_sourceNode, OBJECT_NODE);
   }
   else
   {
      if (m_owner->getObjectClass() == OBJECT_NODE)
      {
         node = (Node *)m_owner;
      }
      else if (m_owner->getObjectClass() == OBJECT_CHASSIS)
      {
         node = (Node *)FindObjectById(((Chassis *)m_owner)->getControllerId(), OBJECT_NODE);
      }
   }
   if (node == NULL)
      return AGENT_CACHE_OFF;

   if ((m_source == DS_SNMP_AGENT) && (node->getEffectiveSnmpProxy() == 0))
      return AGENT_CACHE_OFF;

   INT16 mode = DCF_GET_CACHE_MODE(m_flags);
   if (mode != AGENT_CACHE_DEFAULT)
      return mode;
   return node->getAgentCacheMode();
}

/**
 * Create DCObject from import file
 */
void DCObject::updateFromImport(ConfigEntry *config)
{
   lock();
   nx_strncpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("unnamed")), MAX_ITEM_NAME);
   nx_strncpy(m_description, config->getSubEntryValue(_T("description"), 0, m_name), MAX_DB_STRING);
   nx_strncpy(m_systemTag, config->getSubEntryValue(_T("systemTag"), 0, _T("")), MAX_DB_STRING);
   m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_iPollingInterval = config->getSubEntryValueAsInt(_T("interval"));
   m_iRetentionTime = config->getSubEntryValueAsInt(_T("retention"));
   m_flags = (UINT16)config->getSubEntryValueAsInt(_T("flags"));
   const TCHAR *perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   safe_free(m_pszPerfTabSettings);
   m_pszPerfTabSettings = _tcsdup_ex(perfTabSettings);
   m_snmpPort = (WORD)config->getSubEntryValueAsInt(_T("snmpPort"));

   setTransformationScript(config->getSubEntryValue(_T("transformation")));

   ConfigEntry *schedules = config->findEntry(_T("schedules"));
   if (schedules != NULL)
      schedules = schedules->findEntry(_T("schedule"));
   if ((schedules != NULL) && (schedules->getValueCount() > 0))
   {
      if (m_schedules != NULL)
         m_schedules->clear();
      else
         m_schedules = new StringList();

      int count = schedules->getValueCount();
      for(int i = 0; i < count; i++)
      {
         m_schedules->add(schedules->getValue(i));
      }
   }
   else
   {
      delete_and_null(m_schedules);
   }

   m_instanceDiscoveryMethod = (WORD)config->getSubEntryValueAsInt(_T("instanceDiscoveryMethod"));
   const TCHAR *value = config->getSubEntryValue(_T("instanceDiscoveryData"));
   safe_free(m_instanceDiscoveryData);
   m_instanceDiscoveryData = _tcsdup_ex(value);
   setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   nx_strncpy(m_instance, config->getSubEntryValue(_T("instance"), 0, _T("")), MAX_DB_STRING);
   m_instanceRetentionTime = config->getSubEntryValueAsInt(_T("instanceRetentionTime"), 0, -1);

   unlock();
}

/**
 * Get owner ID
 */
UINT32 DCObject::getOwnerId() const
{
   return (m_owner != NULL) ? m_owner->getId() : 0;
}

/**
 * Get owner name
 */
const TCHAR *DCObject::getOwnerName() const
{
   return (m_owner != NULL) ? m_owner->getName() : _T("(null)");
}

/**
 * Create NXSL object for this data collection object
 */
NXSL_Value *DCObject::createNXSLObject()
{
   return new NXSL_Value(new NXSL_Object(&g_nxslDciClass, new DCObjectInfo(this)));
}

/**
 * Process force poll request
 */
ClientSession *DCObject::processForcePoll()
{
   lock();
   ClientSession *session = m_pollingSession;
   m_pollingSession = NULL;
   unlock();
   return session;
}

/**
 * Request force poll
 */
void DCObject::requestForcePoll(ClientSession *session)
{
   lock();
   if (m_pollingSession != NULL)
      m_pollingSession->decRefCount();
   m_pollingSession = session;
   m_pollingSession->incRefCount();
   unlock();
}

/**
 * Filter callback data
 */
struct FilterCallbackData
{
   StringMap *filteredInstances;
   DCObject *dco;
   NXSL_VM *instanceFilter;
};

/**
 * Callback for filtering instances
 */
static EnumerationCallbackResult FilterCallback(const TCHAR *key, const void *value, void *data)
{
   NXSL_VM *instanceFilter = ((FilterCallbackData *)data)->instanceFilter;
   DCObject *dco = ((FilterCallbackData *)data)->dco;

   instanceFilter->setGlobalVariable(_T("$object"), dco->getOwner()->createNXSLObject());
   instanceFilter->setGlobalVariable(_T("$targetObject"), dco->getOwner()->createNXSLObject());
   if (dco->getOwner()->getObjectClass() == OBJECT_NODE)
      instanceFilter->setGlobalVariable(_T("$node"), dco->getOwner()->createNXSLObject());
   instanceFilter->setGlobalVariable(_T("$dci"), dco->createNXSLObject());
   if (dco->getSourceNode() != 0)
   {
      Node *sourceNode = (Node *)FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (sourceNode != NULL)
         instanceFilter->setGlobalVariable(_T("$sourceNode"), sourceNode->createNXSLObject());
   }

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
                     DbgPrintf(5, _T("DCObject::filterInstanceList(%s [%d]): instance \"%s\" replaced by \"%s\""),
                              dco->getName(), dco->getId(), instance, newValue);
                     instance = newValue;
                  }

                  if (array->size() > 2)
                  {
                     // instance name
                     const TCHAR *newName = array->get(2)->getValueAsCString();
                     if ((newName != NULL) && (*newName != 0))
                     {
                        DbgPrintf(5, _T("DCObject::filterInstanceList(%s [%d]): instance \"%s\" name set to \"%s\""),
                                 dco->getName(), dco->getId(), instance, newName);
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
         DbgPrintf(5, _T("DCObject::filterInstanceList(%s [%d]): instance \"%s\" removed by filtering script"),
                  dco->getName(), dco->getId(), key);
      }
   }
   else
   {
      TCHAR szBuffer[1024];
      _sntprintf(szBuffer, 1024, _T("DCI::%s::%d::InstanceFilter"), dco->getOwnerName(), dco->getId());
      PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, dco->getId(), "ssd", szBuffer, instanceFilter->getErrorText(), dco->getId());
      ((FilterCallbackData *)data)->filteredInstances->set(key, (const TCHAR *)value);
   }
   return _CONTINUE;
}

/**
 * Filter instance list
 */
void DCObject::filterInstanceList(StringMap *instances)
{
   lock();
   if (m_instanceFilter == NULL)
   {
      unlock();
      return;
   }

   FilterCallbackData data;
   data.instanceFilter = new NXSL_VM(new NXSL_ServerEnv());
   if (!data.instanceFilter->load(m_instanceFilter))
   {
      TCHAR szBuffer[1024];
      _sntprintf(szBuffer, 1024, _T("DCI::%s::%d::InstanceFilter"), getOwnerName(), m_id);
      PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", szBuffer, data.instanceFilter->getErrorText(), m_id);
   }
   unlock();

   StringMap filteredInstances;
   data.filteredInstances = &filteredInstances;
   data.dco = this;
   instances->forEach(FilterCallback, &data);
   instances->clear();
   instances->addAll(&filteredInstances);
   delete data.instanceFilter;
}


/**
 * Set new instance discovery filter script
 */
void DCObject::setInstanceFilter(const TCHAR *pszScript)
{
   free(m_instanceFilterSource);
   delete m_instanceFilter;
   if (pszScript != NULL)
   {
      m_instanceFilterSource = _tcsdup(pszScript);
      StrStrip(m_instanceFilterSource);
      if (m_instanceFilterSource[0] != 0)
      {
         TCHAR errorText[1024];
         m_instanceFilter = NXSLCompile(m_instanceFilterSource, errorText, 1024, NULL);
         if (m_instanceFilter == NULL)
         {
            // node can be NULL if this DCO was just created from template
            // in this case compilation error will be reported on template level anyway
            if (m_owner != NULL)
               nxlog_write(MSG_INSTANCE_FILTER_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dsdss", m_owner->getId(), m_owner->getName(), m_id, m_name, errorText);
         }
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
 * Checks if this DCO object is visible by current user
 */
bool DCObject::hasAccess(UINT32 userId)
{
   if (m_accessList->isEmpty())
      return true;

   if(userId == 0)
      return true;

   for(int i = 0; i < m_accessList->size(); i++)
   {
      UINT32 id = m_accessList->get(i);
      if (id & GROUP_FLAG)
      {
         if (CheckUserMembership(userId, id))
            return true;
      }
      else if (id == userId)
      {
         return true;
      }
   }
   return false;
}

/**
 * Serialize object to JSON
 */
json_t *DCObject::toJson()
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "systemTag", json_string_t(m_systemTag));
   json_object_set_new(root, "lastPoll", json_integer(m_tLastPoll));
   json_object_set_new(root, "pollingInterval", json_integer(m_iPollingInterval));
   json_object_set_new(root, "retentionTime", json_integer(m_iRetentionTime));
   json_object_set_new(root, "source", json_integer(m_source));
   json_object_set_new(root, "status", json_integer(m_status));
   json_object_set_new(root, "busy", json_integer(m_busy));
   json_object_set_new(root, "scheduledForDeletion", json_integer(m_scheduledForDeletion));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "dwTemplateId", json_integer(m_dwTemplateId));
   json_object_set_new(root, "dwTemplateItemId", json_integer(m_dwTemplateItemId));
   json_object_set_new(root, "schedules", (m_schedules != NULL) ? m_schedules->toJson() : json_array());
   json_object_set_new(root, "lastCheck", json_integer(m_tLastCheck));
   json_object_set_new(root, "errorCount", json_integer(m_dwErrorCount));
   json_object_set_new(root, "resourceId", json_integer(m_dwResourceId));
   json_object_set_new(root, "sourceNode", json_integer(m_sourceNode));
   json_object_set_new(root, "snmpPort", json_integer(m_snmpPort));
   json_object_set_new(root, "perfTabSettings", json_string_t(m_pszPerfTabSettings));
   json_object_set_new(root, "transformationScript", json_string_t(m_transformationScriptSource));
   json_object_set_new(root, "comments", json_string_t(m_comments));
   json_object_set_new(root, "instanceDiscoveryMethod", json_integer(m_instanceDiscoveryMethod));
   json_object_set_new(root, "instanceDiscoveryData", json_string_t(m_instanceDiscoveryData));
   json_object_set_new(root, "instanceFilter", json_string_t(m_instanceFilterSource));
   json_object_set_new(root, "instance", json_string_t(m_instance));
   json_object_set_new(root, "accessList", m_accessList->toJson());
   json_object_set_new(root, "instanceRetentionTime", json_integer(m_instanceRetentionTime));
   return root;
}

/**
 * Data collection object info - constructor
 */
DCObjectInfo::DCObjectInfo(DCObject *object)
{
   m_id = object->getId();
   m_ownerId = object->getOwnerId();
   m_templateId = object->getTemplateId();
   m_templateItemId = object->getTemplateItemId();
   m_type = object->getType();
   nx_strncpy(m_name, object->getName(), MAX_ITEM_NAME);
   nx_strncpy(m_description, object->getDescription(), MAX_DB_STRING);
   nx_strncpy(m_systemTag, object->getSystemTag(), MAX_DB_STRING);
   nx_strncpy(m_instance, object->getInstance(), MAX_DB_STRING);
   m_comments = _tcsdup_ex(object->getComments());
   m_dataType = (m_type == DCO_TYPE_ITEM) ? ((DCItem *)object)->getDataType() : -1;
   m_origin = object->getDataSource();
   m_status = object->getStatus();
   m_errorCount = object->getErrorCount();
   m_lastPollTime = object->getLastPollTime();
}

/**
 * Data collection object info - destructor
 */
DCObjectInfo::~DCObjectInfo()
{
   free(m_comments);
}
