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
** File: object_timeline.cpp
**
**/

#include "nxcore.h"
#include <nxcore_timeline.h>
#include <nms_alarm.h>
#include <nms_users.h>
#include <nms_pkg.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#define DEBUG_TAG L"obj.timeline"

/**
 * Built-in categories
 */
static const char *CATEGORY_EVENT = "event";
static const char *CATEGORY_THRESHOLD = "threshold";
static const char *CATEGORY_REACHABILITY = "reachability";
static const char *CATEGORY_MAINTENANCE = "maintenance";
static const char *CATEGORY_SOFTWARE = "software";
static const char *CATEGORY_ALARM = "alarm";
static const char *CATEGORY_AUDIT = "audit";
static const char *CATEGORY_DEPLOYMENT = "deployment";

/**
 * Built-in event classification
 */
static const struct
{
   uint32_t eventCode;
   const char *category;
} s_builtinEventCategories[] =
{
   { EVENT_SNMP_FAIL, CATEGORY_REACHABILITY },
   { EVENT_SNMP_OK, CATEGORY_REACHABILITY },
   { EVENT_AGENT_FAIL, CATEGORY_REACHABILITY },
   { EVENT_AGENT_OK, CATEGORY_REACHABILITY },
   { EVENT_ICMP_UNREACHABLE, CATEGORY_REACHABILITY },
   { EVENT_ICMP_OK, CATEGORY_REACHABILITY },
   { EVENT_SSH_UNREACHABLE, CATEGORY_REACHABILITY },
   { EVENT_SSH_OK, CATEGORY_REACHABILITY },
   { EVENT_MODBUS_UNREACHABLE, CATEGORY_REACHABILITY },
   { EVENT_MODBUS_OK, CATEGORY_REACHABILITY },
   { EVENT_ETHERNET_IP_UNREACHABLE, CATEGORY_REACHABILITY },
   { EVENT_ETHERNET_IP_OK, CATEGORY_REACHABILITY },
   { EVENT_NODE_DOWN, CATEGORY_REACHABILITY },
   { EVENT_NODE_UP, CATEGORY_REACHABILITY },
   { EVENT_NODE_UNREACHABLE, CATEGORY_REACHABILITY },
   { EVENT_MAINTENANCE_MODE_ENTERED, CATEGORY_MAINTENANCE },
   { EVENT_MAINTENANCE_MODE_LEFT, CATEGORY_MAINTENANCE },
   { EVENT_PACKAGE_INSTALLED, CATEGORY_SOFTWARE },
   { EVENT_PACKAGE_UPDATED, CATEGORY_SOFTWARE },
   { EVENT_PACKAGE_REMOVED, CATEGORY_SOFTWARE },
   { 0, nullptr }
};

/**
 * Default threshold activation/deactivation events
 */
static const uint32_t s_defaultThresholdEvents[] = { EVENT_THRESHOLD_REACHED, EVENT_THRESHOLD_REARMED, EVENT_TABLE_THRESHOLD_ACTIVATED, EVENT_TABLE_THRESHOLD_DEACTIVATED };

/**
 * Registered provider
 */
struct TimelineProviderRegistration
{
   const char *name;
   std::vector<const char*> categories;
   ObjectTimelineProvider provider;
};

/**
 * Registered providers
 */
static std::vector<TimelineProviderRegistration> s_providers;

/**
 * Event classification registered by modules
 */
static std::unordered_map<uint32_t, const char*> s_moduleEventCategories;

/**
 * Find built-in category for event code
 */
static const char *FindBuiltinEventCategory(uint32_t eventCode)
{
   for(int i = 0; s_builtinEventCategories[i].category != nullptr; i++)
      if (s_builtinEventCategories[i].eventCode == eventCode)
         return s_builtinEventCategories[i].category;
   for(uint32_t c : s_defaultThresholdEvents)
      if (c == eventCode)
         return CATEGORY_THRESHOLD;
   return nullptr;
}

/**
 * Register timeline provider
 */
void NXCORE_EXPORTABLE RegisterObjectTimelineProvider(const char *name, const std::vector<const char*>& categories, ObjectTimelineProvider provider)
{
   for(const TimelineProviderRegistration& r : s_providers)
   {
      if (!strcmp(r.name, name))
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Object timeline provider \"%hs\" already registered", name);
         return;
      }
   }
   s_providers.push_back({ name, categories, provider });
   nxlog_debug_tag(DEBUG_TAG, 3, L"Registered object timeline provider \"%hs\"", name);
}

/**
 * Register event category
 */
void NXCORE_EXPORTABLE RegisterObjectTimelineEventCategory(uint32_t eventCode, const char *category)
{
   if ((FindBuiltinEventCategory(eventCode) != nullptr) || (s_moduleEventCategories.find(eventCode) != s_moduleEventCategories.end()))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Event code %u already assigned to object timeline category", eventCode);
      return;
   }
   s_moduleEventCategories.emplace(eventCode, category);
}

/**
 * Add category to list if not already there
 */
static void AddUniqueCategory(std::vector<const char*> *list, const char *category)
{
   for(const char *c : *list)
      if (!strcmp(c, category))
         return;
   list->push_back(category);
}

/**
 * Get categories produced from event log
 */
static std::vector<const char*> GetEventCategories()
{
   std::vector<const char*> categories;
   categories.push_back(CATEGORY_EVENT);
   categories.push_back(CATEGORY_THRESHOLD);
   for(int i = 0; s_builtinEventCategories[i].category != nullptr; i++)
      AddUniqueCategory(&categories, s_builtinEventCategories[i].category);
   for(const auto& e : s_moduleEventCategories)
      AddUniqueCategory(&categories, e.second);
   return categories;
}

/**
 * Get list of all known timeline categories
 */
std::vector<const char*> NXCORE_EXPORTABLE GetObjectTimelineCategories()
{
   std::vector<const char*> categories = GetEventCategories();
   AddUniqueCategory(&categories, CATEGORY_ALARM);
   AddUniqueCategory(&categories, CATEGORY_AUDIT);
   AddUniqueCategory(&categories, CATEGORY_DEPLOYMENT);
   for(const TimelineProviderRegistration& r : s_providers)
      for(const char *c : r.categories)
         AddUniqueCategory(&categories, c);
   return categories;
}

/**
 * Builder constructor
 */
ObjectTimelineBuilder::ObjectTimelineBuilder(const shared_ptr<NetObj>& object, time_t timeFrom, time_t timeTo, uint32_t userId,
         const StringSet *categories, int limitPerCategory, int minEventSeverity) : m_object(object), m_records(256, 256, Ownership::True)
{
   m_timeFrom = timeFrom;
   m_timeTo = timeTo;
   m_userId = userId;
   m_systemAccessRights = GetEffectiveSystemRights(userId);
   m_categories = categories;
   m_limitPerCategory = limitPerCategory;
   m_minEventSeverity = minEventSeverity;

   // Make every requested category visible in output even if it produces no records
   for(const char *c : GetObjectTimelineCategories())
      if (isCategoryRequested(c))
         m_state[c];
}

/**
 * Check if given category was requested
 */
bool ObjectTimelineBuilder::isCategoryRequested(const char *category) const
{
   if (m_categories == nullptr)
      return true;
   wchar_t name[64];
   utf8_to_wchar(category, -1, name, 64);
   return m_categories->contains(name);
}

/**
 * Mark category as not accessible for current user
 */
void ObjectTimelineBuilder::setAccessDenied(const char *category)
{
   m_state[category].accessDenied = true;
}

/**
 * Prepare range query for this object. Row limit (category limit + 1, so truncation can be detected) is added using
 * syntax of current database. Object ID, start and end of time range are bound to positions 1, 2 and 3.
 */
DB_STATEMENT ObjectTimelineBuilder::prepareRangeQuery(DB_HANDLE hdb, const wchar_t *columns, const wchar_t *tail) const
{
   int limit = m_limitPerCategory + 1;
   StringBuffer query;
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(L"SELECT TOP %d %s %s", limit, columns, tail);
         break;
      case DB_SYNTAX_ORACLE:
         query.appendFormattedString(L"SELECT * FROM (SELECT %s %s) WHERE ROWNUM<=%d", columns, tail, limit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(L"SELECT %s %s FETCH FIRST %d ROWS ONLY", columns, tail, limit);
         break;
      default:
         query.appendFormattedString(L"SELECT %s %s LIMIT %d", columns, tail, limit);
         break;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return nullptr;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_object->getId());
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_timeFrom));
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_timeTo));
   return hStmt;
}

/**
 * Add record
 */
bool ObjectTimelineBuilder::addRecord(time_t timestamp, time_t endTime, bool interval, const char *category, int severity, const wchar_t *summary, json_t *details)
{
   ObjectTimelineCategoryState& state = m_state[category];
   if (state.count >= m_limitPerCategory)
   {
      state.truncated = true;
      json_decref(details);
      return false;
   }
   state.count++;
   if (state.firstSequence == -1)
      state.firstSequence = m_records.size();
   if ((state.oldestTimestamp == 0) || (timestamp < state.oldestTimestamp))
      state.oldestTimestamp = timestamp;
   m_records.add(new ObjectTimelineRecord(timestamp, endTime, interval, category, severity, CHECK_NULL_EX(summary), details, m_records.size()));
   return true;
}

/**
 * Convert timeline to JSON (records sorted in ascending time order)
 */
json_t *ObjectTimelineBuilder::toJson()
{
   std::vector<ObjectTimelineRecord*> records;
   records.reserve(m_records.size());
   for(int i = 0; i < m_records.size(); i++)
      records.push_back(m_records.get(i));
   // Records with same timestamp are grouped by category in order categories were produced (event log based categories
   // first, so events precede alarms created from them); within category order is reversed because providers add records
   // in descending time order
   std::sort(records.begin(), records.end(),
      [this] (const ObjectTimelineRecord *a, const ObjectTimelineRecord *b) -> bool
      {
         if (a->timestamp != b->timestamp)
            return a->timestamp < b->timestamp;
         int ca = m_state.at(a->category).firstSequence;
         int cb = m_state.at(b->category).firstSequence;
         if (ca != cb)
            return ca < cb;
         return a->sequence > b->sequence;
      });

   json_t *root = json_object();

   json_t *object = json_object();
   json_object_set_new(object, "id", json_integer(m_object->getId()));
   json_object_set_new(object, "name", json_string_w(m_object->getName()));
   json_object_set_new(object, "class", json_string_w(m_object->getObjectClassName()));
   json_object_set_new(root, "object", object);
   json_object_set_new(root, "timeFrom", json_time_string(m_timeFrom));
   json_object_set_new(root, "timeTo", json_time_string(m_timeTo));

   json_t *categories = json_object();
   for(const auto& s : m_state)
   {
      json_t *c = json_object();
      if (s.second.accessDenied)
      {
         json_object_set_new(c, "accessDenied", json_true());
      }
      else
      {
         json_object_set_new(c, "returned", json_integer(s.second.count));
         if (s.second.truncated)
         {
            json_object_set_new(c, "truncated", json_true());
            json_object_set_new(c, "completeFrom", json_time_string(s.second.oldestTimestamp));
         }
      }
      json_object_set_new(categories, s.first.c_str(), c);
   }
   json_object_set_new(root, "categories", categories);

   json_t *list = json_array();
   for(ObjectTimelineRecord *r : records)
   {
      json_t *record = json_object();
      json_object_set_new(record, "time", json_time_string(r->timestamp));
      if (r->interval)
         json_object_set_new(record, "endTime", (r->endTime != 0) ? json_time_string(r->endTime) : json_string("open"));
      json_object_set_new(record, "category", json_string(r->category));
      if (r->severity >= 0)
         json_object_set_new(record, "severity", json_string(AlarmSeverityTextFromCode(r->severity)));
      json_object_set_new(record, "summary", json_string_w(r->summary));
      if (r->details != nullptr)
         json_object_update_missing(record, r->details);
      json_array_append_new(list, record);
   }
   json_object_set_new(root, "records", list);
   return root;
}

/**
 * Column expression for timestamp column that is timestamptz on TimescaleDB
 */
static String TimestampColumn(const wchar_t *column)
{
   StringBuffer s;
   if (g_dbSyntax == DB_SYNTAX_TSDB)
      s.append(L"date_part('epoch',").append(column).append(L")::int");
   else
      s.append(column);
   return s;
}

/**
 * Range condition for timestamp column that is timestamptz on TimescaleDB (two bind positions)
 */
static String TimestampRangeCondition(const wchar_t *column)
{
   StringBuffer s(column);
   if (g_dbSyntax == DB_SYNTAX_TSDB)
      s.append(L" BETWEEN to_timestamp(?) AND to_timestamp(?)");
   else
      s.append(L" BETWEEN ? AND ?");
   return s;
}

/**
 * Append comma-separated list of codes
 */
static void AppendCodeList(StringBuffer *query, const std::vector<uint32_t>& codes)
{
   query->append(L'(');
   for(size_t i = 0; i < codes.size(); i++)
   {
      if (i > 0)
         query->append(L',');
      query->append(codes[i]);
   }
   query->append(L')');
}

/**
 * Add events matching given condition as records of given category
 */
static void AddEventRecords(ObjectTimelineBuilder *builder, DB_HANDLE hdb, const char *category, const wchar_t *condition,
         const std::unordered_map<uint32_t, SharedString>& dciNames)
{
   StringBuffer tail(L"FROM event_log WHERE event_source=? AND ");
   tail.append(TimestampRangeCondition(L"event_timestamp"));
   tail.append(L" AND ");
   tail.append(condition);
   tail.append(L" ORDER BY event_timestamp DESC,event_id DESC");

   StringBuffer columns(L"event_id,");
   columns.append(TimestampColumn(L"event_timestamp"));
   columns.append(L",event_code,event_severity,dci_id,event_message,root_event_id");

   DB_STATEMENT hStmt = builder->prepareRangeQuery(hdb, columns, tail);
   if (hStmt == nullptr)
      return;

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         json_t *details = json_object();
         json_object_set_new(details, "eventId", json_integer(DBGetFieldUInt64(hResult, i, 0)));

         uint32_t eventCode = DBGetFieldUInt32(hResult, i, 2);
         wchar_t eventName[MAX_EVENT_NAME];
         EventNameFromCode(eventCode, eventName);
         json_object_set_new(details, "eventName", json_string_w(eventName));

         uint32_t dciId = DBGetFieldUInt32(hResult, i, 4);
         if (dciId != 0)
         {
            json_object_set_new(details, "dciId", json_integer(dciId));
            auto it = dciNames.find(dciId);
            if (it != dciNames.end())
               json_object_set_new(details, "dciName", json_string_w(it->second));
         }

         uint64_t rootEventId = DBGetFieldUInt64(hResult, i, 6);
         if (rootEventId != 0)
            json_object_set_new(details, "rootEventId", json_integer(rootEventId));

         wchar_t *message = DBGetField(hResult, i, 5, nullptr, 0);
         bool added = builder->addPointRecord(DBGetFieldUInt32(hResult, i, 1), category, DBGetFieldInt32(hResult, i, 3), message, details);
         MemFree(message);
         if (!added)
            break;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Event log based categories
 */
static void EventLogProvider(ObjectTimelineBuilder *builder, DB_HANDLE hdb)
{
   std::vector<const char*> categories = GetEventCategories();
   if (!(builder->getSystemAccessRights() & SYSTEM_ACCESS_VIEW_EVENT_LOG))
   {
      for(const char *c : categories)
         if (builder->isCategoryRequested(c))
            builder->setAccessDenied(c);
      return;
   }

   // Fixed classification (by event code only)
   std::map<std::string, std::vector<uint32_t>> fixedCodes;
   std::unordered_set<uint32_t> fixedCodeSet;
   for(int i = 0; s_builtinEventCategories[i].category != nullptr; i++)
   {
      fixedCodes[s_builtinEventCategories[i].category].push_back(s_builtinEventCategories[i].eventCode);
      fixedCodeSet.insert(s_builtinEventCategories[i].eventCode);
   }
   for(const auto& e : s_moduleEventCategories)
   {
      fixedCodes[e.second].push_back(e.first);
      fixedCodeSet.insert(e.first);
   }

   // Threshold events: default ones plus activation/deactivation events configured in object's thresholds
   std::unordered_set<uint32_t> thresholdCodeSet(std::begin(s_defaultThresholdEvents), std::end(s_defaultThresholdEvents));
   std::unordered_map<uint32_t, SharedString> dciNames;
   if (builder->getObject().isDataCollectionTarget())
   {
      unique_ptr<SharedObjectArray<DCObject>> dcObjects = static_cast<const DataCollectionTarget&>(builder->getObject()).getAllDCObjects(builder->getUserId());
      for(int i = 0; i < dcObjects->size(); i++)
      {
         DCObject *dco = dcObjects->get(i);
         dciNames.emplace(dco->getId(), dco->getName());
         if (dco->getType() == DCO_TYPE_ITEM)
         {
            DCItem *dci = static_cast<DCItem*>(dco);
            int count = dci->getThresholdCount();
            for(int j = 0; j < count; j++)
            {
               Threshold *t = dci->getThreshold(j);
               if (t != nullptr)
               {
                  thresholdCodeSet.insert(t->getEventCode());
                  thresholdCodeSet.insert(t->getRearmEventCode());
               }
            }
         }
         else if (dco->getType() == DCO_TYPE_TABLE)
         {
            DCTable *dct = static_cast<DCTable*>(dco);
            int count = dct->getThresholdCount();
            for(int j = 0; j < count; j++)
            {
               DCTableThreshold *t = dct->getThreshold(j);
               if (t != nullptr)
               {
                  thresholdCodeSet.insert(t->getActivationEvent());
                  thresholdCodeSet.insert(t->getDeactivationEvent());
               }
            }
         }
      }
   }
   std::vector<uint32_t> thresholdCodes;
   for(uint32_t c : thresholdCodeSet)
      if ((c != 0) && (fixedCodeSet.find(c) == fixedCodeSet.end()))
         thresholdCodes.push_back(c);
   std::sort(thresholdCodes.begin(), thresholdCodes.end());

   StringBuffer thresholdCondition(L"dci_id<>0 AND event_code IN ");
   AppendCodeList(&thresholdCondition, thresholdCodes);

   for(const auto& f : fixedCodes)
   {
      const char *category = nullptr;
      for(const char *c : categories)
         if (f.first == c)
            category = c;
      if ((category == nullptr) || !builder->isCategoryRequested(category))
         continue;

      StringBuffer condition(L"event_code IN ");
      AppendCodeList(&condition, f.second);
      AddEventRecords(builder, hdb, category, condition, dciNames);
   }

   if (builder->isCategoryRequested(CATEGORY_THRESHOLD))
      AddEventRecords(builder, hdb, CATEGORY_THRESHOLD, thresholdCondition, dciNames);

   if (builder->isCategoryRequested(CATEGORY_EVENT))
   {
      std::vector<uint32_t> allFixedCodes(fixedCodeSet.begin(), fixedCodeSet.end());
      std::sort(allFixedCodes.begin(), allFixedCodes.end());
      StringBuffer condition(L"event_code NOT IN ");
      AppendCodeList(&condition, allFixedCodes);
      condition.append(L" AND NOT (");
      condition.append(thresholdCondition);
      condition.append(L')');
      if (builder->getMinEventSeverity() > SEVERITY_NORMAL)
      {
         condition.append(L" AND event_severity>=");
         condition.append(builder->getMinEventSeverity());
      }
      AddEventRecords(builder, hdb, CATEGORY_EVENT, condition, dciNames);
   }
}

/**
 * Alarm state name
 */
static const wchar_t *AlarmStateName(int state)
{
   static const wchar_t *names[] = { L"outstanding", L"acknowledged", L"resolved", L"terminated" };
   state &= ALARM_STATE_MASK;
   return ((state >= 0) && (state <= ALARM_STATE_TERMINATED)) ? names[state] : L"unknown";
}

/**
 * Check alarm category access for alarm categories list loaded from database
 */
static bool CheckAlarmCategoryListAccess(const wchar_t *categoryList, uint32_t userId, uint64_t systemAccessRights)
{
   if ((userId == 0) || (systemAccessRights & SYSTEM_ACCESS_VIEW_ALL_ALARMS))
      return true;
   if (categoryList == nullptr)
      return false;
   StringList categories = String(categoryList).split(L",");
   for(int i = 0; i < categories.size(); i++)
   {
      uint32_t id = wcstoul(categories.get(i), nullptr, 10);
      if ((id != 0) && CheckAlarmCategoryAccess(userId, id))
         return true;
   }
   return false;
}

/**
 * Alarm information for timeline
 */
struct TimelineAlarmInfo
{
   String message;
   int severity;
   time_t creationTime;
   uint32_t eventCode;
};

/**
 * Pending alarm timeline record (alarm records are collected first and then added in descending time order)
 */
struct TimelineAlarmRecord
{
   time_t timestamp;
   uint64_t recordId;   // state change record ID (0 for alarm creation)
   uint32_t alarmId;
   int prevState;    // -1 for alarm creation
   int newState;
   uint32_t changedBy;
};

/**
 * Alarms (creation and state changes)
 */
static void AlarmProvider(ObjectTimelineBuilder *builder, DB_HANDLE hdb)
{
   if (!builder->isCategoryRequested(CATEGORY_ALARM))
      return;

   if (!builder->getObject().checkAccessRights(builder->getUserId(), OBJECT_ACCESS_READ_ALARMS))
   {
      builder->setAccessDenied(CATEGORY_ALARM);
      return;
   }

   // Alarms active at any moment within requested range
   std::unordered_map<uint32_t, TimelineAlarmInfo> alarms;
   std::vector<TimelineAlarmRecord> records;
   DB_STATEMENT hStmt = DBPrepare(hdb,
            L"SELECT alarm_id,creation_time,message,original_severity,source_event_code,alarm_category_ids FROM alarms "
            L"WHERE source_object_id=? AND creation_time<=? AND last_change_time>=?");
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, builder->getObjectId());
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(builder->getTimeTo()));
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(builder->getTimeFrom()));
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         wchar_t *categoryList = DBGetField(hResult, i, 5, nullptr, 0);
         bool accessible = CheckAlarmCategoryListAccess(categoryList, builder->getUserId(), builder->getSystemAccessRights());
         MemFree(categoryList);
         if (!accessible)
            continue;

         uint32_t alarmId = DBGetFieldUInt32(hResult, i, 0);
         time_t creationTime = DBGetFieldUInt32(hResult, i, 1);
         wchar_t *message = DBGetField(hResult, i, 2, nullptr, 0);
         alarms.emplace(alarmId, TimelineAlarmInfo{ String(message, -1, Ownership::True), DBGetFieldInt32(hResult, i, 3), creationTime, DBGetFieldUInt32(hResult, i, 4) });
         if ((creationTime >= builder->getTimeFrom()) && (creationTime <= builder->getTimeTo()))
            records.push_back(TimelineAlarmRecord{ creationTime, 0, alarmId, -1, ALARM_STATE_OUTSTANDING, 0 });
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   if (alarms.empty())
      return;

   hStmt = DBPrepare(hdb,
            L"SELECT s.alarm_id,s.change_time,s.prev_state,s.new_state,s.change_by,s.record_id FROM alarm_state_changes s "
            L"INNER JOIN alarms a ON a.alarm_id=s.alarm_id WHERE a.source_object_id=? AND s.change_time BETWEEN ? AND ?");
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, builder->getObjectId());
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(builder->getTimeFrom()));
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(builder->getTimeTo()));
   hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t alarmId = DBGetFieldUInt32(hResult, i, 0);
         if (alarms.find(alarmId) != alarms.end())
            records.push_back(TimelineAlarmRecord{ static_cast<time_t>(DBGetFieldUInt32(hResult, i, 1)), DBGetFieldUInt64(hResult, i, 5), alarmId,
                     DBGetFieldInt32(hResult, i, 2), DBGetFieldInt32(hResult, i, 3), DBGetFieldUInt32(hResult, i, 4) });
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   // Descending order; alarm creation (record ID 0) is the oldest record within same second
   std::sort(records.begin(), records.end(),
      [] (const TimelineAlarmRecord& a, const TimelineAlarmRecord& b) -> bool
      {
         return (a.timestamp != b.timestamp) ? (a.timestamp > b.timestamp) : (a.recordId > b.recordId);
      });

   for(const TimelineAlarmRecord& r : records)
   {
      const TimelineAlarmInfo& alarm = alarms.at(r.alarmId);
      json_t *details = json_object();
      json_object_set_new(details, "alarmId", json_integer(r.alarmId));

      StringBuffer summary;
      if (r.prevState == -1)
      {
         summary.append(L"Alarm created: ");
         wchar_t eventName[MAX_EVENT_NAME];
         EventNameFromCode(alarm.eventCode, eventName);
         json_object_set_new(details, "eventName", json_string_w(eventName));
      }
      else
      {
         summary.append(L"Alarm ");
         summary.append(AlarmStateName(r.newState));
         if (r.changedBy != 0)
         {
            wchar_t userName[MAX_USER_NAME];
            summary.append(L" by ");
            summary.append(ResolveUserId(r.changedBy, userName, true));
         }
         summary.append(L": ");
         json_object_set_new(details, "previousState", json_string_w(AlarmStateName(r.prevState)));
         json_object_set_new(details, "newState", json_string_w(AlarmStateName(r.newState)));
      }
      summary.append(alarm.message);

      if (!builder->addPointRecord(r.timestamp, CATEGORY_ALARM, alarm.severity, summary, details))
         break;
   }
}

/**
 * Audit log records for object
 */
static void AuditLogProvider(ObjectTimelineBuilder *builder, DB_HANDLE hdb)
{
   if (!builder->isCategoryRequested(CATEGORY_AUDIT))
      return;

   if (!(builder->getSystemAccessRights() & SYSTEM_ACCESS_VIEW_AUDIT_LOG))
   {
      builder->setAccessDenied(CATEGORY_AUDIT);
      return;
   }

   DB_STATEMENT hStmt = builder->prepareRangeQuery(hdb, L"record_id,timestamp,subsystem,success,user_id,workstation,message",
            L"FROM audit_log WHERE object_id=? AND timestamp BETWEEN ? AND ? ORDER BY timestamp DESC,record_id DESC");
   if (hStmt == nullptr)
      return;

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         json_t *details = json_object();
         json_object_set_new(details, "subsystem", json_string_w(DBGetFieldAsSharedString(hResult, i, 2)));
         json_object_set_new(details, "success", json_boolean(DBGetFieldInt32(hResult, i, 3) != 0));

         uint32_t userId = DBGetFieldUInt32(hResult, i, 4);
         if (userId == 0x7FFFFFFF)
         {
            json_object_set_new(details, "user", json_string("system"));
         }
         else
         {
            wchar_t userName[MAX_USER_NAME];
            json_object_set_new(details, "user", json_string_w(ResolveUserId(userId, userName, true)));
         }
         json_object_set_new(details, "workstation", json_string_w(DBGetFieldAsSharedString(hResult, i, 5)));

         wchar_t *message = DBGetField(hResult, i, 6, nullptr, 0);
         bool added = builder->addPointRecord(DBGetFieldUInt32(hResult, i, 1), CATEGORY_AUDIT, -1, message, details);
         MemFree(message);
         if (!added)
            break;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Package deployment job status name
 */
static const wchar_t *DeploymentStatusName(int status)
{
   static const wchar_t *names[] = { L"scheduled", L"pending", L"initializing", L"file transfer running", L"installation running",
            L"waiting for agent", L"completed", L"failed", L"cancelled" };
   return ((status >= PKG_JOB_SCHEDULED) && (status <= PKG_JOB_CANCELLED)) ? names[status] : L"unknown";
}

/**
 * Package deployment jobs for node
 */
static void DeploymentProvider(ObjectTimelineBuilder *builder, DB_HANDLE hdb)
{
   if (!builder->isCategoryRequested(CATEGORY_DEPLOYMENT) || (builder->getObject().getObjectClass() != OBJECT_NODE))
      return;

   if (!(builder->getSystemAccessRights() & SYSTEM_ACCESS_MANAGE_PACKAGES))
   {
      builder->setAccessDenied(CATEGORY_DEPLOYMENT);
      return;
   }

   StringBuffer columns(L"job_id,");
   columns.append(TimestampColumn(L"execution_time"));
   columns.append(L",");
   columns.append(TimestampColumn(L"completion_time"));
   columns.append(L",user_id,status,failure_reason,pkg_type,pkg_name,version,description");

   StringBuffer tail(L"FROM package_deployment_log WHERE node_id=? AND ");
   tail.append(TimestampRangeCondition(L"execution_time"));
   tail.append(L" ORDER BY execution_time DESC,job_id DESC");

   DB_STATEMENT hStmt = builder->prepareRangeQuery(hdb, columns, tail);
   if (hStmt == nullptr)
      return;

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         int status = DBGetFieldInt32(hResult, i, 4);
         SharedString failureReason = DBGetFieldAsSharedString(hResult, i, 5);
         SharedString packageName = DBGetFieldAsSharedString(hResult, i, 7);
         SharedString version = DBGetFieldAsSharedString(hResult, i, 8);

         json_t *details = json_object();
         json_object_set_new(details, "jobId", json_integer(DBGetFieldUInt32(hResult, i, 0)));
         json_object_set_new(details, "status", json_string_w(DeploymentStatusName(status)));
         wchar_t userName[MAX_USER_NAME];
         json_object_set_new(details, "user", json_string_w(ResolveUserId(DBGetFieldUInt32(hResult, i, 3), userName, true)));
         json_object_set_new(details, "packageType", json_string_w(DBGetFieldAsSharedString(hResult, i, 6)));
         json_object_set_new(details, "packageName", json_string_w(packageName));
         json_object_set_new(details, "packageVersion", json_string_w(version));
         SharedString description = DBGetFieldAsSharedString(hResult, i, 9);
         if (!description.isEmpty())
            json_object_set_new(details, "packageDescription", json_string_w(description));

         StringBuffer summary(L"Package deployment ");
         summary.append(packageName.cstr());
         if (!version.isEmpty())
            summary.append(L" ").append(version.cstr());
         summary.append(L": ").append(DeploymentStatusName(status));
         if (!failureReason.isEmpty())
            summary.append(L" (").append(failureReason.cstr()).append(L")");

         if (!builder->addIntervalRecord(DBGetFieldUInt32(hResult, i, 1), DBGetFieldUInt32(hResult, i, 2), CATEGORY_DEPLOYMENT,
                  (status == PKG_JOB_FAILED) ? SEVERITY_MAJOR : -1, summary, details))
            break;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Maintenance journal entries
 */
static void MaintenanceJournalProvider(ObjectTimelineBuilder *builder, DB_HANDLE hdb)
{
   if (!builder->isCategoryRequested(CATEGORY_MAINTENANCE))
      return;

   StringBuffer columns(L"record_id,");
   columns.append(TimestampColumn(L"creation_time"));
   columns.append(L",author,description");

   StringBuffer tail(L"FROM maintenance_journal WHERE object_id=? AND ");
   tail.append(TimestampRangeCondition(L"creation_time"));
   tail.append(L" ORDER BY creation_time DESC,record_id DESC");

   DB_STATEMENT hStmt = builder->prepareRangeQuery(hdb, columns, tail);
   if (hStmt == nullptr)
      return;

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         json_t *details = json_object();
         json_object_set_new(details, "journalRecordId", json_integer(DBGetFieldUInt32(hResult, i, 0)));
         wchar_t userName[MAX_USER_NAME];
         json_object_set_new(details, "author", json_string_w(ResolveUserId(DBGetFieldUInt32(hResult, i, 2), userName, true)));

         StringBuffer summary(L"Maintenance journal: ");
         wchar_t *text = DBGetField(hResult, i, 3, nullptr, 0);
         summary.append(CHECK_NULL_EX(text));
         MemFree(text);

         if (!builder->addPointRecord(DBGetFieldUInt32(hResult, i, 1), CATEGORY_MAINTENANCE, -1, summary, details))
            break;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
}

/**
 * Build timeline for given object
 */
json_t NXCORE_EXPORTABLE *BuildObjectTimeline(const shared_ptr<NetObj>& object, time_t timeFrom, time_t timeTo, uint32_t userId,
         const StringSet *categories, int limitPerCategory, int minEventSeverity)
{
   ObjectTimelineBuilder builder(object, timeFrom, timeTo, userId, categories, limitPerCategory, minEventSeverity);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   EventLogProvider(&builder, hdb);
   AlarmProvider(&builder, hdb);
   AuditLogProvider(&builder, hdb);
   DeploymentProvider(&builder, hdb);
   MaintenanceJournalProvider(&builder, hdb);
   for(const TimelineProviderRegistration& r : s_providers)
   {
      bool requested = false;
      for(const char *c : r.categories)
      {
         if (builder.isCategoryRequested(c))
         {
            requested = true;
            break;
         }
      }
      if (requested)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"Calling timeline provider \"%hs\" for object %s [%u]", r.name, object->getName(), object->getId());
         r.provider(&builder, hdb);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);

   return builder.toJson();
}
