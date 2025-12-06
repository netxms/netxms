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
** File: alarm.cpp
**
**/

#include "nxcore.h"
#include <netxms-regex.h>
#include <nms_users.h>
#include <iris.h>

#define DEBUG_TAG L"alarm"

/**
 * Column list for loading alarms from database
 */
#define ALARM_LOAD_COLUMN_LIST \
   L"alarm_id,source_object_id,zone_uin,source_event_code,source_event_id,message,original_severity,current_severity," \
   L"alarm_key,creation_time,last_change_time,hd_state,hd_ref,ack_by,repeat_count,alarm_state,timeout,timeout_event," \
   L"resolved_by,ack_timeout,dci_id,alarm_category_ids,rule_guid,rule_description,parent_alarm_id,event_tags," \
   L"rca_script_name,impact,last_state_change_time"

/**
 * Alarm comments constructor
 */
AlarmComment::AlarmComment(uint32_t id, time_t changeTime, uint32_t userId, TCHAR *text)
{
   m_id = id;
   m_changeTime = changeTime;
   m_userId = userId;
   m_text = text;
}

/**
 * Alarm severity information for object
 */
class ObjectAlarmSeverity
{
private:
   Mutex m_mutex;
   StructArray<std::pair<uint32_t, int>> m_severities;
   int m_mostCritical;

   int find(uint32_t alarmId)
   {
      for(int i = 0; i < m_severities.size(); i++)
         if (m_severities.get(i)->first == alarmId)
            return i;
      return -1;
   }

   void updateMostCritical()
   {
      m_mostCritical = -1;
      for(int i = 0; (i < m_severities.size()) && (m_mostCritical < STATUS_CRITICAL); i++)
         if (m_severities.get(i)->second > m_mostCritical)
            m_mostCritical = m_severities.get(i)->second;
   }

public:
   ObjectAlarmSeverity() : m_mutex(MutexType::FAST)
   {
      m_mostCritical = -1;
   }

   void updateAlarm(uint32_t alarmId, int severity)
   {
      LockGuard lockGuard(m_mutex);
      int index = find(alarmId);
      if (index == -1)
      {
         m_severities.add(std::pair<uint32_t, int>(alarmId, severity));
         if (severity > m_mostCritical)
            m_mostCritical = severity;
      }
      else if (m_severities.get(index)->second != severity)
      {
         if (m_severities.get(index)->second > severity)
         {
            m_severities.get(index)->second = severity;
            updateMostCritical();
         }
         else
         {
            m_severities.get(index)->second = severity;
            if (severity > m_mostCritical)
               m_mostCritical = severity;
         }
      }
   }

   void removeAlarm(uint32_t alarmId)
   {
      LockGuard lockGuard(m_mutex);
      int index = find(alarmId);
      if (index == -1)
         return;
      int severity = m_severities.get(index)->second;
      m_severities.remove(index);
      if (severity >= m_mostCritical)
         updateMostCritical();
   }

   int getMostCritical() const
   {
      return (m_mostCritical != -1) ? m_mostCritical : STATUS_UNKNOWN;
   }

   std::array<int, 5> getStats() const
   {
      LockGuard lockGuard(m_mutex);
      std::array<int, 5> stats = { 0, 0, 0, 0, 0 };
      for(int i = 0; i < m_severities.size(); i++)
         stats[m_severities.get(i)->second]++;
      return stats;
   }
};

/**
 * Alarm severity summary by object
 */
static SharedPointerIndex<ObjectAlarmSeverity> s_alarmSeverityByObject;

/**
 * Get most critical status among active alarms for given object
 * Will return STATUS_UNKNOWN if there are no active alarms
 */
int GetMostCriticalAlarmForObject(uint32_t objectId)
{
   shared_ptr<ObjectAlarmSeverity> s = s_alarmSeverityByObject.get(objectId);
   return (s != nullptr) ? s->getMostCritical() : STATUS_UNKNOWN;
}

/**
 * Get alarm statistic for object
 */
std::array<int, 5> GetAlarmStatsForObject(uint32_t objectId)
{
   shared_ptr<ObjectAlarmSeverity> s = s_alarmSeverityByObject.get(objectId);
   return (s != nullptr) ? s->getStats() : std::array<int, 5>({ 0, 0, 0, 0, 0 });
}

/**
 * Force object status recalculation
 */
static inline void RecalculateObjectStatus(uint32_t objectId)
{
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object != nullptr)
      object->calculateCompoundStatus();
}

/**
 * Update object status after alarm creation or update
 */
static void UpdateObjectOnAlarmUpdate(uint32_t objectId, Alarm *alarm, bool recalculateStatus)
{
   shared_ptr<ObjectAlarmSeverity> s = s_alarmSeverityByObject.get(objectId);
   if (s == nullptr)
   {
      s = make_shared<ObjectAlarmSeverity>();
      s_alarmSeverityByObject.put(objectId, s);
   }
   s->updateAlarm(alarm->getAlarmId(), alarm->getCurrentSeverity());

   if (recalculateStatus)
      RecalculateObjectStatus(objectId);
}

/**
 * Update object status after alarm resolution or deletion
 */
static void UpdateObjectOnAlarmResolve(uint32_t objectId, uint32_t alarmId, bool recalculateStatus)
{
   shared_ptr<ObjectAlarmSeverity> s = s_alarmSeverityByObject.get(objectId);
   if (s != nullptr)
      s->removeAlarm(alarmId);

   if (recalculateStatus)
      RecalculateObjectStatus(objectId);
}

/**
 * Alarm list
 */
class AlarmList
{
private:
   Mutex m_lock;
   AbstractIndexWithDestructor<Alarm> m_primaryIndex;
   StringObjectMap<Alarm> m_keyIndex;

public:
   AlarmList() : m_primaryIndex(Ownership::True), m_keyIndex(Ownership::False) { }
   ~AlarmList() { }

   void lock() { m_lock.lock(); }
   void unlock() { m_lock.unlock(); }

   size_t size() { return m_primaryIndex.size(); }

   uint64_t memoryUsage()
   {
      uint64_t memUsage = sizeof(AlarmList);
      lock();
      m_primaryIndex.forEach(
         [&memUsage] (Alarm *a) -> EnumerationCallbackResult
         {
            memUsage += a->getMemoryUsage();
            return _CONTINUE;
         });
      unlock();
      return memUsage;
   }

   Alarm *find(const TCHAR *key) { return m_keyIndex.get(key); }
   Alarm *find(uint32_t id) { return m_primaryIndex.get(id); }
   Alarm *find(std::function<bool (Alarm*)> comparator) { return m_primaryIndex.find(comparator); }

   void forEach(std::function<EnumerationCallbackResult (Alarm*)> callback) { m_primaryIndex.forEach(callback); }
   void forEachKey(std::function<EnumerationCallbackResult (const TCHAR*, Alarm*)> callback) { m_keyIndex.forEach(callback); }

   void add(Alarm *alarm)
   {
      m_primaryIndex.put(alarm->getAlarmId(), alarm);
      if (*alarm->getKey() != 0)
         m_keyIndex.set(alarm->getKey(), alarm);
   }

   void remove(Alarm *alarm)
   {
      if (alarm->getParentAlarmId() != 0)
      {
         Alarm *parent = find(alarm->getParentAlarmId());
         if (parent != nullptr)
            parent->removeSubordinateAlarm(alarm->getAlarmId());
      }
      if (*alarm->getKey() != 0)
         m_keyIndex.remove(alarm->getKey());
      m_primaryIndex.remove(alarm->getAlarmId());
   }
};

/**
 * Global instance of alarm manager
 */
static AlarmList s_alarmList;
static Condition s_shutdown(true);
static THREAD s_watchdogThread = INVALID_THREAD_HANDLE;
static THREAD s_rootCauseUpdateThread = INVALID_THREAD_HANDLE;
static uint32_t s_resolveExpirationTime = 0;
static VolatileCounter64 s_stateChangeLogRecordId = 0;
static bool s_rootCauseUpdateNeeded = false;
static bool s_rootCauseUpdatePossible = false;

/**
 * Callback for client session enumeration
 */
static void SendAlarmNotification(ClientSession *session, std::pair<uint32_t, const Alarm*> *data)
{
   session->onAlarmUpdate(data->first, data->second);
}

/**
 * Callback for client session enumeration
 */
static void SendBulkAlarmTerminateNotification(ClientSession *session, NXCPMessage *msg)
{
   session->sendMessage(msg);
}

/**
 * Notify connected clients about changes
 */
static void NotifyClients(uint32_t code, const Alarm *alarm)
{
   CALL_ALL_MODULES(pfAlarmChangeHook, (code, alarm));

   std::pair<uint32_t, const Alarm*> data(code, alarm);
   EnumerateClientSessions(SendAlarmNotification, &data);
}

/**
 * Notify clients in background
 */
static void NotifyClientsInBackground(Alarm *alarm)
{
   NotifyClients(alarm->getNotificationCode(), alarm);
   delete alarm;
}

/**
 * Background processing data
 */
struct AlarmBackgroundProcessingData
{
   IntegerArray<uint32_t> alarms;
   bool terminate;
   bool sticky;
   bool includeSubordinates;
   uint32_t ackTime;

   AlarmBackgroundProcessingData(const IntegerArray<uint32_t>& _alarms, bool _terminate, bool _includeSubordinates) : alarms(_alarms)
   {
      terminate = _terminate;
      sticky = false;
      ackTime = 0;
      includeSubordinates = _includeSubordinates;
   }

   AlarmBackgroundProcessingData(const IntegerArray<uint32_t>& _alarms, bool _sticky, uint32_t _ackTime, bool _includeSubordinates) : alarms(_alarms)
   {
      terminate = false;
      sticky = _sticky;
      ackTime = _ackTime;
      includeSubordinates = _includeSubordinates;
   }
};

/**
 * Background callback for resolving alarms
 */
static void ResolveAlarmsInBackground(AlarmBackgroundProcessingData *data)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Processing background alarm %s"), data->terminate ? _T("termination") : _T("resolution"));
   IntegerArray<uint32_t> failIds, failCodes;
   ResolveAlarmsById(data->alarms, &failIds, &failCodes, nullptr, data->terminate, data->includeSubordinates);
   delete data;
}

/**
 * Background callback for acknowledging alarms
 */
static void AckAlarmsInBackground(AlarmBackgroundProcessingData *data)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Processing background alarm acknowledgment"));
   for(int i = 0; i < data->alarms.size(); i++)
   {
      AckAlarmById(data->alarms.get(i), nullptr, data->sticky, data->ackTime, data->includeSubordinates);
   }
   delete data;
}

/**
 * Get number of comments for alarm
 */
static uint32_t GetCommentCount(DB_HANDLE hdb, uint32_t alarmId)
{
   uint32_t value = 0;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT count(*) FROM alarm_notes WHERE alarm_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            value = DBGetFieldULong(hResult, 0, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   return value;
}

/**
 * Create new alarm from event
 */
Alarm::Alarm(Event *event, uint32_t parentAlarmId, const TCHAR *rcaScriptName, const uuid& ruleGuid, const TCHAR *ruleDescription,
         const TCHAR *message, const TCHAR *key, const TCHAR *impact, int severity, uint32_t timeout,
         uint32_t timeoutEvent, uint32_t ackTimeout, const IntegerArray<uint32_t>& alarmCategoryList) : m_relatedEvents(0, 64), m_alarmCategoryList(alarmCategoryList)
{
   m_alarmId = CreateUniqueId(IDG_ALARM);
   m_parentAlarmId = parentAlarmId;
   m_rcaScriptName = MemCopyString(rcaScriptName);
   m_sourceEventId = event->getId();
   m_sourceEventCode = event->getCode();
   m_eventTags = MemCopyString(event->getTagsAsList());
   m_ruleGuid = ruleGuid;

   StringBuffer buffer = StringBuffer(ruleDescription);
   buffer.replace(L"\n", L" ");
   buffer.replace(L"\r", L" ");
   buffer.replace(L"\t", L" ");
   buffer.replace(L"  ", L" ");
   wcslcpy(m_ruleDescription, buffer, MAX_DB_STRING);

   m_sourceObject = event->getSourceId();
   m_zoneUIN = event->getZoneUIN();
   m_dciId = event->getDciId();
   m_creationTime = time(nullptr);
   m_lastChangeTime = m_creationTime;
   m_lastStateChangeTime = m_creationTime;
   m_state = ALARM_STATE_OUTSTANDING;
   m_originalSeverity = severity;
   m_currentSeverity = severity;
   m_repeatCount = 1;
   m_helpDeskState = ALARM_HELPDESK_IGNORED;
   m_helpDeskRef[0] = 0;
   m_impact = MemCopyString(impact);
   m_timeout = timeout;
   m_timeoutEvent = timeoutEvent;
   m_commentCount = 0;
   m_ackTimeout = 0;
   m_ackByUser = INVALID_UID;
   m_resolvedByUser = INVALID_UID;
   m_termByUser = INVALID_UID;
   m_relatedEvents.add(event->getId());
   wcslcpy(m_message, message, MAX_EVENT_MSG_LENGTH);
   wcslcpy(m_key, key, MAX_DB_STRING);
   m_notificationCode = 0;
}

/**
 * Create alarm object from database record
 */
Alarm::Alarm(DB_HANDLE hdb, DB_RESULT hResult, int row) : m_relatedEvents(16, 16)
{
   m_alarmId = DBGetFieldULong(hResult, row, 0);
   m_sourceObject = DBGetFieldULong(hResult, row, 1);
   m_zoneUIN = DBGetFieldULong(hResult, row, 2);
   m_sourceEventCode = DBGetFieldULong(hResult, row, 3);
   m_sourceEventId = DBGetFieldUInt64(hResult, row, 4);
   DBGetField(hResult, row, 5, m_message, MAX_EVENT_MSG_LENGTH);
   m_originalSeverity = (BYTE)DBGetFieldLong(hResult, row, 6);
   m_currentSeverity = (BYTE)DBGetFieldLong(hResult, row, 7);
   DBGetField(hResult, row, 8, m_key, MAX_DB_STRING);
   m_creationTime = DBGetFieldULong(hResult, row, 9);
   m_lastChangeTime = DBGetFieldULong(hResult, row, 10);
   m_helpDeskState = (BYTE)DBGetFieldLong(hResult, row, 11);
   DBGetField(hResult, row, 12, m_helpDeskRef, MAX_HELPDESK_REF_LEN);
   m_ackByUser = DBGetFieldULong(hResult, row, 13);
   m_repeatCount = DBGetFieldULong(hResult, row, 14);
   m_state = (BYTE)DBGetFieldLong(hResult, row, 15);
   m_timeout = DBGetFieldULong(hResult, row, 16);
   m_timeoutEvent = DBGetFieldULong(hResult, row, 17);
   m_resolvedByUser = DBGetFieldULong(hResult, row, 18);
   m_ackTimeout = DBGetFieldULong(hResult, row, 19);
   m_dciId = DBGetFieldULong(hResult, row, 20);

   TCHAR categoryList[MAX_DB_STRING];
   DBGetField(hResult, row, 21, categoryList, MAX_DB_STRING);
   if (categoryList[0] != 0)
   {
      int count;
      TCHAR **ids = SplitString(categoryList, _T(','), &count);
      for(int i = 0; i < count; i++)
      {
         m_alarmCategoryList.add(_tcstoul(ids[i], nullptr, 10));
         MemFree(ids[i]);
      }
      MemFree(ids);
   }

   m_ruleGuid = DBGetFieldGUID(hResult, row, 22);
   DBGetField(hResult, row, 23, m_ruleDescription, MAX_DB_STRING);
   m_parentAlarmId = DBGetFieldULong(hResult, row, 24);
   m_eventTags = DBGetField(hResult, row, 25, nullptr, 0);
   m_rcaScriptName = DBGetField(hResult, row, 26, nullptr, 0);
   m_impact = DBGetField(hResult, row, 27, nullptr, 0);
   m_lastStateChangeTime = DBGetFieldULong(hResult, row, 28);
   m_notificationCode = 0;
   m_commentCount = GetCommentCount(hdb, m_alarmId);
   m_termByUser = 0;

   wchar_t query[256];
   _sntprintf(query, 256, L"SELECT event_id FROM alarm_events WHERE alarm_id=%u", m_alarmId);
   DB_RESULT eventResult = DBSelect(hdb, query);
   if (eventResult != nullptr)
   {
      int count = DBGetNumRows(eventResult);
      for(int j = 0; j < count; j++)
      {
         m_relatedEvents.add(DBGetFieldUInt64(eventResult, j, 0));
      }
      DBFreeResult(eventResult);
   }
}

/**
 * Copy constructor
 */
Alarm::Alarm(const Alarm *src, bool copyEvents, uint32_t notificationCode) : m_relatedEvents(16, 16), m_alarmCategoryList(src->m_alarmCategoryList), m_subordinateAlarms(src->m_subordinateAlarms)
{
   m_sourceEventId = src->m_sourceEventId;
   m_alarmId = src->m_alarmId;
   m_parentAlarmId = src->m_parentAlarmId;
   m_rcaScriptName = MemCopyString(src->m_rcaScriptName);
   m_creationTime = src->m_creationTime;
   m_lastChangeTime = src->m_lastChangeTime;
   m_lastStateChangeTime = src->m_lastStateChangeTime;
   m_ruleGuid = src->m_ruleGuid;
   wcscpy(m_ruleDescription, src->m_ruleDescription);
   m_sourceObject = src->m_sourceObject;
   m_zoneUIN = src->m_zoneUIN;
   m_sourceEventCode = src->m_sourceEventCode;
   m_eventTags = MemCopyString(src->m_eventTags);
   m_dciId = src->m_dciId;
   m_currentSeverity = src->m_currentSeverity;
   m_originalSeverity = src->m_originalSeverity;
   m_state = src->m_state;
   m_helpDeskState = src->m_helpDeskState;
   m_ackByUser = src->m_ackByUser;
   m_resolvedByUser = src->m_resolvedByUser;
   m_termByUser = src->m_termByUser;
   m_ackTimeout = src->m_ackTimeout;
   m_repeatCount = src->m_repeatCount;
   m_timeout = src->m_timeout;
   m_timeoutEvent = src->m_timeoutEvent;
   wcscpy(m_message, src->m_message);
   wcscpy(m_key, src->m_key);
   wcscpy(m_helpDeskRef, src->m_helpDeskRef);
   m_impact = MemCopyString(src->m_impact);
   m_commentCount = src->m_commentCount;
   if (copyEvents)
   {
      m_relatedEvents.addAll(src->m_relatedEvents);
   }
   m_notificationCode = notificationCode;
}

/**
 * Alarm destructor
 */
Alarm::~Alarm()
{
   MemFree(m_eventTags);
   MemFree(m_impact);
   MemFree(m_rcaScriptName);
}

/**
 * Execute alarm state change hook script in separate thread
 */
static void ExecuteHookScript(NXSL_VM *vm)
{
   if (!vm->run())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Alarm::executeHookScript: hook script execution error (%s)", vm->getErrorText());
      ReportScriptError(SCRIPT_CONTEXT_ALARM, nullptr, 0, vm->getErrorText(), L"Hook::AlarmStateChange");
   }
   delete vm;
}

/**
 * Execute hook script when alarm state changes
 */
void Alarm::executeHookScript()
{
   ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::AlarmStateChange"), shared_ptr<NetObj>());
   if (!vm.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Alarm::executeHookScript: hook script %s"),
               (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
      return;
   }

   vm->setGlobalVariable("$alarm", vm->createValue(vm->createObject(&g_nxslAlarmClass, new Alarm(this, false))));
   ThreadPoolExecute(g_mainThreadPool, ExecuteHookScript, vm.vm());
}

/**
 * Get approximate memory usage by this alarm
 */
uint64_t Alarm::getMemoryUsage() const
{
   uint64_t mu = sizeof(Alarm) + m_alarmCategoryList.memoryUsage() + m_subordinateAlarms.memoryUsage() + m_relatedEvents.memoryUsage();
   if (m_rcaScriptName != nullptr)
      mu += (_tcslen(m_rcaScriptName) + 1) * sizeof(TCHAR);
   if (m_impact != nullptr)
      mu += (_tcslen(m_impact) + 1) * sizeof(TCHAR);
   if (m_eventTags != nullptr)
      mu += (_tcslen(m_eventTags) + 1) * sizeof(TCHAR);
   return mu;
}

/**
 * Add subordinate alarm to list
 */
void Alarm::addSubordinateAlarm(uint32_t alarmId)
{
   if (!m_subordinateAlarms.contains(alarmId))
      m_subordinateAlarms.add(alarmId);
}

/**
 * Remove subordinate alarm from list
 */
void Alarm::removeSubordinateAlarm(uint32_t alarmId)
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Removing subordinate alarm %u from alarm %u"), alarmId, m_alarmId);
   m_subordinateAlarms.remove(m_subordinateAlarms.indexOf(alarmId));
   ThreadPoolExecute(g_mainThreadPool, NotifyClientsInBackground, new Alarm(this, false, NX_NOTIFY_ALARM_CHANGED));
}

/**
 * Get list of related event identifiers as NXSL array
 */
NXSL_Value *Alarm::relatedEventsToNXSLArray(NXSL_VM *vm) const
{
   NXSL_Array *a = new NXSL_Array(vm);
   for(int i = 0; i < m_relatedEvents.size(); i++)
   {
      a->append(vm->createValue(m_relatedEvents.get(i)));
   }
   return vm->createValue(a);
}

/**
 * Convert alarm category list to string
 */
StringBuffer Alarm::categoryListToString()
{
   StringBuffer buffer;
   for(int i = 0; i < m_alarmCategoryList.size(); i++)
   {
      if (buffer.length() > 0)
         buffer.append(_T(','));
      buffer.append(m_alarmCategoryList.get(i));
   }
   return buffer;
}

/**
 * Convert alarm category list to NXSL array
 */
NXSL_Value *Alarm::categoryListToNXSLArray(NXSL_VM *vm) const
{
   NXSL_Array *a = new NXSL_Array(vm);
   for(int i = 0; i < m_alarmCategoryList.size(); i++)
   {
      AlarmCategory *c = GetAlarmCategory(m_alarmCategoryList.get(i));
      if (c != nullptr)
      {
         a->append(vm->createValue(c->getName()));
         delete c;
      }
   }
   return vm->createValue(a);
}

/**
 * Check alarm category access
 */
bool Alarm::checkCategoryAccess(uint32_t userId, uint64_t systemAccessRights) const
{
   if (userId == 0)
      return true;

   if (systemAccessRights & SYSTEM_ACCESS_VIEW_ALL_ALARMS)
      return true;

   for(int i = 0; i < m_alarmCategoryList.size(); i++)
   {
      if (CheckAlarmCategoryAccess(userId, m_alarmCategoryList.get(i)))
         return true;
   }
   return false;
}

/**
 * Check alarm category access
 */
bool Alarm::checkCategoryAccess(GenericClientSession *session) const
{
   return checkCategoryAccess(session->getUserId(), session->getSystemAccessRights());
}

/**
 * Create alarm record in database
 */
void Alarm::createInDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
              _T("INSERT INTO alarms (alarm_id,parent_alarm_id,creation_time,last_change_time,source_object_id,zone_uin,")
              _T("source_event_code,message,original_severity,current_severity,alarm_key,alarm_state,ack_by,resolved_by,")
              _T("hd_state,hd_ref,repeat_count,term_by,timeout,timeout_event,source_event_id,ack_timeout,dci_id,")
              _T("alarm_category_ids,rule_guid,rule_description,event_tags,rca_script_name,impact,last_state_change_time) ")
              _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_alarmId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_parentAlarmId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastChangeTime));
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_sourceObject);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_zoneUIN);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_sourceEventCode);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_message, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_originalSeverity));
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_currentSeverity));
      DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_key, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_state));
      DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_ackByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_resolvedByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_helpDeskState));
      DBBind(hStmt, 16, DB_SQLTYPE_VARCHAR, m_helpDeskRef, DB_BIND_STATIC, MAX_HELPDESK_REF_LEN);
      DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_repeatCount);
      DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_termByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_timeout);
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, m_timeoutEvent);
      DBBind(hStmt, 21, DB_SQLTYPE_BIGINT, m_sourceEventId);
      DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_ackTimeout));
      DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_dciId);
      DBBind(hStmt, 24, DB_SQLTYPE_VARCHAR, categoryListToString(), DB_BIND_TRANSIENT);
      if (!m_ruleGuid.isNull())
      {
         DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_ruleGuid);
      }
      else
      {
         DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
      }
      DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_ruleDescription, DB_BIND_STATIC);
      DBBind(hStmt, 27, DB_SQLTYPE_VARCHAR, m_eventTags, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, m_rcaScriptName, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 29, DB_SQLTYPE_VARCHAR, m_impact, DB_BIND_STATIC, 1000);
      DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastStateChangeTime));

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update alarm information in database
 */
void Alarm::updateInDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
            _T("UPDATE alarms SET alarm_state=?,ack_by=?,term_by=?,last_change_time=?,current_severity=?,repeat_count=?,")
            _T("hd_state=?,hd_ref=?,timeout=?,timeout_event=?,message=?,resolved_by=?,ack_timeout=?,source_object_id=?,")
            _T("dci_id=?,alarm_category_ids=?,rule_guid=?,rule_description=?,event_tags=?,parent_alarm_id=?,rca_script_name=?,")
            _T("impact=?,last_state_change_time=? WHERE alarm_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_state));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_ackByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_termByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastChangeTime));
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_currentSeverity));
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_repeatCount);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_helpDeskState));
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_helpDeskRef, DB_BIND_STATIC, MAX_HELPDESK_REF_LEN);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_timeout);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_timeoutEvent);
      DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_message, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH);
      DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_resolvedByUser));  // cast to signed to save -1 for invalid UID
      DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_ackTimeout));
      DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_sourceObject);
      DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_dciId);
      DBBind(hStmt, 16, DB_SQLTYPE_VARCHAR, categoryListToString(), DB_BIND_TRANSIENT);
      if (!m_ruleGuid.isNull())
      {
         DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_ruleGuid);
      }
      else
      {
         DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
      }
      DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_ruleDescription, DB_BIND_STATIC);
      DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, m_eventTags, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, m_parentAlarmId);
      DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_rcaScriptName, DB_BIND_STATIC, MAX_DB_STRING);
      DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_impact, DB_BIND_STATIC, 1000);
      DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastStateChangeTime));
      DBBind(hStmt, 24, DB_SQLTYPE_INTEGER, m_alarmId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

	if (m_state == ALARM_STATE_TERMINATED)
	{
	   wchar_t query[256];
		_sntprintf(query, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%u"), m_alarmId);
		QueueSQLRequest(query);
	}
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update alarm state change log
 */
void Alarm::updateStateChangeLog(int prevState, uint32_t userId)
{
   wchar_t valRecordId[32], valAlarmId[16], valPrevState[16], valNewState[16], valChangeTime[16], valDuration[16], valChangeBy[16];
   const wchar_t *values[7] = { valRecordId, valAlarmId, valPrevState, valNewState, valChangeTime, valDuration, valChangeBy };
   IntegerToString(InterlockedIncrement64(&s_stateChangeLogRecordId), valRecordId);
   IntegerToString(m_alarmId, valAlarmId);
   IntegerToString(prevState, valPrevState);
   IntegerToString(static_cast<int32_t>(m_state & ALARM_STATE_MASK), valNewState);
   IntegerToString(static_cast<uint32_t>(m_lastChangeTime), valChangeTime);
   if (m_lastStateChangeTime < m_lastChangeTime)
      IntegerToString(static_cast<int32_t>(m_lastChangeTime - m_lastStateChangeTime), valDuration);
   else
      memcpy(valDuration, L"0", sizeof(wchar_t) * 2);
   IntegerToString(userId, valChangeBy);
   static int sqlTypes[7] = { DB_SQLTYPE_BIGINT, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER };
   QueueSQLRequest(L"INSERT INTO alarm_state_changes (record_id,alarm_id,prev_state,new_state,change_time,prev_state_duration,change_by) VALUES (?,?,?,?,?,?,?)", 7, sqlTypes, values);
   m_lastStateChangeTime = m_lastChangeTime;
}

/**
 * Fill NXCP message with alarm data
 */
void Alarm::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_ALARM_ID, m_alarmId);
   msg->setField(VID_PARENT_ALARM_ID, m_parentAlarmId);
   msg->setField(VID_RCA_SCRIPT_NAME, m_rcaScriptName);
   msg->setField(VID_ACK_BY_USER, m_ackByUser);
   msg->setField(VID_RESOLVED_BY_USER, m_resolvedByUser);
   msg->setField(VID_TERMINATED_BY_USER, m_termByUser);
   msg->setField(VID_RULE_ID, m_ruleGuid);
   msg->setField(VID_RULE_DESCRIPTION, m_ruleDescription);
   msg->setField(VID_EVENT_CODE, m_sourceEventCode);
   msg->setField(VID_EVENT_ID, m_sourceEventId);
   msg->setField(VID_TAGS, m_eventTags);
   msg->setField(VID_OBJECT_ID, m_sourceObject);
   msg->setField(VID_DCI_ID, m_dciId);
   msg->setFieldFromTime(VID_CREATION_TIME, m_creationTime);
   msg->setFieldFromTime(VID_LAST_CHANGE_TIME, m_lastChangeTime);
   msg->setField(VID_ALARM_KEY, m_key);
   msg->setField(VID_ALARM_MESSAGE, m_message);
   msg->setField(VID_IMPACT, m_impact);
   msg->setField(VID_STATE, (WORD)(m_state & ALARM_STATE_MASK)); // send only state to client, without flags
   msg->setField(VID_IS_STICKY, (WORD)((m_state & ALARM_STATE_STICKY) ? 1 : 0));
   msg->setField(VID_CURRENT_SEVERITY, (WORD)m_currentSeverity);
   msg->setField(VID_ORIGINAL_SEVERITY, (WORD)m_originalSeverity);
   msg->setField(VID_HELPDESK_STATE, (WORD)m_helpDeskState);
   msg->setField(VID_HELPDESK_REF, m_helpDeskRef);
   msg->setField(VID_REPEAT_COUNT, m_repeatCount);
   msg->setField(VID_ALARM_TIMEOUT, m_timeout);
   msg->setField(VID_ALARM_TIMEOUT_EVENT, m_timeoutEvent);
   msg->setField(VID_NUM_COMMENTS, m_commentCount);
   msg->setField(VID_TIMESTAMP, (UINT32)((m_ackTimeout != 0) ? (m_ackTimeout - time(nullptr)) : 0));
   msg->setFieldFromInt32Array(VID_CATEGORY_LIST, m_alarmCategoryList);
   msg->setFieldFromInt32Array(VID_SUBORDINATE_ALARMS, m_subordinateAlarms);
   if (m_notificationCode != 0)
      msg->setField(VID_NOTIFICATION_CODE, m_notificationCode);
}

/**
 * Create JSON representation
 */
json_t *Alarm::toJson() const
{
   TCHAR buffer[256];

   json_t *root = json_object();

   json_object_set_new(root, "id", json_integer(m_alarmId));
   json_object_set_new(root, "parentId", json_integer(m_parentAlarmId));
   json_object_set_new(root, "rcaScriptName", json_string_t(m_rcaScriptName));
   json_object_set_new(root, "ackByUser", json_integer(m_ackByUser));
   json_object_set_new(root, "resolvedByUser", json_integer(m_resolvedByUser));
   json_object_set_new(root, "terminatedByUser", json_integer(m_termByUser));
   json_object_set_new(root, "ruleGUID", m_ruleGuid.toJson());
   json_object_set_new(root, "ruleDescription", json_string_t(m_ruleDescription));
   json_object_set_new(root, "eventCode", json_integer(m_sourceEventCode));
   if (EventNameFromCode(m_sourceEventCode, buffer))
   {
      json_object_set_new(root, "eventName", json_string_t(buffer));
   }
   json_object_set_new(root, "eventId", json_integer(m_sourceEventId));
   json_object_set_new(root, "eventTags", json_string_t(m_eventTags));
   json_object_set_new(root, "source", json_integer(m_sourceObject));
   json_object_set_new(root, "dci", json_integer(m_dciId));
   json_object_set_new(root, "creationTime", json_time_string(m_creationTime));
   json_object_set_new(root, "lastChangeTime", json_time_string(m_lastChangeTime));
   json_object_set_new(root, "key", json_string_t(m_key));
   json_object_set_new(root, "message", json_string_t(m_message));
   json_object_set_new(root, "impact", json_string_t(m_impact));
   json_object_set_new(root, "state", json_integer(m_state & ALARM_STATE_MASK));
   json_object_set_new(root, "isSticky", json_boolean(m_state & ALARM_STATE_STICKY));
   json_object_set_new(root, "severity", json_integer(m_currentSeverity));
   json_object_set_new(root, "originalSeverity", json_integer(m_originalSeverity));
   json_object_set_new(root, "helpDeskState", json_integer(m_helpDeskState));
   json_object_set_new(root, "repeatCount", json_integer(m_repeatCount));
   json_object_set_new(root, "subordinateAlarms", m_subordinateAlarms.toJson());
   json_object_set_new(root, "categories", m_alarmCategoryList.toJson());

   return root;
}

/**
 * Get AI assistant comment for alarm
 */
String Alarm::requestAIAssistantComment(GenericClientSession *session) const
{
   shared_ptr<NetObj> object = FindObjectById(m_sourceObject);
   if (object == nullptr)
      return String();

   StringBuffer prompt(L"Analyze the following alarm and provide a concise comment that could help a network administrator to understand and resolve the issue. ");
   prompt.append(L"Do not include any disclaimers or apologies in your response. ");
   prompt.append(L"If the alarm message does not provide enough information to make a meaningful comment, simply state that. ");
   prompt.append(L"Do not include alarm message in your response. ");
   prompt.append(L"Use additional tools and data sources if needed to enrich your analysis.\n");
   prompt.append(L"Here is the alarm information:\n");
   prompt.append(L"Source object name: ").append(object->getName()).append("\n");
   prompt.append(L"Source object ID: ").append(object->getId()).append("\n");
   prompt.append(L"Severity: ").append(AlarmSeverityTextFromCode(m_currentSeverity)).append(L"\n");
   prompt.append(L"Message: ").append(m_message).append(L"\n");
   prompt.append(L"Last change time: ").append(FormatTimestamp(m_lastChangeTime)).append(L"\n");
   char *promptUtf8 = UTF8StringFromWideString(prompt);
   char *response = QueryAIAssistant(promptUtf8, object.get());
   MemFree(promptUtf8);
   if (response == nullptr)
      return String();
   StringBuffer result;
   result.appendUtf8String(response);
   MemFree(response);
   return result.trim();
}

/**
 * Fill NXCP message with event data from SQL query
 * Expected field order: event_id,event_code,event_name,severity,source_object_id,event_timestamp,message
 */
static void FillEventData(NXCPMessage *msg, uint32_t baseId, DB_RESULT hResult, int row, uint64_t rootId)
{
	TCHAR buffer[MAX_EVENT_MSG_LENGTH];

	msg->setField(baseId, DBGetFieldUInt64(hResult, row, 0));
	msg->setField(baseId + 1, rootId);
	msg->setField(baseId + 2, DBGetFieldULong(hResult, row, 1));
	msg->setField(baseId + 3, DBGetField(hResult, row, 2, buffer, MAX_DB_STRING));
	msg->setField(baseId + 4, static_cast<uint16_t>(DBGetFieldLong(hResult, row, 3)));	// severity
	msg->setField(baseId + 5, DBGetFieldULong(hResult, row, 4));  // source object
	msg->setField(baseId + 6, DBGetFieldULong(hResult, row, 5));  // timestamp
	msg->setField(baseId + 7, DBGetField(hResult, row, 6, buffer, MAX_EVENT_MSG_LENGTH));
}

/**
 * Get events correlated to given event into NXCP message
 *
 * @return number of consumed variable identifiers
 */
static uint32_t GetCorrelatedEvents(uint64_t eventId, NXCPMessage *msg, uint32_t baseId, DB_HANDLE hdb)
{
	uint32_t fieldId = baseId;
	DB_STATEMENT hStmt;
	switch(g_dbSyntax)
	{
	   case DB_SYNTAX_ORACLE:
	      hStmt = DBPrepare(hdb,
	               _T("SELECT e.event_id,e.event_code,c.event_name,e.event_severity,e.event_source,e.event_timestamp,e.event_message ")
	               _T("FROM event_log e,event_cfg c WHERE zero_to_null(e.root_event_id)=? AND c.event_code=e.event_code"));
	      break;
	   case DB_SYNTAX_TSDB:
         hStmt = DBPrepare(hdb,
                  _T("SELECT e.event_id,e.event_code,c.event_name,e.event_severity,e.event_source,date_part('epoch',e.event_timestamp)::int,e.event_message ")
                  _T("FROM event_log e,event_cfg c WHERE e.root_event_id=? AND c.event_code=e.event_code"));
         break;
	   default:
         hStmt = DBPrepare(hdb,
                  _T("SELECT e.event_id,e.event_code,c.event_name,e.event_severity,e.event_source,e.event_timestamp,e.event_message ")
                  _T("FROM event_log e,event_cfg c WHERE e.root_event_id=? AND c.event_code=e.event_code"));
         break;
	}
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, eventId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				FillEventData(msg, fieldId, hResult, i, eventId);
				fieldId += 10;
				uint64_t eventId = DBGetFieldUInt64(hResult, i, 0);
				fieldId += GetCorrelatedEvents(eventId, msg, fieldId, hdb);
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	return fieldId - baseId;
}

/**
 * Fill NXCP message with alarm's related events
 */
static void FillAlarmEventsMessage(NXCPMessage *msg, uint32_t alarmId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   const TCHAR *query;
   switch(g_dbSyntax)
   {
		case DB_SYNTAX_ORACLE:
			query = _T("SELECT * FROM (SELECT event_id,event_code,event_name,severity,source_object_id,event_timestamp,message FROM alarm_events WHERE alarm_id=? ORDER BY event_timestamp DESC) WHERE ROWNUM<=200");
			break;
		case DB_SYNTAX_MSSQL:
			query = _T("SELECT TOP 200 event_id,event_code,event_name,severity,source_object_id,event_timestamp,message FROM alarm_events WHERE alarm_id=? ORDER BY event_timestamp DESC");
			break;
		case DB_SYNTAX_DB2:
			query = _T("SELECT event_id,event_code,event_name,severity,source_object_id,event_timestamp,message ")
			   _T("FROM alarm_events WHERE alarm_id=? ORDER BY event_timestamp DESC FETCH FIRST 200 ROWS ONLY");
			break;
		default:
			query = _T("SELECT event_id,event_code,event_name,severity,source_object_id,event_timestamp,message FROM alarm_events WHERE alarm_id=? ORDER BY event_timestamp DESC LIMIT 200");
			break;
	}
	DB_STATEMENT hStmt = DBPrepare(hdb, query);
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			UINT32 varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < count; i++)
			{
				FillEventData(msg, varId, hResult, i, 0);
				varId += 10;
				uint64_t eventId = DBGetFieldUInt64(hResult, i, 0);
				varId += GetCorrelatedEvents(eventId, msg, varId, hdb);
			}
			DBFreeResult(hResult);
			msg->setField(VID_NUM_ELEMENTS, (varId - VID_ELEMENT_LIST_BASE) / 10);
		}
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update existing alarm from event
 */
void Alarm::updateFromEvent(Event *event, uint32_t parentAlarmId, const TCHAR *rcaScriptName, const uuid& ruleGuid, const TCHAR* ruleDescription, int state, int severity, uint32_t timeout, uint32_t timeoutEvent,
         uint32_t ackTimeout, const TCHAR *message, const TCHAR *impact, const IntegerArray<uint32_t>& alarmCategoryList)
{
   m_repeatCount++;
   m_parentAlarmId = parentAlarmId;
   MemFree(m_rcaScriptName);
   m_rcaScriptName = MemCopyString(rcaScriptName);
   m_ruleGuid = ruleGuid;
   StringBuffer buffer = StringBuffer(ruleDescription);
   buffer.replace(_T("\n"), _T(" "));
   buffer.replace(_T("\r"), _T(" "));
   buffer.replace(_T("\t"), _T(" "));
   buffer.replace(_T("  "), _T(" "));
   _tcslcpy(m_ruleDescription, buffer, MAX_DB_STRING);
   m_lastChangeTime = time(nullptr);
   m_sourceObject = event->getSourceId();
   m_dciId = event->getDciId();
   bool stateChanged = false;
   int prevState = m_state & ALARM_STATE_MASK;
   if (((m_state & ALARM_STATE_STICKY) == 0) && (m_state != state))
   {
      m_state = state;
      stateChanged = true;
   }
   m_sourceEventId = event->getId();
   m_sourceEventCode = event->getCode();
   m_currentSeverity = severity;
   m_timeout = timeout;
   m_timeoutEvent = timeoutEvent;
   if ((m_state & ALARM_STATE_STICKY) == 0)
      m_ackTimeout = ackTimeout;
   _tcslcpy(m_message, message, MAX_EVENT_MSG_LENGTH);
   MemFree(m_impact);
   m_impact = MemCopyString(impact);
   m_alarmCategoryList.clear();
   m_alarmCategoryList.addAll(alarmCategoryList);

   NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
   if (stateChanged)
   {
      executeHookScript();
      updateStateChangeLog(prevState, 0);
   }
   updateInDatabase();
}

/**
 * Update parent alarm ID
 */
void Alarm::updateParentAlarm(uint32_t parentAlarmId)
{
   // Update parent's subordinate list if parent is changed
   if (m_parentAlarmId != parentAlarmId)
   {
      Alarm *parent = (m_parentAlarmId != 0) ? s_alarmList.find(m_parentAlarmId) : nullptr;
      if (parent != nullptr)
         parent->removeSubordinateAlarm(m_alarmId);
      parent = (parentAlarmId != 0) ? s_alarmList.find(parentAlarmId) : nullptr;
      if (parent != nullptr)
         parent->addSubordinateAlarm(m_alarmId);
   }
   m_parentAlarmId = parentAlarmId;

   NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
   updateInDatabase();
}

/**
 * Add AI assistant comment to alarm
 */
static void AddAIAssistantComment(uint32_t alarmId)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("AddAIAssistantComment: requesting AI assistant comment for alarm %u"), alarmId);

   Alarm *alarm = FindAlarmById(alarmId);
   if (alarm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("AddAIAssistantComment: alarm %u not found"), alarmId);
      return;
   }

   String comment = alarm->requestAIAssistantComment();
   if (!comment.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("AddAIAssistantComment: adding AI assistant comment to alarm %u (message=\"%s\")"), alarmId, alarm->getMessage());
      nxlog_debug_tag(DEBUG_TAG, 5, _T("AddAIAssistantComment: AI assistant comment: %s"), comment.cstr());
      s_alarmList.lock();
      Alarm *alarm = s_alarmList.find(alarmId);
      if (alarm != nullptr)
      {
         uint32_t commentId;
         alarm->updateAlarmComment(&commentId, comment, 0, true);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("AddAIAssistantComment: alarm %u not found in active alarm list"), alarmId);
      }
      s_alarmList.unlock();
   }

   // FindAlarmById creates a copy of the alarm, so we need to delete it here
   delete alarm;
}

/**
 * Create new alarm
 */
uint32_t NXCORE_EXPORTABLE CreateNewAlarm(const uuid& ruleGuid, const TCHAR *ruleDescription, const TCHAR *message, const TCHAR *key, const TCHAR *impact,
         int severity, uint32_t timeout, uint32_t timeoutEvent, uint32_t parentAlarmId, const TCHAR *rcaScriptName, Event *event,
         uint32_t ackTimeout, const IntegerArray<uint32_t>& alarmCategoryList, bool openHelpdeskIssue, bool addAiComment)
{
   uint32_t alarmId = 0;
   bool newAlarm = true;
   bool updateRelatedEvent = false;

   // Check if we have a duplicate alarm
   if (key[0] != 0)
   {
      s_alarmList.lock();

      Alarm *alarm = s_alarmList.find(key);
      if (alarm != nullptr)
      {
         // Update parent's subordinate list if parent is changed
         if (alarm->getParentAlarmId() != parentAlarmId)
         {
            Alarm *parent = (alarm->getParentAlarmId() != 0) ? s_alarmList.find(alarm->getParentAlarmId()) : nullptr;
            if (parent != nullptr)
               parent->removeSubordinateAlarm(alarm->getAlarmId());
            parent = (parentAlarmId != 0) ? s_alarmList.find(parentAlarmId) : nullptr;
            if (parent != nullptr)
               parent->addSubordinateAlarm(alarm->getAlarmId());
         }
         uint32_t currSourceObject = alarm->getSourceObject();
         alarm->updateFromEvent(event, parentAlarmId, rcaScriptName, ruleGuid, ruleDescription, ALARM_STATE_OUTSTANDING, severity, timeout, timeoutEvent, ackTimeout, message, impact, alarmCategoryList);
         if (!alarm->isEventRelated(event->getId()))
         {
            alarmId = alarm->getAlarmId();      // needed for correct update of related events
            updateRelatedEvent = true;
            alarm->addRelatedEvent(event->getId());
         }

         if (openHelpdeskIssue)
            alarm->openHelpdeskIssue(nullptr);

         if (currSourceObject != alarm->getSourceObject())
            UpdateObjectOnAlarmResolve(currSourceObject, alarm->getAlarmId(), true);
         UpdateObjectOnAlarmUpdate(event->getSourceId(), alarm, true);

         newAlarm = false;
      }

      s_alarmList.unlock();
   }

   if (newAlarm)
   {
      // Create new alarm structure
      Alarm *alarm = new Alarm(event, parentAlarmId, rcaScriptName, ruleGuid, ruleDescription, message, key, impact, severity, timeout, timeoutEvent, ackTimeout, alarmCategoryList);
      alarmId = alarm->getAlarmId();

      // Open helpdesk issue
      if (openHelpdeskIssue)
         alarm->openHelpdeskIssue(nullptr);

      // Add new alarm to active alarm list
      s_alarmList.lock();
      nxlog_debug_tag(DEBUG_TAG, 7, _T("AlarmManager: adding new active alarm, current alarm count %d"), s_alarmList.size());
      s_alarmList.add(alarm);
      UpdateObjectOnAlarmUpdate(event->getSourceId(), alarm, false);
      s_alarmList.unlock();

		alarm->createInDatabase();
      updateRelatedEvent = true;

      if (parentAlarmId != 0)
      {
         Alarm *parent = s_alarmList.find(parentAlarmId);
         if (parent != nullptr)
         {
            parent->addSubordinateAlarm(alarm->getAlarmId());
            NotifyClients(NX_NOTIFY_ALARM_CHANGED, parent);
         }
      }

      if (event->getDciId() != 0)
      {
         shared_ptr<NetObj> object = FindObjectById(event->getSourceId());
         if ((object != nullptr) && object->isDataCollectionTarget())
         {
            shared_ptr<DCObject> dcObject = static_cast<DataCollectionOwner&>(*object).getDCObjectById(event->getDciId(), 0);
            if (dcObject != nullptr)
            {
               SharedString comments = dcObject->getComments();
               if (!comments.isEmpty())
               {
                  uint32_t commentId = 0;
                  alarm->updateAlarmComment(&commentId, comments, 0, true);
               }
            }
         }
      }

      if (addAiComment)
      {
         ThreadPoolExecute(g_mainThreadPool,
            [alarmId] () -> void
            {
               AddAIAssistantComment(alarmId);
            });
      }

      NotifyClients(NX_NOTIFY_NEW_ALARM, alarm);
      RecalculateObjectStatus(event->getSourceId());
   }

   if (updateRelatedEvent)
   {
      // Add record to alarm_events table
      TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16], *msg = nullptr;
      const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, event->getName(), valSeverity, valSource, valTimestamp, event->getMessage() };
      IntegerToString(alarmId, valAlarmId);
      IntegerToString(event->getId(), valEventId);
      IntegerToString(event->getCode(), valEventCode);
      IntegerToString(event->getSeverity(), valSeverity);
      IntegerToString(event->getSourceId(), valSource);
      IntegerToString(static_cast<uint32_t>(event->getTimestamp()), valTimestamp);
      if (_tcslen(values[7]) > MAX_EVENT_MSG_LENGTH)
      {
         msg = MemAllocString(MAX_EVENT_MSG_LENGTH + 1);
         _tcslcpy(msg, values[7], MAX_EVENT_MSG_LENGTH + 1);
         values[7] = msg;
      }
      static int sqlTypes[8] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_BIGINT, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };
      QueueSQLRequest(_T("INSERT INTO alarm_events (alarm_id,event_id,event_code,event_name,severity,source_object_id,event_timestamp,message) VALUES (?,?,?,?,?,?,?,?)"), 8, sqlTypes, values);
      MemFree(msg);
   }

   if (s_rootCauseUpdateNeeded)
   {
      s_rootCauseUpdatePossible = true;
   }
   else if ((parentAlarmId == 0) && (rcaScriptName != nullptr) && (*rcaScriptName != 0))
   {
      // Alarm has root cause analyze script but root cause was not found
      s_rootCauseUpdateNeeded = true;
   }

   return alarmId;
}

/**
 * Do acknowledge
 */
uint32_t Alarm::acknowledge(GenericClientSession *session, bool sticky, uint32_t acknowledgmentActionTime, bool includeSubordinates)
{
   if ((m_state & ALARM_STATE_MASK) != ALARM_STATE_OUTSTANDING)
      return RCC_ALARM_NOT_OUTSTANDING;

   if (session != nullptr)
   {
      session->writeAuditLog(AUDIT_OBJECTS, true, m_sourceObject, _T("Acknowledged alarm %u (%s) on object %s"), m_alarmId, m_message, GetObjectName(m_sourceObject, _T("")));
   }

   uint32_t endTime = acknowledgmentActionTime != 0 ? (uint32_t)time(nullptr) + acknowledgmentActionTime : 0;
   m_ackTimeout = endTime;
   int prevState = m_state & ALARM_STATE_MASK;
   m_state = ALARM_STATE_ACKNOWLEDGED;
	if (sticky)
      m_state |= ALARM_STATE_STICKY;
   m_ackByUser = (session != nullptr) ? session->getUserId() : 0;
   m_lastChangeTime = time(nullptr);
   NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
   updateStateChangeLog(prevState, m_ackByUser);
   updateInDatabase();
   executeHookScript();

   if (includeSubordinates && !m_subordinateAlarms.isEmpty())
      ThreadPoolExecute(g_mainThreadPool, AckAlarmsInBackground, new AlarmBackgroundProcessingData(m_subordinateAlarms, sticky, acknowledgmentActionTime, true));

   return RCC_SUCCESS;
}

/**
 * Acknowledge alarm with given ID
 */
uint32_t NXCORE_EXPORTABLE AckAlarmById(uint32_t alarmId, GenericClientSession *session, bool sticky, uint32_t acknowledgmentActionTime, bool includeSubordinates)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      rcc = alarm->acknowledge(session, sticky, acknowledgmentActionTime, includeSubordinates);
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Acknowledge alarm with given helpdesk reference
 */
uint32_t NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, GenericClientSession *session, bool sticky, uint32_t acknowledgmentActionTime)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(
      [hdref] (Alarm *alarm) -> bool
      {
         return _tcscmp(alarm->getHelpDeskRef(), hdref) == 0;
      });
   if (alarm != nullptr)
   {
      rcc = alarm->acknowledge(session, sticky, acknowledgmentActionTime, false);
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Resolve alarm
 */
void Alarm::resolve(uint32_t userId, Event *event, bool terminate, bool notify, bool includeSubordinates)
{
   if (includeSubordinates && !m_subordinateAlarms.isEmpty())
      ThreadPoolExecute(g_mainThreadPool, ResolveAlarmsInBackground, new AlarmBackgroundProcessingData(m_subordinateAlarms, terminate, true));

   if (event != nullptr)
      event->setLastAlarmMessage(m_message);

   if (terminate)
      m_termByUser = userId;
   else
      m_resolvedByUser = userId;
   m_lastChangeTime = time(nullptr);
   int prevState = m_state & ALARM_STATE_MASK;
   m_state = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
   m_ackTimeout = 0;
   if (m_helpDeskState != ALARM_HELPDESK_IGNORED)
      m_helpDeskState = ALARM_HELPDESK_CLOSED;
   if (notify)
      NotifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, this);
   updateStateChangeLog(prevState, userId);
   updateInDatabase();
   executeHookScript();

   if (!terminate && (event != nullptr) && !m_relatedEvents.contains(event->getId()))
   {
      // Add record to alarm_events table if alarm is resolved
      m_relatedEvents.add(event->getId());

      wchar_t valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
      const wchar_t *values[8] = { valAlarmId, valEventId, valEventCode, event->getName(), valSeverity, valSource, valTimestamp, event->getMessage() };
      IntegerToString(m_alarmId, valAlarmId);
      IntegerToString(event->getId(), valEventId);
      IntegerToString(event->getCode(), valEventCode);
      IntegerToString(event->getSeverity(), valSeverity);
      IntegerToString(event->getSourceId(), valSource);
      IntegerToString(static_cast<uint32_t>(event->getTimestamp()), valTimestamp);
      static int sqlTypes[8] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_BIGINT, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };
      QueueSQLRequest(L"INSERT INTO alarm_events (alarm_id,event_id,event_code,event_name,severity,source_object_id,event_timestamp,message) VALUES (?,?,?,?,?,?,?,?)", 8, sqlTypes, values);
   }
}

/**
 * Resolve and possibly terminate alarm with given ID
 * Should return RCC which can be sent to client
 */
uint32_t NXCORE_EXPORTABLE ResolveAlarmById(uint32_t alarmId, GenericClientSession *session, bool terminate, bool includeSubordinates)
{
   IntegerArray<uint32_t> list(1), failIds, failCodes;
   list.add(alarmId);
   ResolveAlarmsById(list, &failIds, &failCodes, session, terminate, includeSubordinates);
   return failCodes.isEmpty() ? RCC_SUCCESS : failCodes.get(0);
}

/**
 * Resolve and possibly terminate alarms with given ID
 * Should return RCC which can be sent to client
 */
void NXCORE_EXPORTABLE ResolveAlarmsById(const IntegerArray<uint32_t>& alarmIds, IntegerArray<uint32_t> *failIds,
         IntegerArray<uint32_t> *failCodes, GenericClientSession *session, bool terminate, bool includeSubordinates)
{
   IntegerArray<uint32_t> processedAlarms, updatedObjects;

   s_alarmList.lock();
   time_t changeTime = time(nullptr);
   for(int i = 0; i < alarmIds.size(); i++)
   {
      uint32_t currentId = alarmIds.get(i);

      Alarm *alarm = s_alarmList.find(currentId);
      if (alarm != nullptr)
      {
         // If alarm is open in helpdesk, it cannot be terminated. Check with helpdesk system if it is closed now.
         if (alarm->getHelpDeskState() == ALARM_HELPDESK_OPEN)
         {
            bool isOpen;
            if (GetHelpdeskIssueState(alarm->getHelpDeskRef(), &isOpen) == RCC_SUCCESS)
            {
               if (!isOpen)
                  alarm->onHelpdeskIssueClose();
            }
         }
         if ((alarm->getHelpDeskState() != ALARM_HELPDESK_OPEN) || ConfigReadBoolean(_T("Alarms.IgnoreHelpdeskState"), false))
         {
            if (terminate || (alarm->getState() != ALARM_STATE_RESOLVED))
            {
               // Allow to resolve/terminate alarms for objects that are already deleted
               shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
               bool allowResolve = true;
               if ((session != nullptr) && (object != nullptr))
               {
                  // If user does not have the required object access rights, the alarm cannot be terminated
                  if (object->checkAccessRights(session->getUserId(), terminate ? OBJECT_ACCESS_TERM_ALARMS : OBJECT_ACCESS_UPDATE_ALARMS))
                  {
                     session->writeAuditLog(AUDIT_OBJECTS, true, object->getId(),
                        _T("%s alarm %u (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
                        alarm->getAlarmId(), alarm->getMessage(), object->getName());
                  }
                  else
                  {
                     failIds->add(currentId);
                     failCodes->add(RCC_ACCESS_DENIED);
                     allowResolve = false;
                  }
               }

               if (allowResolve)
               {
                  alarm->resolve((session != nullptr) ? session->getUserId() : 0, nullptr, terminate, false, includeSubordinates);
                  processedAlarms.add(alarm->getAlarmId());
                  if (object != nullptr)
                  {
                     if (!updatedObjects.contains(object->getId()))
                        updatedObjects.add(object->getId());
                  }
                  UpdateObjectOnAlarmResolve(alarm->getSourceObject(), alarm->getAlarmId(), false);
                  if (terminate)
                  {
                     s_alarmList.remove(alarm);
                  }
               }
            }
            else
            {
               // Alarm is already resolved, just mark it as processed
               processedAlarms.add(alarm->getAlarmId());
            }
         }
         else
         {
            failIds->add(currentId);
            failCodes->add(RCC_ALARM_OPEN_IN_HELPDESK);
         }
      }
      else
      {
         failIds->add(currentId);
         failCodes->add(RCC_INVALID_ALARM_ID);
      }
   }
   s_alarmList.unlock();

   NXCPMessage notification(CMD_BULK_ALARM_STATE_CHANGE, 0);
   notification.setField(VID_NOTIFICATION_CODE, terminate ? NX_NOTIFY_MULTIPLE_ALARMS_TERMINATED : NX_NOTIFY_MULTIPLE_ALARMS_RESOLVED);
   notification.setField(VID_USER_ID, (session != nullptr) ? session->getUserId() : 0);
   notification.setFieldFromTime(VID_LAST_CHANGE_TIME, changeTime);
   notification.setFieldFromInt32Array(VID_ALARM_ID_LIST, &processedAlarms);
   EnumerateClientSessions(SendBulkAlarmTerminateNotification, &notification);

   for(int i = 0; i < updatedObjects.size(); i++)
      RecalculateObjectStatus(updatedObjects.get(i));
}

/**
 * Resolve alarm(s) by matching alarm key with regular expression
 */
static void ResolveAlarmByKeyRegexp(const TCHAR *keyPattern, bool terminate, Event *event)
{
   const char *errptr;
   int erroffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(keyPattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
   if (preg != nullptr)
   {
      IntegerArray<uint32_t> objectList;
      ObjectArray<Alarm> matchedAlarms(128, 128, Ownership::False);

      s_alarmList.lock();

      s_alarmList.forEachKey(
         [preg, terminate, &matchedAlarms, &objectList] (const TCHAR *key, Alarm *alarm) -> EnumerationCallbackResult
         {
            int ovector[60];
            if ((_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(key), static_cast<int>(_tcslen(key)), 0, 0, ovector, 60) >= 0) &&
                ((alarm->getHelpDeskState() != ALARM_HELPDESK_OPEN) || ConfigReadBoolean(_T("Alarms.IgnoreHelpdeskState"), false)) &&
                (terminate || (alarm->getState() != ALARM_STATE_RESOLVED)))
            {
               // Add alarm and alarm's source object to update list
               matchedAlarms.add(alarm);
               if (!objectList.contains(alarm->getSourceObject()))
                  objectList.add(alarm->getSourceObject());
            }
            return _CONTINUE;
         });

      // Resolve or terminate matched alarms
      for(int i = 0; i < matchedAlarms.size(); i++)
      {
         Alarm *alarm = matchedAlarms.get(i);
         alarm->resolve(0, event, terminate, true, false);
         UpdateObjectOnAlarmResolve(alarm->getSourceObject(), alarm->getAlarmId(), false);
         if (terminate)
         {
            s_alarmList.remove(alarm);
         }
      }

      s_alarmList.unlock();

      // Update status of objects
      for(int i = 0; i < objectList.size(); i++)
         RecalculateObjectStatus(objectList.get(i));

      _pcre_free_t(preg);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ResolveAlarmByKey: cannot compile regular expression \"%s\" (%hs)"), keyPattern, errptr);
   }
}

/**
 * Resolve alarm with specific key (exact match)
 */
static void ResolveAlarmByKeyExact(const TCHAR *key, bool terminate, Event *event)
{
   uint32_t objectId = 0, alarmId = 0;
   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(key);
   if ((alarm != nullptr) &&
       ((alarm->getHelpDeskState() != ALARM_HELPDESK_OPEN) || ConfigReadBoolean(_T("Alarms.IgnoreHelpdeskState"), false)) &&
       (terminate || (alarm->getState() != ALARM_STATE_RESOLVED)))
   {
      // Add alarm's source object to update list
      objectId = alarm->getSourceObject();
      alarmId = alarm->getAlarmId();

      // Resolve or terminate alarm
      alarm->resolve(0, event, terminate, true, false);
      if (terminate)
      {
         s_alarmList.remove(alarm);
      }
   }
   s_alarmList.unlock();

   if (objectId != 0)
      UpdateObjectOnAlarmResolve(objectId, alarmId, true);
}

/**
 * Resolve and possibly terminate all alarms with given key
 */
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *key, bool useRegexp, bool terminate, Event *event)
{
   if (useRegexp)
      ResolveAlarmByKeyRegexp(key, terminate, event);
   else
      ResolveAlarmByKeyExact(key, terminate, event);
}

/**
 * Resolve and possibly terminate all alarms related to given data collection object
 */
void NXCORE_EXPORTABLE ResolveAlarmByDCObjectId(uint32_t dciId, bool terminate)
{
   IntegerArray<uint32_t> objectList;
   ObjectArray<Alarm> matchedAlarms(128, 128, Ownership::False);

   s_alarmList.lock();
   s_alarmList.forEach(
      [dciId, terminate, &matchedAlarms, &objectList] (Alarm *alarm) -> EnumerationCallbackResult
      {
         if ((alarm->getDciId() == dciId) &&
             ((alarm->getHelpDeskState() != ALARM_HELPDESK_OPEN) || ConfigReadBoolean(_T("Alarms.IgnoreHelpdeskState"), false)) &&
             (terminate || (alarm->getState() != ALARM_STATE_RESOLVED)))
         {
            // Add alarm and alarm's source object to update list
            matchedAlarms.add(alarm);
            if (!objectList.contains(alarm->getSourceObject()))
               objectList.add(alarm->getSourceObject());
         }
         return _CONTINUE;
      });

   // Resolve or terminate matched alarms
   for(int i = 0; i < matchedAlarms.size(); i++)
   {
      Alarm *alarm = matchedAlarms.get(i);
      alarm->resolve(0, nullptr, terminate, true, false);
      UpdateObjectOnAlarmResolve(alarm->getSourceObject(), alarm->getAlarmId(), false);
      if (terminate)
      {
         s_alarmList.remove(alarm);
      }
   }

   s_alarmList.unlock();

   // Update status of objects
   for (int i = 0; i < objectList.size(); i++)
      RecalculateObjectStatus(objectList.get(i));
}

/**
 * Resolve and possibly terminate alarm with given helpdesk reference.
 * Automatically change alarm's helpdesk state to "closed"
 */
uint32_t NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, GenericClientSession *session, bool terminate)
{
   uint32_t objectId = 0, alarmId = 0;
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(
      [hdref] (Alarm *alarm) -> bool
      {
         return _tcscmp(alarm->getHelpDeskRef(), hdref) == 0;
      });
   if (alarm != nullptr)
   {
      if (terminate || (alarm->getState() != ALARM_STATE_RESOLVED))
      {
         objectId = alarm->getSourceObject();
         alarmId = alarm->getAlarmId();
         if (session != nullptr)
         {
            session->writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("%s alarm %u (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
                  alarm->getAlarmId(), alarm->getMessage(), GetObjectName(objectId, _T("")));
         }

         alarm->resolve((session != nullptr) ? session->getUserId() : 0, nullptr, terminate, true, false);
         if (terminate)
         {
            s_alarmList.remove(alarm);
         }
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Alarm with helpdesk reference \"%s\" %s"), hdref, terminate ? _T("terminated") : _T("resolved"));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Alarm with helpdesk reference \"%s\" already resolved"), hdref);
      }
      rcc = RCC_SUCCESS;
   }
   s_alarmList.unlock();

   if (objectId != 0)
      UpdateObjectOnAlarmResolve(objectId, alarmId, true);
   return rcc;
}

/**
 * Resolve alarm by helpdesk reference
 */
uint32_t NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref)
{
   return ResolveAlarmByHDRef(hdref, nullptr, false);
}

/**
 * Terminate alarm by helpdesk reference
 */
uint32_t NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref)
{
   return ResolveAlarmByHDRef(hdref, nullptr, true);
}

/**
 * Open issue in helpdesk system
 */
uint32_t Alarm::openHelpdeskIssue(TCHAR *hdref)
{
   uint32_t rcc;
   if (m_helpDeskState == ALARM_HELPDESK_IGNORED)
   {
      /* TODO: unlock alarm list before call */
      const TCHAR *nodeName = GetObjectName(m_sourceObject, _T("[unknown]"));
      size_t messageLen = _tcslen(nodeName) + _tcslen(m_message) + 32;
      TCHAR *message = MemAllocArrayNoInit<TCHAR>(messageLen);
      _sntprintf(message, messageLen, _T("%s: %s"), nodeName, m_message);
      rcc = CreateHelpdeskIssue(message, m_helpDeskRef);
      MemFree(message);
      if (rcc == RCC_SUCCESS)
      {
         m_helpDeskState = ALARM_HELPDESK_OPEN;
         NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
         updateInDatabase();
         if (hdref != nullptr)
            _tcslcpy(hdref, m_helpDeskRef, MAX_HELPDESK_REF_LEN);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Helpdesk issue created for alarm %d, reference \"%s\""), m_alarmId, m_helpDeskRef);
      }
   }
   else
   {
      rcc = RCC_OUT_OF_STATE_REQUEST;
   }
   return rcc;
}

/**
 * Open issue in helpdesk system
 */
uint32_t OpenHelpdeskIssue(uint32_t alarmId, GenericClientSession *session, TCHAR *hdref)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;
   *hdref = 0;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      if (alarm->checkCategoryAccess(session))
         rcc = alarm->openHelpdeskIssue(hdref);
      else
         rcc = RCC_ACCESS_DENIED;
   }
   s_alarmList.unlock();
   return rcc;
}

/**
 * Get helpdesk issue URL for given alarm
 */
uint32_t GetHelpdeskIssueUrlFromAlarm(uint32_t alarmId, uint32_t userId, TCHAR *url, size_t size, GenericClientSession *session)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      if (alarm->checkCategoryAccess(session))
      {
         if ((alarm->getHelpDeskState() != ALARM_HELPDESK_IGNORED) && (alarm->getHelpDeskRef()[0] != 0))
         {
            rcc = GetHelpdeskIssueUrl(alarm->getHelpDeskRef(), url, size);
         }
         else
         {
            rcc = RCC_OUT_OF_STATE_REQUEST;
         }
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }
   s_alarmList.unlock();
   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
uint32_t UnlinkHelpdeskIssueById(uint32_t alarmId, GenericClientSession *session)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      if (session != nullptr)
      {
         session->writeAuditLog(AUDIT_OBJECTS, true,
            alarm->getSourceObject(), _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"),
            alarm->getHelpDeskRef(), alarm->getAlarmId(), alarm->getMessage(),
            GetObjectName(alarm->getSourceObject(), _T("")));
      }
      alarm->unlinkFromHelpdesk();
      NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
      alarm->updateInDatabase();
      rcc = RCC_SUCCESS;
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
uint32_t UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, GenericClientSession *session)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(
      [hdref] (Alarm *alarm) -> bool
      {
         return _tcscmp(alarm->getHelpDeskRef(), hdref) == 0;
      });
   if (alarm != nullptr)
   {
      if (session != nullptr)
      {
         session->writeAuditLog(AUDIT_OBJECTS, true,
            alarm->getSourceObject(), _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"),
            alarm->getHelpDeskRef(), alarm->getAlarmId(), alarm->getMessage(),
            GetObjectName(alarm->getSourceObject(), _T("")));
      }
      alarm->unlinkFromHelpdesk();
      NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
      alarm->updateInDatabase();
      rcc = RCC_SUCCESS;
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Delete alarm with given ID
 */
void NXCORE_EXPORTABLE DeleteAlarm(uint32_t alarmId, bool objectCleanup)
{
   uint32_t objectId;
   bool found = false;

   // Delete alarm from in-memory list
   if (!objectCleanup)  // otherwise already locked
      s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      objectId = alarm->getSourceObject();
      NotifyClients(NX_NOTIFY_ALARM_DELETED, alarm);
      s_alarmList.remove(alarm);
      found = true;
   }
   if (!objectCleanup)
      s_alarmList.unlock();

   // Delete from database
   if (found && !objectCleanup)
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, 256, _T("DELETE FROM alarms WHERE alarm_id=%u"), alarmId);
      QueueSQLRequest(szQuery);

      _sntprintf(szQuery, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%u"), alarmId);
      QueueSQLRequest(szQuery);

      _sntprintf(szQuery, 256, _T("DELETE FROM alarm_notes WHERE alarm_id=%u"), alarmId);
      QueueSQLRequest(szQuery);

      _sntprintf(szQuery, 256, _T("DELETE FROM alarm_state_changes WHERE alarm_id=%u"), alarmId);
      QueueSQLRequest(szQuery);

      UpdateObjectOnAlarmResolve(objectId, alarmId, true);
   }
}

/**
 * Delete all alarms of given object. Intended to be called only
 * on final stage of object deletion.
 */
bool DeleteObjectAlarms(uint32_t objectId, DB_HANDLE hdb)
{
   IntegerArray<uint32_t> deleteList;

	s_alarmList.lock();
	s_alarmList.forEach(
	   [objectId, &deleteList] (Alarm *alarm) -> EnumerationCallbackResult
	   {
	      if (alarm->getSourceObject() == objectId)
	         deleteList.add(alarm->getAlarmId());
	      return _CONTINUE;
	   });
	for(int i = 0; i < deleteList.size(); i++)
   {
      DeleteAlarm(deleteList.get(i), true);
	}
	s_alarmList.unlock();

   // Delete all object alarms from database
   bool success = false;
	DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT alarm_id FROM alarms WHERE source_object_id=?");
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
         success = true;
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
         {
            uint32_t alarmId = DBGetFieldULong(hResult, i, 0);
            ExecuteQueryOnObject(hdb, alarmId, L"DELETE FROM alarm_notes WHERE alarm_id=?");
            ExecuteQueryOnObject(hdb, alarmId, L"DELETE FROM alarm_events WHERE alarm_id=?");
            ExecuteQueryOnObject(hdb, alarmId, L"DELETE FROM alarm_state_changes WHERE alarm_id=?");
         }
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

   if (success)
      success = ExecuteQueryOnObject(hdb, objectId, L"DELETE FROM alarms WHERE source_object_id=?");

   return success;
}

/**
 * Send all alarms to client
 */
void SendAlarmsToClient(uint32_t requestId, ClientSession *session)
{
   uint32_t userId = session->getUserId();

   // Prepare message
   NXCPMessage msg(CMD_ALARM_DATA, requestId);

   ObjectArray<Alarm> *alarms = GetAlarms();
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          object->checkAccessRights(userId, OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(session))
      {
         alarm->fillMessage(&msg);
         session->sendMessage(msg);
         msg.deleteAllFields();
      }
   }
   delete alarms;

   // Send end-of-list indicator
   msg.setField(VID_ALARM_ID, (uint32_t)0);
   session->sendMessage(msg);
}

/**
 * Get alarm or load from database with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
uint32_t NXCORE_EXPORTABLE GetAlarm(uint32_t alarmId, NXCPMessage *msg, GenericClientSession *session)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      if (alarm->checkCategoryAccess(session))
      {
         alarm->fillMessage(msg);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }
   s_alarmList.unlock();

   if (rcc == RCC_INVALID_ALARM_ID)
   {
      Alarm *alarm = LoadAlarmFromDatabase(alarmId);
      if (alarm != nullptr)
      {
         if (alarm->checkCategoryAccess(session))
         {
            alarm->fillMessage(msg);
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_ACCESS_DENIED;
         }
      }
      delete alarm;
   }

   return rcc;
}

/**
 * Get all related events for alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
uint32_t NXCORE_EXPORTABLE GetAlarmEvents(uint32_t alarmId, NXCPMessage *msg, GenericClientSession *session)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      if (alarm->checkCategoryAccess(session))
      {
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_ACCESS_DENIED;
      }
   }

   s_alarmList.unlock();

   if (rcc == RCC_INVALID_ALARM_ID)
   {
      Alarm *alarm = LoadAlarmFromDatabase(alarmId);
      if (alarm != nullptr)
      {
         if (alarm->checkCategoryAccess(session))
         {
            //No need to fill alarm events as alarm is terminated and there is no event history for it
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_ACCESS_DENIED;
         }
      }
      delete alarm;
   }
   else if (rcc == RCC_SUCCESS)
   {
      // we don't call FillAlarmEventsMessage from within loop
      // to prevent alarm list lock for a long time
      FillAlarmEventsMessage(msg, alarmId);
   }

	return rcc;
}

/**
 * Get source object for given alarm id
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(uint32_t alarmId, bool alreadyLocked, bool useDatabase)
{
   uint32_t objectId = 0;

   if (!alreadyLocked)
      s_alarmList.lock();

   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      objectId = alarm->getSourceObject();
   }

   if (!alreadyLocked)
      s_alarmList.unlock();

   if ((objectId == 0) && useDatabase)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT stmt = DBPrepare(hdb, L"SELECT source_object_id FROM alarms WHERE alarm_id=?");
      if (stmt != nullptr)
      {
         DBBind(stmt, 1, DB_SQLTYPE_INTEGER, alarmId);
         DB_RESULT result = DBSelectPrepared(stmt);
         if (result != nullptr)
         {
            objectId = (DBGetNumRows(result) > 0) ? DBGetFieldULong(result, 0 , 0) : 0;
            DBFreeResult(result);
         }
         DBFreeStatement(stmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }

   return (objectId != 0) ? FindObjectById(objectId) : shared_ptr<NetObj>();
}

/**
 * Get source object for given alarm helpdesk reference
 */
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(const TCHAR *hdref)
{
   uint32_t objectId = 0;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(
      [hdref] (Alarm *alarm) -> bool
      {
         return _tcscmp(alarm->getHelpDeskRef(), hdref) == 0;
      });
   if (alarm != nullptr)
   {
      objectId = alarm->getSourceObject();
   }
   s_alarmList.unlock();

   return (objectId != 0) ? FindObjectById(objectId) : shared_ptr<NetObj>();
}

/**
 * Fill message with alarm stats
 */
void GetAlarmStats(NXCPMessage *msg)
{
   uint32_t counters[5];
   memset(counters, 0, sizeof(counters));

   s_alarmList.lock();
   msg->setField(VID_NUM_ALARMS, static_cast<uint32_t>(s_alarmList.size()));
   s_alarmList.forEach(
      [&counters] (Alarm *alarm) -> EnumerationCallbackResult
      {
         counters[alarm->getCurrentSeverity()]++;
         return _CONTINUE;
      });
   s_alarmList.unlock();

   msg->setFieldFromInt32Array(VID_ALARMS_BY_SEVERITY, 5, counters);
}

/**
 * Get number of active alarms
 */
uint32_t GetAlarmCount()
{
   s_alarmList.lock();
   uint32_t count = static_cast<uint32_t>(s_alarmList.size());
   s_alarmList.unlock();
   return count;
}

/**
 * Get alarm memory usage
 */
uint64_t GetAlarmMemoryUsage()
{
   return s_alarmList.memoryUsage();
}

/**
 * Watchdog thread
 */
static void WatchdogThread()
{
   ThreadSetName("AlarmWatchdog");

	while(!s_shutdown.wait(1000))
	{
		if (!(g_flags & AF_SERVER_INITIALIZED))
		   continue;   // Server not initialized yet

		ObjectArray<Alarm> terminatedAlarms(0, 128, Ownership::False);

		s_alarmList.lock();
		time_t now = time(nullptr);
		s_alarmList.forEach(
		   [now, &terminatedAlarms] (Alarm *alarm) -> EnumerationCallbackResult
         {
            if ((alarm->getTimeout() > 0) &&
                ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_OUTSTANDING) &&
                (((time_t)alarm->getLastChangeTime() + (time_t)alarm->getTimeout()) < now))
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Outstanding timeout: alarm_id=%u, last_change=%u, timeout=%u, now=%u"),
                        alarm->getAlarmId(), alarm->getLastChangeTime(), alarm->getTimeout(), (UINT32)now);

               TCHAR eventName[MAX_EVENT_NAME];
               if (!EventNameFromCode(alarm->getSourceEventCode(), eventName))
               {
                  _sntprintf(eventName, MAX_EVENT_NAME, _T("[%u]"), alarm->getSourceEventCode());
               }
               EventBuilder(alarm->getTimeoutEvent(), alarm->getSourceObject())
                  .param(_T("alarmId"), alarm->getAlarmId())
                  .param(_T("alarmMessage"), alarm->getMessage())
                  .param(_T("alarmKey"), alarm->getKey())
                  .param(_T("originalEventCode"), alarm->getSourceEventCode())
                  .param(_T("originalEventName"), eventName)
                  .post();
               alarm->clearTimeout();	// Disable repeated timeout events
               alarm->updateInDatabase();
            }

            if ((alarm->getAckTimeout() != 0) &&
                ((alarm->getState() & ALARM_STATE_STICKY) != 0) &&
                (((time_t)alarm->getAckTimeout() <= now)))
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Acknowledgment timeout: alarm_id=%u, timeout=%u, now=%u"),
                        alarm->getAlarmId(), alarm->getAckTimeout(), (UINT32)now);

               EventBuilder(alarm->getTimeoutEvent(), alarm->getSourceObject())
                  .param(_T("alarmId"), alarm->getAlarmId())
                  .param(_T("alarmMessage"), alarm->getMessage())
                  .param(_T("alarmKey"), alarm->getKey())
                  .param(_T("originalEventCode"), alarm->getSourceEventCode())
                  .post();

               alarm->onAckTimeoutExpiration();
               alarm->updateInDatabase();
               NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
            }

            if ((s_resolveExpirationTime > 0) &&
                ((alarm->getState() & ALARM_STATE_MASK) == ALARM_STATE_RESOLVED) &&
                (alarm->getLastChangeTime() + s_resolveExpirationTime <= now) &&
                (alarm->getHelpDeskState() != ALARM_HELPDESK_OPEN))
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Resolve timeout: alarm_id=%u, last_change=%u, timeout=%u, now=%u"),
                        alarm->getAlarmId(), alarm->getLastChangeTime(), s_resolveExpirationTime, (UINT32)now);
               terminatedAlarms.add(alarm);
            }

            return _CONTINUE;
         });

		// Terminate alarms
		for(int i = 0; i < terminatedAlarms.size(); i++)
		{
		   Alarm *alarm = terminatedAlarms.get(i);
         alarm->resolve(0, nullptr, true, true, false);
         UpdateObjectOnAlarmResolve(alarm->getSourceObject(), alarm->getAlarmId(), true);
		   s_alarmList.remove(alarm);
		}

		s_alarmList.unlock();
	}
}

/**
 * Check if given alarm/note id pair is valid
 */
static bool IsValidCommentId(uint32_t alarmId, uint32_t noteId)
{
	bool isValid = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT note_id FROM alarm_notes WHERE alarm_id=? AND note_id=?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, noteId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			isValid = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	return isValid;
}

/**
 * Update alarm's comment
 * Will update commentId to correct id if new comment is created
 */
uint32_t Alarm::updateAlarmComment(uint32_t *commentId, const TCHAR *text, uint32_t userId, bool syncWithHelpdesk)
{
   bool newNote = false;
   uint32_t rcc;

	if (*commentId != 0)
	{
      if (IsValidCommentId(m_alarmId, *commentId))
		{
			DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
			DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE alarm_notes SET change_time=?,user_id=?,note_text=? WHERE note_id=?"));
			if (hStmt != nullptr)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(nullptr));
				DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, userId);
				DBBind(hStmt, 3, DB_SQLTYPE_TEXT, text, DB_BIND_STATIC);
				DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, *commentId);
				rcc = DBExecute(hStmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
				DBFreeStatement(hStmt);
			}
			else
			{
				rcc = RCC_DB_FAILURE;
			}
			DBConnectionPoolReleaseConnection(hdb);
		}
		else
		{
			rcc = RCC_INVALID_ALARM_NOTE_ID;
		}
	}
	else
	{
		// new note
		newNote = true;
		*commentId = CreateUniqueId(IDG_ALARM_NOTE);
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_notes (note_id,alarm_id,change_time,user_id,note_text) VALUES (?,?,?,?,?)"));
		if (hStmt != nullptr)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, *commentId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_alarmId);
			DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (UINT32)time(nullptr));
			DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, userId);
			DBBind(hStmt, 5, DB_SQLTYPE_TEXT, text, DB_BIND_STATIC);
			rcc = DBExecute(hStmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
			DBFreeStatement(hStmt);
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	if (rcc == RCC_SUCCESS)
	{
      if (newNote)
         m_commentCount++;
		NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
      if (syncWithHelpdesk && (m_helpDeskState == ALARM_HELPDESK_OPEN))
      {
         AddHelpdeskIssueComment(m_helpDeskRef, text);
      }
	}

   return rcc;
}

/**
 * Add alarm's comment by helpdesk reference
 */
uint32_t AddAlarmComment(const TCHAR *hdref, const TCHAR *text, uint32_t userId)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(
      [hdref] (Alarm *alarm) -> bool
      {
         return _tcscmp(alarm->getHelpDeskRef(), hdref) == 0;
      });
   if (alarm != nullptr)
   {
      uint32_t id = 0;
      rcc = alarm->updateAlarmComment(&id, text, userId, false);
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Add alarm's comment by helpdesk reference from system user
 */
uint32_t AddAlarmSystemComment(const TCHAR *hdref, const TCHAR *text)
{
   return AddAlarmComment(hdref, text, 0);
}

/**
 * Update alarm's comment
 */
uint32_t UpdateAlarmComment(uint32_t alarmId, uint32_t *noteId, const TCHAR *text, uint32_t userId, bool syncWithHelpdesk)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      rcc = alarm->updateAlarmComment(noteId, text, userId, syncWithHelpdesk);
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Delete comment
 */
uint32_t Alarm::deleteComment(uint32_t commentId)
{
   uint32_t rcc;
   if (IsValidCommentId(m_alarmId, commentId))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      rcc = ExecuteQueryOnObject(hdb, commentId, L"DELETE FROM alarm_notes WHERE note_id=?") ? RCC_SUCCESS : RCC_DB_FAILURE;
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      rcc = RCC_INVALID_ALARM_NOTE_ID;
   }
   if (rcc == RCC_SUCCESS)
   {
      m_commentCount--;
      NotifyClients(NX_NOTIFY_ALARM_CHANGED, this);
   }
   return rcc;
}

/**
 * Delete comment
 */
uint32_t DeleteAlarmCommentByID(uint32_t alarmId, uint32_t noteId)
{
   uint32_t rcc = RCC_INVALID_ALARM_ID;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
   {
      rcc = alarm->deleteComment(noteId);
   }
   s_alarmList.unlock();

   return rcc;
}

/**
 * Get alarm's comments
 * Will return object array that is not the owner of objects
 */
ObjectArray<AlarmComment> *GetAlarmComments(uint32_t alarmId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ObjectArray<AlarmComment> *comments = new ObjectArray<AlarmComment>(16, 16, Ownership::False);

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT note_id,change_time,user_id,note_text FROM alarm_notes WHERE alarm_id=?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);

         for(int i = 0; i < count; i++)
         {
            comments->add(new AlarmComment(DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1),
                  DBGetFieldULong(hResult, i, 2), DBGetField(hResult, i, 3, nullptr, 0)));
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return comments;
}

/**
 * Get alarm's comments
 */
uint32_t GetAlarmComments(uint32_t alarmId, NXCPMessage *msg)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	uint32_t rcc = RCC_DB_FAILURE;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT note_id,change_time,user_id,note_text FROM alarm_notes WHERE alarm_id=?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			int count = DBGetNumRows(hResult);
			msg->setField(VID_NUM_ELEMENTS, (UINT32)count);

			uint32_t fieldId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < count; i++)
			{
				msg->setField(fieldId++, DBGetFieldULong(hResult, i, 0));
				msg->setField(fieldId++, alarmId);
				msg->setField(fieldId++, DBGetFieldULong(hResult, i, 1));
            uint32_t userId = DBGetFieldULong(hResult, i, 2);
				msg->setField(fieldId++, userId);

            TCHAR *text = DBGetField(hResult, i, 3, nullptr, 0);
				msg->setField(fieldId++, CHECK_NULL_EX(text));
				MemFree(text);

            TCHAR userName[MAX_USER_NAME];
            if (ResolveUserId(userId, userName) != nullptr)
            {
   				msg->setField(fieldId++, userName);
            }
            else
            {
               fieldId++;
            }

            fieldId += 4;
			}
			DBFreeResult(hResult);
			rcc = RCC_SUCCESS;
		}
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
	return rcc;
}

/**
 * Get alarms for given object. If objectId set to 0, all alarms will be returned.
 * This method returns copies of alarm objects, so changing them will not
 * affect alarm states. Returned array must be destroyed by the caller.
 *
 * @param objectId object ID or 0 to get all alarms
 * @param recursive if true, will return alarms for child objects
 * @return array of active alarms for given object
 */
ObjectArray<Alarm> NXCORE_EXPORTABLE *GetAlarms(uint32_t objectId, bool recursive)
{
   ObjectArray<Alarm> *result = new ObjectArray<Alarm>(128, 128, Ownership::True);
   s_alarmList.lock();
   s_alarmList.forEach(
      [objectId, recursive, result] (Alarm *alarm) -> EnumerationCallbackResult
      {
         if ((objectId == 0) || (alarm->getSourceObject() == objectId) ||
             (recursive && IsParentObject(objectId, alarm->getSourceObject())))
         {
            result->add(new Alarm(alarm, true));
         }
         return _CONTINUE;
      });
   s_alarmList.unlock();
   return result;
}

/**
 * NXSL extension: Find alarm by ID
 */
int F_FindAlarmById(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   uint32_t alarmId = argv[0]->getValueAsUInt32();
   Alarm *alarm = FindAlarmById(alarmId);
   *result = (alarm != nullptr) ? vm->createValue(vm->createObject(&g_nxslAlarmClass, alarm)) : vm->createValue();
   return 0;
}

/**
 * NXSL extension: Find alarm by key
 */
int F_FindAlarmByKey(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *key = argv[0]->getValueAsCString();

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(key);
   if (alarm != nullptr)
      alarm = new Alarm(alarm, false);
   s_alarmList.unlock();

   *result = (alarm != nullptr) ? vm->createValue(vm->createObject(&g_nxslAlarmClass, alarm)) : vm->createValue();
   return 0;
}

/**
 * NXSL extension: Find alarm by key using regular expression
 */
int F_FindAlarmByKeyRegex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   const TCHAR *keyPattern = argv[0]->getValueAsCString();
   Alarm *alarm = nullptr;

   const char *errptr;
   int erroffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(keyPattern), PCRE_COMMON_FLAGS, &errptr, &erroffset, nullptr);
   if (preg != nullptr)
   {
      s_alarmList.lock();
      s_alarmList.forEachKey(
         [preg, &alarm] (const TCHAR *key, Alarm *a) -> EnumerationCallbackResult
         {
            int ovector[60];
            if (_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(key), static_cast<int>(_tcslen(key)), 0, 0, ovector, 60) >= 0)
            {
               alarm = new Alarm(a, false);
               return _STOP;
            }
            return _CONTINUE;
         });
      s_alarmList.unlock();
      _pcre_free_t(preg);
   }

   *result = (alarm != nullptr) ? vm->createValue(vm->createObject(&g_nxslAlarmClass, alarm)) : vm->createValue();
   return 0;
}

/**
 * Get alarm by id
 */
Alarm NXCORE_EXPORTABLE *FindAlarmById(UINT32 alarmId)
{
   if (alarmId == 0)
      return nullptr;

   s_alarmList.lock();
   Alarm *alarm = s_alarmList.find(alarmId);
   if (alarm != nullptr)
      alarm = new Alarm(alarm, false);
   s_alarmList.unlock();
   return alarm;
}

/**
 * Load alarm from database
 */
Alarm NXCORE_EXPORTABLE *LoadAlarmFromDatabase(uint32_t alarmId)
{
   if (alarmId == 0)
      return nullptr;

   Alarm *alarm = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT ") ALARM_LOAD_COLUMN_LIST _T(" FROM alarms WHERE alarm_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            alarm = new Alarm(hdb, hResult, 0);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return alarm;
}

/**
 * Update alarm expiration times
 */
void UpdateAlarmExpirationTimes()
{
   s_resolveExpirationTime = ConfigReadInt(L"Alarms.ResolveExpirationTime", 0);
   nxlog_debug_tag(DEBUG_TAG, 3, L"Resolved alarms expiration time set to %u seconds", s_resolveExpirationTime);
}

/**
 * Root cause update thread
 */
static void RootCauseUpdateThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, L"Root cause update thread started");
   while(!s_shutdown.wait(60000))
   {
      if (!(g_flags & AF_SERVER_INITIALIZED))
         continue;   // Server not initialized yet

      if (!s_rootCauseUpdateNeeded || !s_rootCauseUpdatePossible)
         continue;

      s_rootCauseUpdatePossible = false;
      s_rootCauseUpdateNeeded = false;

      ObjectArray<Alarm> updateList(0, 32, Ownership::True);
      s_alarmList.lock();
      s_alarmList.forEach(
         [&updateList] (Alarm *a) -> EnumerationCallbackResult
         {
            if ((*a->getRcaScriptName() != 0) && (a->getParentAlarmId() == 0))
            {
               updateList.add(new Alarm(a, false));
            }
            return _CONTINUE;
         });
      s_alarmList.unlock();

      nxlog_debug_tag(DEBUG_TAG, 5, _T("%d alarms for re-evaluation by background root cause analyzer"));

      for(int i = 0; i < updateList.size(); i++)
      {
         Alarm *alarm = updateList.get(i);
         shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
         NXSL_VM *vm = CreateServerScriptVM(alarm->getRcaScriptName(), object);
         if (vm != nullptr)
         {
            Event *event = LoadEventFromDatabase(alarm->getSourceEventId());
            if (event != nullptr)
               vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, false)));
            if (vm->run())
            {
               NXSL_Value *result = vm->getResult();
               if (result->isObject(_T("Alarm")))
               {
                  uint32_t parentAlarmId = static_cast<Alarm*>(result->getValueAsObject()->getData())->getAlarmId();
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("Background root cause analysis script in has found parent alarm %u (%s)"),
                           parentAlarmId, static_cast<Alarm*>(result->getValueAsObject()->getData())->getMessage());

                  s_alarmList.lock();
                  Alarm *originalAlarm = s_alarmList.find(alarm->getAlarmId());
                  if (originalAlarm != nullptr)
                  {
                     originalAlarm->updateParentAlarm(parentAlarmId);
                  }
                  s_alarmList.unlock();
               }
            }
            else
            {
               ReportScriptError(SCRIPT_CONTEXT_ALARM, object.get(), 0, vm->getErrorText(), alarm->getRcaScriptName());
               nxlog_write(NXLOG_ERROR, _T("Failed to execute background root cause analysis script %s (%s)"), alarm->getRcaScriptName(), vm->getErrorText());
            }
            delete vm;
         }
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Root cause update thread stopped"));
}

/**
 * Initialize alarm manager at system startup
 */
bool InitAlarmManager()
{
   s_watchdogThread = INVALID_THREAD_HANDLE;
   s_resolveExpirationTime = ConfigReadInt(L"Alarms.ResolveExpirationTime", 0);

   // Load active alarms into memory
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT ") ALARM_LOAD_COLUMN_LIST _T(" FROM alarms WHERE alarm_state<>3"));
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   DB_HANDLE cachedb = (g_flags & AF_CACHE_DB_ON_STARTUP) ? DBOpenInMemoryDatabase() : nullptr;
   if (cachedb != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Caching alarm data tables"));
      if (!DBCacheTable(cachedb, hdb, _T("alarm_events"), _T("alarm_id,event_id"), _T("*")) ||
          !DBCacheTable(cachedb, hdb, _T("alarm_notes"), _T("note_id"), _T("note_id,alarm_id")))
      {
         DBCloseInMemoryDatabase(cachedb);
         cachedb = nullptr;
      }
   }

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      s_alarmList.add(new Alarm((cachedb != nullptr) ? cachedb : hdb, hResult, i));

   DBFreeResult(hResult);

   hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM alarm_state_changes"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_stateChangeLogRecordId = DBGetFieldInt64(hResult, 0, 0);
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);

   if (cachedb != nullptr)
      DBCloseInMemoryDatabase(cachedb);

   // Populate per object status mappings and update subordinate alarm lists
   s_alarmList.forEach(
      [] (Alarm *curr) -> EnumerationCallbackResult
      {
         if (curr->getState() != ALARM_STATE_RESOLVED)
         {
            UpdateObjectOnAlarmUpdate(curr->getSourceObject(), curr, false);
         }
         if (curr->getParentAlarmId() != 0)
         {
            Alarm *parent = s_alarmList.find(curr->getParentAlarmId());
            if (parent != nullptr)
               parent->addSubordinateAlarm(curr->getAlarmId());
         }
         return _CONTINUE;
      });

   s_watchdogThread = ThreadCreateEx(WatchdogThread);
   s_rootCauseUpdateThread = ThreadCreateEx(RootCauseUpdateThread);
   return true;
}

/**
 * Alarm manager destructor
 */
void ShutdownAlarmManager()
{
   s_shutdown.set();
   ThreadJoin(s_watchdogThread);
   ThreadJoin(s_rootCauseUpdateThread);
}
