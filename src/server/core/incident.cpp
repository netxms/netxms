/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: incident.cpp
**
**/

#include "nxcore.h"
#include <nms_incident.h>
#include <nms_users.h>
#include <nxai.h>

#define DEBUG_TAG L"incident"

/**
 * Incident list class
 */
class IncidentList
{
private:
   Mutex m_lock;
   SharedPointerIndex<Incident> m_incidents;

public:
   IncidentList() : m_lock(MutexType::FAST), m_incidents() { }

   void lock() { m_lock.lock(); }
   void unlock() { m_lock.unlock(); }

   void add(const shared_ptr<Incident>& incident) { m_incidents.put(incident->getId(), incident); }
   void remove(uint32_t id) { m_incidents.remove(id); }
   shared_ptr<Incident> get(uint32_t id) { return m_incidents.get(id); }

   shared_ptr<Incident> findByAlarmId(uint32_t alarmId)
   {
      return m_incidents.find(
         [alarmId] (Incident *incident) -> bool
         {
            return incident->isAlarmLinked(alarmId);
         });
   }

   void forEach(std::function<EnumerationCallbackResult(Incident*)> callback)
   {
      m_incidents.forEach(callback);
   }

   size_t size() const { return m_incidents.size(); }
};

/**
 * Global incident list
 */
static IncidentList s_incidents;

/**
 * State name table
 */
static const wchar_t *s_stateNames[] = { L"Open", L"In Progress", L"Blocked", L"Resolved", L"Closed" };

/**
 * Get incident state name
 */
const wchar_t NXCORE_EXPORTABLE *GetIncidentStateName(int state)
{
   return ((state >= 0) && (state <= 4)) ? s_stateNames[state] : L"Unknown";
}

/**
 * Notify clients about incident update
 */
static void NotifyClientsOnIncidentUpdate(uint32_t code, const Incident *incident)
{
   NXCPMessage msg(code, 0);
   incident->fillMessage(&msg);
   EnumerateClientSessions(
      [&msg] (ClientSession *session) -> void
      {
         if (session->isAuthenticated())
            session->postMessage(msg);
      });
}

/*** IncidentComment implementation ***/

/**
 * Constructor from data
 */
IncidentComment::IncidentComment(uint32_t id, uint32_t incidentId, time_t creationTime, uint32_t userId, const TCHAR *text, bool aiGenerated)
{
   m_id = id;
   m_incidentId = incidentId;
   m_creationTime = creationTime;
   m_userId = userId;
   m_text = MemCopyString(text);
   m_aiGenerated = aiGenerated;
}

/**
 * Constructor from database
 */
IncidentComment::IncidentComment(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldUInt32(hResult, row, 0);
   m_incidentId = DBGetFieldUInt32(hResult, row, 1);
   m_creationTime = DBGetFieldTime(hResult, row, 2);
   m_userId = DBGetFieldUInt32(hResult, row, 3);
   m_text = DBGetField(hResult, row, 4, nullptr, 0);
   m_aiGenerated = DBGetFieldInt32(hResult, row, 5) != 0;
}

/**
 * Destructor
 */
IncidentComment::~IncidentComment()
{
   MemFree(m_text);
}

/**
 * Fill NXCP message
 */
void IncidentComment::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_incidentId);
   msg->setFieldFromTime(baseId + 2, m_creationTime);
   msg->setField(baseId + 3, m_userId);
   msg->setField(baseId + 4, m_text);
   msg->setField(baseId + 5, m_aiGenerated);
}

/**
 * Save comment to database
 */
bool IncidentComment::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const wchar_t *columns[] = { L"incident_id", L"creation_time", L"user_id", L"comment_text", L"ai_generated", nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"incident_comments", L"id", m_id, columns);
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_incidentId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_userId);
   DBBind(hStmt, 4, DB_SQLTYPE_TEXT, m_text, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_aiGenerated ? L"1" : L"0", DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Create JSON representation
 */
json_t *IncidentComment::toJson() const
{
   wchar_t userName[MAX_USER_NAME];

   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "incidentId", json_integer(m_incidentId));
   json_object_set_new(root, "userId", json_integer(m_userId));
   json_object_set_new(root, "userName", json_string_t(ResolveUserId(m_userId, userName, true)));
   json_object_set_new(root, "creationTime", json_time_string(m_creationTime));
   json_object_set_new(root, "text", json_string_t(CHECK_NULL_EX(m_text)));
   json_object_set_new(root, "aiGenerated", json_boolean(m_aiGenerated));
   return root;
}

/*** Incident implementation ***/

/**
 * Default constructor
 */
Incident::Incident()
{
   m_id = 0;
   m_creationTime = 0;
   m_lastChangeTime = 0;
   m_state = INCIDENT_STATE_OPEN;
   m_assignedUserId = 0;
   m_title = nullptr;
   m_sourceAlarmId = 0;
   m_sourceObjectId = 0;
   m_createdByUser = 0;
   m_resolvedByUser = 0;
   m_closedByUser = 0;
   m_resolveTime = 0;
   m_closeTime = 0;
}

/**
 * Constructor for new incident
 */
Incident::Incident(uint32_t id, uint32_t sourceObjectId, const TCHAR *title,
                   uint32_t sourceAlarmId, uint32_t createdByUser)
{
   m_id = id;
   m_creationTime = time(nullptr);
   m_lastChangeTime = m_creationTime;
   m_state = INCIDENT_STATE_OPEN;
   m_assignedUserId = 0;
   m_title = MemCopyString(title);
   m_sourceAlarmId = sourceAlarmId;
   m_sourceObjectId = sourceObjectId;
   m_createdByUser = createdByUser;
   m_resolvedByUser = 0;
   m_closedByUser = 0;
   m_resolveTime = 0;
   m_closeTime = 0;
}

/**
 * Constructor from database
 */
Incident::Incident(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_creationTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 1));
   m_lastChangeTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 2));
   m_state = DBGetFieldLong(hResult, row, 3);
   m_assignedUserId = DBGetFieldULong(hResult, row, 4);
   m_title = DBGetField(hResult, row, 5, nullptr, 0);
   m_sourceAlarmId = DBGetFieldULong(hResult, row, 6);
   m_sourceObjectId = DBGetFieldULong(hResult, row, 7);
   m_createdByUser = DBGetFieldULong(hResult, row, 8);
   m_resolvedByUser = DBGetFieldULong(hResult, row, 9);
   m_closedByUser = DBGetFieldULong(hResult, row, 10);
   m_resolveTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 11));
   m_closeTime = static_cast<time_t>(DBGetFieldInt64(hResult, row, 12));
}

/**
 * Destructor
 */
Incident::~Incident()
{
   MemFree(m_title);
}

/**
 * Load linked alarms from database
 */
bool Incident::loadLinkedAlarms(DB_HANDLE hdb)
{
   wchar_t query[256];
   nx_swprintf(query, 256, L"SELECT alarm_id FROM incident_alarms WHERE incident_id=%u", m_id);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;

   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      m_linkedAlarms.add(DBGetFieldULong(hResult, i, 0));
   }
   DBFreeResult(hResult);
   return true;
}

/**
 * Save incident to database
 */
bool Incident::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = {
      L"creation_time", L"last_change_time", L"state", L"assigned_user_id",
      L"title", L"source_alarm_id", L"source_object_id",
      L"created_by_user", L"resolved_by_user", L"closed_by_user",
      L"resolve_time", L"close_time", nullptr
   };

   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"incidents", L"id", m_id, columns);
   if (hStmt == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastChangeTime));
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_state);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_assignedUserId);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_title, DB_BIND_STATIC, 255);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_sourceAlarmId);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_sourceObjectId);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_createdByUser);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_resolvedByUser);
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_closedByUser);
   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_resolveTime));
   DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_closeTime));
   DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Update incident in database
 */
void Incident::updateInDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("UPDATE incidents SET last_change_time=?,state=?,assigned_user_id=?,title=?,")
      _T("resolved_by_user=?,closed_by_user=?,resolve_time=?,close_time=? WHERE id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastChangeTime));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_state);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_assignedUserId);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_title, DB_BIND_STATIC, 255);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_resolvedByUser);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_closedByUser);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_resolveTime));
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_closeTime));
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Log activity to database
 */
void Incident::logActivity(int activityType, uint32_t userId, const TCHAR *oldValue, const TCHAR *newValue, const TCHAR *details)
{
   uint32_t activityId = CreateUniqueId(IDG_INCIDENT_ACTIVITY);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("INSERT INTO incident_activity_log (id,incident_id,timestamp,user_id,activity_type,old_value,new_value,details) VALUES (?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, activityId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, userId);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, activityType);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, oldValue, DB_BIND_STATIC, 255);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, newValue, DB_BIND_STATIC, 255);
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, details, DB_BIND_STATIC, 1000);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Change incident state
 */
uint32_t Incident::changeState(int newState, uint32_t userId, const wchar_t *comment)
{
   lock();

   if (m_state == INCIDENT_STATE_CLOSED)
   {
      unlock();
      return RCC_INCIDENT_CLOSED;
   }

   if ((newState < INCIDENT_STATE_OPEN) || (newState > INCIDENT_STATE_CLOSED))
   {
      unlock();
      return RCC_INVALID_INCIDENT_STATE;
   }

   // Require comment for BLOCKED state
   if ((newState == INCIDENT_STATE_BLOCKED) && ((comment == nullptr) || (*comment == 0)))
   {
      unlock();
      return RCC_COMMENT_REQUIRED;
   }

   int oldState = m_state;
   m_state = newState;
   m_lastChangeTime = time(nullptr);

   if (newState == INCIDENT_STATE_RESOLVED)
   {
      m_resolvedByUser = userId;
      m_resolveTime = m_lastChangeTime;

      // Resolve all linked alarms
      for (int i = 0; i < m_linkedAlarms.size(); i++)
      {
         ResolveAlarmById(m_linkedAlarms.get(i), nullptr, false, true);
      }
   }
   else if (newState == INCIDENT_STATE_CLOSED)
   {
      m_closedByUser = userId;
      m_closeTime = m_lastChangeTime;

      // Terminate all linked alarms
      for (int i = 0; i < m_linkedAlarms.size(); i++)
      {
         ResolveAlarmById(m_linkedAlarms.get(i), nullptr, true, true);
      }
   }

   updateInDatabase();
   logActivity(INCIDENT_ACTIVITY_STATE_CHANGE, userId, GetIncidentStateName(oldState), GetIncidentStateName(newState), nullptr);

   EventBuilder(EVENT_INCIDENT_STATE_CHANGED, m_sourceObjectId)
      .param(L"incidentId", m_id)
      .param(L"title", m_title)
      .param(L"oldState", oldState)
      .param(L"oldStateName", GetIncidentStateName(oldState))
      .param(L"newState", newState)
      .param(L"newStateName", GetIncidentStateName(newState))
      .param(L"userId", userId)
      .post();

   // Add comment if provided
   if ((comment != nullptr) && (*comment != 0))
   {
      IncidentComment incidentComment(CreateUniqueId(IDG_INCIDENT_COMMENT), m_id, time(nullptr), userId, comment);
      incidentComment.saveToDatabase();
      logAddCommentActivity(userId);
   }

   unlock();

   // Closed state is terminal, and only active incidents are kept in memory (see InitIncidentManager),
   // so drop closed incident from the list. Caller holds shared pointer, so object stays valid.
   if (newState == INCIDENT_STATE_CLOSED)
   {
      s_incidents.lock();
      s_incidents.remove(m_id);
      s_incidents.unlock();
   }

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, this);
   return RCC_SUCCESS;
}

/**
 * Assign incident to user
 */
uint32_t Incident::assign(uint32_t userId, uint32_t assignedBy)
{
   lock();

   if (m_state == INCIDENT_STATE_CLOSED)
   {
      unlock();
      return RCC_INCIDENT_CLOSED;
   }

   uint32_t oldUserId = m_assignedUserId;
   m_assignedUserId = userId;
   m_lastChangeTime = time(nullptr);

   updateInDatabase();

   wchar_t oldValue[32], newValue[32];
   logActivity(INCIDENT_ACTIVITY_ASSIGNED, assignedBy, IntegerToString(oldUserId, oldValue), IntegerToString(userId, newValue), nullptr);

   unlock();

   wchar_t userName[MAX_USER_NAME];
   EventBuilder(EVENT_INCIDENT_ASSIGNED, m_sourceObjectId)
      .param(L"incidentId", m_id)
      .param(L"title", m_title)
      .param(L"assignedUserId", userId)
      .param(L"assignedUserName", ResolveUserId(userId, userName, true))
      .param(L"assignedByUserId", assignedBy)
      .post();

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, this);
   return RCC_SUCCESS;
}

/**
 * Update incident title
 */
uint32_t Incident::update(const wchar_t *title, uint32_t userId)
{
   lock();

   if (m_state == INCIDENT_STATE_CLOSED)
   {
      unlock();
      return RCC_INCIDENT_CLOSED;
   }

   // Update title if provided
   if ((title != nullptr) && (*title != 0))
   {
      MemFree(m_title);
      m_title = MemCopyString(title);
   }

   m_lastChangeTime = time(nullptr);

   updateInDatabase();
   logActivity(INCIDENT_ACTIVITY_UPDATED, userId, nullptr, nullptr, nullptr);

   unlock();

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, this);
   return RCC_SUCCESS;
}

/**
 * Link alarm to incident
 */
uint32_t Incident::linkAlarm(uint32_t alarmId, uint32_t userId)
{
   lock();

   if (m_state == INCIDENT_STATE_CLOSED)
   {
      unlock();
      return RCC_INCIDENT_CLOSED;
   }

   // Check if alarm is already linked to another incident
   s_incidents.lock();
   shared_ptr<Incident> other = s_incidents.findByAlarmId(alarmId);
   if ((other != nullptr) && (other->getId() != m_id))
   {
      s_incidents.unlock();
      unlock();
      return RCC_ALARM_ALREADY_IN_INCIDENT;
   }
   s_incidents.unlock();

   // Check if already linked to this incident
   if (m_linkedAlarms.contains(alarmId))
   {
      unlock();
      return RCC_SUCCESS;
   }

   m_linkedAlarms.add(alarmId);
   m_lastChangeTime = time(nullptr);

   // Save link to database
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      _T("INSERT INTO incident_alarms (incident_id,alarm_id,linked_time,linked_by_user) VALUES (?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, alarmId);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastChangeTime));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, userId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   updateInDatabase();

   TCHAR alarmIdStr[32];
   _sntprintf(alarmIdStr, 32, _T("%u"), alarmId);
   logActivity(INCIDENT_ACTIVITY_ALARM_LINKED, userId, nullptr, alarmIdStr, nullptr);

   unlock();

   EventBuilder(EVENT_INCIDENT_ALARM_LINKED, m_sourceObjectId)
      .param(L"incidentId", m_id)
      .param(L"title", m_title)
      .param(L"alarmId", alarmId)
      .param(L"userId", userId)
      .post();

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, this);
   return RCC_SUCCESS;
}

/**
 * Unlink alarm from incident
 */
uint32_t Incident::unlinkAlarm(uint32_t alarmId, uint32_t userId)
{
   lock();

   if (m_state == INCIDENT_STATE_CLOSED)
   {
      unlock();
      return RCC_INCIDENT_CLOSED;
   }

   int index = m_linkedAlarms.indexOf(alarmId);
   if (index == -1)
   {
      unlock();
      return RCC_SUCCESS;
   }

   m_linkedAlarms.remove(index);
   m_lastChangeTime = time(nullptr);

   // Remove link from database
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM incident_alarms WHERE incident_id=? AND alarm_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, alarmId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   updateInDatabase();

   TCHAR alarmIdStr[32];
   _sntprintf(alarmIdStr, 32, _T("%u"), alarmId);
   logActivity(INCIDENT_ACTIVITY_ALARM_UNLINKED, userId, alarmIdStr, nullptr, nullptr);

   unlock();

   EventBuilder(EVENT_INCIDENT_ALARM_UNLINKED, m_sourceObjectId)
      .param(L"incidentId", m_id)
      .param(L"title", m_title)
      .param(L"alarmId", alarmId)
      .param(L"userId", userId)
      .post();

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, this);
   return RCC_SUCCESS;
}

/**
 * Fill message with incident comments
 */
static void IncidentCommentsToMessage(uint32_t incidentId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[256];
   nx_swprintf(query, 256,
      L"SELECT id,incident_id,creation_time,user_id,comment_text,ai_generated FROM incident_comments WHERE incident_id=%u ORDER BY creation_time",
      incidentId);

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      msg->setField(VID_NUM_COMMENTS, static_cast<uint32_t>(count));

      uint32_t baseId = VID_COMMENT_LIST_BASE;
      for (int i = 0; i < count; i++)
      {
         IncidentComment comment(hResult, i);
         comment.fillMessage(msg, baseId);
         baseId += 10;
      }
      DBFreeResult(hResult);
   }
   else
   {
      msg->setField(VID_NUM_COMMENTS, static_cast<uint32_t>(0));
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fill NXCP message with full incident data
 */
void Incident::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_INCIDENT_ID, m_id);
   msg->setFieldFromTime(VID_CREATION_TIME, m_creationTime);
   msg->setFieldFromTime(VID_LAST_CHANGE_TIME, m_lastChangeTime);
   msg->setField(VID_INCIDENT_STATE, static_cast<uint16_t>(m_state));
   msg->setField(VID_INCIDENT_ASSIGNED_USER, m_assignedUserId);
   msg->setField(VID_INCIDENT_TITLE, m_title);
   msg->setField(VID_ALARM_ID, m_sourceAlarmId);
   msg->setField(VID_OBJECT_ID, m_sourceObjectId);
   msg->setField(VID_USER_ID, m_createdByUser);
   msg->setField(VID_RESOLVED_BY_USER, m_resolvedByUser);
   msg->setField(VID_CLOSED_BY_USER, m_closedByUser);
   msg->setFieldFromTime(VID_RESOLVE_TIME, m_resolveTime);
   msg->setFieldFromTime(VID_CLOSE_TIME, m_closeTime);

   msg->setField(VID_NUM_ALARMS, static_cast<uint32_t>(m_linkedAlarms.size()));
   msg->setFieldFromInt32Array(VID_ALARM_ID_LIST, &m_linkedAlarms);

   IncidentCommentsToMessage(m_id, msg);
}

/**
 * Fill NXCP message with incident summary data
 */
void Incident::fillSummaryMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_id);
   msg->setFieldFromTime(baseId + 1, m_creationTime);
   msg->setFieldFromTime(baseId + 2, m_lastChangeTime);
   msg->setField(baseId + 3, static_cast<uint16_t>(m_state));
   msg->setField(baseId + 4, m_assignedUserId);
   msg->setField(baseId + 5, m_title);
   msg->setField(baseId + 6, m_sourceObjectId);
   msg->setField(baseId + 7, static_cast<uint32_t>(m_linkedAlarms.size()));
}

/**
 * Create JSON value from nullable text database field, or JSON null if it is empty
 */
static inline json_t *OptionalFieldToJson(DB_RESULT hResult, int row, int col)
{
   String value = DBGetFieldAsString(hResult, row, col);
   return value.isEmpty() ? json_null() : json_string_t(value.cstr());
}

/**
 * Create JSON representation
 */
json_t *Incident::toJson(bool includeComments) const
{
   // User IDs are always reported together with resolved user names, including ID 0 used as
   // "unassigned" / "created by event processing rule" sentinel - it resolves to built-in
   // "system" account, and interpreting the sentinel is left to the client.
   wchar_t userName[MAX_USER_NAME];

   // Serialization is done under lock, because title can be replaced and linked alarm
   // list can be modified concurrently. Comments are read outside of it to avoid holding
   // incident lock for the duration of a database query.
   lock();

   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "creationTime", json_time_string(m_creationTime));
   json_object_set_new(root, "lastChangeTime", json_time_string(m_lastChangeTime));
   json_object_set_new(root, "state", json_integer(m_state));
   json_object_set_new(root, "stateName", json_string_t(GetIncidentStateName(m_state)));
   json_object_set_new(root, "assignedUserId", json_integer(m_assignedUserId));
   json_object_set_new(root, "assignedUserName", json_string_t(ResolveUserId(m_assignedUserId, userName, true)));
   json_object_set_new(root, "title", json_string_t(CHECK_NULL_EX(m_title)));
   json_object_set_new(root, "sourceAlarmId", json_integer(m_sourceAlarmId));
   json_object_set_new(root, "sourceObjectId", json_integer(m_sourceObjectId));
   json_object_set_new(root, "sourceObjectName", json_string_t(GetObjectName(m_sourceObjectId, L"")));
   json_object_set_new(root, "createdByUser", json_integer(m_createdByUser));
   json_object_set_new(root, "createdByUserName", json_string_t(ResolveUserId(m_createdByUser, userName, true)));
   json_object_set_new(root, "resolvedByUser", json_integer(m_resolvedByUser));
   json_object_set_new(root, "resolvedByUserName", json_string_t(ResolveUserId(m_resolvedByUser, userName, true)));
   json_object_set_new(root, "closedByUser", json_integer(m_closedByUser));
   json_object_set_new(root, "closedByUserName", json_string_t(ResolveUserId(m_closedByUser, userName, true)));
   json_object_set_new(root, "resolveTime", json_time_string(m_resolveTime));
   json_object_set_new(root, "closeTime", json_time_string(m_closeTime));

   json_t *alarms = json_array();
   for (int i = 0; i < m_linkedAlarms.size(); i++)
   {
      json_array_append_new(alarms, json_integer(m_linkedAlarms.get(i)));
   }
   json_object_set_new(root, "linkedAlarms", alarms);
   json_object_set_new(root, "alarmCount", json_integer(m_linkedAlarms.size()));

   unlock();

   if (includeComments)
      json_object_set_new(root, "comments", GetIncidentCommentsAsJson(m_id));

   return root;
}

/*** Global incident subsystem functions ***/

/**
 * Initialize incident manager at system startup
 */
bool InitIncidentManager()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Initializing incident manager"));

   // Load active incidents into memory
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb,
      _T("SELECT id,creation_time,last_change_time,state,assigned_user_id,title,")
      _T("source_alarm_id,source_object_id,created_by_user,resolved_by_user,closed_by_user,")
      _T("resolve_time,close_time FROM incidents WHERE state<4"));
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   int count = DBGetNumRows(hResult);
   for (int i = 0; i < count; i++)
   {
      auto incident = make_shared<Incident>(hResult, i);
      incident->loadLinkedAlarms(hdb);
      s_incidents.add(incident);
   }
   DBFreeResult(hResult);

   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Incident manager initialized, %d active incidents loaded"), s_incidents.size());
   return true;
}

/**
 * Shutdown incident manager
 */
void ShutdownIncidentManager()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Incident manager shutdown"));
}

/**
 * Create new incident
 */
uint32_t NXCORE_EXPORTABLE CreateIncident(uint32_t sourceObjectId, const TCHAR *title,
   const TCHAR *initialComment, uint32_t sourceAlarmId, uint32_t userId, uint32_t *incidentId)
{
   // Check if alarm is already linked to another incident
   if (sourceAlarmId != 0)
   {
      s_incidents.lock();
      shared_ptr<Incident> existing = s_incidents.findByAlarmId(sourceAlarmId);
      s_incidents.unlock();
      if (existing != nullptr)
         return RCC_ALARM_ALREADY_IN_INCIDENT;
   }

   uint32_t id = CreateUniqueId(IDG_INCIDENT);
   auto incident = make_shared<Incident>(id, sourceObjectId, title, sourceAlarmId, userId);
   if (!incident->saveToDatabase())
   {
      return RCC_DB_FAILURE;
   }

   // Log creation activity
   incident->logCreationActivity(userId);

   // Add initial comment if provided
   if ((initialComment != nullptr) && (*initialComment != 0))
   {
      IncidentComment comment(CreateUniqueId(IDG_INCIDENT_COMMENT), id, time(nullptr), userId, initialComment);
      comment.saveToDatabase();
      incident->logAddCommentActivity(userId);
   }

   // Link source alarm if specified
   if (sourceAlarmId != 0)
   {
      incident->linkAlarm(sourceAlarmId, userId);
   }

   s_incidents.lock();
   s_incidents.add(incident);
   s_incidents.unlock();

   // Post event
   EventBuilder(EVENT_INCIDENT_OPENED, sourceObjectId)
      .param(L"incidentId", id)
      .param(L"title", title)
      .param(L"sourceAlarmId", sourceAlarmId)
      .param(L"userId", userId)
      .post();

   if (incidentId != nullptr)
      *incidentId = id;

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Created incident %u \"%s\" from object %u, alarm %u"),
      id, title, sourceObjectId, sourceAlarmId);

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, incident.get());
   return RCC_SUCCESS;
}

/**
 * Load incident by ID. Returns in-memory instance for active incidents, and reads from
 * database for closed ones (those are dropped from memory). Returns nullptr if incident
 * does not exist at all.
 */
shared_ptr<Incident> NXCORE_EXPORTABLE LoadIncidentById(uint32_t incidentId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();
   if (incident != nullptr)
      return incident;

   // Check in database for closed incidents
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 256,
      _T("SELECT id,creation_time,last_change_time,state,assigned_user_id,title,")
      _T("source_alarm_id,source_object_id,created_by_user,resolved_by_user,closed_by_user,")
      _T("resolve_time,close_time FROM incidents WHERE id=%u"), incidentId);

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         incident = make_shared<Incident>(hResult, 0);
         incident->loadLinkedAlarms(hdb);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return incident;
}

/**
 * Find incident by ID (in-memory only, returns nullptr for closed incidents)
 */
shared_ptr<Incident> NXCORE_EXPORTABLE FindIncidentById(uint32_t incidentId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();
   return incident;
}

/**
 * Find incident by alarm ID
 */
shared_ptr<Incident> NXCORE_EXPORTABLE FindIncidentByAlarmId(uint32_t alarmId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.findByAlarmId(alarmId);
   s_incidents.unlock();
   return incident;
}

/**
 * Change incident state
 */
uint32_t NXCORE_EXPORTABLE ChangeIncidentState(uint32_t incidentId, int newState, uint32_t userId, const TCHAR *comment)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->changeState(newState, userId, comment);
}

/**
 * Assign incident
 */
uint32_t NXCORE_EXPORTABLE AssignIncident(uint32_t incidentId, uint32_t userId, uint32_t assignedBy)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->assign(userId, assignedBy);
}

/**
 * Resolve incident
 */
uint32_t NXCORE_EXPORTABLE ResolveIncident(uint32_t incidentId, uint32_t userId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->changeState(INCIDENT_STATE_RESOLVED, userId);
}

/**
 * Close incident
 */
uint32_t NXCORE_EXPORTABLE CloseIncident(uint32_t incidentId, uint32_t userId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->changeState(INCIDENT_STATE_CLOSED, userId);
}

/**
 * Update incident title
 */
uint32_t NXCORE_EXPORTABLE UpdateIncident(uint32_t incidentId, const TCHAR *title, uint32_t userId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->update(title, userId);
}

/**
 * Link alarm to incident
 */
uint32_t NXCORE_EXPORTABLE LinkAlarmToIncident(uint32_t incidentId, uint32_t alarmId, uint32_t userId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->linkAlarm(alarmId, userId);
}

/**
 * Unlink alarm from incident
 */
uint32_t NXCORE_EXPORTABLE UnlinkAlarmFromIncident(uint32_t incidentId, uint32_t alarmId, uint32_t userId)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   return incident->unlinkAlarm(alarmId, userId);
}

/**
 * Add comment to incident
 */
uint32_t NXCORE_EXPORTABLE AddIncidentComment(uint32_t incidentId, const TCHAR *text, uint32_t userId, uint32_t *commentId, bool aiGenerated)
{
   s_incidents.lock();
   shared_ptr<Incident> incident = s_incidents.get(incidentId);
   s_incidents.unlock();

   if (incident == nullptr)
      return RCC_INVALID_INCIDENT_ID;

   if (incident->getState() == INCIDENT_STATE_CLOSED)
      return RCC_INCIDENT_CLOSED;

   uint32_t id = CreateUniqueId(IDG_INCIDENT_COMMENT);
   IncidentComment comment(id, incidentId, time(nullptr), userId, text, aiGenerated);

   if (!comment.saveToDatabase())
      return RCC_DB_FAILURE;

   incident->logAddCommentActivity(userId);

   if (commentId != nullptr)
      *commentId = id;

   NotifyClientsOnIncidentUpdate(CMD_INCIDENT_UPDATE, incident.get());
   return RCC_SUCCESS;
}


/**
 * Get incident activity log
 */
uint32_t NXCORE_EXPORTABLE GetIncidentActivity(uint32_t incidentId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[512];
   nx_swprintf(query, 512,
      L"SELECT id,incident_id,timestamp,user_id,activity_type,old_value,new_value,details "
      L"FROM incident_activity_log WHERE incident_id=%u ORDER BY timestamp",
      incidentId);

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   int count = DBGetNumRows(hResult);
   msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(count));

   wchar_t buffer[256];
   uint32_t fieldId = VID_ACTIVITY_LIST_BASE;
   for (int i = 0; i < count; i++)
   {
      msg->setField(fieldId++, DBGetFieldUInt32(hResult, i, 0));  // id
      msg->setField(fieldId++, DBGetFieldUInt32(hResult, i, 1));  // incident_id
      msg->setField(fieldId++, DBGetFieldInt64(hResult, i, 2));   // timestamp
      msg->setField(fieldId++, DBGetFieldUInt32(hResult, i, 3));  // user_id
      msg->setField(fieldId++, DBGetFieldInt32(hResult, i, 4));   // activity_type
      msg->setField(fieldId++, DBGetField(hResult, i, 5, buffer, 256)); // old_value
      msg->setField(fieldId++, DBGetField(hResult, i, 6, buffer, 256)); // new_value
      msg->setField(fieldId++, DBGetFieldAsString(hResult, i, 7)); // details
      fieldId += 3;
   }

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);
   return RCC_SUCCESS;
}

/**
 * Default and maximum number of incident summaries returned by database-backed queries
 */
#define INCIDENT_QUERY_DEFAULT_LIMIT   1000
#define INCIDENT_QUERY_MAX_LIMIT       10000

/**
 * Enumerate incident summaries from database, newest first. Reads from database, so closed
 * incidents (which are not kept in memory) are included. Only incidents on source objects
 * readable by given user are passed to callback. Result columns are:
 *    0 - id, 1 - creation_time, 2 - last_change_time, 3 - state, 4 - assigned_user_id,
 *    5 - title, 6 - source_object_id, 7 - number of linked alarms
 *
 * Filters: objectId (0 = any object), states (nullptr or empty = any state), from/to
 * on creation time (0 = unbounded), limit (0 = default; capped at maximum).
 *
 * Row limit is applied after access check rather than as SQL LIMIT, because inaccessible
 * rows would otherwise consume the limit and the caller would get fewer summaries
 * than requested (same approach as connection history endpoint). Result set is read
 * unbuffered so that fetching stops once the limit is reached - unlike alarms, closed
 * incidents accumulate indefinitely and must not be materialized in full.
 */
static void EnumerateIncidentSummaries(uint32_t userId, uint32_t objectId, const IntegerArray<int32_t> *states,
   time_t from, time_t to, int limit, std::function<void(DB_UNBUFFERED_RESULT, const NetObj&)> callback)
{
   if (limit <= 0)
      limit = INCIDENT_QUERY_DEFAULT_LIMIT;
   else if (limit > INCIDENT_QUERY_MAX_LIMIT)
      limit = INCIDENT_QUERY_MAX_LIMIT;

   StringBuffer query(
      L"SELECT i.id,i.creation_time,i.last_change_time,i.state,i.assigned_user_id,i.title,i.source_object_id,"
      L"(SELECT COUNT(*) FROM incident_alarms a WHERE a.incident_id=i.id) FROM incidents i WHERE 1=1");

   if (objectId != 0)
      query.appendFormattedString(L" AND i.source_object_id=%u", objectId);

   if ((states != nullptr) && !states->isEmpty())
   {
      query.append(L" AND i.state IN (");
      for (int i = 0; i < states->size(); i++)
      {
         if (i > 0)
            query.append(L',');
         query.appendFormattedString(L"%d", states->get(i));
      }
      query.append(L')');
   }

   if (from != 0)
      query.appendFormattedString(L" AND i.creation_time>=" INT64_FMT, static_cast<int64_t>(from));
   if (to != 0)
      query.appendFormattedString(L" AND i.creation_time<=" INT64_FMT, static_cast<int64_t>(to));

   query.append(L" ORDER BY i.id DESC");

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, query.cstr());
   if (hResult != nullptr)
   {
      int count = 0;
      while ((count < limit) && DBFetch(hResult))
      {
         shared_ptr<NetObj> object = FindObjectById(DBGetFieldUInt32(hResult, 6));
         if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
            continue;

         callback(hResult, *object);
         count++;
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Send incident summaries to client. Filter semantics match EnumerateIncidentSummaries.
 * Request is served from in-memory list when it asks only for active states without time
 * range or limit; any other combination is served from the database.
 */
void SendIncidentsToClient(uint32_t objectId, const IntegerArray<int32_t>& states, time_t from, time_t to, int limit,
   uint32_t requestId, ClientSession *session)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);

   if (objectId != 0)
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if ((object == nullptr) || !object->checkAccessRights(session->getUserId(), OBJECT_ACCESS_READ))
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         session->sendMessage(msg);
         return;
      }
   }

   uint32_t count = 0;
   uint32_t baseId = VID_INCIDENT_LIST_BASE;

   bool activeOnly = !states.isEmpty() && !states.contains(INCIDENT_STATE_CLOSED);
   if (activeOnly && (from == 0) && (to == 0) && (limit == 0))
   {
      uint32_t userId = session->getUserId();
      s_incidents.lock();
      s_incidents.forEach(
         [&msg, &count, &baseId, &states, objectId, userId] (Incident *incident) -> EnumerationCallbackResult
         {
            if ((objectId != 0) && (incident->getSourceObjectId() != objectId))
               return _CONTINUE;
            if (!states.contains(incident->getState()))
               return _CONTINUE;
            if (objectId == 0)
            {
               shared_ptr<NetObj> object = FindObjectById(incident->getSourceObjectId());
               if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
                  return _CONTINUE;
            }
            incident->fillSummaryMessage(&msg, baseId);
            count++;
            baseId += 10;
            return _CONTINUE;
         });
      s_incidents.unlock();
   }
   else
   {
      EnumerateIncidentSummaries(session->getUserId(), objectId, &states, from, to, limit,
         [&msg, &count, &baseId] (DB_UNBUFFERED_RESULT hResult, const NetObj& object)
         {
            wchar_t title[256];
            msg.setField(baseId, DBGetFieldUInt32(hResult, 0));
            msg.setFieldFromTime(baseId + 1, DBGetFieldTime(hResult, 1));
            msg.setFieldFromTime(baseId + 2, DBGetFieldTime(hResult, 2));
            msg.setField(baseId + 3, static_cast<uint16_t>(DBGetFieldInt32(hResult, 3)));
            msg.setField(baseId + 4, DBGetFieldUInt32(hResult, 4));
            msg.setField(baseId + 5, DBGetField(hResult, 5, title, 256));
            msg.setField(baseId + 6, object.getId());
            msg.setField(baseId + 7, DBGetFieldUInt32(hResult, 7));
            count++;
            baseId += 10;
         });
   }

   msg.setField(VID_NUM_ELEMENTS, count);
   msg.setField(VID_RCC, RCC_SUCCESS);
   session->sendMessage(msg);
}

/**
 * Get incident comments as JSON array, oldest first
 */
json_t NXCORE_EXPORTABLE *GetIncidentCommentsAsJson(uint32_t incidentId)
{
   json_t *output = json_array();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[256];
   nx_swprintf(query, 256,
      L"SELECT id,incident_id,creation_time,user_id,comment_text,ai_generated FROM incident_comments WHERE incident_id=%u ORDER BY creation_time,id",
      incidentId);

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         IncidentComment comment(hResult, i);
         json_array_append_new(output, comment.toJson());
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return output;
}

/**
 * Get incident activity log as JSON array, newest first
 */
json_t NXCORE_EXPORTABLE *GetIncidentActivityAsJson(uint32_t incidentId)
{
   json_t *output = json_array();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   wchar_t query[512];
   nx_swprintf(query, 512,
      L"SELECT id,incident_id,timestamp,user_id,activity_type,old_value,new_value,details "
      L"FROM incident_activity_log WHERE incident_id=%u ORDER BY timestamp DESC,id DESC",
      incidentId);

   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         uint32_t userId = DBGetFieldUInt32(hResult, i, 3);
         wchar_t userName[MAX_USER_NAME];

         json_t *entry = json_object();
         json_object_set_new(entry, "id", json_integer(DBGetFieldUInt32(hResult, i, 0)));
         json_object_set_new(entry, "incidentId", json_integer(DBGetFieldUInt32(hResult, i, 1)));
         json_object_set_new(entry, "timestamp", json_time_string(DBGetFieldTime(hResult, i, 2)));
         json_object_set_new(entry, "userId", json_integer(userId));
         json_object_set_new(entry, "userName", json_string_t(ResolveUserId(userId, userName, true)));
         json_object_set_new(entry, "activityType", json_integer(DBGetFieldInt32(hResult, i, 4)));
         json_object_set_new(entry, "oldValue", OptionalFieldToJson(hResult, i, 5));
         json_object_set_new(entry, "newValue", OptionalFieldToJson(hResult, i, 6));
         json_object_set_new(entry, "details", OptionalFieldToJson(hResult, i, 7));
         json_array_append_new(output, entry);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return output;
}

/**
 * Get incident summaries as JSON array, newest first. Filter semantics match
 * EnumerateIncidentSummaries.
 */
json_t NXCORE_EXPORTABLE *GetIncidentSummariesAsJson(uint32_t userId, uint32_t objectId,
   const IntegerArray<int32_t> *states, time_t from, time_t to, int limit)
{
   json_t *output = json_array();
   EnumerateIncidentSummaries(userId, objectId, states, from, to, limit,
      [output] (DB_UNBUFFERED_RESULT hResult, const NetObj& object)
      {
         uint32_t assignedUserId = DBGetFieldUInt32(hResult, 4);
         int state = DBGetFieldInt32(hResult, 3);
         wchar_t title[256], userName[MAX_USER_NAME];

         json_t *summary = json_object();
         json_object_set_new(summary, "id", json_integer(DBGetFieldUInt32(hResult, 0)));
         json_object_set_new(summary, "state", json_integer(state));
         json_object_set_new(summary, "stateName", json_string_t(GetIncidentStateName(state)));
         json_object_set_new(summary, "title", json_string_t(DBGetField(hResult, 5, title, 256)));
         json_object_set_new(summary, "sourceObjectId", json_integer(object.getId()));
         json_object_set_new(summary, "sourceObjectName", json_string_t(object.getName()));
         json_object_set_new(summary, "assignedUserId", json_integer(assignedUserId));
         json_object_set_new(summary, "assignedUserName", json_string_t(ResolveUserId(assignedUserId, userName, true)));
         json_object_set_new(summary, "alarmCount", json_integer(DBGetFieldInt32(hResult, 7)));
         json_object_set_new(summary, "creationTime", json_time_string(DBGetFieldTime(hResult, 1)));
         json_object_set_new(summary, "lastChangeTime", json_time_string(DBGetFieldTime(hResult, 2)));
         json_array_append_new(output, summary);
      });
   return output;
}

/**
 * Scheduled task handler for delayed incident creation from alarm
 * Persistent data format: "alarm=<id>;object=<id>;title=<text>;comment=<text>[;ai_analyze=1;ai_depth=<n>;ai_assign=<0|1>;ai_prompt=<text>]"
 */
void CreateIncidentFromAlarmTask(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("CreateIncidentFromAlarmTask: processing delayed incident creation"));

   const wchar_t *data = parameters->m_persistentData;
   if ((data == nullptr) || (data[0] == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateIncidentFromAlarmTask: no persistent data"));
      return;
   }

   uint32_t alarmId = 0;
   uint32_t objectId = 0;
   wchar_t title[256] = L"";
   wchar_t initialComment[2001] = L"";

   // AI analysis parameters
   bool aiAnalyze = false;
   int aiDepth = 0;
   bool aiAutoAssign = false;
   wchar_t aiPrompt[2001] = L"";

   // Parse persistent data
   StringList parts(data, L";");
   for (int i = 0; i < parts.size(); i++)
   {
      const wchar_t *part = parts.get(i);
      if (wcsncmp(part, L"alarm=", 6) == 0)
         alarmId = wcstoul(part + 6, nullptr, 10);
      else if (_tcsncmp(part, L"object=", 7) == 0)
         objectId = wcstoul(part + 7, nullptr, 10);
      else if (wcsncmp(part, L"title=", 6) == 0)
         wcslcpy(title, part + 6, 256);
      else if (wcsncmp(part, L"comment=", 8) == 0)
         wcslcpy(initialComment, part + 8, 2001);
      else if (wcsncmp(part, L"ai_analyze=", 11) == 0)
         aiAnalyze = (wcstoul(part + 11, nullptr, 10) != 0);
      else if (wcsncmp(part, L"ai_depth=", 9) == 0)
         aiDepth = static_cast<int>(wcstol(part + 9, nullptr, 10));
      else if (wcsncmp(part, L"ai_assign=", 10) == 0)
         aiAutoAssign = (wcstoul(part + 10, nullptr, 10) != 0);
      else if (wcsncmp(part, L"ai_prompt=", 10) == 0)
         wcslcpy(aiPrompt, part + 10, 2001);
   }

   if (alarmId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"CreateIncidentFromAlarmTask: invalid alarm ID");
      return;
   }

   // Check if alarm still active
   Alarm *alarm = FindAlarmById(alarmId);
   if (alarm == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"CreateIncidentFromAlarmTask: alarm [%u] not found (may have been deleted)", alarmId);
      return;
   }

   int state = alarm->getState() & ALARM_STATE_MASK;
   if (state == ALARM_STATE_RESOLVED || state == ALARM_STATE_TERMINATED)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"CreateIncidentFromAlarmTask: alarm [%u] already resolved/terminated, skipping incident creation", alarmId);
      return;
   }

   // Check if alarm already in an incident
   if (FindIncidentByAlarmId(alarmId) != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"CreateIncidentFromAlarmTask: alarm [%u] already linked to an incident", alarmId);
      return;
   }

   // Use alarm message as title if not provided
   const wchar_t *finalTitle = (title[0] != 0) ? title : alarm->getMessage();

   // Create the incident (initial comment will be added as first comment)
   uint32_t incidentId;
   uint32_t rcc = CreateIncident(objectId, finalTitle, (initialComment[0] != 0) ? initialComment : nullptr, alarmId, 0, &incidentId);

   if (rcc == RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateIncidentFromAlarmTask: created incident [%u] from alarm [%u]"), incidentId, alarmId);

      // Trigger AI analysis if enabled
      if (aiAnalyze)
      {
         SpawnIncidentAIAnalysis(incidentId, aiDepth, aiAutoAssign, (aiPrompt[0] != 0) ? aiPrompt : nullptr);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("CreateIncidentFromAlarmTask: failed to create incident from alarm [%u] (rcc=%u)"), alarmId, rcc);
   }
}
