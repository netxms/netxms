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
** File: dcobject.cpp
**
**/

#include "nxcore.h"

/**
 * Default retention time for collected data
 */
int __EXPORT DCObject::m_defaultRetentionTime = 30;

/**
 * Default data collection polling interval
 */
int __EXPORT DCObject::m_defaultPollingInterval = 60;

/**
 * Get storage class from retention time
 */
DCObjectStorageClass DCObject::storageClassFromRetentionTime(int retentionTime)
{
   if (retentionTime == 0)
      return DCObjectStorageClass::DEFAULT;
   if (retentionTime <= 7)
      return DCObjectStorageClass::BELOW_7;
   if (retentionTime <= 30)
      return DCObjectStorageClass::BELOW_30;
   if (retentionTime <= 90)
      return DCObjectStorageClass::BELOW_90;
   if (retentionTime <= 180)
      return DCObjectStorageClass::BELOW_180;
   return DCObjectStorageClass::OTHER;
}

/**
 * Get name of storage class
 */
const TCHAR *DCObject::getStorageClassName(DCObjectStorageClass storageClass)
{
   static const TCHAR *names[] = { _T("default"), _T("7"), _T("30"), _T("90"), _T("180"), _T("other") };
   return names[static_cast<int>(storageClass)];
}

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
	m_pollingScheduleType = DC_POLLING_SCHEDULE_DEFAULT;
   m_pollingInterval = 0;
   m_pollingIntervalSrc = NULL;
   m_retentionType = DC_RETENTION_DEFAULT;
   m_retentionTime = 0;
   m_retentionTimeSrc = NULL;
   m_source = DS_INTERNAL;
   m_status = ITEM_STATUS_NOT_SUPPORTED;
   m_lastPoll = 0;
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
   m_lastScriptErrorReport = 0;
   m_comments = NULL;
   m_doForcePoll = false;
   m_pollingSession = NULL;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_relatedObject = 0;
}

/**
 * Create DCObject from another DCObject
 */
DCObject::DCObject(const DCObject *src, bool shadowCopy) :
         m_name(src->m_name), m_description(src->m_description), m_systemTag(src->m_systemTag),
         m_instanceDiscoveryData(src->m_instanceDiscoveryData), m_instance(src->m_instance)
{
   m_id = src->m_id;
   m_guid = src->m_guid;
   m_dwTemplateId = src->m_dwTemplateId;
   m_dwTemplateItemId = src->m_dwTemplateItemId;
   m_busy = shadowCopy ? src->m_busy : 0;
	m_scheduledForDeletion = 0;
	m_pollingScheduleType = src->m_pollingScheduleType;
   m_pollingInterval = src->m_pollingInterval;
   m_pollingIntervalSrc = MemCopyString(src->m_pollingIntervalSrc);
   m_retentionType = src->m_retentionType;
   m_retentionTime = src->m_retentionTime;
   m_retentionTimeSrc = MemCopyString(src->m_retentionTimeSrc);
   m_source = src->m_source;
   m_status = src->m_status;
   m_lastPoll = shadowCopy ? src->m_lastPoll : 0;
   m_owner = src->m_owner;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = shadowCopy ? src->m_tLastCheck : 0;
   m_dwErrorCount = shadowCopy ? src->m_dwErrorCount : 0;
	m_flags = src->m_flags;
	m_dwResourceId = src->m_dwResourceId;
	m_sourceNode = src->m_sourceNode;
	m_pszPerfTabSettings = MemCopyString(src->m_pszPerfTabSettings);
	m_snmpPort = src->m_snmpPort;
	m_comments = MemCopyString(src->m_comments);
	m_doForcePoll = false;
	m_pollingSession = NULL;

   m_transformationScriptSource = NULL;
   m_transformationScript = NULL;
   m_lastScriptErrorReport = 0;
   setTransformationScript(src->m_transformationScriptSource);

   m_schedules = (src->m_schedules != NULL) ? new StringList(src->m_schedules) : NULL;

   m_instanceDiscoveryMethod = src->m_instanceDiscoveryMethod;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   setInstanceFilter(src->m_instanceFilterSource);
   m_accessList = new IntegerArray<UINT32>(src->m_accessList);
   m_instanceRetentionTime = src->m_instanceRetentionTime;
   m_instanceGracePeriodStart = src->m_instanceGracePeriodStart;
   m_startTime = src->m_startTime;
   m_relatedObject = src->m_relatedObject;
}

/**
 * Constructor for creating new DCObject from scratch
 */
DCObject::DCObject(UINT32 id, const TCHAR *name, int source,
               const TCHAR *pollingInterval, const TCHAR *retentionTime, DataCollectionOwner *owner,
               const TCHAR *description, const TCHAR *systemTag) :
         m_name(name), m_description(description), m_systemTag(systemTag),
         m_instanceDiscoveryData(_T("")), m_instance(_T(""))
{
   m_id = id;
   m_guid = uuid::generate();
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   m_source = source;
   m_pollingScheduleType = (pollingInterval != NULL) ? DC_POLLING_SCHEDULE_CUSTOM : DC_POLLING_SCHEDULE_DEFAULT;
   m_pollingIntervalSrc = MemCopyString(pollingInterval);
   m_retentionTime = (retentionTime != NULL) ? DC_RETENTION_CUSTOM : DC_RETENTION_DEFAULT;
   m_retentionTimeSrc = MemCopyString(retentionTime);
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
   m_lastPoll = 0;
   m_owner = owner;
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
   m_lastScriptErrorReport = 0;
   m_comments = NULL;
   m_doForcePoll = false;
   m_pollingSession = NULL;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_relatedObject = 0;

   updateTimeIntervalsInternal();
}

/**
 * Create DCObject from import file
 */
DCObject::DCObject(ConfigEntry *config, DataCollectionOwner *owner)
{
   m_id = CreateUniqueId(IDG_ITEM);
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   m_name = config->getSubEntryValue(_T("name"), 0, _T("unnamed"));
   m_description = config->getSubEntryValue(_T("description"), 0, m_name);
   m_systemTag = config->getSubEntryValue(_T("systemTag"), 0, NULL);
	m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_pollingIntervalSrc = MemCopyString(config->getSubEntryValue(_T("interval"), 0, _T("0")));
   m_pollingScheduleType = !_tcscmp(m_pollingIntervalSrc, _T("0")) ? DC_POLLING_SCHEDULE_DEFAULT : DC_POLLING_SCHEDULE_CUSTOM;
   m_retentionTimeSrc = MemCopyString(config->getSubEntryValue(_T("retention"), 0, _T("0")));
   m_retentionType = !_tcscmp(m_retentionTimeSrc, _T("0")) ? DC_RETENTION_DEFAULT : DC_RETENTION_CUSTOM;
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
	m_scheduledForDeletion = 0;
	m_flags = (UINT16)config->getSubEntryValueAsInt(_T("flags"));
	if (m_flags & 1)  // for compatibility with old format
	   m_pollingScheduleType = DC_POLLING_SCHEDULE_ADVANCED;
   if (m_flags & 0x200) // for compatibility with old format
      m_retentionType = DC_RETENTION_NONE;
   m_lastPoll = 0;
   m_owner = owner;
   m_hMutex = MutexCreateRecursive();
   m_tLastCheck = 0;
   m_dwErrorCount = 0;
	m_dwResourceId = 0;
	m_sourceNode = 0;
   const TCHAR *perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   m_pszPerfTabSettings = MemCopyString(perfTabSettings);
	m_snmpPort = (WORD)config->getSubEntryValueAsInt(_T("snmpPort"));
   m_schedules = NULL;

	m_transformationScriptSource = NULL;
	m_transformationScript = NULL;
   m_lastScriptErrorReport = 0;
   m_comments = MemCopyString(config->getSubEntryValue(_T("comments")));
   m_doForcePoll = false;
   m_pollingSession = NULL;
	setTransformationScript(config->getSubEntryValue(_T("transformation")));

   // for compatibility with old format
	if (config->getSubEntryValueAsInt(_T("advancedSchedule")))
      m_pollingScheduleType = DC_POLLING_SCHEDULE_ADVANCED;

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
   m_instanceDiscoveryData = config->getSubEntryValue(_T("instanceDiscoveryData"));
   m_instanceFilterSource = NULL;
   m_instanceFilter = NULL;
   setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   m_instance = config->getSubEntryValue(_T("instance"));
   m_accessList = new IntegerArray<UINT32>(0, 16);
   m_instanceRetentionTime = config->getSubEntryValueAsInt(_T("instanceRetentionTime"), 0, -1);
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_relatedObject = 0;

   updateTimeIntervalsInternal();
}

/**
 * Destructor
 */
DCObject::~DCObject()
{
   MemFree(m_retentionTimeSrc);
   MemFree(m_pollingIntervalSrc);
   MemFree(m_transformationScriptSource);
   delete m_transformationScript;
   delete m_schedules;
   MemFree(m_pszPerfTabSettings);
   MemFree(m_comments);
   MutexDestroy(m_hMutex);
   MemFree(m_instanceFilterSource);
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
   if (m_pollingScheduleType != DC_POLLING_SCHEDULE_ADVANCED)
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
StringBuffer DCObject::expandMacros(const TCHAR *src, size_t dstLen)
{
	ssize_t index = 0;
	StringBuffer dst = src;
	while((index = dst.find(_T("%{"), index)) != String::npos)
	{
		String head = dst.substring(0, index);
		ssize_t index2 = dst.find(_T("}"), index);
		if (index2 == String::npos)
			break;	// Missing closing }

		String rest = dst.substring(index2 + 1, -1);
		StringBuffer macro = dst.substring(index + 2, index2 - index - 2);
		macro.trim();

		dst = head;
		if (!_tcscmp(macro, _T("node_id")))
		{
			if (m_owner != NULL)
			{
				dst.append(m_owner->getId());
			}
			else
			{
				dst += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_name")))
		{
			if (m_owner != NULL)
			{
				dst += m_owner->getName();
			}
			else
			{
				dst += _T("(error)");
			}
		}
		else if (!_tcscmp(macro, _T("node_primary_ip")))
		{
			if ((m_owner != NULL) && (m_owner->getObjectClass() == OBJECT_NODE))
			{
				TCHAR ipAddr[64];
				dst += static_cast<Node*>(m_owner)->getIpAddress().toString(ipAddr);
			}
			else
			{
				dst += _T("(error)");
			}
		}
		else if (!_tcsncmp(macro, _T("script:"), 7))
		{
			NXSL_VM *vm = CreateServerScriptVM(&macro[7], m_owner, this);
			if (vm != NULL)
			{
				if (vm->run(0, NULL))
				{
					NXSL_Value *result = vm->getResult();
					if (result != NULL)
						dst.append(result->getValueAsCString());
		         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s executed successfully"), m_id, src, &macro[7]);
				}
				else
				{
		         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s execution error: %s"),
					          m_id, src, &macro[7], vm->getErrorText());
					PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", &macro[7], vm->getErrorText(), m_id);
				}
            delete vm;
			}
			else
			{
	         DbgPrintf(4, _T("DCObject::expandMacros(%d,\"%s\"): Cannot find script %s"), m_id, src, &macro[7]);
			}
		}
		dst.append(rest);
	}
	if (dst.length() > dstLen)
	   dst.shrink(dst.length() - dstLen);
	return dst;
}

/**
 * Delete all collected data
 */
bool DCObject::deleteAllData()
{
	return false;
}

/**
 * Delete single DCI entry
 */
bool DCObject::deleteEntry(time_t timestamp)
{
   return false;
}

/**
 * Set new ID and node/template association
 */
void DCObject::changeBinding(UINT32 newId, DataCollectionOwner *newOwner, bool doMacroExpansion)
{
   lock();
   m_owner = newOwner;
	if (newId != 0)
	{
		m_id = newId;
		m_guid = uuid::generate();
	}

	if (doMacroExpansion)
	{
		m_name = expandMacros(m_name, MAX_ITEM_NAME);
		m_description = expandMacros(m_description, MAX_DB_STRING);
		m_instance = expandMacros(m_instance, MAX_ITEM_NAME);
	}

   updateTimeIntervalsInternal();

   unlock();
}

/**
 * Set DCI status
 */
void DCObject::setStatus(int status, bool generateEvent)
{
   if ((m_owner != NULL) && (m_status != (BYTE)status))
   {
      NotifyClientsOnDCIStatusChange(getOwner(), getId(), status);
      if (generateEvent && IsEventSource(m_owner->getObjectClass()))
      {
         static UINT32 eventCode[3] = { EVENT_DCI_ACTIVE, EVENT_DCI_DISABLED, EVENT_DCI_UNSUPPORTED };
         static const TCHAR *originName[11] =
            {
               _T("Internal"), _T("NetXMS Agent"), _T("SNMP"),
               _T("CheckPoint SNMP"), _T("Push"), _T("WinPerf"),
               _T("iLO"), _T("Script"), _T("SSH"), _T("MQTT"),
               _T("Device Driver")
            };
         PostSystemEvent(eventCode[status], m_owner->getId(), "dssds", m_id, m_name.cstr(), m_description.cstr(), m_source, originName[m_source]);
      }
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

            NXSL_VM *vm = CreateServerScriptVM(scriptName, m_owner, this);
            if (vm != NULL)
            {
               if (vm->run(0, NULL))
               {
                  NXSL_Value *result = vm->getResult();
                  if (result != NULL)
                  {
                     const TCHAR *temp = result->getValueAsCString();
                     if (temp != NULL)
                     {
                        DbgPrintf(7, _T("DCObject::matchSchedule(%%[%s]) expanded to \"%s\""), scriptName, temp);
                        _tcslcpy(expandedSchedule, temp, 1024);
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
            DbgPrintf(4, _T("DCObject::matchSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name.cstr());
         }
         free(scriptName);
         if (!success)
            return false;
      }
      else
      {
         DbgPrintf(4, _T("DCObject::matchSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name.cstr());
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

   if (m_doForcePoll && !m_busy)
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
         nxlog_debug(6, _T("Forced poll of DC object %s [%d] on node %s [%d] cancelled"), m_name.cstr(), m_id, m_owner->getName(), m_owner->getId());
         if (m_pollingSession != NULL)
         {
            m_pollingSession->decRefCount();
            m_pollingSession = NULL;
         }
         m_doForcePoll = false;
         unlock();
         return false;
      }
   }

   bool result;
   if ((m_status != ITEM_STATUS_DISABLED) && (!m_busy) &&
       isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
       matchClusterResource() && hasValue() && (getAgentCacheMode() == AGENT_CACHE_OFF))
   {
      if (m_pollingScheduleType == DC_POLLING_SCHEDULE_ADVANCED)
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
		      result = ((m_lastPoll + getEffectivePollingInterval() * 10 <= currTime) && (m_startTime <= currTime));
			else
		      result = ((m_lastPoll + getEffectivePollingInterval() <= currTime) && (m_startTime <= currTime));
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
   pMsg->setField(VID_POLLING_SCHEDULE_TYPE, static_cast<INT16>(m_pollingScheduleType));
   pMsg->setField(VID_POLLING_INTERVAL, m_pollingIntervalSrc);
   pMsg->setField(VID_RETENTION_TYPE, static_cast<INT16>(m_retentionType));
   pMsg->setField(VID_RETENTION_TIME, m_retentionTimeSrc);
   pMsg->setField(VID_DCI_SOURCE_TYPE, (WORD)m_source);
   pMsg->setField(VID_DCI_STATUS, (WORD)m_status);
	pMsg->setField(VID_RESOURCE_ID, m_dwResourceId);
	pMsg->setField(VID_AGENT_PROXY, m_sourceNode);
	pMsg->setField(VID_SNMP_PORT, m_snmpPort);
   pMsg->setField(VID_COMMENTS, m_comments);
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
   pMsg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   pMsg->setField(VID_INSTD_FILTER, m_instanceFilterSource);
   pMsg->setField(VID_INSTANCE, m_instance);
   pMsg->setFieldFromInt32Array(VID_ACL, m_accessList);
   pMsg->setField(VID_INSTANCE_RETENTION, m_instanceRetentionTime);
   pMsg->setField(VID_RELATED_OBJECT, m_relatedObject);
   unlock();
}

/**
 * Update data collection object from NXCP message
 */
void DCObject::updateFromMessage(NXCPMessage *pMsg)
{
   lock();

   m_name = pMsg->getFieldAsSharedString(VID_NAME, MAX_ITEM_NAME);
   m_description = pMsg->getFieldAsSharedString(VID_DESCRIPTION, MAX_DB_STRING);
   m_systemTag = pMsg->getFieldAsSharedString(VID_SYSTEM_TAG, MAX_DB_STRING);
	m_flags = pMsg->getFieldAsUInt16(VID_FLAGS);
   m_source = (BYTE)pMsg->getFieldAsUInt16(VID_DCI_SOURCE_TYPE);
   setStatus(pMsg->getFieldAsUInt16(VID_DCI_STATUS), true);
	m_dwResourceId = pMsg->getFieldAsUInt32(VID_RESOURCE_ID);
	m_sourceNode = pMsg->getFieldAsUInt32(VID_AGENT_PROXY);
   m_snmpPort = pMsg->getFieldAsUInt16(VID_SNMP_PORT);
	pMsg->getFieldAsString(VID_PERFTAB_SETTINGS, &m_pszPerfTabSettings);
   pMsg->getFieldAsString(VID_COMMENTS, &m_comments);

   m_pollingScheduleType = static_cast<BYTE>(pMsg->getFieldAsUInt16(VID_POLLING_SCHEDULE_TYPE));
   if (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM)
      pMsg->getFieldAsString(VID_POLLING_INTERVAL, &m_pollingIntervalSrc);
   else
      MemFreeAndNull(m_pollingIntervalSrc);
   m_retentionType = static_cast<BYTE>(pMsg->getFieldAsUInt16(VID_RETENTION_TYPE));
   if (m_retentionType == DC_POLLING_SCHEDULE_CUSTOM)
      pMsg->getFieldAsString(VID_RETENTION_TIME, &m_retentionTimeSrc);
   else
      MemFreeAndNull(m_retentionTimeSrc);
   updateTimeIntervalsInternal();

   TCHAR *pszStr = pMsg->getFieldAsString(VID_TRANSFORMATION_SCRIPT);
   setTransformationScript(pszStr);
   MemFree(pszStr);

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
   m_instanceDiscoveryData = pMsg->getFieldAsSharedString(VID_INSTD_DATA, MAX_INSTANCE_LEN);

   pszStr = pMsg->getFieldAsString(VID_INSTD_FILTER);
   setInstanceFilter(pszStr);
   MemFree(pszStr);

   m_instance = pMsg->getFieldAsSharedString(VID_INSTANCE, MAX_INSTANCE_LEN);

   m_accessList->clear();
   pMsg->getFieldAsInt32Array(VID_ACL, m_accessList);

   m_instanceRetentionTime = pMsg->getFieldAsInt32(VID_INSTANCE_RETENTION);
   m_relatedObject = pMsg->getFieldAsUInt32(VID_RELATED_OBJECT);

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
	if (success)
	{
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE dci_id=?"));
	}
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
 * Terminate alarms related to DCI
 */
static void TerminateRelatedAlarms(void *arg)
{
   ResolveAlarmByDCObjectId(CAST_FROM_POINTER(arg, UINT32), true);
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

   if (ConfigReadBoolean(_T("DataCollection.OnDCIDelete.TerminateRelatedAlarms"), true))
      ThreadPoolExecuteSerialized(g_mainThreadPool, _T("TerminateDataCollectionAlarms"), TerminateRelatedAlarms, CAST_TO_POINTER(m_id, void*));
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
   StringBuffer temp = m_name;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instance);
   m_name = temp;

   temp = m_description;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instance);
   m_description = temp;
}

/**
 * Update DC object from template object
 */
void DCObject::updateFromTemplate(DCObject *src)
{
	lock();

	m_name = expandMacros(src->m_name, MAX_ITEM_NAME);
	m_description = expandMacros(src->m_description, MAX_DB_STRING);
	m_systemTag = expandMacros(src->m_systemTag, MAX_DB_STRING);

	m_pollingScheduleType = src->m_pollingScheduleType;
	MemFree(m_pollingIntervalSrc);
   m_pollingIntervalSrc = MemCopyString(src->m_pollingIntervalSrc);
   m_retentionType = src->m_retentionType;
   MemFree(m_retentionTimeSrc);
   m_retentionTimeSrc = MemCopyString(src->m_retentionTimeSrc);
   updateTimeIntervalsInternal();

   m_source = src->m_source;
	m_flags = src->m_flags;
	m_sourceNode = src->m_sourceNode;
	m_dwResourceId = src->m_dwResourceId;
	m_snmpPort = src->m_snmpPort;

	MemFree(m_pszPerfTabSettings);
	m_pszPerfTabSettings = MemCopyString(src->m_pszPerfTabSettings);

   setTransformationScript(src->m_transformationScriptSource);

   delete m_accessList;
   m_accessList = new IntegerArray<UINT32>(src->m_accessList);

   // Copy schedules
   delete m_schedules;
   m_schedules = (src->m_schedules != NULL) ? new StringList(src->m_schedules) : NULL;

   if ((src->getInstanceDiscoveryMethod() != IDM_NONE) && (m_instanceDiscoveryMethod == IDM_NONE))
   {
      expandInstance();
   }
   else
   {
      m_instance = expandMacros(src->m_instance, MAX_ITEM_NAME);
      m_instanceDiscoveryMethod = src->m_instanceDiscoveryMethod;
      m_instanceDiscoveryData = src->m_instanceDiscoveryData;
      MemFreeAndNull(m_instanceFilterSource);
      delete_and_null(m_instanceFilter);
      setInstanceFilter(src->m_instanceFilterSource);
   }
   m_instanceRetentionTime = src->m_instanceRetentionTime;

   if (((m_status != ITEM_STATUS_DISABLED) || (g_flags & AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE)) &&
       !((m_owner->getId() == m_dwTemplateId) && (m_instanceGracePeriodStart > 0))) // check if DCI is not disabled by instance discovery
   {
      setStatus(src->m_status, true);
   }

   unlock();
}

/**
 * Process new collected value. Should return true on success.
 * If returns false, current poll result will be converted into data collection error.
 *
 * @return true on success
 */
bool DCObject::processNewValue(time_t nTimeStamp, void *value, bool *updateStatus)
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
 * Save information about threshold state before maintenance
 */
void DCObject::updateThresholdsBeforeMaintenanceState()
{
}

/**
 * Generate events that persist after maintenance
 */
void DCObject::generateEventsBasedOnThrDiff()
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
            nxlog_write(NXLOG_WARNING, _T("Failed to compile transformation script for object %s [%u] DCI %s [%u] (%s)"),
                     getOwnerName(), getOwnerId(), m_name.cstr(), m_id, errorText);
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
   m_lastScriptErrorReport = 0;  // allow immediate error report after script change
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
 * Update data collection object from import file
 */
void DCObject::updateFromImport(ConfigEntry *config)
{
   lock();

   m_name = config->getSubEntryValue(_T("name"), 0, _T("unnamed"));
   m_description = config->getSubEntryValue(_T("description"), 0, m_name);
   m_systemTag = config->getSubEntryValue(_T("systemTag"), 0, NULL);
   m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_flags = (UINT16)config->getSubEntryValueAsInt(_T("flags"));
   const TCHAR *perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   MemFree(m_pszPerfTabSettings);
   m_pszPerfTabSettings = MemCopyString(perfTabSettings);
   m_snmpPort = (WORD)config->getSubEntryValueAsInt(_T("snmpPort"));

   MemFree(m_pollingIntervalSrc);
   MemFree(m_retentionTimeSrc);
   m_pollingIntervalSrc = MemCopyString(config->getSubEntryValue(_T("interval"), 0, _T("0")));
   m_retentionTimeSrc = MemCopyString(config->getSubEntryValue(_T("retention"), 0, _T("0")));
   updateTimeIntervalsInternal();

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
   m_instanceDiscoveryData = config->getSubEntryValue(_T("instanceDiscoveryData"));
   setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   m_instance = config->getSubEntryValue(_T("instance"));
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
NXSL_Value *DCObject::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslDciClass, createDescriptor()));
}

/**
 * Process force poll request
 */
ClientSession *DCObject::processForcePoll()
{
   lock();
   ClientSession *session = m_pollingSession;
   m_pollingSession = NULL;
   m_doForcePoll = false;
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
   if (m_pollingSession != NULL)
      m_pollingSession->incRefCount();
   m_doForcePoll = true;
   unlock();
}

/**
 * Filter callback data
 */
struct FilterCallbackData
{
   StringObjectMap<InstanceDiscoveryData> *filteredInstances;
   DCObject *dco;
   NXSL_VM *instanceFilter;
};

/**
 * Callback for filtering instances
 */
static EnumerationCallbackResult FilterCallback(const TCHAR *key, const TCHAR *value, FilterCallbackData *data)
{
   if (g_flags & AF_SHUTDOWN)
      return _STOP;

   NXSL_VM *instanceFilter = data->instanceFilter;
   DCObject *dco = data->dco;
   SharedString dcObjectName = dco->getName();

   SetupServerScriptVM(instanceFilter, dco->getOwner(), dco);
   instanceFilter->setGlobalVariable("$targetObject", dco->getOwner()->createNXSLObject(instanceFilter));
   if (dco->getSourceNode() != 0)
   {
      Node *sourceNode = (Node *)FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (sourceNode != NULL)
         instanceFilter->setGlobalVariable("$sourceNode", sourceNode->createNXSLObject(instanceFilter));
   }

   NXSL_Value *argv[2];
   argv[0] = instanceFilter->createValue(key);
   argv[1] = instanceFilter->createValue((const TCHAR *)value);

   if (instanceFilter->run(2, argv))
   {
      bool accepted;
      const TCHAR *instance = key;
      const TCHAR *name = (const TCHAR *)value;
      UINT32 relatedObject = 0;
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
                              dcObjectName.cstr(), dco->getId(), instance, newValue);
                     instance = newValue;
                  }
               }
               if (array->size() > 2)
               {
                  // instance name
                  const TCHAR *newName = array->get(2)->getValueAsCString();
                  if ((newName != NULL) && (*newName != 0))
                  {
                     DbgPrintf(5, _T("DCObject::filterInstanceList(%s [%d]): instance \"%s\" name set to \"%s\""),
                              dcObjectName.cstr(), dco->getId(), instance, newName);
                     name = newName;
                  }


               }
               if (array->size() > 3 && array->get(3)->isObject(_T("NetObj")))
               {
                  relatedObject = ((NetObj *)array->get(3)->getValueAsObject()->getData())->getId();
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
         data->filteredInstances->set(instance, new InstanceDiscoveryData(name, relatedObject));
      }
      else
      {
         DbgPrintf(5, _T("DCObject::filterInstanceList(%s [%d]): instance \"%s\" removed by filtering script"),
                  dcObjectName.cstr(), dco->getId(), key);
      }
   }
   else
   {
      TCHAR szBuffer[1024];
      _sntprintf(szBuffer, 1024, _T("DCI::%s::%d::InstanceFilter"), dco->getOwnerName(), dco->getId());
      PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, dco->getId(), "ssd", szBuffer, instanceFilter->getErrorText(), dco->getId());
      data->filteredInstances->set(key, new InstanceDiscoveryData((const TCHAR *)value, 0));
   }
   return _CONTINUE;
}

/**
 * Copy instance discovery data elements
 */
static EnumerationCallbackResult CopyElements(const TCHAR *key, const TCHAR *value, StringObjectMap<InstanceDiscoveryData> *map)
{
   map->set(key, new InstanceDiscoveryData(value, 0));
   return _CONTINUE;
}

/**
 * Filter instance list
 */
StringObjectMap<InstanceDiscoveryData> *DCObject::filterInstanceList(StringMap *instances)
{
   auto filteredInstances = new StringObjectMap<InstanceDiscoveryData>(true);

   lock();
   if (m_instanceFilter == NULL)
   {
      unlock();
      instances->forEach(CopyElements, filteredInstances);
      return filteredInstances;
   }

   FilterCallbackData data;
   data.instanceFilter = new NXSL_VM(new NXSL_ServerEnv());
   if (!data.instanceFilter->load(m_instanceFilter))
   {
      TCHAR scriptIdentification[1024];
      _sntprintf(scriptIdentification, 1024, _T("DCI::%s::%d::InstanceFilter"), getOwnerName(), m_id);
      PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", scriptIdentification, data.instanceFilter->getErrorText(), m_id);
   }
   unlock();

   data.filteredInstances = filteredInstances;
   data.dco = this;
   instances->forEach(FilterCallback, &data);
   delete data.instanceFilter;
   return filteredInstances;
}

/**
 * Set new instance discovery filter script
 */
void DCObject::setInstanceFilter(const TCHAR *pszScript)
{
   MemFree(m_instanceFilterSource);
   delete m_instanceFilter;
   if (pszScript != NULL)
   {
      m_instanceFilterSource = MemCopyString(pszScript);
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
            {
               nxlog_write(NXLOG_WARNING, _T("Failed to compile instance filter script for object %s [%u] DCI %s [%u] (%s)"),
                        getOwnerName(), getOwnerId(), m_name.cstr(), m_id, errorText);

               TCHAR scriptIdentification[1024];
               _sntprintf(scriptIdentification, 1024, _T("DCI::%s::%d::InstanceFilter"), getOwnerName(), m_id);
               PostDciEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, m_id, "ssd", scriptIdentification, errorText, m_id);
            }
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
 * Update time intervals (polling and retention) from sources
 */
void DCObject::updateTimeIntervalsInternal()
{
   if ((m_retentionTimeSrc != NULL) && (*m_retentionTimeSrc != 0))
   {
      StringBuffer exp = m_owner->expandText(m_retentionTimeSrc, NULL, NULL, NULL, NULL, NULL);
      m_retentionTime = _tcstol(exp, NULL, 10);
   }
   else
   {
      m_retentionTime = 0;
   }

   if ((m_pollingIntervalSrc != NULL) && (*m_pollingIntervalSrc != 0))
   {
      StringBuffer exp = m_owner->expandText(m_pollingIntervalSrc, NULL, NULL, NULL, NULL, NULL);
      m_pollingInterval = _tcstol(exp, NULL, 10);
   }
   else
   {
      m_pollingInterval = 0;
   }

   nxlog_debug(6, _T("DCObject::updateTimeIntervalsInternal(%s [%d]): retentionTime=%d, pollingInterval=%d"), m_name.cstr(), m_id, m_retentionTime, m_pollingInterval);
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
   lock();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "systemTag", json_string_t(m_systemTag));
   json_object_set_new(root, "lastPoll", json_integer(m_lastPoll));
   json_object_set_new(root, "pollingInterval", json_string_t(m_pollingIntervalSrc));
   json_object_set_new(root, "retentionTime", json_string_t(m_retentionTimeSrc));
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
   unlock();
   return root;
}

/**
 * Create descriptor for this object
 */
DCObjectInfo *DCObject::createDescriptor() const
{
   lock();
   DCObjectInfo *info = new DCObjectInfo(this);
   unlock();
   return info;
}

/**
 * Data collection object info - constructor
 */
DCObjectInfo::DCObjectInfo(const DCObject *object)
{
   m_id = object->m_id;
   m_ownerId = object->getOwnerId();
   m_templateId = object->m_dwTemplateId;
   m_templateItemId = object->m_dwTemplateItemId;
   m_type = object->getType();
   _tcslcpy(m_name, object->m_name, MAX_ITEM_NAME);
   _tcslcpy(m_description, object->m_description, MAX_DB_STRING);
   _tcslcpy(m_systemTag, object->m_systemTag, MAX_DB_STRING);
   _tcslcpy(m_instance, object->m_instance, MAX_DB_STRING);
   m_instanceData = MemCopyString(object->m_instanceDiscoveryData);
   m_comments = MemCopyString(object->m_comments);
   m_dataType = (m_type == DCO_TYPE_ITEM) ? static_cast<const DCItem*>(object)->m_dataType : -1;
   m_origin = object->m_source;
   m_status = object->m_status;
   m_errorCount = object->m_dwErrorCount;
   m_pollingInterval = object->getEffectivePollingInterval();
   m_lastPollTime = object->m_lastPoll;
   m_hasActiveThreshold = false;
   m_thresholdSeverity = SEVERITY_NORMAL;
   m_relatedObject = object->getRelatedObject();
}

/**
 * Data collection object info - constructor for client provided DCI info
 */
DCObjectInfo::DCObjectInfo(const NXCPMessage *msg, const DCObject *object)
{
   m_id = msg->getFieldAsUInt32(VID_DCI_ID);
   m_ownerId = msg->getFieldAsUInt32(VID_OBJECT_ID);
   m_templateId = (object != NULL) ? object->getTemplateId() : 0;
   m_templateItemId = (object != NULL) ? object->getTemplateItemId() : 0;
   m_type = msg->getFieldAsInt16(VID_DCOBJECT_TYPE);
   msg->getFieldAsString(VID_NAME, m_name, MAX_ITEM_NAME);
   msg->getFieldAsString(VID_DESCRIPTION, m_description, MAX_DB_STRING);
   msg->getFieldAsString(VID_SYSTEM_TAG, m_systemTag, MAX_DB_STRING);
   msg->getFieldAsString(VID_INSTANCE, m_instance, MAX_DB_STRING);
   m_instanceData = msg->getFieldAsString(VID_INSTD_DATA);
   m_comments = msg->getFieldAsString(VID_COMMENTS);
   m_dataType = (m_type == DCO_TYPE_ITEM) ? msg->getFieldAsInt16(VID_DCI_DATA_TYPE) : -1;
   m_origin = msg->getFieldAsInt16(VID_DCI_SOURCE_TYPE);
   m_status = msg->getFieldAsInt16(VID_DCI_STATUS);
   m_pollingInterval = (object != NULL) ? object->getEffectivePollingInterval() : 0;
   m_errorCount = (object != NULL) ? object->getErrorCount() : 0;
   m_lastPollTime = (object != NULL) ? object->getLastPollTime() : 0;
   m_hasActiveThreshold = false;
   m_thresholdSeverity = SEVERITY_NORMAL;
   m_relatedObject = (object != NULL) ? object->getRelatedObject() : 0;
}

/**
 * Data collection object info - destructor
 */
DCObjectInfo::~DCObjectInfo()
{
   MemFree(m_instanceData);
   MemFree(m_comments);
}
