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
** File: dcobject.cpp
**
**/

#include "nxcore.h"
#include <nms_users.h>

#define DEBUG_TAG_DC_CONFIG      _T("dc.config")
#define DEBUG_TAG_DC_SCHEDULER   _T("dc.scheduler")

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
DCObject::DCObject(const shared_ptr<DataCollectionOwner>& owner) : m_owner(owner), m_mutex(MutexType::RECURSIVE), m_accessList(0, 16)
{
   m_id = 0;
   m_guid = uuid::generate();
   m_ownerId = (owner != nullptr) ? owner->getId() : 0;
   m_templateId = 0;
   m_templateItemId = 0;
   m_busy = 0;
	m_scheduledForDeletion = 0;
	m_pollingScheduleType = DC_POLLING_SCHEDULE_DEFAULT;
   m_pollingInterval = 0;
   m_retentionType = DC_RETENTION_DEFAULT;
   m_retentionTime = 0;
   m_source = DS_INTERNAL;
   m_status = ITEM_STATUS_NOT_SUPPORTED;
   m_lastPollTime = Timestamp::fromMilliseconds(0);
   m_lastValueTimestamp = Timestamp::fromMilliseconds(0);
   m_nextPollTime = 0;
   m_schedules = nullptr;
   m_tLastCheck = 0;
	m_flags = 0;
   m_stateFlags = 0;
   m_errorCount = 0;
	m_resourceId = 0;
	m_sourceNode = 0;
	m_snmpPort = 0;	// use default
	m_snmpVersion = SNMP_VERSION_DEFAULT;
   m_lastScriptErrorReport = 0;
   m_doForcePoll = false;
   m_pollingSession = nullptr;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_thresholdDisableEndTime = 0;
   m_relatedObject = 0;
}

/**
 * Create DCObject from another DCObject
 */
DCObject::DCObject(const DCObject *src, bool shadowCopy) :
         m_owner(src->m_owner), m_name(src->m_name), m_description(src->m_description), m_systemTag(src->m_systemTag),
         m_userTag(src->m_userTag), m_pollingIntervalSrc(src->m_pollingIntervalSrc), m_retentionTimeSrc(src->m_retentionTimeSrc),
         m_mutex(MutexType::RECURSIVE), m_perfTabSettings(src->m_perfTabSettings),
         m_transformationScriptSource(src->m_transformationScriptSource), m_transformationScript(src->m_transformationScript),
         m_comments(src->m_comments), m_instanceDiscoveryData(src->m_instanceDiscoveryData),
         m_instanceFilterSource(src->m_instanceFilterSource), m_instanceFilter(src->m_instanceFilter),
         m_instanceName(src->m_instanceName), m_accessList(src->m_accessList)
{
   m_id = src->m_id;
   m_guid = src->m_guid;
   m_ownerId = src->m_ownerId;
   m_templateId = src->m_templateId;
   m_templateItemId = src->m_templateItemId;
   m_busy = shadowCopy ? src->m_busy : 0;
	m_scheduledForDeletion = 0;
	m_pollingScheduleType = src->m_pollingScheduleType;
   m_pollingInterval = src->m_pollingInterval;
   m_retentionType = src->m_retentionType;
   m_retentionTime = src->m_retentionTime;
   m_source = src->m_source;
   m_status = src->m_status;
   m_lastPollTime = shadowCopy ? src->m_lastPollTime : Timestamp::fromMilliseconds(0);
   m_lastValueTimestamp = shadowCopy ? src->m_lastValueTimestamp : Timestamp::fromMilliseconds(0);
   m_nextPollTime = shadowCopy ? src->m_nextPollTime : 0;
   m_tLastCheck = shadowCopy ? src->m_tLastCheck : 0;
   m_errorCount = shadowCopy ? src->m_errorCount : 0;
	m_flags = src->m_flags;
   m_stateFlags = src->m_stateFlags;
	m_resourceId = src->m_resourceId;
	m_sourceNode = src->m_sourceNode;
	m_snmpPort = src->m_snmpPort;
   m_snmpVersion = src->m_snmpVersion;
	m_doForcePoll = false;
	m_pollingSession = nullptr;
   m_lastScriptErrorReport = 0;
   m_schedules = (src->m_schedules != nullptr) ? new StringList(src->m_schedules) : nullptr;
   m_instanceDiscoveryMethod = src->m_instanceDiscoveryMethod;
   m_instanceRetentionTime = src->m_instanceRetentionTime;
   m_instanceGracePeriodStart = src->m_instanceGracePeriodStart;
   m_startTime = src->m_startTime;
   m_thresholdDisableEndTime = src->m_thresholdDisableEndTime;
   m_relatedObject = src->m_relatedObject;
}

/**
 * Constructor for creating new DCObject from scratch
 */
DCObject::DCObject(uint32_t id, const TCHAR *name, int source, BYTE scheduleType, const TCHAR *pollingInterval,
         BYTE retentionType, const TCHAR *retentionTime, const shared_ptr<DataCollectionOwner>& owner,
         const TCHAR *description, const TCHAR *systemTag) :
         m_owner(owner), m_name(name), m_description(description), m_systemTag(systemTag),
         m_pollingIntervalSrc(pollingInterval), m_retentionTimeSrc(retentionTime), m_mutex(MutexType::RECURSIVE), m_accessList(0, 16)
{
   m_id = id;
   m_guid = uuid::generate();
   m_ownerId = (owner != nullptr) ? owner->getId() : 0;
   m_templateId = 0;
   m_templateItemId = 0;
   m_source = source;
   m_pollingScheduleType = scheduleType;
   m_retentionType = retentionType;
   m_status = ITEM_STATUS_ACTIVE;
   m_busy = 0;
   m_scheduledForDeletion = 0;
   m_lastPollTime = Timestamp::fromMilliseconds(0);
   m_lastValueTimestamp = Timestamp::fromMilliseconds(0);
   m_nextPollTime = 0;
   m_flags = 0;
   m_stateFlags = 0;
   m_schedules = nullptr;
   m_tLastCheck = 0;
   m_errorCount = 0;
   m_resourceId = 0;
   m_sourceNode = 0;
   m_snmpPort = 0;	// use default
   m_snmpVersion = SNMP_VERSION_DEFAULT;
   m_lastScriptErrorReport = 0;
   m_doForcePoll = false;
   m_pollingSession = nullptr;
   m_instanceDiscoveryMethod = IDM_NONE;
   m_instanceRetentionTime = -1;
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_thresholdDisableEndTime = 0;
   m_relatedObject = 0;

   updateTimeIntervalsInternal();
}

/**
 * Create DCObject from import file
 */
DCObject::DCObject(ConfigEntry *config, const shared_ptr<DataCollectionOwner>& owner, bool nxslV5) : m_owner(owner), m_mutex(MutexType::RECURSIVE), m_accessList(0, 16)
{
   m_id = CreateUniqueId(IDG_ITEM);
   m_guid = config->getSubEntryValueAsUUID(_T("guid"));
   if (m_guid.isNull())
      m_guid = uuid::generate();
   m_ownerId = (owner != nullptr) ? owner->getId() : 0;
   m_templateId = 0;
   m_templateItemId = 0;
   m_name = config->getSubEntryValue(_T("name"), 0, _T("unnamed"));
   m_description = config->getSubEntryValue(_T("description"), 0, m_name);
   m_systemTag = config->getSubEntryValue(_T("systemTag"), 0, nullptr);
   m_userTag = config->getSubEntryValue(_T("userTag"), 0, nullptr);
   m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_flags = config->getSubEntryValueAsInt(_T("flags"));
   m_pollingIntervalSrc = config->getSubEntryValue(_T("interval"), 0, nullptr);
   if (config->findEntry(_T("scheduleType")) != nullptr)
   {
      m_pollingScheduleType = config->getSubEntryValueAsInt(_T("scheduleType"));
   }
   else
   {
      m_pollingScheduleType = (m_pollingIntervalSrc.isEmpty() || !_tcscmp(m_pollingIntervalSrc, _T("0"))) ? DC_POLLING_SCHEDULE_DEFAULT : DC_POLLING_SCHEDULE_CUSTOM;
      if (m_flags & 1)  // for compatibility with old format
         m_pollingScheduleType = DC_POLLING_SCHEDULE_ADVANCED;
   }
   m_retentionTimeSrc = config->getSubEntryValue(_T("retention"), 0, nullptr);
   if (config->findEntry(_T("retentionType")) != nullptr)
   {
      m_retentionType = config->getSubEntryValueAsInt(_T("retentionType"));
   }
   else
   {
      m_retentionType = (m_retentionTimeSrc.isEmpty() || !_tcscmp(m_retentionTimeSrc, _T("0"))) ? DC_RETENTION_DEFAULT : DC_RETENTION_CUSTOM;
      if (m_flags & 0x200) // for compatibility with old format
         m_retentionType = DC_RETENTION_NONE;
   }
   m_status = config->getSubEntryValueAsBoolean(_T("isDisabled")) ? ITEM_STATUS_DISABLED : ITEM_STATUS_ACTIVE;
   m_busy = 0;
   m_scheduledForDeletion = 0;
   m_lastPollTime = Timestamp::fromMilliseconds(0);
   m_lastValueTimestamp = Timestamp::fromMilliseconds(0);
   m_nextPollTime = 0;
   m_tLastCheck = 0;
   m_errorCount = 0;
   m_resourceId = 0;
   m_sourceNode = 0;
   m_perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   m_snmpPort = static_cast<uint16_t>(config->getSubEntryValueAsInt(_T("snmpPort")));
   m_snmpVersion = static_cast<SNMP_Version>(config->getSubEntryValueAsInt(_T("snmpVersion"), 0, SNMP_VERSION_DEFAULT));
   m_schedules = nullptr;
   m_lastScriptErrorReport = 0;
   m_comments = config->getSubEntryValue(_T("comments"));
   m_doForcePoll = false;
   m_pollingSession = nullptr;
   if (nxslV5)
   {
      setTransformationScript(config->getSubEntryValue(_T("transformation")));
   }
   else
   {
      setTransformationScript(NXSLConvertToV5(config->getSubEntryValue(_T("transformation"), 0 , _T(""))));
   }

   // for compatibility with old format
   if (config->getSubEntryValueAsInt(_T("advancedSchedule")))
      m_pollingScheduleType = DC_POLLING_SCHEDULE_ADVANCED;

   ConfigEntry *schedules = config->findEntry(_T("schedules"));
   if (schedules != nullptr)
      schedules = schedules->findEntry(_T("schedule"));
   if ((schedules != nullptr) && (schedules->getValueCount() > 0))
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
   if (nxslV5)
   {
      setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   }
   else
   {
      setInstanceFilter(NXSLConvertToV5(config->getSubEntryValue(_T("instanceFilter"), 0, _T(""))));
   }
   m_instanceName = config->getSubEntryValue(_T("instance"));
   m_instanceRetentionTime = config->getSubEntryValueAsInt(_T("instanceRetentionTime"), 0, -1);
   m_instanceGracePeriodStart = 0;
   m_startTime = 0;
   m_thresholdDisableEndTime = 0;
   m_relatedObject = 0;

   updateTimeIntervalsInternal();
}

/**
 * Destructor
 */
DCObject::~DCObject()
{
   delete m_schedules;
}

/**
 * Load access list
 */
bool DCObject::loadAccessList(DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
{
   m_accessList.clear();

   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DCI_ACL, _T("SELECT user_id FROM dci_access WHERE dci_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      m_accessList.add(DBGetFieldULong(hResult, i, 0));
   }
   DBFreeResult(hResult);

   return true;
}

/**
 * Load custom schedules from database
 * (assumes that no schedules was created before this call)
 */
bool DCObject::loadCustomSchedules(DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
{
   if (m_pollingScheduleType != DC_POLLING_SCHEDULE_ADVANCED)
		return true;

   DB_STATEMENT hStmt = PrepareObjectLoadStatement(hdb, preparedStatements, LSI_DCI_SCHEDULES, _T("SELECT schedule FROM dci_schedules WHERE item_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      m_schedules = new StringList();
      for(int i = 0; i < count; i++)
      {
         m_schedules->addPreallocated(DBGetField(hResult, i, 0, nullptr, 0));
      }
   }
   DBFreeResult(hResult);

	return true;
}

/**
 * Check if associated cluster resource is active. Returns true also if
 * DCI has no resource association
 */
bool DCObject::matchClusterResource()
{
   auto owner = m_owner.lock();
   if ((m_resourceId == 0) || (owner->getObjectClass() != OBJECT_NODE))
		return true;

	shared_ptr<Cluster> cluster = static_cast<Node&>(*owner).getCluster();
	if (cluster == nullptr)
		return false;	// Has association, but cluster object cannot be found

	return cluster->isResourceOnNode(m_resourceId, m_ownerId);
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
			if (m_ownerId != 0)
			{
				dst.append(m_ownerId);
			}
			else
			{
            dst.append(_T("(error)"));
			}
		}
		else if (!_tcscmp(macro, _T("node_name")))
		{
         auto owner = m_owner.lock();
			if (owner != nullptr)
			{
				dst.append(owner->getName());
			}
			else
			{
				dst.append(_T("(error)"));
			}
		}
		else if (!_tcscmp(macro, _T("node_primary_ip")))
		{
		   auto owner = m_owner.lock();
			if ((owner != nullptr) && (owner->getObjectClass() == OBJECT_NODE))
			{
				TCHAR ipAddr[64];
				dst.append(static_cast<Node*>(owner.get())->getIpAddress().toString(ipAddr));
			}
			else
			{
            dst.append(_T("(error)"));
			}
		}
		else if (!_tcsncmp(macro, _T("script:"), 7))
		{
			NXSL_VM *vm = CreateServerScriptVM(&macro[7], m_owner.lock(), createDescriptorInternal());
			if (vm != nullptr)
			{
				if (vm->run(0, nullptr))
				{
					NXSL_Value *result = vm->getResult();
					if (result != nullptr)
						dst.append(result->getValueAsCString());
		         nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s executed successfully"), m_id, src, &macro[7]);
				}
				else
				{
				   nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 4, _T("DCObject::expandMacros(%d,\"%s\"): Script %s execution error: %s"), m_id, src, &macro[7], vm->getErrorText());
				   ReportScriptError(SCRIPT_CONTEXT_DCI, FindObjectById(m_ownerId).get(), m_id, vm->getErrorText(), &macro[7]);
				}
            delete vm;
			}
			else
			{
			   nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 4, _T("DCObject::expandMacros(%d,\"%s\"): Cannot find script %s"), m_id, src, &macro[7]);
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
bool DCObject::deleteEntry(Timestamp timestamp)
{
   return false;
}

/**
 * Set new ID and node/template association
 */
void DCObject::changeBinding(uint32_t newId, shared_ptr<DataCollectionOwner> newOwner, bool doMacroExpansion)
{
   lock();
   m_owner = newOwner;
   m_ownerId = (newOwner != nullptr) ? newOwner->getId() : 0;
	if (newId != 0)
	{
		m_id = newId;
		m_guid = uuid::generate();
	}

	if (doMacroExpansion)
	{
		m_name = expandMacros(m_name, MAX_ITEM_NAME);
		m_description = expandMacros(m_description, MAX_DB_STRING);
		m_instanceName = expandMacros(m_instanceName, MAX_ITEM_NAME);
      m_instanceDiscoveryData = expandMacros(m_instanceDiscoveryData, MAX_ITEM_NAME);
	}

   updateTimeIntervalsInternal();

   unlock();
}

/**
 * Set DCI status
 */
void DCObject::setStatus(int status, bool generateEvent, bool userChange)
{
   if ((m_status != static_cast<BYTE>(status)) && (userChange || !isDisabledByUser()))
   {
      if (userChange)
      {
         if (status == ITEM_STATUS_DISABLED)
            m_stateFlags |= DCO_STATE_DISABLED_BY_USER;
         else
            m_stateFlags &= ~DCO_STATE_DISABLED_BY_USER;
      }

      auto owner = m_owner.lock();
      if (owner != nullptr)
      {
         NotifyClientsOnDCIStatusChange(*owner, getId(), status);
         if (generateEvent && IsEventSource(owner->getObjectClass()))
         {
            static uint32_t eventCode[3] = { EVENT_DCI_ACTIVE, EVENT_DCI_DISABLED, EVENT_DCI_UNSUPPORTED };
            static const TCHAR *originName[13] =
            {
               _T("Internal"), _T("NetXMS Agent"), _T("SNMP"),
               _T("Web Service"), _T("Push"), _T("WinPerf"),
               _T("iLO"), _T("Script"), _T("SSH"), _T("MQTT"),
               _T("Device Driver"), _T("Modbus"), _T("EtherNet/IP")
            };
            EventBuilder(eventCode[status], owner->getId())
               .dci(m_id)
               .param(_T("dciId"), m_id, EventBuilder::OBJECT_ID_FORMAT)
               .param(_T("metric"), m_name)
               .param(_T("description"), m_description)
               .param(_T("originCode"), m_source)
               .param(_T("origin"), originName[m_source])
               .post();
         }
      }

      m_status = static_cast<BYTE>(status);
   }
}

/**
 * Update polling schedule type
 */
void DCObject::setPollingIntervalType(BYTE pollingScheduleType)
{
   lock();
   m_pollingScheduleType = pollingScheduleType;
   if (m_pollingScheduleType != DC_POLLING_SCHEDULE_CUSTOM)
      m_pollingIntervalSrc.reset();
   unlock();
}

/**
 * Update polling interval
 */
void DCObject::setPollingInterval(const TCHAR *interval)
{
   lock();
   m_pollingIntervalSrc = interval;
   updateTimeIntervalsInternal();
   unlock();

}

/**
 * Update retention type
 */
void DCObject::setRetentionType(BYTE retentionType)
{
   lock();
   m_retentionType = retentionType;
   if (m_retentionType != DC_RETENTION_CUSTOM)
      m_retentionTimeSrc.reset();
   unlock();
}

/**
 * Update retention time
 */
void DCObject::setRetention(const TCHAR *retention)
{
   lock();
   m_retentionTimeSrc = retention;
   updateTimeIntervalsInternal();
   unlock();
}

/**
 * Set threshold disable end time, 0 - to process thresholds
 */
void DCObject::setThresholdDisableEndTime(time_t thresholdDisableEndTime)
{
   lock();
   m_thresholdDisableEndTime = thresholdDisableEndTime;
   unlock();
}

/**
 * Expand custom schedule string
 */
String DCObject::expandSchedule(const TCHAR *schedule)
{
   StringBuffer expandedSchedule;
   if (_tcslen(schedule) > 4 && !_tcsncmp(schedule, _T("%["), 2))
   {
      TCHAR *scriptName = MemCopyString(schedule + 2);
      if (scriptName != nullptr)
      {
         TCHAR *closingBracker = _tcschr(scriptName, _T(']'));
         if (closingBracker != nullptr)
         {
            *closingBracker = 0;
            StringBuffer output;
            m_owner.lock()->expandScriptMacro(scriptName, nullptr, nullptr, createDescriptorInternal(), &expandedSchedule);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 4, _T("DCObject::expandSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name.cstr());
         }
         MemFree(scriptName);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 4, _T("DCObject::expandSchedule: invalid script schedule syntax in %d [%s]"), m_id, m_name.cstr());
      }
   }
   else
   {
      expandedSchedule = schedule;
   }
   return expandedSchedule;
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
      nxlog_debug_tag(DEBUG_TAG_DC_SCHEDULER, 5, _T("DCObject::isReadyForPolling: cannot obtain lock for data collection object [%u]"), m_id);
      return false;
   }

   if (m_doForcePoll && !m_busy)
   {
      if ((m_status != ITEM_STATUS_DISABLED) &&
          isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
          matchClusterResource() && hasValue()) // Ignore agent cache mode for forced polls and always request data as if cache is off
      {
         unlock();
         return true;
      }
      else
      {
         // DCI cannot be force polled at the moment, clear force poll request
         nxlog_debug_tag(DEBUG_TAG_DC_SCHEDULER, 6, _T("Forced poll of DC object %s [%u] on node %s [%u] cancelled"), m_name.cstr(), m_id, getOwnerName(), m_ownerId);
         if (m_pollingSession != nullptr)
         {
            m_pollingSession->decRefCount();
            m_pollingSession = nullptr;
         }
         m_doForcePoll = false;
         unlock();
         return false;
      }
   }

   bool result;
   if ((m_status != ITEM_STATUS_DISABLED) && (!m_busy) &&
       isCacheLoaded() && (m_source != DS_PUSH_AGENT) &&
       matchClusterResource() && hasValue() && (getAgentCacheMode() == AGENT_CACHE_OFF) &&
       (m_nextPollTime <= currTime))
   {
      if (m_pollingScheduleType == DC_POLLING_SCHEDULE_ADVANCED)
      {
         if (m_schedules != nullptr)
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

               String schedule = expandSchedule(m_schedules->get(i));
               if (MatchSchedule(schedule, &withSeconds, &tmCurrLocal, currTime))
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
		      result = ((m_lastPollTime.asTime() + getEffectivePollingInterval() * 10 <= currTime) && (m_startTime <= currTime));
			else
		      result = ((m_lastPollTime.asTime() + getEffectivePollingInterval() <= currTime) && (m_startTime <= currTime));
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
   nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 9, _T("DCObject::prepareForDeletion for DCO %d"), m_id);

	lock();
   m_status = ITEM_STATUS_DISABLED;   // Prevent future polls
	m_scheduledForDeletion = 1;
	bool canDelete = (m_busy ? false : true);
   unlock();
   nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 9, _T("DCObject::prepareForDeletion: completed for DCO %d, canDelete=%d"), m_id, (int)canDelete);

	return canDelete;
}

/**
 * Create NXCP message with object data
 */
void DCObject::createMessage(NXCPMessage *msg)
{
	lock();
   msg->setField(VID_DCI_ID, m_id);
	msg->setField(VID_DCOBJECT_TYPE, static_cast<uint16_t>(getType()));
   msg->setField(VID_TEMPLATE_ID, m_templateId);
   msg->setField(VID_TEMPLATE_ITEM_ID, m_templateItemId);
   msg->setField(VID_NAME, m_name);
   msg->setField(VID_DESCRIPTION, m_description);
   msg->setField(VID_TRANSFORMATION_SCRIPT, m_transformationScriptSource);
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_STATE_FLAGS, m_stateFlags);
   msg->setField(VID_SYSTEM_TAG, m_systemTag);
   msg->setField(VID_USER_TAG, m_userTag);
   msg->setField(VID_POLLING_SCHEDULE_TYPE, static_cast<int16_t>(m_pollingScheduleType));
   msg->setField(VID_POLLING_INTERVAL, m_pollingIntervalSrc);
   msg->setField(VID_RETENTION_TYPE, static_cast<int16_t>(m_retentionType));
   msg->setField(VID_RETENTION_TIME, m_retentionTimeSrc);
   msg->setField(VID_DCI_SOURCE_TYPE, static_cast<uint16_t>(m_source));
   msg->setField(VID_DCI_STATUS, static_cast<uint16_t>(m_status));
	msg->setField(VID_RESOURCE_ID, m_resourceId);
	msg->setField(VID_AGENT_PROXY, m_sourceNode);
	msg->setField(VID_SNMP_PORT, m_snmpPort);
   msg->setField(VID_SNMP_VERSION, static_cast<int16_t>(m_snmpVersion));
   msg->setField(VID_COMMENTS, m_comments);
   msg->setField(VID_PERFTAB_SETTINGS, m_perfTabSettings);
	if (m_schedules != nullptr)
	{
      msg->setField(VID_NUM_SCHEDULES, static_cast<uint32_t>(m_schedules->size()));
      uint32_t fieldId = VID_DCI_SCHEDULE_BASE;
      for(int i = 0; i < m_schedules->size(); i++, fieldId++)
         msg->setField(fieldId, m_schedules->get(i));
	}
	else
	{
      msg->setField(VID_NUM_SCHEDULES, static_cast<uint32_t>(0));
	}
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   msg->setField(VID_INSTD_FILTER, m_instanceFilterSource);
   msg->setField(VID_INSTANCE, m_instanceName);
   msg->setFieldFromInt32Array(VID_ACL, m_accessList);
   msg->setField(VID_INSTANCE_RETENTION, m_instanceRetentionTime);
   msg->setFieldFromTime(VID_THRESHOLD_ENABLE_TIME, m_thresholdDisableEndTime);
   msg->setField(VID_RELATED_OBJECT, m_relatedObject);
   unlock();
}

/**
 * Update data collection object from NXCP message
 */
void DCObject::updateFromMessage(const NXCPMessage& msg)
{
   lock();

   m_name = msg.getFieldAsSharedString(VID_NAME, MAX_ITEM_NAME);
   m_description = msg.getFieldAsSharedString(VID_DESCRIPTION, MAX_DB_STRING);
   m_systemTag = msg.getFieldAsSharedString(VID_SYSTEM_TAG, MAX_DCI_TAG_LENGTH);
   m_userTag = msg.getFieldAsSharedString(VID_USER_TAG, MAX_DCI_TAG_LENGTH);
	m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   m_source = (BYTE)msg.getFieldAsUInt16(VID_DCI_SOURCE_TYPE);
   setStatus(msg.getFieldAsUInt16(VID_DCI_STATUS), true, true);
	m_resourceId = msg.getFieldAsUInt32(VID_RESOURCE_ID);
	m_sourceNode = msg.getFieldAsUInt32(VID_AGENT_PROXY);
   m_snmpPort = msg.getFieldAsUInt16(VID_SNMP_PORT);
   m_snmpVersion = msg.isFieldExist(VID_SNMP_VERSION) ? static_cast<SNMP_Version>(msg.getFieldAsInt16(VID_SNMP_VERSION)) : SNMP_VERSION_DEFAULT;
	m_perfTabSettings = msg.getFieldAsSharedString(VID_PERFTAB_SETTINGS);
	m_comments = msg.getFieldAsSharedString(VID_COMMENTS);

   m_pollingScheduleType = static_cast<BYTE>(msg.getFieldAsUInt16(VID_POLLING_SCHEDULE_TYPE));
   if (m_pollingScheduleType == DC_POLLING_SCHEDULE_CUSTOM)
   {
      TCHAR buffer[MAX_DB_STRING];
      m_pollingIntervalSrc = msg.getFieldAsString(VID_POLLING_INTERVAL, buffer, MAX_DB_STRING);
   }
   else
   {
      m_pollingIntervalSrc.reset();
   }
   m_retentionType = static_cast<BYTE>(msg.getFieldAsUInt16(VID_RETENTION_TYPE));
   if (m_retentionType == DC_RETENTION_CUSTOM)
   {
      TCHAR buffer[MAX_DB_STRING];
      m_retentionTimeSrc = msg.getFieldAsString(VID_RETENTION_TIME, buffer, MAX_DB_STRING);
   }
   else
   {
      m_retentionTimeSrc.reset();
   }
   updateTimeIntervalsInternal();

   TCHAR *tmp = msg.getFieldAsString(VID_TRANSFORMATION_SCRIPT);
   setTransformationScript(tmp);
   MemFree(tmp);

   // Update schedules
   int count = msg.getFieldAsInt32(VID_NUM_SCHEDULES);
   if (count > 0)
   {
      if (m_schedules != nullptr)
         m_schedules->clear();
      else
         m_schedules = new StringList();

      uint32_t fieldId = VID_DCI_SCHEDULE_BASE;
      for(int i = 0; i < count; i++, fieldId++)
      {
         TCHAR *s = msg.getFieldAsString(fieldId);
         if (s != nullptr)
         {
            m_schedules->addPreallocated(s);
         }
      }
   }
   else
   {
      delete_and_null(m_schedules);
   }

   m_instanceDiscoveryMethod = msg.getFieldAsUInt16(VID_INSTD_METHOD);
   m_instanceDiscoveryData = msg.getFieldAsSharedString(VID_INSTD_DATA, MAX_INSTANCE_LEN);

   wchar_t *pszStr = msg.getFieldAsString(VID_INSTD_FILTER);
   setInstanceFilter(pszStr);
   MemFree(pszStr);

   m_instanceName = msg.getFieldAsSharedString(VID_INSTANCE, MAX_INSTANCE_LEN);

   m_accessList.clear();
   msg.getFieldAsInt32Array(VID_ACL, &m_accessList);

   m_instanceRetentionTime = msg.getFieldAsInt32(VID_INSTANCE_RETENTION);
   m_relatedObject = msg.getFieldAsUInt32(VID_RELATED_OBJECT);

   m_thresholdDisableEndTime = msg.getFieldAsTime(VID_THRESHOLD_ENABLE_TIME);

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
	if (success && (m_schedules != nullptr) && !m_schedules->isEmpty())
   {
	   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dci_schedules (item_id,schedule_id,schedule) VALUES (?,?,?)"));
	   if (hStmt != nullptr)
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
   if (success && !m_accessList.isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO dci_access (dci_id,user_id) VALUES (?,?)"), m_accessList.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_accessList.size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_accessList.get(i));
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
   ResolveAlarmByDCObjectId(CAST_FROM_POINTER(arg, uint32_t), true);
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
bool DCObject::loadThresholdsFromDB(DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
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
   temp.replace(_T("{instance-value}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instanceName);
   m_name = temp;

   temp = m_description;
   temp.replace(_T("{instance}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-value}"), m_instanceDiscoveryData);
   temp.replace(_T("{instance-name}"), m_instanceName);
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
   m_userTag = expandMacros(src->m_userTag, MAX_DB_STRING);

   m_pollingScheduleType = src->m_pollingScheduleType;
   m_pollingIntervalSrc = src->m_pollingIntervalSrc;
   m_retentionType = src->m_retentionType;
   m_retentionTimeSrc = src->m_retentionTimeSrc;
   updateTimeIntervalsInternal();

   m_source = src->m_source;
   m_flags = src->m_flags;
   m_sourceNode = src->m_sourceNode;
   m_resourceId = src->m_resourceId;
   m_snmpPort = src->m_snmpPort;
   m_snmpVersion = src->m_snmpVersion;
   m_comments = src->m_comments;
   m_perfTabSettings = src->m_perfTabSettings;

   m_transformationScriptSource = src->m_transformationScriptSource;
   m_transformationScript = src->m_transformationScript;

   m_accessList.clear();
   m_accessList.addAll(src->m_accessList);

   // Copy schedules
   delete m_schedules;
   m_schedules = (src->m_schedules != nullptr) ? new StringList(src->m_schedules) : nullptr;

   // DCObject::updateFromTemplate can be called in two different scenarios:
   // 1. When template DCI was changed and we have to update DCI on data collection target;
   // 2. When instance discovery DCI was changed and we have to update DCIs on same node created 
   //    from that instance discovery DCI.
   // In second case, instance discovery method should not be changed, and instance data should be updated instead.
   // Owner object being the same as template object is an indicator that this DCI was created by instance discovery.
   if ((src->getInstanceDiscoveryMethod() != IDM_NONE) && (m_instanceDiscoveryMethod == IDM_NONE) && (m_ownerId == m_templateId))
   {
      expandInstance();
   }
   else
   {
      m_instanceName = expandMacros(src->m_instanceName, MAX_ITEM_NAME);
      m_instanceDiscoveryMethod = src->m_instanceDiscoveryMethod;
      m_instanceDiscoveryData = expandMacros(m_instanceDiscoveryData, MAX_ITEM_NAME);
      m_instanceFilterSource = src->m_instanceFilterSource;
      m_instanceFilter = src->m_instanceFilter;
   }
   m_instanceRetentionTime = src->m_instanceRetentionTime;

   if (((m_status != ITEM_STATUS_DISABLED) || (g_flags & AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE)) &&
       !((m_ownerId == m_templateId) && (m_instanceGracePeriodStart > 0))) // check if DCI is not disabled by instance discovery
   {
      setStatus(src->m_status, true);
   }

   unlock();
}

/**
 * Process new data collection error
 */
void DCObject::processNewError(bool noInstance, Timestamp timestamp)
{
}

/**
 * Should return true if object has (or can have) value
 */
bool DCObject::hasValue()
{
   auto owner = m_owner.lock();
   if ((owner != nullptr) && (owner->getObjectClass() == OBJECT_CLUSTER))
      return isAggregateOnCluster() && (m_instanceDiscoveryMethod == IDM_NONE);
   return m_instanceDiscoveryMethod == IDM_NONE;
}

/**
 * Set new transformation script
 */
void DCObject::setTransformationScript(const TCHAR *source)
{
   if ((source != nullptr) && !IsBlankString(source))
   {
      m_transformationScriptSource = source;
      m_transformationScript = shared_ptr<NXSL_Program>(CompileServerScript(m_transformationScriptSource, SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, _T("DCI::%s::%d::TransformationScript"), getOwnerName(), m_id));
   }
   else
   {
      m_transformationScriptSource = nullptr;
      m_transformationScript.reset();
   }
   m_lastScriptErrorReport = 0;  // allow immediate error report after script change
}

/**
 * Get actual agent cache mode
 */
int16_t DCObject::getAgentCacheMode()
{
   if ((m_source != DS_NATIVE_AGENT) && (m_source != DS_SNMP_AGENT) && (m_source != DS_MODBUS))
      return AGENT_CACHE_OFF;

   shared_ptr<Node> node;
   if (m_sourceNode != 0)
   {
      node = static_pointer_cast<Node>(FindObjectById(m_sourceNode, OBJECT_NODE));
   }
   else
   {
      auto owner = m_owner.lock();
      if (owner->getObjectClass() == OBJECT_NODE)
      {
         node = static_pointer_cast<Node>(owner);
      }
      else if (owner->getObjectClass() == OBJECT_CHASSIS)
      {
         node = static_pointer_cast<Node>(FindObjectById(static_cast<Chassis*>(owner.get())->getControllerId(), OBJECT_NODE));
      }
   }
   if (node == nullptr)
      return AGENT_CACHE_OFF;

   if ((m_source == DS_SNMP_AGENT) && (node->getEffectiveSnmpProxy() == 0))
      return AGENT_CACHE_OFF;

   if ((m_source == DS_MODBUS) && (node->getEffectiveModbusProxy() == 0))
      return AGENT_CACHE_OFF;

   uint32_t mode = DCF_GET_CACHE_MODE(m_flags);
   if (mode != AGENT_CACHE_DEFAULT)
      return mode;
   return node->getAgentCacheMode();
}

/**
 * Update data collection object from import file
 */
void DCObject::updateFromImport(ConfigEntry *config, bool nxslV5)
{
   lock();

   m_name = config->getSubEntryValue(_T("name"), 0, _T("unnamed"));
   m_description = config->getSubEntryValue(_T("description"), 0, m_name);
   m_systemTag = config->getSubEntryValue(_T("systemTag"), 0, nullptr);
   m_userTag = config->getSubEntryValue(_T("userTag"), 0, nullptr);
   m_source = (BYTE)config->getSubEntryValueAsInt(_T("origin"));
   m_flags = config->getSubEntryValueAsInt(_T("flags"));
   m_perfTabSettings = config->getSubEntryValue(_T("perfTabSettings"));
   m_snmpPort = static_cast<uint16_t>(config->getSubEntryValueAsInt(_T("snmpPort")));
   m_snmpVersion = static_cast<SNMP_Version>(config->getSubEntryValueAsInt(_T("snmpVersion"), 0, SNMP_VERSION_DEFAULT));
   if (config->getSubEntryValueAsBoolean(_T("isDisabled")))
      m_status = ITEM_STATUS_DISABLED;

   m_pollingIntervalSrc = config->getSubEntryValue(_T("interval"), 0, nullptr);
   if (config->findEntry(_T("scheduleType")) != nullptr)
   {
      m_pollingScheduleType = config->getSubEntryValueAsInt(_T("scheduleType"));
   }
   else
   {
      m_pollingScheduleType = (m_pollingIntervalSrc.isEmpty() || !_tcscmp(m_pollingIntervalSrc, _T("0"))) ? DC_POLLING_SCHEDULE_DEFAULT : DC_POLLING_SCHEDULE_CUSTOM;
      if (m_flags & 1)  // for compatibility with old format
         m_pollingScheduleType = DC_POLLING_SCHEDULE_ADVANCED;
   }
   m_retentionTimeSrc = config->getSubEntryValue(_T("retention"), 0, nullptr);
   if (config->findEntry(_T("retentionType")) != nullptr)
   {
      m_retentionType = config->getSubEntryValueAsInt(_T("retentionType"));
   }
   else
   {
      m_retentionType = (m_retentionTimeSrc.isEmpty() || !_tcscmp(m_retentionTimeSrc, _T("0"))) ? DC_RETENTION_DEFAULT : DC_RETENTION_CUSTOM;
      if (m_flags & 0x200) // for compatibility with old format
         m_retentionType = DC_RETENTION_NONE;
   }

   updateTimeIntervalsInternal();

   if (nxslV5)
   {
      setTransformationScript(config->getSubEntryValue(_T("transformation")));
   }
   else
   {
      setTransformationScript(NXSLConvertToV5(config->getSubEntryValue(_T("transformation"), 0, _T(""))));
   }

   ConfigEntry *schedules = config->findEntry(_T("schedules"));
   if (schedules != nullptr)
      schedules = schedules->findEntry(_T("schedule"));
   if ((schedules != nullptr) && (schedules->getValueCount() > 0))
   {
      if (m_schedules != nullptr)
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

   if (nxslV5)
   {
      setInstanceFilter(config->getSubEntryValue(_T("instanceFilter")));
   }
   else
   {
      setInstanceFilter(NXSLConvertToV5(config->getSubEntryValue(_T("instanceFilter"), 0, _T(""))));
   }
   m_instanceName = config->getSubEntryValue(_T("instance"));
   m_instanceRetentionTime = config->getSubEntryValueAsInt(_T("instanceRetentionTime"), 0, -1);

   unlock();
}

/**
 * Get owner name
 */
const TCHAR *DCObject::getOwnerName() const
{
   auto owner = m_owner.lock();
   return (owner != nullptr) ? owner->getName() : _T("(null)");
}

/**
 * Create NXSL object for this data collection object
 */
NXSL_Value *DCObject::createNXSLObject(NXSL_VM *vm) const
{
   return vm->createValue(vm->createObject(&g_nxslDciClass, new shared_ptr<DCObjectInfo>(createDescriptor())));
}

/**
 * Process force poll request
 */
ClientSession *DCObject::processForcePoll()
{
   lock();
   ClientSession *session = m_pollingSession;
   m_pollingSession = nullptr;
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
   if (m_pollingSession != nullptr)
      m_pollingSession->decRefCount();
   m_pollingSession = session;
   if (m_pollingSession != nullptr)
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

   SetupServerScriptVM(instanceFilter, dco->getOwner(), dco->createDescriptor());
   instanceFilter->setGlobalVariable("$targetObject", dco->getOwner()->createNXSLObject(instanceFilter));
   if (dco->getSourceNode() != 0)
   {
      shared_ptr<NetObj> sourceNode = FindObjectById(dco->getSourceNode(), OBJECT_NODE);
      if (sourceNode != nullptr)
         instanceFilter->setGlobalVariable("$sourceNode", sourceNode->createNXSLObject(instanceFilter));
   }

   NXSL_Value *argv[2];
   argv[0] = instanceFilter->createValue(key);
   argv[1] = instanceFilter->createValue(value);

   if (!instanceFilter->run(2, argv))
   {
      ReportScriptError(SCRIPT_CONTEXT_DCI, dco->getOwner().get(), dco->getId(), instanceFilter->getErrorText(), _T("DCI::%s::%d::InstanceFilter"), dco->getOwnerName(), dco->getId());
      nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 5, _T("DCObject::filterInstanceList(%s [%u]): filtering script runtime error while processing instance \"%s\" (%s)"),
               dcObjectName.cstr(), dco->getId(), key, instanceFilter->getErrorText());
      dco->getOwner()->sendPollerMsg(POLLER_ERROR _T("      Filtering script runtime error while processing instance \"%s\" (%s)\r\n"), key, instanceFilter->getErrorText());
      return _STOP;
   }

   bool accepted;
   const TCHAR *instance = key;
   const TCHAR *name = value;
   uint32_t relatedObject = 0;
   NXSL_Value *result = instanceFilter->getResult();
   if (result != nullptr)
   {
      if (result->isArray())
      {
         NXSL_Array *array = result->getValueAsArray();
         if (array->size() > 0)
         {
            int index = 0;

            // If first array element is not boolean value or null, assume implicit accept and expect transformed instance as first element
            if ((array->get(0)->getDataType() == NXSL_DT_BOOLEAN) || array->get(0)->isNull())
            {
               accepted = array->get(0)->getValueAsBoolean();
               index++;
            }
            else
            {
               accepted = true;
            }

            if (accepted)
            {
               if (array->size() > index)
               {
                  // transformed value
                  const TCHAR *newValue = array->get(index++)->getValueAsCString();
                  if ((newValue != nullptr) && (*newValue != 0))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 5, _T("DCObject::filterInstanceList(%s [%u]): instance \"%s\" replaced by \"%s\""),
                              dcObjectName.cstr(), dco->getId(), instance, newValue);
                     instance = newValue;
                  }
               }
               if (array->size() > index)
               {
                  // instance name
                  const TCHAR *newName = array->get(index++)->getValueAsCString();
                  if ((newName != nullptr) && (*newName != 0))
                  {
                     nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 5, _T("DCObject::filterInstanceList(%s [%u]): instance \"%s\" name set to \"%s\""),
                              dcObjectName.cstr(), dco->getId(), instance, newName);
                     name = newName;
                  }
               }
               if ((array->size() > index) && array->get(index)->isObject(g_nxslNetObjClass.getName()))
               {
                  relatedObject = (*static_cast<shared_ptr<NetObj>*>(array->get(index)->getValueAsObject()->getData()))->getId();
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
         accepted = result->getValueAsBoolean();
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
      nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 5, _T("DCObject::filterInstanceList(%s [%u]): instance \"%s\" removed by filtering script"),
               dcObjectName.cstr(), dco->getId(), key);
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
 * Filter instance list. Returns nullptr on filtering error.
 */
StringObjectMap<InstanceDiscoveryData> *DCObject::filterInstanceList(StringMap *instances)
{
   auto filteredInstances = new StringObjectMap<InstanceDiscoveryData>(Ownership::True);

   lock();
   if (m_instanceFilter == nullptr)
   {
      unlock();
      instances->forEach(CopyElements, filteredInstances);
      return filteredInstances;
   }

   FilterCallbackData data;
   data.instanceFilter = new NXSL_VM(new NXSL_ServerEnv());
   data.instanceFilter->setUserData(getOwner().get());
   if (!data.instanceFilter->load(m_instanceFilter.get()))
   {
      ReportScriptError(SCRIPT_CONTEXT_DCI, getOwner().get(), m_id, data.instanceFilter->getErrorText(), _T("DCI::%s::%d::InstanceFilter"), getOwnerName(), m_id);
      unlock();
      delete data.instanceFilter;
      delete filteredInstances;
      nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 5, _T("DCObject::filterInstanceList(%s [%u]): all instances removed because filtering script cannot be loaded"), m_name.cstr(), m_id);
      getOwner()->sendPollerMsg(POLLER_ERROR _T("      Cannot load instance filtering script\r\n"));
      return nullptr;
   }
   unlock();

   data.filteredInstances = filteredInstances;
   data.dco = this;
   if (instances->forEach(FilterCallback, &data) == _STOP)
   {
      delete_and_null(filteredInstances);
   }
   delete data.instanceFilter;
   return filteredInstances;
}

/**
 * Set new instance discovery filter script
 */
void DCObject::setInstanceFilter(const TCHAR *script)
{
   if ((script != nullptr) && !IsBlankString(script))
   {
      m_instanceFilterSource = script;

      NXSL_CompilationDiagnostic diag;
      NXSL_ServerEnv env;
      m_instanceFilter = shared_ptr<NXSL_Program>(NXSLCompile(m_instanceFilterSource, &env, &diag));
      if (m_instanceFilter == nullptr)
      {
         // node can be nullptr if this DCO was just created from template
         // in this case compilation error will be reported on template level anyway
         auto owner = m_owner.lock();
         if (owner != nullptr)
         {
            ReportScriptError(SCRIPT_CONTEXT_DCI, owner.get(), m_id, diag.errorText, _T("DCI::%s::%d::InstanceFilter"), owner->getName(), m_id);
         }
      }
   }
   else
   {
      m_instanceFilterSource = nullptr;
      m_instanceFilter.reset();
   }
}

/**
 * Update time intervals (polling and retention) from sources
 */
void DCObject::updateTimeIntervalsInternal()
{
   if (!m_retentionTimeSrc.isEmpty())
   {
      StringBuffer exp = m_owner.lock()->expandText(m_retentionTimeSrc, nullptr, nullptr, createDescriptorInternal(), nullptr, nullptr, m_instanceName, nullptr, nullptr);
      m_retentionTime = _tcstol(exp, nullptr, 10);
   }
   else
   {
      m_retentionTime = 0;
   }

   if (!m_pollingIntervalSrc.isEmpty())
   {
      StringBuffer exp = m_owner.lock()->expandText(m_pollingIntervalSrc, nullptr, nullptr, createDescriptorInternal(), nullptr, nullptr, m_instanceName, nullptr, nullptr);
      m_pollingInterval = _tcstol(exp, nullptr, 10);
   }
   else
   {
      m_pollingInterval = 0;
   }

   nxlog_debug_tag(DEBUG_TAG_DC_CONFIG, 6, _T("DCObject::updateTimeIntervalsInternal(%s [%d]): retentionTime=%d, pollingInterval=%d"), m_name.cstr(), m_id, m_retentionTime, m_pollingInterval);
}

/**
 * Checks if this DCO object is visible by given user
 */
bool DCObject::hasAccess(uint32_t userId)
{
   if (m_accessList.isEmpty() || (userId == 0))
      return true;

   for(int i = 0; i < m_accessList.size(); i++)
   {
      uint32_t id = m_accessList.get(i);
      if ((id == userId) || ((id & GROUP_FLAG) && CheckUserMembership(userId, id)))
         return true;
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
   json_object_set_new(root, "description", json_string_w(m_description));
   json_object_set_new(root, "systemTag", json_string_w(m_systemTag));
   json_object_set_new(root, "userTag", json_string_w(m_userTag));
   json_object_set_new(root, "lastPollTime", m_lastPollTime.asJson());
   json_object_set_new(root, "pollingInterval", json_string_w(m_pollingIntervalSrc));
   json_object_set_new(root, "retentionTime", json_string_w(m_retentionTimeSrc));
   json_object_set_new(root, "source", json_integer(m_source));
   json_object_set_new(root, "status", json_integer(m_status));
   json_object_set_new(root, "busy", json_integer(m_busy));
   json_object_set_new(root, "scheduledForDeletion", json_integer(m_scheduledForDeletion));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "templateId", json_integer(m_templateId));
   json_object_set_new(root, "templateItemId", json_integer(m_templateItemId));
   json_object_set_new(root, "schedules", (m_schedules != nullptr) ? m_schedules->toJson() : json_array());
   json_object_set_new(root, "lastCheck", json_time_string(m_tLastCheck));
   json_object_set_new(root, "errorCount", json_integer(m_errorCount));
   json_object_set_new(root, "resourceId", json_integer(m_resourceId));
   json_object_set_new(root, "sourceNode", json_integer(m_sourceNode));
   json_object_set_new(root, "snmpPort", json_integer(m_snmpPort));
   json_object_set_new(root, "snmpVersion", json_integer(m_snmpVersion));
   json_object_set_new(root, "perfTabSettings", json_string_w(m_perfTabSettings));
   json_object_set_new(root, "transformationScript", json_string_w(m_transformationScriptSource));
   json_object_set_new(root, "comments", json_string_w(m_comments));
   json_object_set_new(root, "instanceDiscoveryMethod", json_integer(m_instanceDiscoveryMethod));
   json_object_set_new(root, "instanceDiscoveryData", json_string_w(m_instanceDiscoveryData));
   json_object_set_new(root, "instanceFilter", json_string_w(m_instanceFilterSource));
   json_object_set_new(root, "instanceName", json_string_w(m_instanceName));
   json_object_set_new(root, "accessList", m_accessList.toJson());
   json_object_set_new(root, "instanceRetentionTime", json_integer(m_instanceRetentionTime));
   unlock();
   return root;
}

/**
 * Create descriptor for this object (external entry point)
 */
shared_ptr<DCObjectInfo> DCObject::createDescriptor() const
{
   lock();
   shared_ptr<DCObjectInfo> info = createDescriptorInternal();
   unlock();
   return info;
}

/**
 * Create descriptor for this object (internal implementation)
 */
shared_ptr<DCObjectInfo> DCObject::createDescriptorInternal() const
{
   return make_shared<DCObjectInfo>(*this);
}

/**
 * Fill message with scheduling data
 */
void DCObject::fillSchedulingDataMessage(NXCPMessage *msg, uint32_t base) const
{
   if (m_schedules != nullptr)
   {
      msg->setField(base++, static_cast<uint32_t>(m_schedules->size()));
      for(int i = 0; i < m_schedules->size(); i++)
      {
         msg->setField(base++, m_schedules->get(i));
      }
   }
   else
   {
      msg->setField(base++, static_cast<uint32_t>(0));
   }
}

/**
 * Add dependencies from library script with given name (and given script itself)
 */
static void AddScriptDependencies(StringSet *dependencies, const TCHAR *name)
{
   TCHAR buffer[256];
   const TCHAR *p = _tcschr(name, _T('('));
   const TCHAR *scriptName;
   if (p != nullptr)
   {
      _tcslcpy(buffer, name, p - name + 1);
      scriptName = buffer;
   }
   else
   {
      scriptName = name;
   }
   dependencies->add(scriptName);

   StringList *scriptDependencies = GetServerScriptLibrary()->getScriptDependencies(scriptName);
   if (scriptDependencies != nullptr)
   {
      for(int i = 0; i < scriptDependencies->size(); i++)
      {
         const TCHAR *s = scriptDependencies->get(i);
         if (!dependencies->contains(s))  // To avoid infinite loop on circular reference
         {
            dependencies->add(s);
            AddScriptDependencies(dependencies, s);
         }
      }
      delete scriptDependencies;
   }
}

/**
 * Add dependencies from compiled script
 */
static void AddScriptDependencies(StringSet *dependencies, const NXSL_Program *script)
{
   if (script != nullptr)
   {
      StringList *modules = script->getRequiredModules();
      if (modules != nullptr)
      {
         for(int i = 0; i < modules->size(); i++)
         {
            const TCHAR *s = modules->get(i);
            if (!dependencies->contains(s))  // To avoid infinite loop on circular reference
            {
               dependencies->add(s);
               AddScriptDependencies(dependencies, s);
            }
         }
         delete modules;
      }
   }
}

/**
 * Extract script names from text
 * Possible options:
 * "%[name]",
 * "%[name.function(123,"A textual parameter")]"
 * "%[name/function(123,"A textual parameter")]"
 */
void FindScriptMacrosInText(const wchar_t *origin, StringSet *dependencies)
{
   wchar_t name[512];
   wcslcpy(name, origin, 512);
   TrimW(name);

   TCHAR *searchStart = name;
   while(true)
   {
      wchar_t *macroStart = wcschr(searchStart, L'%');
      if (macroStart == nullptr || *(macroStart + 1) == 0)
         break;

      if (*(macroStart + 1) != L'[')
      {
         searchStart = macroStart + 2;
         continue;
      }

      macroStart += 2;
      wchar_t *macroEnd = wcschr(macroStart, L']');
      if (macroEnd == nullptr)
         break;

      *macroEnd = 0;

      wchar_t *p = wcschr(macroStart, L'(');
      if (p != nullptr)
      {
         *p = 0;
      }

      char entryPoint[MAX_IDENTIFIER_LENGTH];
      ExtractScriptEntryPoint(macroStart, entryPoint);

      dependencies->add(macroStart);

      searchStart = macroEnd + 1;
   }
}

/**
 * Extract script names from text
 * Possible options:
 * "%{script:name}"
 */
static void FindScriptTemplateMacrosInText(const wchar_t *name, StringSet *dependencies)
{
   const wchar_t *searchStart = name;
   while(true)
   {
      const wchar_t *macroStart = wcschr(searchStart, L'%');
      if (macroStart == nullptr || *(macroStart + 1) == 0)
         break;

      if (wcsnicmp(macroStart, L"%{script:", 9) != 0)
      {
         searchStart = macroStart + 2;
         continue;
      }

      macroStart += 9;
      const wchar_t *macroEnd = wcschr(macroStart, _T('}'));
      if (macroEnd == nullptr)
         break;

      size_t len = macroEnd - macroStart;
      wchar_t *scriptName = (wchar_t *)MemAlloc((len + 1) * sizeof(wchar_t));
      wcsncpy(scriptName, macroStart, len);
      scriptName[len] = 0;

      dependencies->add(scriptName);
      MemFree(scriptName);

      searchStart = macroEnd + 1;
   }
}

/**
 * Get all script dependencies for this object
 */
void DCObject::getScriptDependencies(StringSet *dependencies) const
{
   if (m_source == DS_SCRIPT)
      AddScriptDependencies(dependencies, m_name);

   if (m_instanceDiscoveryMethod == IDM_SCRIPT)
      AddScriptDependencies(dependencies, m_instanceDiscoveryData);

   AddScriptDependencies(dependencies, m_transformationScript.get());
   AddScriptDependencies(dependencies, m_instanceFilter.get());

   if (!m_retentionTimeSrc.isEmpty())
   {
      FindScriptMacrosInText(m_retentionTimeSrc, dependencies);
   }

   if (!m_pollingIntervalSrc.isEmpty())
   {
      FindScriptMacrosInText(m_pollingIntervalSrc, dependencies);
   }

   if (m_pollingScheduleType == DC_POLLING_SCHEDULE_ADVANCED)
   {
      if (m_schedules != nullptr)
      {
         for(int i = 0; i < m_schedules->size(); i++)
         {
            FindScriptMacrosInText(m_schedules->get(i), dependencies);
         }
      }
   }
   FindScriptMacrosInText(m_pollingIntervalSrc, dependencies);

   FindScriptTemplateMacrosInText(m_name, dependencies);
   FindScriptTemplateMacrosInText(m_description, dependencies);
   FindScriptTemplateMacrosInText(m_instanceName, dependencies);
   FindScriptTemplateMacrosInText(m_instanceDiscoveryData, dependencies);
}

/**
 * Get data source name
 */
const TCHAR *DCObject::getDataProviderName(int dataProvider)
{
   static const TCHAR *names[] = { _T("internal"), _T("nxagent"), _T("snmp"), _T("websvc"), _T("push"), _T("winperf"), _T("smclp"), _T("script"), _T("ssh"), _T("mqtt"), _T("driver"), _T("modbus") };
   return ((dataProvider >= DS_INTERNAL) && (dataProvider <= DS_MODBUS)) ? names[dataProvider] : _T("unknown");
}

/**
 * Get text for this object (part of interface SearchAttributeProvider)
 */
SharedString DCObject::getText() const
{
   return m_description;
}

/**
 * Get named attribute for this object (part of interface SearchAttributeProvider)
 */
SharedString DCObject::getAttribute(const TCHAR *attribute) const
{
   SharedString value;
   if (!_tcsicmp(attribute, _T("description")))
   {
      value = m_description;
   }
   else if (!_tcsicmp(attribute, _T("guid")))
   {
      value = m_guid.toString();
   }
   else if (!_tcsicmp(attribute, _T("id")))
   {
      value = String::toString(m_id);
   }
   else if (!_tcsicmp(attribute, _T("name")))
   {
      value = m_name;
   }
   else if (!_tcsicmp(attribute, _T("pollingInterval")))
   {
      value = String::toString(m_pollingInterval);
   }
   else if (!_tcsicmp(attribute, _T("retentionTime")))
   {
      value = String::toString(m_retentionTime);
   }
   else if (!_tcsicmp(attribute, _T("sourceNode")))
   {
      shared_ptr<DataCollectionOwner> owner = m_owner.lock();
      if (owner != nullptr)
         value = owner->getName();
   }
   return value;
}

/**
 * Data collection object info - constructor
 */
DCObjectInfo::DCObjectInfo(const DCObject& object) : m_pollingSchedules(object.getPollingSchedules())
{
   m_id = object.m_id;
   m_ownerId = object.getOwnerId();
   m_templateId = object.m_templateId;
   m_templateItemId = object.m_templateItemId;
   m_flags = object.m_flags;
   m_type = object.getType();
   wcslcpy(m_name, object.m_name, MAX_ITEM_NAME);
   wcslcpy(m_description, object.m_description, MAX_DB_STRING);
   wcslcpy(m_systemTag, object.m_systemTag, MAX_DCI_TAG_LENGTH);
   wcslcpy(m_userTag, object.m_userTag, MAX_DCI_TAG_LENGTH);
   wcslcpy(m_instanceName, object.m_instanceName, MAX_DB_STRING);
   if (m_type == DCO_TYPE_ITEM)
   {
      m_unitName = static_cast<const DCItem&>(object).m_unitName;
      m_multiplier = static_cast<const DCItem&>(object).m_multiplier;
      m_dataType = static_cast<const DCItem&>(object).m_dataType;
      m_transformedDataType = static_cast<const DCItem&>(object).m_transformedDataType;
   }
   else
   {
      m_multiplier = 0;
      m_dataType = -1;
      m_transformedDataType = -1;
   }
   m_instanceData = MemCopyString(object.m_instanceDiscoveryData);
   m_comments = MemCopyString(object.m_comments);
   m_origin = object.m_source;
   m_status = object.m_status;
   m_errorCount = object.m_errorCount;
   m_pollingInterval = object.getEffectivePollingInterval();
   m_pollingScheduleType = object.m_pollingScheduleType;
   m_lastPollTime = object.m_lastPollTime;
   m_lastCollectionTime = object.m_lastValueTimestamp;
   m_hasActiveThreshold = false;
   m_thresholdSeverity = SEVERITY_NORMAL;
   m_relatedObject = object.getRelatedObject();
   m_thresholdDisableEndTime = object.m_thresholdDisableEndTime;
}

/**
 * Data collection object info - constructor for client provided DCI info
 */
DCObjectInfo::DCObjectInfo(const NXCPMessage& msg, const DCObject *object) : m_pollingSchedules((object != nullptr) ? object->m_schedules : nullptr)
{
   m_id = msg.getFieldAsUInt32(VID_DCI_ID);
   m_ownerId = msg.getFieldAsUInt32(VID_OBJECT_ID);
   m_templateId = (object != nullptr) ? object->getTemplateId() : 0;
   m_templateItemId = (object != nullptr) ? object->getTemplateItemId() : 0;
   m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   m_type = msg.getFieldAsInt16(VID_DCOBJECT_TYPE);
   msg.getFieldAsString(VID_NAME, m_name, MAX_ITEM_NAME);
   msg.getFieldAsString(VID_DESCRIPTION, m_description, MAX_DB_STRING);
   msg.getFieldAsString(VID_SYSTEM_TAG, m_systemTag, MAX_DCI_TAG_LENGTH);
   msg.getFieldAsString(VID_USER_TAG, m_userTag, MAX_DCI_TAG_LENGTH);
   msg.getFieldAsString(VID_INSTANCE, m_instanceName, MAX_DB_STRING);
   if ((m_type == DCO_TYPE_ITEM) && (object != nullptr))
   {
      m_unitName = static_cast<const DCItem*>(object)->m_unitName;
      m_multiplier = static_cast<const DCItem*>(object)->m_multiplier;
      m_dataType = msg.getFieldAsInt16(VID_DCI_DATA_TYPE);
      m_transformedDataType = msg.getFieldAsInt16(VID_TRANSFORMED_DATA_TYPE);
   }
   else
   {
      m_multiplier = 0;
      m_dataType = -1;
      m_transformedDataType = -1;
   }
   m_instanceData = msg.getFieldAsString(VID_INSTD_DATA);
   m_comments = msg.getFieldAsString(VID_COMMENTS);
   m_origin = msg.getFieldAsInt16(VID_DCI_SOURCE_TYPE);
   m_status = msg.getFieldAsInt16(VID_DCI_STATUS);
   m_pollingInterval = (object != nullptr) ? object->getEffectivePollingInterval() : 0;
   m_pollingScheduleType = (object != nullptr) ? object->getPollingScheduleType() : 0;
   m_errorCount = (object != nullptr) ? object->getErrorCount() : 0;
   m_lastPollTime = (object != nullptr) ? object->getLastPollTime() : Timestamp::fromMilliseconds(0);
   m_lastCollectionTime = (object != nullptr) ? object->getLastValueTimestamp() : Timestamp::fromMilliseconds(0);
   m_hasActiveThreshold = false;
   m_thresholdSeverity = SEVERITY_NORMAL;
   m_relatedObject = (object != nullptr) ? object->getRelatedObject() : 0;
   m_thresholdDisableEndTime = (object != nullptr) ? object->getThresholdDisableEndTime() : 0;
}

/**
 * Data collection object info - destructor
 */
DCObjectInfo::~DCObjectInfo()
{
   MemFree(m_instanceData);
   MemFree(m_comments);
}

/**
 * List of units that should be exempt from multiplication
 */
static const TCHAR *s_unitsWithoutMultipliers[] = { _T("%"), _T("°C"), _T("°F"), _T("dbm") };

/**
 * Format DCI value
 */
String DCObjectInfo::formatValue(const TCHAR *value, const StringList *parameters) const
{
   if (parameters == nullptr || parameters->isEmpty())
      return String(value);

   bool useUnits = parameters->containsIgnoreCase(_T("u")) || parameters->containsIgnoreCase(_T("units"));
   bool useMultiplier = parameters->containsIgnoreCase(_T("m")) || parameters->containsIgnoreCase(_T("multipliers"));
   StringBuffer units = m_unitName;
   bool useBinaryPrefixes = units.containsIgnoreCase(_T(" (IEC)"));
   units.replace(_T(" (IEC)"), _T(""));
   units.replace(_T(" (Metric)"), _T(""));

   for (size_t i = 0; useMultiplier && !units.isEmpty() && i < sizeof(s_unitsWithoutMultipliers) / sizeof(TCHAR*); i++)
   {
      if (!_tcscmp(s_unitsWithoutMultipliers[i], units))
      {
         useMultiplier = false;
         break;
      }
   }

   if (useUnits && !_tcsicmp(m_unitName, _T("Uptime")))
   {
      TCHAR *eptr;
      uint64_t time = _tcstoull(value, &eptr, 10);
      return (*eptr != 0) ? String(value) : SecondsToUptime(time, true);
   }

   if (useUnits && !_tcsicmp(m_unitName, _T("Epoch time")))
   {
      TCHAR *eptr;
      time_t time = static_cast<time_t>(_tcstoll(value, &eptr, 10));
      return (*eptr != 0) ? String(value) : FormatTimestamp(time);
   }

   TCHAR *eptr;
   double inVal = _tcstod(value, &eptr);
   if (*eptr != 0)
      return String(value);

   StringBuffer result;
   TCHAR prefixSymbol[8] = _T("");
   if (useMultiplier)
   {
      result.append(FormatNumber(inVal, useBinaryPrefixes, m_multiplier, 2));
   }
   else
   {
      result.append(value);
   }

   if (useUnits && !units.isEmpty())
   {
      if (*prefixSymbol == 0)
         result.append(_T(" "));
      result.append(units);
   }

   return result;
}
