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
** File: operational.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>
#include <nms_alarm.h>
#include <unordered_map>
#include <algorithm>

#define SUMMARY_CAP            5
#define FULL_CAP_DOWN_NODES   20
#define FULL_CAP_ALARM_GROUPS 30
#define FULL_CAP_EVENTS       20
#define FULL_CAP_ANOMALIES    30
#define EVENT_FETCH_HARD_CAP 500
#define SCOPE_INLINE_LIMIT  1000

/**
 * Parse minimum severity from request. Accepts: "critical", "major" (default), "minor", "warning".
 */
static int ParseMinSeverity(json_t *arguments)
{
   const char *s = json_object_get_string_utf8(arguments, "min_severity", "major");
   if (!stricmp(s, "critical"))
      return SEVERITY_CRITICAL;
   if (!stricmp(s, "minor"))
      return SEVERITY_MINOR;
   if (!stricmp(s, "warning"))
      return SEVERITY_WARNING;
   return SEVERITY_MAJOR;
}

/**
 * Render minimum severity as canonical string.
 */
static const char *MinSeverityToString(int s)
{
   switch(s)
   {
      case SEVERITY_CRITICAL: return "critical";
      case SEVERITY_MAJOR:    return "major";
      case SEVERITY_MINOR:    return "minor";
      case SEVERITY_WARNING:  return "warning";
      default:                return "unknown";
   }
}

/**
 * Build a short one-line reason describing why a node is in a broken state.
 * Output is ASCII, written to caller-supplied buffer. Returns the buffer.
 */
static const char *BuildNodeStateReason(const Node& node, char *buffer, size_t size)
{
   uint32_t state = node.getState();
   StringBuffer parts;

   if (state & DCSF_UNREACHABLE)
      parts.append(_T("unreachable"));

   const TCHAR *flagNames[] = {
      _T("agent"), _T("SNMP"), _T("ICMP"), _T("SSH"), _T("EtherNet/IP"), _T("Modbus")
   };
   uint32_t flagBits[] = {
      NSF_AGENT_UNREACHABLE, NSF_SNMP_UNREACHABLE, NSF_ICMP_UNREACHABLE,
      NSF_SSH_UNREACHABLE, NSF_ETHERNET_IP_UNREACHABLE, NSF_MODBUS_UNREACHABLE
   };
   StringBuffer unreachable;
   for(size_t i = 0; i < sizeof(flagBits) / sizeof(flagBits[0]); i++)
   {
      if (state & flagBits[i])
      {
         if (!unreachable.isEmpty())
            unreachable.append(_T("/"));
         unreachable.append(flagNames[i]);
      }
   }
   if (!unreachable.isEmpty())
   {
      if (!parts.isEmpty())
         parts.append(_T(": "));
      parts.append(unreachable);
      parts.append(_T(" unreachable"));
   }

   if (state & DCSF_NETWORK_PATH_PROBLEM)
   {
      if (!parts.isEmpty())
         parts.append(_T("; "));
      parts.append(_T("network path problem"));
   }

   if (parts.isEmpty())
   {
      char statusText[64];
      wchar_to_utf8(GetStatusAsText(node.getStatus(), false), -1, statusText, sizeof(statusText));
      snprintf(buffer, size, "status %s", statusText);
   }
   else
   {
      char *utf8 = parts.getUTF8String();
      strlcpy(buffer, utf8, size);
      MemFree(utf8);
   }
   return buffer;
}

/**
 * One down node entry (sorted oldest-down first).
 */
struct DownNodeEntry
{
   shared_ptr<NetObj> object;
   time_t downSince;
};

/**
 * Build node access predicate honoring scope and quiet filter.
 */
static bool IsNodeInScope(NetObj *node, uint32_t userId, const NetObj *scope, bool includeQuiet)
{
   if (!node->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return false;
   if (scope != nullptr)
   {
      uint32_t scopeId = scope->getId();
      if ((node->getId() != scopeId) && !node->isParent(scopeId))
         return false;
   }
   if (!includeQuiet)
   {
      if (node->isInMaintenanceMode())
         return false;
      int status = node->getStatus();
      if ((status == STATUS_UNMANAGED) || (status == STATUS_DISABLED))
         return false;
   }
   return true;
}

/**
 * Collect down/critical nodes. Returns true total before capping.
 * When since is set (> 0), only nodes that went down at or after that time are included
 * (nodes without a known down-since timestamp are excluded because newness cannot be established).
 */
static int CollectDownNodes(uint32_t userId, const NetObj *scope, bool includeQuiet, time_t since, int cap, bool fullDetail,
   json_t *output, bool *truncated)
{
   unique_ptr<SharedObjectArray<NetObj>> matched = g_idxNodeById.getObjects(
      [userId, scope, includeQuiet, since] (NetObj *o) -> bool
      {
         if (!IsNodeInScope(o, userId, scope, includeQuiet))
            return false;
         Node *node = static_cast<Node*>(o);
         if ((node->getStatus() != STATUS_CRITICAL) && !node->isDown())
            return false;
         if (since > 0)
         {
            time_t downSince = node->getDownSince();
            if ((downSince == 0) || (downSince < since))
               return false;
         }
         return true;
      });

   std::vector<DownNodeEntry> entries;
   entries.reserve(matched->size());
   for(int i = 0; i < matched->size(); i++)
   {
      shared_ptr<NetObj> o = matched->getShared(i);
      Node *node = static_cast<Node*>(o.get());
      entries.push_back({ o, node->getDownSince() });
   }

   // Oldest-down first; nodes without a down-since timestamp sort to the end.
   std::sort(entries.begin(), entries.end(), [](const DownNodeEntry& a, const DownNodeEntry& b) {
      if ((a.downSince == 0) != (b.downSince == 0))
         return a.downSince != 0;
      return a.downSince < b.downSince;
   });

   int total = static_cast<int>(entries.size());
   int emitted = std::min(total, cap);
   *truncated = (total > emitted);

   wchar_t nameBuffer[MAX_OBJECT_NAME];
   for(int i = 0; i < emitted; i++)
   {
      const DownNodeEntry& e = entries[i];
      Node *node = static_cast<Node*>(e.object.get());
      json_t *entry = json_object();
      json_object_set_new(entry, "id", json_integer(node->getId()));
      json_object_set_new(entry, "name", json_string_w(node->getName()));
      json_object_set_new(entry, "status", json_string_w(GetStatusAsText(node->getStatus(), false)));
      if (e.downSince > 0)
         json_object_set_new(entry, "since", json_time_string(e.downSince));

      if (fullDetail)
      {
         char reason[256];
         json_object_set_new(entry, "status_reason", json_string(BuildNodeStateReason(*node, reason, sizeof(reason))));

         // Parent path: walk up the first non-system parent chain (cap depth to avoid runaway).
         unique_ptr<SharedObjectArray<NetObj>> parents = node->getParents(-1);
         if (parents->size() > 0)
         {
            json_t *parentArray = json_array();
            for(int p = 0; (p < parents->size()) && (p < 4); p++)
               json_array_append_new(parentArray, json_string_w(parents->get(p)->getName()));
            json_object_set_new(entry, "parents", parentArray);
         }

         if (node->isPollable())
            node->getAsPollable()->pollStateToJson(entry);

         InetAddress ipAddr = node->getPrimaryIpAddress();
         if (ipAddr.isValid())
         {
            char addrBuffer[64];
            json_object_set_new(entry, "primary_ip", json_string(ipAddr.toStringA(addrBuffer)));
         }
      }
      json_array_append_new(output, entry);
   }
   (void)nameBuffer;
   return total;
}

/**
 * UTF-8 conversion helper for short bounded wide strings.
 */
static std::string WideToUtf8(const wchar_t *src)
{
   if (src == nullptr)
      return std::string();
   size_t wlen = wcslen(src);
   size_t bufSize = wlen * 4 + 1;
   std::vector<char> buf(bufSize);
   wchar_to_utf8(src, -1, buf.data(), bufSize);
   return std::string(buf.data());
}

/**
 * Per-source alarm group, accumulated while iterating active alarms.
 */
struct AlarmGroup
{
   uint32_t sourceId;
   std::string sourceName;
   std::string sourceClass;
   int alarmCount;
   int worstSeverity;
   time_t newestChange;
   // For full-mode samples: up to 5 alarms per group; cheap to track always.
   struct SampleAlarm
   {
      uint32_t id;
      int severity;
      time_t lastChange;
      time_t creation;
      std::string message;
   };
   std::vector<SampleAlarm> samples;
};

/**
 * Collect active alarms grouped by source object. Returns true source-group count before capping.
 * Sets criticalCount/majorCount to true totals of matching alarms (not groups), and affectedSources
 * to distinct source count. When since is set (> 0), only alarms created or changed at or after
 * that time are included.
 */
static int CollectAlarmGroups(uint32_t userId, const NetObj *scope, int minSeverity, bool includeQuiet,
   time_t since, int cap, bool fullDetail, json_t *output, bool *truncated,
   int *criticalCount, int *majorCount, int *affectedSources)
{
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   uint32_t scopeId = (scope != nullptr) ? scope->getId() : 0;

   std::unordered_map<uint32_t, AlarmGroup> groups;
   *criticalCount = 0;
   *majorCount = 0;

   ObjectArray<Alarm> *alarms = GetAlarms();
   for(int i = 0; i < alarms->size(); i++)
   {
      Alarm *alarm = alarms->get(i);

      int state = alarm->getState() & ALARM_STATE_MASK;
      if ((state != ALARM_STATE_OUTSTANDING) && (state != ALARM_STATE_ACKNOWLEDGED))
         continue;

      int severity = alarm->getCurrentSeverity();
      if (severity < minSeverity)
         continue;

      if ((since > 0) && (alarm->getLastChangeTime() < since))
         continue;

      shared_ptr<NetObj> source = FindObjectById(alarm->getSourceObject());
      if (source == nullptr)
         continue;
      if (!source->checkAccessRights(userId, OBJECT_ACCESS_READ_ALARMS))
         continue;
      if (!alarm->checkCategoryAccess(userId, systemRights))
         continue;
      if (!includeQuiet)
      {
         if (source->isInMaintenanceMode())
            continue;
         int sourceStatus = source->getStatus();
         if ((sourceStatus == STATUS_UNMANAGED) || (sourceStatus == STATUS_DISABLED))
            continue;
      }
      if ((scopeId != 0) && (source->getId() != scopeId) && !source->isParent(scopeId))
         continue;

      if (severity == SEVERITY_CRITICAL)
         (*criticalCount)++;
      else if (severity == SEVERITY_MAJOR)
         (*majorCount)++;

      uint32_t sid = source->getId();
      auto it = groups.find(sid);
      if (it == groups.end())
      {
         AlarmGroup g;
         g.sourceId = sid;
         g.sourceName = WideToUtf8(source->getName());
         g.sourceClass = WideToUtf8(source->getObjectClassName());
         g.alarmCount = 0;
         g.worstSeverity = severity;
         g.newestChange = alarm->getLastChangeTime();
         it = groups.emplace(sid, std::move(g)).first;
      }
      AlarmGroup& g = it->second;
      g.alarmCount++;
      if (severity > g.worstSeverity)
         g.worstSeverity = severity;
      if (alarm->getLastChangeTime() > g.newestChange)
         g.newestChange = alarm->getLastChangeTime();

      if (fullDetail && (g.samples.size() < 5))
      {
         AlarmGroup::SampleAlarm s;
         s.id = alarm->getAlarmId();
         s.severity = severity;
         s.lastChange = alarm->getLastChangeTime();
         s.creation = alarm->getCreationTime();
         s.message = WideToUtf8(alarm->getMessage());
         g.samples.push_back(std::move(s));
      }
   }
   delete alarms;

   *affectedSources = static_cast<int>(groups.size());

   // Move groups into a sortable vector
   std::vector<AlarmGroup> sorted;
   sorted.reserve(groups.size());
   for(auto& kv : groups)
      sorted.push_back(std::move(kv.second));

   std::sort(sorted.begin(), sorted.end(), [](const AlarmGroup& a, const AlarmGroup& b) {
      if (a.worstSeverity != b.worstSeverity)
         return a.worstSeverity > b.worstSeverity;
      if (a.alarmCount != b.alarmCount)
         return a.alarmCount > b.alarmCount;
      return a.newestChange > b.newestChange;
   });

   int total = static_cast<int>(sorted.size());
   int emitted = std::min(total, cap);
   *truncated = (total > emitted);

   for(int i = 0; i < emitted; i++)
   {
      const AlarmGroup& g = sorted[i];
      json_t *entry = json_object();
      json_t *source = json_object();
      json_object_set_new(source, "id", json_integer(g.sourceId));
      json_object_set_new(source, "name", json_string(g.sourceName.c_str()));
      json_object_set_new(source, "class", json_string(g.sourceClass.c_str()));
      json_object_set_new(entry, "source", source);
      json_object_set_new(entry, "alarm_count", json_integer(g.alarmCount));
      json_object_set_new(entry, "worst_severity", json_string(AlarmSeverityTextFromCode(g.worstSeverity)));
      json_object_set_new(entry, "newest", json_time_string(g.newestChange));

      if (fullDetail)
      {
         json_t *samples = json_array();
         for(const auto& s : g.samples)
         {
            json_t *aj = json_object();
            json_object_set_new(aj, "id", json_integer(s.id));
            json_object_set_new(aj, "severity", json_string(AlarmSeverityTextFromCode(s.severity)));
            json_object_set_new(aj, "message", json_string(s.message.c_str()));
            json_object_set_new(aj, "creation_time", json_time_string(s.creation));
            json_object_set_new(aj, "last_change_time", json_time_string(s.lastChange));
            json_array_append_new(samples, aj);
         }
         json_object_set_new(entry, "alarms", samples);
         json_object_set_new(entry, "total_in_group", json_integer(g.alarmCount));
      }
      json_array_append_new(output, entry);
   }

   return total;
}

/**
 * Collect event-source object IDs in scope subtree for SQL IN-clause.
 * Returns true if list was built, false if subtree is too large to inline.
 */
static bool CollectScopeEventSourceIds(const NetObj *scope, uint32_t userId, IntegerArray<uint32_t>& ids)
{
   uint32_t scopeId = scope->getId();
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [scopeId, userId] (NetObj *o) -> bool
      {
         if (!o->isEventSource() || !o->checkAccessRights(userId, OBJECT_ACCESS_READ))
            return false;
         return (o->getId() == scopeId) || o->isParent(scopeId);
      });

   if (objects->size() > SCOPE_INLINE_LIMIT)
      return false;
   for(int i = 0; i < objects->size(); i++)
      ids.add(objects->get(i)->getId());
   return true;
}

/**
 * Collect recent events from event_log. Returns true total fetched (already access-filtered).
 */
static int CollectRecentEvents(uint32_t userId, const NetObj *scope, int minSeverity, time_t windowStart,
   int cap, bool fullDetail, json_t *output, bool *truncated)
{
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
   {
      *truncated = false;
      return 0;
   }

   int fetchLimit = std::min(cap * 4, EVENT_FETCH_HARD_CAP);

   StringBuffer query;
   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   if (g_dbSyntax == DB_SYNTAX_MSSQL)
      query.appendFormattedString(_T("SELECT TOP %d event_id,"), fetchLimit);
   else
      query.append(_T("SELECT event_id,"));

   if (useTsdbTimestamp)
      query.append(_T("date_part('epoch',event_timestamp)::int AS event_timestamp,"));
   else
      query.append(_T("event_timestamp,"));

   query.append(_T("event_source,event_code,event_severity,event_message FROM event_log WHERE "));

   if (useTsdbTimestamp)
      query.appendFormattedString(_T("event_timestamp >= to_timestamp(%u)"), static_cast<uint32_t>(windowStart));
   else
      query.appendFormattedString(_T("event_timestamp >= %u"), static_cast<uint32_t>(windowStart));

   query.appendFormattedString(_T(" AND event_severity >= %d"), minSeverity);

   IntegerArray<uint32_t> scopeIds;
   bool scopeInlined = false;
   if (scope != nullptr)
   {
      scopeInlined = CollectScopeEventSourceIds(scope, userId, scopeIds);
      if (scopeInlined)
      {
         if (scopeIds.size() == 0)
         {
            query.append(_T(" AND 1=0"));
         }
         else
         {
            query.append(_T(" AND event_source IN ("));
            for(int i = 0; i < scopeIds.size(); i++)
            {
               if (i > 0)
                  query.append(_T(","));
               query.append(scopeIds.get(i));
            }
            query.append(_T(")"));
         }
      }
      else
      {
         AddObjectAccessConstraint(query, userId, _T("event_source"));
      }
   }
   else
   {
      AddObjectAccessConstraint(query, userId, _T("event_source"));
   }

   query.append(_T(" ORDER BY event_timestamp DESC"));

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(_T(" LIMIT %d"), fetchLimit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), fetchLimit);
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("F_OperationalStatus: events query: %s"), query.cstr());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);

   int total = 0;
   int emitted = 0;
   if (hResult != nullptr)
   {
      int rowCount = DBGetNumRows(hResult);
      for(int i = 0; i < rowCount; i++)
      {
         uint32_t sourceId = DBGetFieldUInt32(hResult, i, 2);

         // When subtree size forced us to use AddObjectAccessConstraint (or no scope), enforce scope filter in-memory.
         if ((scope != nullptr) && !scopeInlined)
         {
            shared_ptr<NetObj> source = FindObjectById(sourceId);
            if (source == nullptr)
               continue;
            uint32_t scopeId = scope->getId();
            if ((source->getId() != scopeId) && !source->isParent(scopeId))
               continue;
         }

         total++;
         if (emitted >= cap)
            continue;

         json_t *entry = json_object();
         json_object_set_new(entry, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         json_object_set_new(entry, "timestamp", json_time_string(static_cast<time_t>(DBGetFieldUInt32(hResult, i, 1))));
         json_object_set_new(entry, "source_object_id", json_integer(sourceId));
         json_object_set_new(entry, "source_object_name", json_string_w(GetObjectName(sourceId, L"unknown")));

         uint32_t code = DBGetFieldUInt32(hResult, i, 3);
         shared_ptr<EventTemplate> tmpl = FindEventTemplateByCode(code);
         if (tmpl != nullptr)
            json_object_set_new(entry, "name", json_string_t(tmpl->getName()));
         if (fullDetail)
            json_object_set_new(entry, "event_code", json_integer(code));

         int severity = DBGetFieldLong(hResult, i, 4);
         json_object_set_new(entry, "severity", json_string_t(GetEventSeverityName(severity)));

         if (fullDetail)
         {
            wchar_t *msg = DBGetField(hResult, i, 5, nullptr, 0);
            if (msg != nullptr)
            {
               // Cap message to ~200 chars to keep response token-bounded.
               if (wcslen(msg) > 200)
                  msg[200] = 0;
               json_object_set_new(entry, "message", json_string_w(msg));
               MemFree(msg);
            }
         }

         json_array_append_new(output, entry);
         emitted++;
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   *truncated = (total > emitted);
   return total;
}

/**
 * Collect DCIs with currently detected anomalies across data collection targets in scope.
 * Returns true total before capping. Optional glob pattern filters DCIs by user tag.
 */
static int CollectAnomalies(uint32_t userId, const NetObj *scope, bool includeQuiet, const wchar_t *tagPattern,
   int cap, bool fullDetail, json_t *output, bool *truncated)
{
   unique_ptr<SharedObjectArray<NetObj>> targets = g_idxObjectById.getObjects(
      [userId, scope, includeQuiet] (NetObj *o) -> bool
      {
         return o->isDataCollectionTarget() && IsNodeInScope(o, userId, scope, includeQuiet);
      });

   int total = 0;
   int emitted = 0;
   for(int i = 0; i < targets->size(); i++)
   {
      DataCollectionTarget *target = static_cast<DataCollectionTarget*>(targets->get(i));
      SharedObjectArray<DCObject> anomalies = target->getDCObjectsByFilter(
         [] (DCObject *dco) -> bool
         {
            if (dco->getType() != DCO_TYPE_ITEM)
               return false;
            DCItem *dci = static_cast<DCItem*>(dco);
            return dci->isAnomalyDetected() || dci->isAnomalyDetectedAI();
         }, userId);

      for(int j = 0; j < anomalies.size(); j++)
      {
         DCItem *dci = static_cast<DCItem*>(anomalies.get(j));

         SharedString userTag = dci->getUserTag();
         if ((tagPattern != nullptr) && !MatchString(tagPattern, CHECK_NULL_EX(userTag.cstr()), false))
            continue;

         total++;
         if (emitted >= cap)
            continue;

         json_t *entry = json_object();
         json_t *object = json_object();
         json_object_set_new(object, "id", json_integer(target->getId()));
         json_object_set_new(object, "name", json_string_w(target->getName()));
         json_object_set_new(object, "class", json_string_w(target->getObjectClassName()));
         json_object_set_new(entry, "object", object);
         json_object_set_new(entry, "dci_id", json_integer(dci->getId()));
         json_object_set_new(entry, "name", json_string_t(dci->getName().cstr()));
         json_object_set_new(entry, "description", json_string_t(dci->getDescription().cstr()));
         if (!userTag.isEmpty())
            json_object_set_new(entry, "tag", json_string_t(userTag.cstr()));

         const char *detectedBy;
         if (dci->isAnomalyDetected() && dci->isAnomalyDetectedAI())
            detectedBy = "both";
         else if (dci->isAnomalyDetectedAI())
            detectedBy = "ai";
         else
            detectedBy = "statistical";
         json_object_set_new(entry, "detected_by", json_string(detectedBy));

         ItemValue *lastValue = dci->getInternalLastValue();
         if (lastValue != nullptr)
         {
            json_object_set_new(entry, "last_value", json_string_w(lastValue->getString()));
            delete lastValue;
         }
         if (!dci->getLastValueTimestamp().isNull())
            json_object_set_new(entry, "last_value_timestamp", dci->getLastValueTimestamp().asJson());

         if (fullDetail)
         {
            SharedString instance = dci->getInstanceName();
            if (!instance.isEmpty())
               json_object_set_new(entry, "instance", json_string_t(instance.cstr()));
            SharedString unitName = dci->getUnitName();
            if (!unitName.isEmpty())
               json_object_set_new(entry, "unit", json_string_t(unitName.cstr()));
         }

         json_array_append_new(output, entry);
         emitted++;
      }
   }

   *truncated = (total > emitted);
   return total;
}

/**
 * Composite triage view: down nodes + grouped alarms + recent critical events + DCI anomalies.
 */
std::string F_OperationalStatus(json_t *arguments, uint32_t userId)
{
   const char *detailStr = json_object_get_string_utf8(arguments, "detail", "summary");
   bool fullDetail = !stricmp(detailStr, "full");

   int minSeverity = ParseMinSeverity(arguments);

   int windowMinutes = json_object_get_int32(arguments, "event_window_minutes", 60);
   if (windowMinutes < 5)
      windowMinutes = 5;
   else if (windowMinutes > 1440)
      windowMinutes = 1440;

   bool includeQuiet = json_object_get_boolean(arguments, "include_quiet", false);

   time_t since = 0;
   const char *sinceStr = json_object_get_string_utf8(arguments, "since", nullptr);
   if ((sinceStr != nullptr) && (*sinceStr != 0))
   {
      since = ParseTimestamp(sinceStr);
      if (since == 0)
         return std::string("Error: invalid \"since\" timestamp");
   }

   wchar_t tagPattern[256];
   bool tagPatternSet = false;
   const char *tagPatternStr = json_object_get_string_utf8(arguments, "dci_tag", nullptr);
   if ((tagPatternStr != nullptr) && (*tagPatternStr != 0))
   {
      utf8_to_wchar(tagPatternStr, -1, tagPattern, 256);
      tagPattern[255] = 0;
      tagPatternSet = true;
   }

   shared_ptr<NetObj> scope = FindObjectByNameOrId(arguments, "scope");
   if ((scope != nullptr) && !scope->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   int downCap = fullDetail ? FULL_CAP_DOWN_NODES : SUMMARY_CAP;
   int groupCap = fullDetail ? FULL_CAP_ALARM_GROUPS : SUMMARY_CAP;
   int eventCap = fullDetail ? FULL_CAP_EVENTS : SUMMARY_CAP;
   int anomalyCap = fullDetail ? FULL_CAP_ANOMALIES : SUMMARY_CAP;

   json_t *downNodes = json_array();
   bool downTruncated = false;
   int totalDown = CollectDownNodes(userId, scope.get(), includeQuiet, since, downCap, fullDetail, downNodes, &downTruncated);

   json_t *alarmGroups = json_array();
   bool alarmTruncated = false;
   int criticalCount = 0, majorCount = 0, affectedSources = 0;
   /* totalGroups intentionally unused beyond truncation flag */
   CollectAlarmGroups(userId, scope.get(), minSeverity, includeQuiet, since, groupCap, fullDetail,
      alarmGroups, &alarmTruncated, &criticalCount, &majorCount, &affectedSources);

   json_t *recentEvents = json_array();
   bool eventsTruncated = false;
   time_t eventWindowStart = (since > 0) ? since : time(nullptr) - static_cast<time_t>(windowMinutes) * 60;
   int totalEvents = CollectRecentEvents(userId, scope.get(), SEVERITY_CRITICAL, eventWindowStart,
      eventCap, fullDetail, recentEvents, &eventsTruncated);

   json_t *anomalies = json_array();
   bool anomaliesTruncated = false;
   int totalAnomalies = CollectAnomalies(userId, scope.get(), includeQuiet, tagPatternSet ? tagPattern : nullptr,
      anomalyCap, fullDetail, anomalies, &anomaliesTruncated);

   json_t *output = json_object();
   json_object_set_new(output, "generated_at", json_time_string(time(nullptr)));

   json_t *scopeJson = json_object();
   if (scope != nullptr)
   {
      json_object_set_new(scopeJson, "type", json_string("object"));
      json_object_set_new(scopeJson, "id", json_integer(scope->getId()));
      json_object_set_new(scopeJson, "name", json_string_w(scope->getName()));
      json_object_set_new(scopeJson, "class", json_string_w(scope->getObjectClassName()));
   }
   else
   {
      json_object_set_new(scopeJson, "type", json_string("infrastructure"));
   }
   json_object_set_new(output, "scope", scopeJson);

   json_t *filters = json_object();
   json_object_set_new(filters, "detail", json_string(fullDetail ? "full" : "summary"));
   json_object_set_new(filters, "min_severity", json_string(MinSeverityToString(minSeverity)));
   if (since > 0)
      json_object_set_new(filters, "since", json_time_string(since));
   else
      json_object_set_new(filters, "event_window_minutes", json_integer(windowMinutes));
   if (tagPatternSet)
      json_object_set_new(filters, "dci_tag", json_string(tagPatternStr));
   json_object_set_new(filters, "include_quiet", json_boolean(includeQuiet));
   json_object_set_new(output, "filters", filters);

   json_t *headline = json_object();
   json_object_set_new(headline, "down_nodes", json_integer(totalDown));
   json_object_set_new(headline, "critical_alarms", json_integer(criticalCount));
   json_object_set_new(headline, "major_alarms", json_integer(majorCount));
   json_object_set_new(headline, "recent_critical_events", json_integer(totalEvents));
   json_object_set_new(headline, "affected_sources", json_integer(affectedSources));
   json_object_set_new(headline, "dci_anomalies", json_integer(totalAnomalies));
   json_object_set_new(output, "headline", headline);

   json_object_set_new(output, "down_nodes", downNodes);
   json_object_set_new(output, "alarm_groups", alarmGroups);
   json_object_set_new(output, "recent_events", recentEvents);
   json_object_set_new(output, "dci_anomalies", anomalies);

   json_t *truncated = json_object();
   json_object_set_new(truncated, "down_nodes", json_boolean(downTruncated));
   json_object_set_new(truncated, "alarm_groups", json_boolean(alarmTruncated));
   json_object_set_new(truncated, "recent_events", json_boolean(eventsTruncated));
   json_object_set_new(truncated, "dci_anomalies", json_boolean(anomaliesTruncated));
   json_object_set_new(output, "truncated", truncated);

   return JsonToString(output);
}
