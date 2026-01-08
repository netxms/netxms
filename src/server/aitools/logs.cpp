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
** File: logs.cpp
**
**/

#include "aitools.h"
#include <nms_users.h>
#include <math.h>

/**
 * Syslog severity names (RFC 5424)
 */
static const TCHAR *s_syslogSeverityNames[] =
{
   _T("emergency"),     // 0
   _T("alert"),         // 1
   _T("critical"),      // 2
   _T("error"),         // 3
   _T("warning"),       // 4
   _T("notice"),        // 5
   _T("informational"), // 6
   _T("debug")          // 7
};

/**
 * Syslog facility names (RFC 5424)
 */
static const TCHAR *s_syslogFacilityNames[] =
{
   _T("kern"),      // 0
   _T("user"),      // 1
   _T("mail"),      // 2
   _T("daemon"),    // 3
   _T("auth"),      // 4
   _T("syslog"),    // 5
   _T("lpr"),       // 6
   _T("news"),      // 7
   _T("uucp"),      // 8
   _T("cron"),      // 9
   _T("authpriv"),  // 10
   _T("ftp"),       // 11
   _T("ntp"),       // 12
   _T("security"),  // 13
   _T("console"),   // 14
   _T("clock"),     // 15
   _T("local0"),    // 16
   _T("local1"),    // 17
   _T("local2"),    // 18
   _T("local3"),    // 19
   _T("local4"),    // 20
   _T("local5"),    // 21
   _T("local6"),    // 22
   _T("local7")     // 23
};

/**
 * Windows event severity names
 */
static const TCHAR *GetWindowsEventSeverityName(int severity)
{
   switch(severity)
   {
      case 1: return _T("error");
      case 2: return _T("warning");
      case 4: return _T("information");
      case 8: return _T("auditSuccess");
      case 16: return _T("auditFailure");
      default: return _T("unknown");
   }
}

/**
 * NetXMS event severity names
 */
static const TCHAR *s_eventSeverityNames[] =
{
   _T("normal"),   // 0
   _T("warning"),  // 1
   _T("minor"),    // 2
   _T("major"),    // 3
   _T("critical")  // 4
};

/**
 * Event origin names
 */
static const TCHAR *GetEventOriginName(int origin)
{
   switch(origin)
   {
      case 0: return _T("SYSTEM");
      case 1: return _T("AGENT");
      case 2: return _T("SNMP");
      case 3: return _T("SYSLOG");
      case 4: return _T("WINDOWS_EVENT");
      case 5: return _T("SCRIPT");
      case 6: return _T("REMOTE_SERVER");
      default: return _T("UNKNOWN");
   }
}

/**
 * Get syslog severity name
 */
static inline const TCHAR *GetSyslogSeverityName(int severity)
{
   return ((severity >= 0) && (severity <= 7)) ? s_syslogSeverityNames[severity] : _T("unknown");
}

/**
 * Get syslog facility name
 */
static inline const TCHAR *GetSyslogFacilityName(int facility)
{
   return ((facility >= 0) && (facility <= 23)) ? s_syslogFacilityNames[facility] : _T("unknown");
}

/**
 * Get NetXMS event severity name
 */
static inline const TCHAR *GetEventSeverityName(int severity)
{
   return ((severity >= 0) && (severity <= 4)) ? s_eventSeverityNames[severity] : _T("unknown");
}

/**
 * Escape string for SQL LIKE pattern
 */
static StringBuffer EscapeSQLLikePattern(const TCHAR *pattern)
{
   StringBuffer result;
   for (const TCHAR *p = pattern; *p != 0; p++)
   {
      if ((*p == _T('%')) || (*p == _T('_')) || (*p == _T('\\')))
         result.append(_T('\\'));
      result.append(*p);
   }
   return result;
}

/**
 * Check if character is hex digit
 */
static inline bool IsHexDigit(TCHAR c)
{
   return (c >= _T('0') && c <= _T('9')) || (c >= _T('a') && c <= _T('f')) || (c >= _T('A') && c <= _T('F'));
}

/**
 * Normalize log message for pattern detection
 * Replaces variable parts with placeholders using simple scanning
 */
static StringBuffer NormalizeLogMessage(const TCHAR *message)
{
   StringBuffer result;
   const TCHAR *p = message;

   while (*p != 0)
   {
      // Check for IPv4 address pattern: d.d.d.d
      if (_istdigit(*p))
      {
         const TCHAR *start = p;
         int dotCount = 0;
         int octetDigits = 0;
         bool isIP = true;

         while (*p != 0 && isIP)
         {
            if (_istdigit(*p))
            {
               octetDigits++;
               if (octetDigits > 3)
                  isIP = false;
               p++;
            }
            else if (*p == _T('.') && octetDigits > 0 && octetDigits <= 3)
            {
               dotCount++;
               octetDigits = 0;
               p++;
            }
            else
            {
               break;
            }
         }

         if (isIP && dotCount == 3 && octetDigits > 0 && octetDigits <= 3)
         {
            result.append(_T("<IP>"));
            continue;
         }
         else
         {
            // Not an IP, check if it's a large number (4+ digits)
            p = start;
            int digits = 0;
            while (_istdigit(*p))
            {
               digits++;
               p++;
            }
            if (digits >= 4)
            {
               result.append(_T("<NUM>"));
               continue;
            }
            else
            {
               // Just copy the digits
               for (int i = 0; i < digits; i++)
                  result.append(start[i]);
               continue;
            }
         }
      }

      // Check for hex strings: 0x...
      if (*p == _T('0') && *(p + 1) == _T('x'))
      {
         p += 2;
         int hexDigits = 0;
         while (IsHexDigit(*p))
         {
            hexDigits++;
            p++;
         }
         if (hexDigits > 0)
         {
            result.append(_T("<HEX>"));
            continue;
         }
         else
         {
            result.append(_T("0x"));
            continue;
         }
      }

      // Check for MAC address: XX:XX:XX:XX:XX:XX
      if (IsHexDigit(*p) && IsHexDigit(*(p + 1)) && *(p + 2) == _T(':'))
      {
         const TCHAR *start = p;
         int segments = 0;
         bool isMAC = true;

         while (isMAC && segments < 6)
         {
            if (IsHexDigit(*p) && IsHexDigit(*(p + 1)))
            {
               segments++;
               p += 2;
               if (segments < 6)
               {
                  if (*p == _T(':'))
                     p++;
                  else
                     isMAC = false;
               }
            }
            else
            {
               isMAC = false;
            }
         }

         if (isMAC && segments == 6)
         {
            result.append(_T("<MAC>"));
            continue;
         }
         else
         {
            // Not a MAC, copy original
            p = start;
            result.append(*p++);
            continue;
         }
      }

      // Default: copy character
      result.append(*p++);
   }

   return result;
}

/**
 * Get object name by ID (returns empty string if not found)
 */
static String GetObjectNameById(uint32_t objectId)
{
   shared_ptr<NetObj> object = FindObjectById(objectId);
   return (object != nullptr) ? String(object->getName()) : String();
}

/**
 * Add object access constraint to query
 */
static void AddObjectAccessConstraint(StringBuffer& query, uint32_t userId, const TCHAR *objectIdColumn)
{
   if (userId == 0)
      return;  // System user has access to everything

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [userId] (NetObj *object) -> bool
      {
         return object->isEventSource() && object->checkAccessRights(userId, OBJECT_ACCESS_READ);
      });

   if (objects->size() == 0)
   {
      query.append(_T(" AND 1=0"));  // No access to any objects
      return;
   }

   // Build IN clause with accessible object IDs
   query.append(_T(" AND "));
   query.append(objectIdColumn);
   query.append(_T(" IN ("));
   for (int i = 0; i < objects->size(); i++)
   {
      if (i > 0)
         query.append(_T(","));
      query.append(objects->get(i)->getId());
   }
   query.append(_T(")"));
}

/**
 * Search syslog messages
 */
std::string F_SearchSyslog(json_t *arguments, uint32_t userId)
{
   // Check system access rights
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   if ((systemRights & SYSTEM_ACCESS_VIEW_SYSLOG) == 0)
      return std::string("User does not have permission to view syslog");

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-60m");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *textPattern = json_object_get_string_utf8(arguments, "text_pattern", nullptr);
   const char *tagFilter = json_object_get_string_utf8(arguments, "tag", nullptr);
   int32_t limit = json_object_get_int32(arguments, "limit", 100);
   if (limit > 1000)
      limit = 1000;

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t sourceObjectId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      sourceObjectId = object->getId();
   }

   // Parse severity filter
   IntegerArray<int32_t> severities;
   json_t *severityArg = json_object_get(arguments, "severity");
   if (severityArg != nullptr)
   {
      if (json_is_array(severityArg))
      {
         size_t index;
         json_t *value;
         json_array_foreach(severityArg, index, value)
         {
            if (json_is_integer(value))
               severities.add(static_cast<int32_t>(json_integer_value(value)));
         }
      }
      else if (json_is_integer(severityArg))
      {
         severities.add(static_cast<int32_t>(json_integer_value(severityArg)));
      }
   }

   // Parse facility filter
   IntegerArray<int32_t> facilities;
   json_t *facilityArg = json_object_get(arguments, "facility");
   if (facilityArg != nullptr)
   {
      if (json_is_array(facilityArg))
      {
         size_t index;
         json_t *value;
         json_array_foreach(facilityArg, index, value)
         {
            if (json_is_integer(value))
               facilities.add(static_cast<int32_t>(json_integer_value(value)));
         }
      }
      else if (json_is_integer(facilityArg))
      {
         facilities.add(static_cast<int32_t>(json_integer_value(facilityArg)));
      }
   }

   // Build query
   StringBuffer query;
   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(_T("SELECT TOP %d msg_id,"), limit);
         break;
      default:
         query.append(_T("SELECT msg_id,"));
         break;
   }

   if (useTsdbTimestamp)
      query.append(_T("date_part('epoch',msg_timestamp)::int AS msg_timestamp,"));
   else
      query.append(_T("msg_timestamp,"));

   query.append(_T("source_object_id,facility,severity,hostname,msg_tag,msg_text FROM syslog WHERE "));

   if (useTsdbTimestamp)
      query.appendFormattedString(_T("msg_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      query.appendFormattedString(_T("msg_timestamp BETWEEN %u AND %u"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (sourceObjectId != 0)
      query.appendFormattedString(_T(" AND source_object_id=%u"), sourceObjectId);
   else
      AddObjectAccessConstraint(query, userId, _T("source_object_id"));

   if (severities.size() > 0)
   {
      query.append(_T(" AND severity IN ("));
      for (int i = 0; i < severities.size(); i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(severities.get(i));
      }
      query.append(_T(")"));
   }

   if (facilities.size() > 0)
   {
      query.append(_T(" AND facility IN ("));
      for (int i = 0; i < facilities.size(); i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(facilities.get(i));
      }
      query.append(_T(")"));
   }

   if ((textPattern != nullptr) && (textPattern[0] != 0))
   {
      String escapedPattern = EscapeSQLLikePattern(String(textPattern, "utf-8"));
      query.append(_T(" AND LOWER(msg_text) LIKE LOWER('%"));
      query.append(escapedPattern);
      query.append(_T("%')"));
   }

   if ((tagFilter != nullptr) && (tagFilter[0] != 0))
   {
      String tag(tagFilter, "utf-8");
      query.append(_T(" AND msg_tag='"));
      query.append(tag);
      query.append(_T("'"));
   }

   query.append(_T(" ORDER BY msg_timestamp DESC"));

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(_T(" LIMIT %d"), limit);
         break;
      case DB_SYNTAX_ORACLE:
         // Oracle needs ROWNUM in a subquery - simplified for now
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limit);
         break;
   }

   // Execute query
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);

   json_t *output = json_object();
   json_t *messages = json_array();

   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      json_object_set_new(output, "count", json_integer(count));

      for (int i = 0; i < count; i++)
      {
         json_t *msg = json_object();

         json_object_set_new(msg, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         json_object_set_new(msg, "timestamp", json_integer(DBGetFieldULong(hResult, i, 1)));

         uint32_t objId = DBGetFieldULong(hResult, i, 2);
         json_object_set_new(msg, "sourceObjectId", json_integer(objId));
         String objName = GetObjectNameById(objId);
         if (!objName.isEmpty())
            json_object_set_new(msg, "sourceObjectName", json_string_t(objName));

         int facility = DBGetFieldLong(hResult, i, 3);
         json_object_set_new(msg, "facility", json_integer(facility));
         json_object_set_new(msg, "facilityName", json_string_t(GetSyslogFacilityName(facility)));

         int severity = DBGetFieldLong(hResult, i, 4);
         json_object_set_new(msg, "severity", json_integer(severity));
         json_object_set_new(msg, "severityName", json_string_t(GetSyslogSeverityName(severity)));

         TCHAR buffer[256];
         json_object_set_new(msg, "hostname", json_string_t(DBGetField(hResult, i, 5, buffer, 256)));
         json_object_set_new(msg, "tag", json_string_t(DBGetField(hResult, i, 6, buffer, 256)));

         TCHAR *msgText = DBGetField(hResult, i, 7, nullptr, 0);
         if (msgText != nullptr)
         {
            json_object_set_new(msg, "message", json_string_t(msgText));
            MemFree(msgText);
         }

         json_array_append_new(messages, msg);
      }
      DBFreeResult(hResult);
   }
   else
   {
      json_object_set_new(output, "count", json_integer(0));
   }

   DBConnectionPoolReleaseConnection(hdb);

   json_object_set_new(output, "messages", messages);
   return JsonToString(output);
}

/**
 * Search Windows event log entries
 */
std::string F_SearchWindowsEvents(json_t *arguments, uint32_t userId)
{
   // Check system access rights
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
      return std::string("User does not have permission to view event log");

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-60m");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *logName = json_object_get_string_utf8(arguments, "log_name", nullptr);
   const char *eventSource = json_object_get_string_utf8(arguments, "event_source", nullptr);
   const char *textPattern = json_object_get_string_utf8(arguments, "text_pattern", nullptr);
   int32_t limit = json_object_get_int32(arguments, "limit", 100);
   if (limit > 1000)
      limit = 1000;

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t nodeId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      nodeId = object->getId();
   }

   // Parse severity filter
   IntegerArray<int32_t> severities;
   json_t *severityArg = json_object_get(arguments, "severity");
   if (severityArg != nullptr)
   {
      if (json_is_array(severityArg))
      {
         size_t index;
         json_t *value;
         json_array_foreach(severityArg, index, value)
         {
            if (json_is_integer(value))
               severities.add(static_cast<int32_t>(json_integer_value(value)));
         }
      }
      else if (json_is_integer(severityArg))
      {
         severities.add(static_cast<int32_t>(json_integer_value(severityArg)));
      }
   }

   // Parse event_code filter
   IntegerArray<int32_t> eventCodes;
   json_t *eventCodeArg = json_object_get(arguments, "event_code");
   if (eventCodeArg != nullptr)
   {
      if (json_is_array(eventCodeArg))
      {
         size_t index;
         json_t *value;
         json_array_foreach(eventCodeArg, index, value)
         {
            if (json_is_integer(value))
               eventCodes.add(static_cast<int32_t>(json_integer_value(value)));
         }
      }
      else if (json_is_integer(eventCodeArg))
      {
         eventCodes.add(static_cast<int32_t>(json_integer_value(eventCodeArg)));
      }
   }

   // Build query
   StringBuffer query;
   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(_T("SELECT TOP %d id,"), limit);
         break;
      default:
         query.append(_T("SELECT id,"));
         break;
   }

   if (useTsdbTimestamp)
      query.append(_T("date_part('epoch',event_timestamp)::int AS event_timestamp,date_part('epoch',origin_timestamp)::int AS origin_timestamp,"));
   else
      query.append(_T("event_timestamp,origin_timestamp,"));

   query.append(_T("node_id,log_name,event_source,event_severity,event_code,message FROM win_event_log WHERE "));

   if (useTsdbTimestamp)
      query.appendFormattedString(_T("event_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      query.appendFormattedString(_T("event_timestamp BETWEEN %u AND %u"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (nodeId != 0)
      query.appendFormattedString(_T(" AND node_id=%u"), nodeId);
   else
      AddObjectAccessConstraint(query, userId, _T("node_id"));

   if ((logName != nullptr) && (logName[0] != 0))
   {
      String log(logName, "utf-8");
      query.append(_T(" AND log_name='"));
      query.append(log);
      query.append(_T("'"));
   }

   if ((eventSource != nullptr) && (eventSource[0] != 0))
   {
      String source(eventSource, "utf-8");
      query.append(_T(" AND event_source='"));
      query.append(source);
      query.append(_T("'"));
   }

   if (severities.size() > 0)
   {
      query.append(_T(" AND event_severity IN ("));
      for (int i = 0; i < severities.size(); i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(severities.get(i));
      }
      query.append(_T(")"));
   }

   if (eventCodes.size() > 0)
   {
      query.append(_T(" AND event_code IN ("));
      for (int i = 0; i < eventCodes.size(); i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(eventCodes.get(i));
      }
      query.append(_T(")"));
   }

   if ((textPattern != nullptr) && (textPattern[0] != 0))
   {
      String escapedPattern = EscapeSQLLikePattern(String(textPattern, "utf-8"));
      query.append(_T(" AND LOWER(message) LIKE LOWER('%"));
      query.append(escapedPattern);
      query.append(_T("%')"));
   }

   query.append(_T(" ORDER BY event_timestamp DESC"));

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(_T(" LIMIT %d"), limit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limit);
         break;
   }

   // Execute query
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);

   json_t *output = json_object();
   json_t *events = json_array();

   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      json_object_set_new(output, "count", json_integer(count));

      for (int i = 0; i < count; i++)
      {
         json_t *event = json_object();

         json_object_set_new(event, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         json_object_set_new(event, "timestamp", json_integer(DBGetFieldULong(hResult, i, 1)));
         json_object_set_new(event, "originTimestamp", json_integer(DBGetFieldULong(hResult, i, 2)));

         uint32_t objId = DBGetFieldULong(hResult, i, 3);
         json_object_set_new(event, "nodeId", json_integer(objId));
         String objName = GetObjectNameById(objId);
         if (!objName.isEmpty())
            json_object_set_new(event, "nodeName", json_string_t(objName));

         TCHAR buffer[256];
         json_object_set_new(event, "logName", json_string_t(DBGetField(hResult, i, 4, buffer, 256)));
         json_object_set_new(event, "eventSource", json_string_t(DBGetField(hResult, i, 5, buffer, 256)));

         int severity = DBGetFieldLong(hResult, i, 6);
         json_object_set_new(event, "severity", json_integer(severity));
         json_object_set_new(event, "severityName", json_string_t(GetWindowsEventSeverityName(severity)));

         json_object_set_new(event, "eventCode", json_integer(DBGetFieldULong(hResult, i, 7)));

         TCHAR *msgText = DBGetField(hResult, i, 8, nullptr, 0);
         if (msgText != nullptr)
         {
            json_object_set_new(event, "message", json_string_t(msgText));
            MemFree(msgText);
         }

         json_array_append_new(events, event);
      }
      DBFreeResult(hResult);
   }
   else
   {
      json_object_set_new(output, "count", json_integer(0));
   }

   DBConnectionPoolReleaseConnection(hdb);

   json_object_set_new(output, "events", events);
   return JsonToString(output);
}

/**
 * Search SNMP trap log entries
 */
std::string F_SearchSnmpTraps(json_t *arguments, uint32_t userId)
{
   // Check system access rights
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   if ((systemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) == 0)
      return std::string("User does not have permission to view trap log");

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-60m");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *trapOid = json_object_get_string_utf8(arguments, "trap_oid", nullptr);
   const char *ipAddress = json_object_get_string_utf8(arguments, "ip_address", nullptr);
   const char *varbindPattern = json_object_get_string_utf8(arguments, "varbind_pattern", nullptr);
   int32_t limit = json_object_get_int32(arguments, "limit", 100);
   if (limit > 1000)
      limit = 1000;

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t objectId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      objectId = object->getId();
   }

   // Build query
   StringBuffer query;
   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(_T("SELECT TOP %d trap_id,"), limit);
         break;
      default:
         query.append(_T("SELECT trap_id,"));
         break;
   }

   if (useTsdbTimestamp)
      query.append(_T("date_part('epoch',trap_timestamp)::int AS trap_timestamp,"));
   else
      query.append(_T("trap_timestamp,"));

   query.append(_T("object_id,ip_addr,trap_oid,trap_varlist FROM snmp_trap_log WHERE "));

   if (useTsdbTimestamp)
      query.appendFormattedString(_T("trap_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      query.appendFormattedString(_T("trap_timestamp BETWEEN %u AND %u"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (objectId != 0)
      query.appendFormattedString(_T(" AND object_id=%u"), objectId);
   else
      AddObjectAccessConstraint(query, userId, _T("object_id"));

   if ((trapOid != nullptr) && (trapOid[0] != 0))
   {
      String oid(trapOid, "utf-8");
      // Support prefix matching (e.g., "1.3.6.1.6.3.1.1.5" matches all standard traps)
      query.append(_T(" AND trap_oid LIKE '"));
      query.append(EscapeSQLLikePattern(oid));
      query.append(_T("%'"));
   }

   if ((ipAddress != nullptr) && (ipAddress[0] != 0))
   {
      String ip(ipAddress, "utf-8");
      query.append(_T(" AND ip_addr='"));
      query.append(ip);
      query.append(_T("'"));
   }

   if ((varbindPattern != nullptr) && (varbindPattern[0] != 0))
   {
      String escapedPattern = EscapeSQLLikePattern(String(varbindPattern, "utf-8"));
      query.append(_T(" AND LOWER(trap_varlist) LIKE LOWER('%"));
      query.append(escapedPattern);
      query.append(_T("%')"));
   }

   query.append(_T(" ORDER BY trap_timestamp DESC"));

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(_T(" LIMIT %d"), limit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limit);
         break;
   }

   // Execute query
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);

   json_t *output = json_object();
   json_t *traps = json_array();

   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      json_object_set_new(output, "count", json_integer(count));

      for (int i = 0; i < count; i++)
      {
         json_t *trap = json_object();

         json_object_set_new(trap, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         json_object_set_new(trap, "timestamp", json_integer(DBGetFieldULong(hResult, i, 1)));

         uint32_t objId = DBGetFieldULong(hResult, i, 2);
         json_object_set_new(trap, "objectId", json_integer(objId));
         String objName = GetObjectNameById(objId);
         if (!objName.isEmpty())
            json_object_set_new(trap, "objectName", json_string_t(objName));

         TCHAR buffer[64];
         json_object_set_new(trap, "sourceIp", json_string_t(DBGetField(hResult, i, 3, buffer, 64)));

         TCHAR oidBuffer[256];
         json_object_set_new(trap, "trapOid", json_string_t(DBGetField(hResult, i, 4, oidBuffer, 256)));

         // Parse varbind list (stored as text)
         TCHAR *varlist = DBGetField(hResult, i, 5, nullptr, 0);
         if (varlist != nullptr)
         {
            json_t *varbinds = json_array();
            // Varbinds are typically stored as "OID=value;OID=value;..."
            StringList parts(varlist, _T(";"));
            for (int j = 0; j < parts.size(); j++)
            {
               const TCHAR *part = parts.get(j);
               const TCHAR *eq = _tcschr(part, _T('='));
               if (eq != nullptr)
               {
                  json_t *vb = json_object();
                  String oid(part, eq - part);
                  json_object_set_new(vb, "oid", json_string_t(oid));
                  json_object_set_new(vb, "value", json_string_t(eq + 1));
                  json_array_append_new(varbinds, vb);
               }
            }
            json_object_set_new(trap, "varbinds", varbinds);
            MemFree(varlist);
         }
         else
         {
            json_object_set_new(trap, "varbinds", json_array());
         }

         json_array_append_new(traps, trap);
      }
      DBFreeResult(hResult);
   }
   else
   {
      json_object_set_new(output, "count", json_integer(0));
   }

   DBConnectionPoolReleaseConnection(hdb);

   json_object_set_new(output, "traps", traps);
   return JsonToString(output);
}

/**
 * Search NetXMS system events
 */
std::string F_SearchEvents(json_t *arguments, uint32_t userId)
{
   // Check system access rights
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
      return std::string("User does not have permission to view event log");

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-60m");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *textPattern = json_object_get_string_utf8(arguments, "text_pattern", nullptr);
   const char *originFilter = json_object_get_string_utf8(arguments, "origin", nullptr);
   const char *tagsFilter = json_object_get_string_utf8(arguments, "tags", nullptr);
   int32_t limit = json_object_get_int32(arguments, "limit", 100);
   if (limit > 1000)
      limit = 1000;

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t sourceObjectId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      sourceObjectId = object->getId();
   }

   // Parse severity filter
   IntegerArray<int32_t> severities;
   json_t *severityArg = json_object_get(arguments, "severity");
   if (severityArg != nullptr)
   {
      if (json_is_array(severityArg))
      {
         size_t index;
         json_t *value;
         json_array_foreach(severityArg, index, value)
         {
            if (json_is_integer(value))
               severities.add(static_cast<int32_t>(json_integer_value(value)));
         }
      }
      else if (json_is_integer(severityArg))
      {
         severities.add(static_cast<int32_t>(json_integer_value(severityArg)));
      }
   }

   // Parse event_code filter
   uint32_t eventCode = 0;
   json_t *eventCodeArg = json_object_get(arguments, "event_code");
   if (eventCodeArg != nullptr)
   {
      if (json_is_integer(eventCodeArg))
      {
         eventCode = static_cast<uint32_t>(json_integer_value(eventCodeArg));
      }
      else if (json_is_string(eventCodeArg))
      {
         // Try to resolve event name to code
         const char *eventName = json_string_value(eventCodeArg);
         WCHAR eventNameW[256];
         utf8_to_wchar(eventName, -1, eventNameW, 256);
         shared_ptr<EventTemplate> tmpl = FindEventTemplateByName(eventNameW);
         if (tmpl != nullptr)
            eventCode = tmpl->getCode();
      }
   }

   // Parse origin to numeric value
   int originValue = -1;
   if ((originFilter != nullptr) && (originFilter[0] != 0))
   {
      if (!stricmp(originFilter, "SYSTEM"))
         originValue = 0;
      else if (!stricmp(originFilter, "AGENT"))
         originValue = 1;
      else if (!stricmp(originFilter, "SNMP"))
         originValue = 2;
      else if (!stricmp(originFilter, "SYSLOG"))
         originValue = 3;
      else if (!stricmp(originFilter, "WINDOWS_EVENT"))
         originValue = 4;
      else if (!stricmp(originFilter, "SCRIPT"))
         originValue = 5;
      else if (!stricmp(originFilter, "REMOTE_SERVER"))
         originValue = 6;
   }

   // Build query
   StringBuffer query;
   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query.appendFormattedString(_T("SELECT TOP %d event_id,"), limit);
         break;
      default:
         query.append(_T("SELECT event_id,"));
         break;
   }

   if (useTsdbTimestamp)
      query.append(_T("date_part('epoch',event_timestamp)::int AS event_timestamp,date_part('epoch',origin_timestamp)::int AS origin_timestamp,"));
   else
      query.append(_T("event_timestamp,origin_timestamp,"));

   query.append(_T("event_source,event_code,event_severity,event_message,event_tags,origin FROM event_log WHERE "));

   if (useTsdbTimestamp)
      query.appendFormattedString(_T("event_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      query.appendFormattedString(_T("event_timestamp BETWEEN %u AND %u"),
                                   static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (sourceObjectId != 0)
      query.appendFormattedString(_T(" AND event_source=%u"), sourceObjectId);
   else
      AddObjectAccessConstraint(query, userId, _T("event_source"));

   if (eventCode != 0)
      query.appendFormattedString(_T(" AND event_code=%u"), eventCode);

   if (severities.size() > 0)
   {
      query.append(_T(" AND event_severity IN ("));
      for (int i = 0; i < severities.size(); i++)
      {
         if (i > 0)
            query.append(_T(","));
         query.append(severities.get(i));
      }
      query.append(_T(")"));
   }

   if (originValue >= 0)
      query.appendFormattedString(_T(" AND origin=%d"), originValue);

   if ((tagsFilter != nullptr) && (tagsFilter[0] != 0))
   {
      String escapedTags = EscapeSQLLikePattern(String(tagsFilter, "utf-8"));
      query.append(_T(" AND LOWER(event_tags) LIKE LOWER('%"));
      query.append(escapedTags);
      query.append(_T("%')"));
   }

   if ((textPattern != nullptr) && (textPattern[0] != 0))
   {
      String escapedPattern = EscapeSQLLikePattern(String(textPattern, "utf-8"));
      query.append(_T(" AND LOWER(event_message) LIKE LOWER('%"));
      query.append(escapedPattern);
      query.append(_T("%')"));
   }

   query.append(_T(" ORDER BY event_timestamp DESC"));

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.appendFormattedString(_T(" LIMIT %d"), limit);
         break;
      case DB_SYNTAX_DB2:
         query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limit);
         break;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("F_SearchEvents: executing query: %s"), query.cstr());

   // Execute query
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);

   json_t *output = json_object();
   json_t *events = json_array();

   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      json_object_set_new(output, "count", json_integer(count));

      for (int i = 0; i < count; i++)
      {
         json_t *event = json_object();

         json_object_set_new(event, "id", json_integer(DBGetFieldInt64(hResult, i, 0)));
         json_object_set_new(event, "timestamp", json_integer(DBGetFieldULong(hResult, i, 1)));
         json_object_set_new(event, "originTimestamp", json_integer(DBGetFieldULong(hResult, i, 2)));

         uint32_t objId = DBGetFieldULong(hResult, i, 3);
         json_object_set_new(event, "sourceObjectId", json_integer(objId));
         String objName = GetObjectNameById(objId);
         if (!objName.isEmpty())
            json_object_set_new(event, "sourceObjectName", json_string_t(objName));

         uint32_t code = DBGetFieldULong(hResult, i, 4);
         json_object_set_new(event, "eventCode", json_integer(code));

         // Try to get event name from template
         shared_ptr<EventTemplate> tmpl = FindEventTemplateByCode(code);
         if (tmpl != nullptr)
            json_object_set_new(event, "eventName", json_string_t(tmpl->getName()));

         int severity = DBGetFieldLong(hResult, i, 5);
         json_object_set_new(event, "severity", json_integer(severity));
         json_object_set_new(event, "severityName", json_string_t(GetEventSeverityName(severity)));

         TCHAR *msgText = DBGetField(hResult, i, 6, nullptr, 0);
         if (msgText != nullptr)
         {
            json_object_set_new(event, "message", json_string_t(msgText));
            MemFree(msgText);
         }

         TCHAR tagsBuffer[1024];
         const TCHAR *tags = DBGetField(hResult, i, 7, tagsBuffer, 1024);
         if ((tags != nullptr) && (tags[0] != 0))
         {
            json_t *tagsArray = json_array();
            StringList tagList(tags, _T(","));
            for (int j = 0; j < tagList.size(); j++)
               json_array_append_new(tagsArray, json_string_t(tagList.get(j)));
            json_object_set_new(event, "tags", tagsArray);
         }

         int origin = DBGetFieldLong(hResult, i, 8);
         json_object_set_new(event, "origin", json_string_t(GetEventOriginName(origin)));

         json_array_append_new(events, event);
      }
      DBFreeResult(hResult);
   }
   else
   {
      json_object_set_new(output, "count", json_integer(0));
   }

   DBConnectionPoolReleaseConnection(hdb);

   json_object_set_new(output, "events", events);
   return JsonToString(output);
}

/**
 * Get log statistics
 */
std::string F_GetLogStatistics(json_t *arguments, uint32_t userId)
{
   // Get log_type parameter (required)
   const char *logType = json_object_get_string_utf8(arguments, "log_type", nullptr);
   if ((logType == nullptr) || (logType[0] == 0))
      return std::string("log_type parameter is required (syslog, windows_events, snmp_traps, events)");

   // Determine table and check permissions
   const TCHAR *tableName = nullptr;
   const TCHAR *timestampColumn = nullptr;
   const TCHAR *objectIdColumn = nullptr;
   const TCHAR *severityColumn = nullptr;

   uint64_t systemRights = GetEffectiveSystemRights(userId);

   if (!stricmp(logType, "syslog"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_SYSLOG) == 0)
         return std::string("User does not have permission to view syslog");
      tableName = _T("syslog");
      timestampColumn = _T("msg_timestamp");
      objectIdColumn = _T("source_object_id");
      severityColumn = _T("severity");
   }
   else if (!stricmp(logType, "windows_events"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
         return std::string("User does not have permission to view event log");
      tableName = _T("win_event_log");
      timestampColumn = _T("event_timestamp");
      objectIdColumn = _T("node_id");
      severityColumn = _T("event_severity");
   }
   else if (!stricmp(logType, "snmp_traps"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) == 0)
         return std::string("User does not have permission to view trap log");
      tableName = _T("snmp_trap_log");
      timestampColumn = _T("trap_timestamp");
      objectIdColumn = _T("object_id");
      severityColumn = nullptr;  // SNMP traps don't have severity
   }
   else if (!stricmp(logType, "events"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
         return std::string("User does not have permission to view event log");
      tableName = _T("event_log");
      timestampColumn = _T("event_timestamp");
      objectIdColumn = _T("event_source");
      severityColumn = _T("event_severity");
   }
   else
   {
      return std::string("Invalid log_type. Valid values: syslog, windows_events, snmp_traps, events");
   }

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-24h");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *groupBy = json_object_get_string_utf8(arguments, "group_by", "hour");

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t objectId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      objectId = object->getId();
   }

   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   json_t *output = json_object();
   json_object_set_new(output, "logType", json_string(logType));

   json_t *timeRange = json_object();
   json_object_set_new(timeRange, "from", json_integer(timeFrom));
   json_object_set_new(timeRange, "to", json_integer(timeTo));
   json_object_set_new(output, "timeRange", timeRange);

   // Build base WHERE clause
   StringBuffer whereClause;
   if (useTsdbTimestamp)
      whereClause.appendFormattedString(_T("%s BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                         timestampColumn, static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      whereClause.appendFormattedString(_T("%s BETWEEN %u AND %u"),
                                         timestampColumn, static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (objectId != 0)
      whereClause.appendFormattedString(_T(" AND %s=%u"), objectIdColumn, objectId);

   // Get total count
   StringBuffer countQuery;
   countQuery.appendFormattedString(_T("SELECT COUNT(*) FROM %s WHERE %s"), tableName, whereClause.cstr());

   DB_RESULT hResult = DBSelect(hdb, countQuery);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         json_object_set_new(output, "totalCount", json_integer(DBGetFieldInt64(hResult, 0, 0)));
      DBFreeResult(hResult);
   }

   // Get statistics by group_by
   json_t *statistics = json_array();

   if (!stricmp(groupBy, "hour") || !stricmp(groupBy, "day"))
   {
      // Time-based grouping
      StringBuffer statsQuery;
      if (useTsdbTimestamp)
      {
         if (!stricmp(groupBy, "hour"))
            statsQuery.appendFormattedString(_T("SELECT date_trunc('hour', %s) AS period, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY period ORDER BY period DESC"),
                                              timestampColumn, tableName, whereClause.cstr());
         else
            statsQuery.appendFormattedString(_T("SELECT date_trunc('day', %s) AS period, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY period ORDER BY period DESC"),
                                              timestampColumn, tableName, whereClause.cstr());
      }
      else
      {
         // For non-TSDB databases, use integer division for grouping
         int divisor = !stricmp(groupBy, "hour") ? 3600 : 86400;
         statsQuery.appendFormattedString(_T("SELECT (%s / %d) * %d AS period, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY period ORDER BY period DESC"),
                                           timestampColumn, divisor, divisor, tableName, whereClause.cstr());
      }

      hResult = DBSelect(hdb, statsQuery);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            json_t *stat = json_object();
            json_object_set_new(stat, "period", json_integer(DBGetFieldInt64(hResult, i, 0)));
            json_object_set_new(stat, "count", json_integer(DBGetFieldInt64(hResult, i, 1)));
            json_array_append_new(statistics, stat);
         }
         DBFreeResult(hResult);
      }
   }
   else if (!stricmp(groupBy, "source"))
   {
      // Group by source object
      StringBuffer statsQuery;
      statsQuery.appendFormattedString(_T("SELECT %s, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY %s ORDER BY cnt DESC"),
                                        objectIdColumn, tableName, whereClause.cstr(), objectIdColumn);

      hResult = DBSelect(hdb, statsQuery);
      if (hResult != nullptr)
      {
         int count = std::min(DBGetNumRows(hResult), 20);  // Top 20 sources
         for (int i = 0; i < count; i++)
         {
            json_t *stat = json_object();
            uint32_t objId = DBGetFieldULong(hResult, i, 0);
            json_object_set_new(stat, "objectId", json_integer(objId));
            String objName = GetObjectNameById(objId);
            if (!objName.isEmpty())
               json_object_set_new(stat, "objectName", json_string_t(objName));
            json_object_set_new(stat, "count", json_integer(DBGetFieldInt64(hResult, i, 1)));
            json_array_append_new(statistics, stat);
         }
         DBFreeResult(hResult);
      }
   }
   else if (!stricmp(groupBy, "severity") && (severityColumn != nullptr))
   {
      // Group by severity
      StringBuffer statsQuery;
      statsQuery.appendFormattedString(_T("SELECT %s, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY %s ORDER BY %s"),
                                        severityColumn, tableName, whereClause.cstr(), severityColumn, severityColumn);

      hResult = DBSelect(hdb, statsQuery);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            json_t *stat = json_object();
            int severity = DBGetFieldLong(hResult, i, 0);
            json_object_set_new(stat, "severity", json_integer(severity));

            // Add severity name based on log type
            if (!stricmp(logType, "syslog"))
               json_object_set_new(stat, "severityName", json_string_t(GetSyslogSeverityName(severity)));
            else if (!stricmp(logType, "windows_events"))
               json_object_set_new(stat, "severityName", json_string_t(GetWindowsEventSeverityName(severity)));
            else if (!stricmp(logType, "events"))
               json_object_set_new(stat, "severityName", json_string_t(GetEventSeverityName(severity)));

            json_object_set_new(stat, "count", json_integer(DBGetFieldInt64(hResult, i, 1)));
            json_array_append_new(statistics, stat);
         }
         DBFreeResult(hResult);
      }
   }

   json_object_set_new(output, "statistics", statistics);

   // Get top sources (always included)
   json_t *topSources = json_array();
   StringBuffer topSourcesQuery;
   topSourcesQuery.appendFormattedString(_T("SELECT %s, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY %s ORDER BY cnt DESC"),
                                          objectIdColumn, tableName, whereClause.cstr(), objectIdColumn);

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         // Already limited by TOP in SELECT
         break;
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         topSourcesQuery.append(_T(" LIMIT 10"));
         break;
      case DB_SYNTAX_DB2:
         topSourcesQuery.append(_T(" FETCH FIRST 10 ROWS ONLY"));
         break;
   }

   hResult = DBSelect(hdb, topSourcesQuery);
   if (hResult != nullptr)
   {
      int count = std::min(DBGetNumRows(hResult), 10);
      for (int i = 0; i < count; i++)
      {
         json_t *source = json_object();
         uint32_t objId = DBGetFieldULong(hResult, i, 0);
         json_object_set_new(source, "objectId", json_integer(objId));
         String objName = GetObjectNameById(objId);
         if (!objName.isEmpty())
            json_object_set_new(source, "objectName", json_string_t(objName));
         json_object_set_new(source, "count", json_integer(DBGetFieldInt64(hResult, i, 1)));
         json_array_append_new(topSources, source);
      }
      DBFreeResult(hResult);
   }
   json_object_set_new(output, "topSources", topSources);

   // Get severity breakdown (if applicable)
   if (severityColumn != nullptr)
   {
      json_t *severityBreakdown = json_object();
      StringBuffer severityQuery;
      severityQuery.appendFormattedString(_T("SELECT %s, COUNT(*) AS cnt FROM %s WHERE %s GROUP BY %s"),
                                           severityColumn, tableName, whereClause.cstr(), severityColumn);

      hResult = DBSelect(hdb, severityQuery);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            int severity = DBGetFieldLong(hResult, i, 0);
            int64_t cnt = DBGetFieldInt64(hResult, i, 1);

            const TCHAR *name = nullptr;
            if (!stricmp(logType, "syslog"))
               name = GetSyslogSeverityName(severity);
            else if (!stricmp(logType, "windows_events"))
               name = GetWindowsEventSeverityName(severity);
            else if (!stricmp(logType, "events"))
               name = GetEventSeverityName(severity);

            if (name != nullptr)
            {
               char nameUtf8[64];
               wchar_to_utf8(name, -1, nameUtf8, 64);
               json_object_set_new(severityBreakdown, nameUtf8, json_integer(cnt));
            }
         }
         DBFreeResult(hResult);
      }
      json_object_set_new(output, "severityBreakdown", severityBreakdown);
   }

   DBConnectionPoolReleaseConnection(hdb);

   return JsonToString(output);
}

/**
 * Correlate logs across multiple sources
 */
std::string F_CorrelateLogs(json_t *arguments, uint32_t userId)
{
   // Get required parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   if ((objectName == nullptr) || (objectName[0] == 0))
      return std::string("object parameter is required");

   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", nullptr);
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   if ((timeFromStr == nullptr) || (timeToStr == nullptr))
      return std::string("time_from and time_to parameters are required");

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = ParseTimestamp(timeToStr);
   if ((timeFrom == 0) || (timeTo == 0))
      return std::string("Invalid time format");

   bool includeNeighbors = json_object_get_boolean(arguments, "include_neighbors", true);
   int32_t limitPerSource = json_object_get_int32(arguments, "limit_per_source", 50);
   if (limitPerSource > 200)
      limitPerSource = 200;

   // Resolve primary object
   shared_ptr<NetObj> primaryObject = FindObjectByNameOrId(objectName);
   if ((primaryObject == nullptr) || !primaryObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      char buffer[256];
      snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
      return std::string(buffer);
   }

   // Check system access rights for all log types
   uint64_t systemRights = GetEffectiveSystemRights(userId);
   bool canViewSyslog = (systemRights & SYSTEM_ACCESS_VIEW_SYSLOG) != 0;
   bool canViewEventLog = (systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) != 0;
   bool canViewTrapLog = (systemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) != 0;

   // Parse log_types filter
   bool includeSyslog = canViewSyslog;
   bool includeWindowsEvents = canViewEventLog;
   bool includeSnmpTraps = canViewTrapLog;
   bool includeEvents = canViewEventLog;

   json_t *logTypesArg = json_object_get(arguments, "log_types");
   if (json_is_array(logTypesArg) && (json_array_size(logTypesArg) > 0))
   {
      includeSyslog = false;
      includeWindowsEvents = false;
      includeSnmpTraps = false;
      includeEvents = false;

      size_t index;
      json_t *value;
      json_array_foreach(logTypesArg, index, value)
      {
         const char *lt = json_string_value(value);
         if (lt != nullptr)
         {
            if (!stricmp(lt, "syslog") && canViewSyslog)
               includeSyslog = true;
            else if (!stricmp(lt, "windows_events") && canViewEventLog)
               includeWindowsEvents = true;
            else if (!stricmp(lt, "snmp_traps") && canViewTrapLog)
               includeSnmpTraps = true;
            else if (!stricmp(lt, "events") && canViewEventLog)
               includeEvents = true;
         }
      }
   }

   // Build list of objects to include
   ObjectArray<NetObj> includedObjects(16, 16, Ownership::False);
   includedObjects.add(primaryObject.get());

   // Get L2 neighbors if requested
   if (includeNeighbors && primaryObject->getObjectClass() == OBJECT_NODE)
   {
      shared_ptr<LinkLayerNeighbors> neighbors = static_pointer_cast<Node>(primaryObject)->getLinkLayerNeighbors();
      if (neighbors != nullptr)
      {
         for (int i = 0; i < neighbors->size(); i++)
         {
            LL_NEIGHBOR_INFO *info = neighbors->getConnection(i);
            if (info != nullptr)
            {
               shared_ptr<NetObj> neighbor = FindObjectById(info->objectId);
               if ((neighbor != nullptr) && neighbor->checkAccessRights(userId, OBJECT_ACCESS_READ))
               {
                  // Avoid duplicates
                  bool found = false;
                  for (int j = 0; j < includedObjects.size(); j++)
                  {
                     if (includedObjects.get(j)->getId() == neighbor->getId())
                     {
                        found = true;
                        break;
                     }
                  }
                  if (!found)
                     includedObjects.add(neighbor.get());
               }
            }
         }
      }
   }

   // Structure to hold timeline entries
   struct TimelineEntry
   {
      time_t timestamp;
      const char *type;
      uint32_t objectId;
      StringBuffer objectName;
      StringBuffer severity;
      StringBuffer message;
      json_t *extra;

      TimelineEntry() : timestamp(0), type(nullptr), objectId(0), extra(nullptr) {}
      ~TimelineEntry() { if (extra != nullptr) json_decref(extra); }
   };

   ObjectArray<TimelineEntry> timeline(256, 64, Ownership::True);

   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Build object ID list for IN clause
   StringBuffer objectIdList;
   for (int i = 0; i < includedObjects.size(); i++)
   {
      if (i > 0)
         objectIdList.append(_T(","));
      objectIdList.append(includedObjects.get(i)->getId());
   }

   // Query syslog
   if (includeSyslog)
   {
      StringBuffer query;
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.appendFormattedString(_T("SELECT TOP %d "), limitPerSource);
            break;
         default:
            query.append(_T("SELECT "));
            break;
      }

      if (useTsdbTimestamp)
         query.append(_T("date_part('epoch',msg_timestamp)::int AS msg_timestamp,"));
      else
         query.append(_T("msg_timestamp,"));

      query.append(_T("source_object_id,severity,msg_text FROM syslog WHERE "));

      if (useTsdbTimestamp)
         query.appendFormattedString(_T("msg_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
      else
         query.appendFormattedString(_T("msg_timestamp BETWEEN %u AND %u"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

      query.appendFormattedString(_T(" AND source_object_id IN (%s) ORDER BY msg_timestamp"), objectIdList.cstr());

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            query.appendFormattedString(_T(" LIMIT %d"), limitPerSource);
            break;
         case DB_SYNTAX_DB2:
            query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limitPerSource);
            break;
      }

      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            TimelineEntry *entry = new TimelineEntry();
            entry->timestamp = DBGetFieldULong(hResult, i, 0);
            entry->type = "syslog";
            entry->objectId = DBGetFieldULong(hResult, i, 1);
            entry->objectName = GetObjectNameById(entry->objectId);
            entry->severity = GetSyslogSeverityName(DBGetFieldLong(hResult, i, 2));

            TCHAR *msg = DBGetField(hResult, i, 3, nullptr, 0);
            if (msg != nullptr)
            {
               entry->message = msg;
               MemFree(msg);
            }
            timeline.add(entry);
         }
         DBFreeResult(hResult);
      }
   }

   // Query SNMP traps
   if (includeSnmpTraps)
   {
      StringBuffer query;
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.appendFormattedString(_T("SELECT TOP %d "), limitPerSource);
            break;
         default:
            query.append(_T("SELECT "));
            break;
      }

      if (useTsdbTimestamp)
         query.append(_T("date_part('epoch',trap_timestamp)::int AS trap_timestamp,"));
      else
         query.append(_T("trap_timestamp,"));

      query.append(_T("object_id,trap_oid,trap_varlist FROM snmp_trap_log WHERE "));

      if (useTsdbTimestamp)
         query.appendFormattedString(_T("trap_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
      else
         query.appendFormattedString(_T("trap_timestamp BETWEEN %u AND %u"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

      query.appendFormattedString(_T(" AND object_id IN (%s) ORDER BY trap_timestamp"), objectIdList.cstr());

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            query.appendFormattedString(_T(" LIMIT %d"), limitPerSource);
            break;
         case DB_SYNTAX_DB2:
            query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limitPerSource);
            break;
      }

      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            TimelineEntry *entry = new TimelineEntry();
            entry->timestamp = DBGetFieldULong(hResult, i, 0);
            entry->type = "snmp_trap";
            entry->objectId = DBGetFieldULong(hResult, i, 1);
            entry->objectName = GetObjectNameById(entry->objectId);

            TCHAR oidBuffer[256];
            DBGetField(hResult, i, 2, oidBuffer, 256);

            TCHAR *varlist = DBGetField(hResult, i, 3, nullptr, 0);
            entry->message = oidBuffer;
            if (varlist != nullptr)
            {
               entry->message.append(_T(" "));
               entry->message.append(varlist);
               MemFree(varlist);
            }
            timeline.add(entry);
         }
         DBFreeResult(hResult);
      }
   }

   // Query system events
   if (includeEvents)
   {
      StringBuffer query;
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.appendFormattedString(_T("SELECT TOP %d "), limitPerSource);
            break;
         default:
            query.append(_T("SELECT "));
            break;
      }

      if (useTsdbTimestamp)
         query.append(_T("date_part('epoch',event_timestamp)::int AS event_timestamp,"));
      else
         query.append(_T("event_timestamp,"));

      query.append(_T("event_source,event_code,event_severity,event_message FROM event_log WHERE "));

      if (useTsdbTimestamp)
         query.appendFormattedString(_T("event_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
      else
         query.appendFormattedString(_T("event_timestamp BETWEEN %u AND %u"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

      query.appendFormattedString(_T(" AND event_source IN (%s) ORDER BY event_timestamp"), objectIdList.cstr());

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            query.appendFormattedString(_T(" LIMIT %d"), limitPerSource);
            break;
         case DB_SYNTAX_DB2:
            query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limitPerSource);
            break;
      }

      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            TimelineEntry *entry = new TimelineEntry();
            entry->timestamp = DBGetFieldULong(hResult, i, 0);
            entry->type = "event";
            entry->objectId = DBGetFieldULong(hResult, i, 1);
            entry->objectName = GetObjectNameById(entry->objectId);
            entry->severity = GetEventSeverityName(DBGetFieldLong(hResult, i, 3));

            TCHAR *msg = DBGetField(hResult, i, 4, nullptr, 0);
            if (msg != nullptr)
            {
               entry->message = msg;
               MemFree(msg);
            }

            // Add event name
            uint32_t eventCode = DBGetFieldULong(hResult, i, 2);
            shared_ptr<EventTemplate> tmpl = FindEventTemplateByCode(eventCode);
            if (tmpl != nullptr)
            {
               entry->extra = json_object();
               json_object_set_new(entry->extra, "eventCode", json_integer(eventCode));
               json_object_set_new(entry->extra, "eventName", json_string_t(tmpl->getName()));
            }

            timeline.add(entry);
         }
         DBFreeResult(hResult);
      }
   }

   // Query Windows events
   if (includeWindowsEvents)
   {
      StringBuffer query;
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.appendFormattedString(_T("SELECT TOP %d "), limitPerSource);
            break;
         default:
            query.append(_T("SELECT "));
            break;
      }

      if (useTsdbTimestamp)
         query.append(_T("date_part('epoch',event_timestamp)::int AS event_timestamp,"));
      else
         query.append(_T("event_timestamp,"));

      query.append(_T("node_id,event_severity,event_code,message FROM win_event_log WHERE "));

      if (useTsdbTimestamp)
         query.appendFormattedString(_T("event_timestamp BETWEEN to_timestamp(%u) AND to_timestamp(%u)"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
      else
         query.appendFormattedString(_T("event_timestamp BETWEEN %u AND %u"),
                                      static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

      query.appendFormattedString(_T(" AND node_id IN (%s) ORDER BY event_timestamp"), objectIdList.cstr());

      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            query.appendFormattedString(_T(" LIMIT %d"), limitPerSource);
            break;
         case DB_SYNTAX_DB2:
            query.appendFormattedString(_T(" FETCH FIRST %d ROWS ONLY"), limitPerSource);
            break;
      }

      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            TimelineEntry *entry = new TimelineEntry();
            entry->timestamp = DBGetFieldULong(hResult, i, 0);
            entry->type = "windows_event";
            entry->objectId = DBGetFieldULong(hResult, i, 1);
            entry->objectName = GetObjectNameById(entry->objectId);
            entry->severity = GetWindowsEventSeverityName(DBGetFieldLong(hResult, i, 2));

            TCHAR *msg = DBGetField(hResult, i, 4, nullptr, 0);
            if (msg != nullptr)
            {
               entry->message = msg;
               MemFree(msg);
            }

            entry->extra = json_object();
            json_object_set_new(entry->extra, "eventCode", json_integer(DBGetFieldULong(hResult, i, 3)));

            timeline.add(entry);
         }
         DBFreeResult(hResult);
      }
   }

   DBConnectionPoolReleaseConnection(hdb);

   // Sort timeline by timestamp
   timeline.sort([] (const TimelineEntry **a, const TimelineEntry **b) -> int
   {
      if ((*a)->timestamp < (*b)->timestamp)
         return -1;
      if ((*a)->timestamp > (*b)->timestamp)
         return 1;
      return 0;
   });

   // Build output
   json_t *output = json_object();

   json_t *correlationWindow = json_object();
   json_object_set_new(correlationWindow, "from", json_integer(timeFrom));
   json_object_set_new(correlationWindow, "to", json_integer(timeTo));
   json_object_set_new(output, "correlationWindow", correlationWindow);

   json_t *primaryObj = json_object();
   json_object_set_new(primaryObj, "id", json_integer(primaryObject->getId()));
   json_object_set_new(primaryObj, "name", json_string_t(primaryObject->getName()));
   json_object_set_new(output, "primaryObject", primaryObj);

   json_t *includedObjs = json_array();
   for (int i = 0; i < includedObjects.size(); i++)
   {
      NetObj *obj = includedObjects.get(i);
      json_t *objInfo = json_object();
      json_object_set_new(objInfo, "id", json_integer(obj->getId()));
      json_object_set_new(objInfo, "name", json_string_t(obj->getName()));
      json_object_set_new(objInfo, "relationship", json_string((obj->getId() == primaryObject->getId()) ? "primary" : "L2_neighbor"));
      json_array_append_new(includedObjs, objInfo);
   }
   json_object_set_new(output, "includedObjects", includedObjs);

   json_t *timelineJson = json_array();
   int syslogCount = 0, trapCount = 0, eventCount = 0, windowsEventCount = 0;
   for (int i = 0; i < timeline.size(); i++)
   {
      TimelineEntry *entry = timeline.get(i);
      json_t *item = json_object();
      json_object_set_new(item, "timestamp", json_integer(entry->timestamp));
      json_object_set_new(item, "type", json_string(entry->type));
      json_object_set_new(item, "objectId", json_integer(entry->objectId));
      if (!entry->objectName.isEmpty())
         json_object_set_new(item, "objectName", json_string_t(entry->objectName));
      if (!entry->severity.isEmpty())
         json_object_set_new(item, "severity", json_string_t(entry->severity));
      if (!entry->message.isEmpty())
         json_object_set_new(item, "message", json_string_t(entry->message));
      if (entry->extra != nullptr)
      {
         const char *key;
         json_t *value;
         json_object_foreach(entry->extra, key, value)
         {
            json_object_set(item, key, value);
         }
      }
      json_array_append_new(timelineJson, item);

      // Count by type
      if (!strcmp(entry->type, "syslog"))
         syslogCount++;
      else if (!strcmp(entry->type, "snmp_trap"))
         trapCount++;
      else if (!strcmp(entry->type, "event"))
         eventCount++;
      else if (!strcmp(entry->type, "windows_event"))
         windowsEventCount++;
   }
   json_object_set_new(output, "timeline", timelineJson);

   json_t *summary = json_object();
   json_object_set_new(summary, "syslogCount", json_integer(syslogCount));
   json_object_set_new(summary, "trapCount", json_integer(trapCount));
   json_object_set_new(summary, "eventCount", json_integer(eventCount));
   json_object_set_new(summary, "windowsEventCount", json_integer(windowsEventCount));
   json_object_set_new(summary, "objectsInvolved", json_integer(includedObjects.size()));
   json_object_set_new(output, "summary", summary);

   return JsonToString(output);
}

/**
 * Pattern statistics for analysis
 */
struct PatternStats
{
   StringBuffer pattern;
   uint32_t count;
   time_t firstSeen;
   time_t lastSeen;
   IntegerArray<time_t> timestamps;
   int maxSeverity;
   CountingHashSet<uint32_t> sourceObjects;

   PatternStats()
   {
      count = 0;
      firstSeen = 0;
      lastSeen = 0;
      maxSeverity = 0;
   }
};

/**
 * Time bucket for volume analysis
 */
struct TimeBucket
{
   time_t startTime;
   uint32_t totalCount;
   uint32_t errorCount;
   uint32_t warningCount;

   TimeBucket()
   {
      startTime = 0;
      totalCount = 0;
      errorCount = 0;
      warningCount = 0;
   }
};

/**
 * Calculate optimal bucket size based on window duration
 */
static time_t CalculateBucketSize(time_t windowDuration)
{
   if (windowDuration <= 3600)           // <= 1 hour
      return 60;                         // 1 minute buckets
   else if (windowDuration <= 21600)     // <= 6 hours
      return 300;                        // 5 minute buckets
   else if (windowDuration <= 86400)     // <= 24 hours
      return 900;                        // 15 minute buckets
   else if (windowDuration <= 604800)    // <= 7 days
      return 3600;                       // 1 hour buckets
   else
      return 86400;                      // 1 day buckets
}

/**
 * Analyze log patterns
 */
std::string F_AnalyzeLogPatterns(json_t *arguments, uint32_t userId)
{
   // Get log_type parameter (required)
   const char *logType = json_object_get_string_utf8(arguments, "log_type", nullptr);
   if ((logType == nullptr) || (logType[0] == 0))
      return std::string("log_type parameter is required (syslog, windows_events, snmp_traps, events)");

   // Determine table and check permissions
   const TCHAR *tableName = nullptr;
   const TCHAR *timestampColumn = nullptr;
   const TCHAR *objectIdColumn = nullptr;
   const TCHAR *messageColumn = nullptr;
   const TCHAR *severityColumn = nullptr;
   int errorSeverityThreshold = 0;
   int warningSeverityThreshold = 0;

   uint64_t systemRights = GetEffectiveSystemRights(userId);

   if (!stricmp(logType, "syslog"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_SYSLOG) == 0)
         return std::string("User does not have permission to view syslog");
      tableName = _T("syslog");
      timestampColumn = _T("msg_timestamp");
      objectIdColumn = _T("source_object_id");
      messageColumn = _T("msg_text");
      severityColumn = _T("severity");
      errorSeverityThreshold = 3;   // error and above
      warningSeverityThreshold = 4; // warning
   }
   else if (!stricmp(logType, "windows_events"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
         return std::string("User does not have permission to view event log");
      tableName = _T("win_event_log");
      timestampColumn = _T("event_timestamp");
      objectIdColumn = _T("node_id");
      messageColumn = _T("message");
      severityColumn = _T("event_severity");
      errorSeverityThreshold = 1;   // Error
      warningSeverityThreshold = 2; // Warning
   }
   else if (!stricmp(logType, "events"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) == 0)
         return std::string("User does not have permission to view event log");
      tableName = _T("event_log");
      timestampColumn = _T("event_timestamp");
      objectIdColumn = _T("event_source");
      messageColumn = _T("event_message");
      severityColumn = _T("event_severity");
      errorSeverityThreshold = 3;   // Major and above
      warningSeverityThreshold = 1; // Warning
   }
   else if (!stricmp(logType, "snmp_traps"))
   {
      if ((systemRights & SYSTEM_ACCESS_VIEW_TRAP_LOG) == 0)
         return std::string("User does not have permission to view trap log");
      tableName = _T("snmp_trap_log");
      timestampColumn = _T("trap_timestamp");
      objectIdColumn = _T("object_id");
      messageColumn = _T("trap_varlist");
      severityColumn = nullptr;
   }
   else
   {
      return std::string("Invalid log_type. Valid values: syslog, windows_events, snmp_traps, events");
   }

   // Parse parameters
   const char *objectName = json_object_get_string_utf8(arguments, "object", nullptr);
   const char *timeFromStr = json_object_get_string_utf8(arguments, "time_from", "-24h");
   const char *timeToStr = json_object_get_string_utf8(arguments, "time_to", nullptr);
   const char *patternType = json_object_get_string_utf8(arguments, "pattern_type", "all");

   // Parse time range
   time_t timeFrom = ParseTimestamp(timeFromStr);
   time_t timeTo = (timeToStr != nullptr) ? ParseTimestamp(timeToStr) : time(nullptr);

   // Resolve object if specified
   uint32_t objectId = 0;
   if ((objectName != nullptr) && (objectName[0] != 0))
   {
      shared_ptr<NetObj> object = FindObjectByNameOrId(objectName);
      if ((object == nullptr) || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         char buffer[256];
         snprintf(buffer, 256, "Object with name or ID \"%s\" is not known or not accessible", objectName);
         return std::string(buffer);
      }
      objectId = object->getId();
   }

   time_t windowDuration = timeTo - timeFrom;
   time_t bucketSize = CalculateBucketSize(windowDuration);
   int numBuckets = static_cast<int>((windowDuration + bucketSize - 1) / bucketSize);

   // Initialize time buckets
   ObjectArray<TimeBucket> buckets(numBuckets, 16, Ownership::True);
   for (int i = 0; i < numBuckets; i++)
   {
      TimeBucket *bucket = new TimeBucket();
      bucket->startTime = timeFrom + (i * bucketSize);
      buckets.add(bucket);
   }

   // Pattern statistics map
   StringObjectMap<PatternStats> patterns(Ownership::True);

   bool useTsdbTimestamp = (g_dbSyntax == DB_SYNTAX_TSDB);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Build query
   StringBuffer query(L"SELECT ");

   if (useTsdbTimestamp)
      query.appendFormattedString(L"date_part('epoch',%s)::int AS ts,", timestampColumn);
   else
      query.appendFormattedString(L"%s AS ts,", timestampColumn);

   query.append(objectIdColumn);
   query.append(_T(","));
   query.append(messageColumn);

   if (severityColumn != nullptr)
   {
      query.append(_T(","));
      query.append(severityColumn);
   }

   query.appendFormattedString(L" FROM %s WHERE ", tableName);

   if (useTsdbTimestamp)
      query.appendFormattedString(L"%s BETWEEN to_timestamp(%u) AND to_timestamp(%u)", timestampColumn, static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));
   else
      query.appendFormattedString(L"%s BETWEEN %u AND %u", timestampColumn, static_cast<uint32_t>(timeFrom), static_cast<uint32_t>(timeTo));

   if (objectId != 0)
      query.appendFormattedString(L" AND %s=%u", objectIdColumn, objectId);
   else
      AddObjectAccessConstraint(query, userId, objectIdColumn);

   query.appendFormattedString(L" ORDER BY %s", timestampColumn);

   nxlog_debug_tag(DEBUG_TAG, 7, L"F_AnalyzeLogPatterns: executing query: %s", query.cstr());

   // Execute query and process results
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, query);
   if (hResult != nullptr)
   {
      while (DBFetch(hResult))
      {
         time_t timestamp = DBGetFieldULong(hResult, 0);
         uint32_t srcObjectId = DBGetFieldULong(hResult, 1);

         TCHAR *msgText = DBGetField(hResult, 2, nullptr, 0);
         if (msgText == nullptr)
            continue;

         int severity = (severityColumn != nullptr) ? DBGetFieldLong(hResult, 3) : 0;

         // Find bucket
         int bucketIndex = static_cast<int>((timestamp - timeFrom) / bucketSize);
         if ((bucketIndex >= 0) && (bucketIndex < buckets.size()))
         {
            TimeBucket *bucket = buckets.get(bucketIndex);
            bucket->totalCount++;

            if (severityColumn != nullptr)
            {
               if (!stricmp(logType, "syslog"))
               {
                  // Syslog: lower value = higher severity
                  if (severity <= errorSeverityThreshold)
                     bucket->errorCount++;
                  else if (severity == warningSeverityThreshold)
                     bucket->warningCount++;
               }
               else
               {
                  // Other logs: specific values
                  if (severity == errorSeverityThreshold || severity >= 3)
                     bucket->errorCount++;
                  else if (severity == warningSeverityThreshold || severity == 1)
                     bucket->warningCount++;
               }
            }
         }

         // Normalize message for pattern detection
         String normalizedMsg = NormalizeLogMessage(msgText);
         MemFree(msgText);

         // Update pattern statistics
         PatternStats *stats = patterns.get(normalizedMsg);
         if (stats == nullptr)
         {
            stats = new PatternStats();
            stats->pattern.append(normalizedMsg);
            stats->firstSeen = timestamp;
            patterns.set(normalizedMsg, stats);
         }

         stats->count++;
         stats->lastSeen = timestamp;
         stats->timestamps.add(static_cast<time_t>(timestamp));

         if (severity > stats->maxSeverity)
            stats->maxSeverity = severity;

         // Track source objects, counting occurrences
         stats->sourceObjects.put(srcObjectId);
      }
      DBFreeResult(hResult);
   }

   DBConnectionPoolReleaseConnection(hdb);

   // Calculate baseline statistics (first 75% of buckets)
   int baselineBuckets = (numBuckets * 3) / 4;
   if (baselineBuckets < 1)
      baselineBuckets = numBuckets;

   double sum = 0;
   for (int i = 0; i < baselineBuckets; i++)
      sum += buckets.get(i)->totalCount;

   double mean = (baselineBuckets > 0) ? (sum / baselineBuckets) : 0;

   double sumSquaredDev = 0;
   for (int i = 0; i < baselineBuckets; i++)
   {
      double dev = buckets.get(i)->totalCount - mean;
      sumSquaredDev += dev * dev;
   }

   double stdDev = (baselineBuckets > 1) ? sqrt(sumSquaredDev / (baselineBuckets - 1)) : 0;

   // Build output
   json_t *output = json_object();

   json_t *analysisWindow = json_object();
   json_object_set_new(analysisWindow, "from", json_integer(timeFrom));
   json_object_set_new(analysisWindow, "to", json_integer(timeTo));
   json_object_set_new(output, "analysisWindow", analysisWindow);

   json_t *baseline = json_object();
   json_object_set_new(baseline, "meanHourlyRate", json_real(mean * 3600.0 / bucketSize));
   json_object_set_new(baseline, "stdDeviation", json_real(stdDev * 3600.0 / bucketSize));
   json_object_set_new(output, "baseline", baseline);

   json_t *patternsJson = json_object();

   // Detect bursts
   bool detectBursts = (!stricmp(patternType, "all") || !stricmp(patternType, "burst"));
   if (detectBursts)
   {
      json_t *bursts = json_array();
      for (int i = 0; i < buckets.size(); i++)
      {
         TimeBucket *bucket = buckets.get(i);
         if (stdDev > 0)
         {
            double zScore = (bucket->totalCount - mean) / stdDev;
            if (zScore >= 3.0)
            {
               json_t *burst = json_object();
               json_object_set_new(burst, "timestamp", json_integer(bucket->startTime));
               json_object_set_new(burst, "durationSeconds", json_integer(bucketSize));
               json_object_set_new(burst, "messageCount", json_integer(bucket->totalCount));
               json_object_set_new(burst, "normalRate", json_real(mean));
               json_object_set_new(burst, "multiplier", json_real(bucket->totalCount / std::max(mean, 1.0)));
               json_object_set_new(burst, "zScore", json_real(zScore));
               json_array_append_new(bursts, burst);
            }
         }
      }
      json_object_set_new(patternsJson, "bursts", bursts);
   }

   // Detect recurring patterns
   bool detectRecurring = (!stricmp(patternType, "all") || !stricmp(patternType, "recurring"));
   if (detectRecurring)
   {
      json_t *recurring = json_array();

      // Expected intervals in seconds
      static const int s_expectedIntervals[] = { 60, 300, 600, 900, 1800, 3600, 7200, 14400, 21600, 43200, 86400, 604800 };
      static const char *s_intervalNames[] = { "1 minute", "5 minutes", "10 minutes", "15 minutes", "30 minutes",
                                                "1 hour", "2 hours", "4 hours", "6 hours", "12 hours", "1 day", "1 week" };

      patterns.forEach([&recurring, timeFrom, timeTo](const TCHAR *key, PatternStats *stats) -> EnumerationCallbackResult
      {
         if (stats->timestamps.size() < 3)
            return _CONTINUE;

         // Sort timestamps
         stats->timestamps.sort([] (const time_t *a, const time_t *b) -> int
         {
            return (*a < *b) ? -1 : ((*a > *b) ? 1 : 0);
         });

         // Calculate intervals
         IntegerArray<int> intervals;
         for (int i = 1; i < stats->timestamps.size(); i++)
            intervals.add(static_cast<int>(stats->timestamps.get(i) - stats->timestamps.get(i - 1)));

         // Try to match expected intervals
         for (int j = 0; j < 12; j++)
         {
            int expectedInterval = s_expectedIntervals[j];
            int tolerance = expectedInterval / 10;  // 10% tolerance
            int matchCount = 0;

            for (int i = 0; i < intervals.size(); i++)
            {
               int interval = intervals.get(i);
               if (abs(interval - expectedInterval) <= tolerance)
                  matchCount++;
            }

            double regularity = (intervals.size() > 0) ? (static_cast<double>(matchCount) / intervals.size()) : 0;
            if (regularity >= 0.7)
            {
               json_t *pattern = json_object();
               json_object_set_new(pattern, "pattern", json_string_t(stats->pattern));
               json_object_set_new(pattern, "occurrences", json_integer(stats->count));
               json_object_set_new(pattern, "intervalSeconds", json_integer(expectedInterval));
               json_object_set_new(pattern, "intervalDescription", json_string(s_intervalNames[j]));
               json_object_set_new(pattern, "regularity", json_real(regularity));
               json_object_set_new(pattern, "firstSeen", json_integer(stats->firstSeen));
               json_object_set_new(pattern, "lastSeen", json_integer(stats->lastSeen));
               json_array_append_new(recurring, pattern);
               break;
            }
         }

         return _CONTINUE;
      });

      json_object_set_new(patternsJson, "recurring", recurring);
   }

   // Detect new patterns
   bool detectNew = (!stricmp(patternType, "all") || !stricmp(patternType, "new"));
   if (detectNew)
   {
      json_t *newPatterns = json_array();
      time_t newThreshold = timeFrom + (windowDuration * 3 / 4);  // Last 25% of window

      // Collect and sort by count
      ObjectArray<PatternStats> newPatternsList(64, 64, Ownership::False);

      patterns.forEach([&newPatternsList, newThreshold](const TCHAR *key, PatternStats *stats) -> EnumerationCallbackResult
      {
         if ((stats->firstSeen >= newThreshold) && (stats->count >= 3))
            newPatternsList.add(stats);
         return _CONTINUE;
      });

      newPatternsList.sort([] (const PatternStats **a, const PatternStats **b) -> int
      {
         return ((*b)->count > (*a)->count) ? 1 : (((*b)->count < (*a)->count) ? -1 : 0);
      });

      for (int i = 0; i < std::min(newPatternsList.size(), 20); i++)
      {
         PatternStats *stats = newPatternsList.get(i);
         json_t *pattern = json_object();
         json_object_set_new(pattern, "pattern", json_string_t(stats->pattern));
         json_object_set_new(pattern, "firstSeen", json_integer(stats->firstSeen));
         json_object_set_new(pattern, "count", json_integer(stats->count));
         json_object_set_new(pattern, "severity", json_integer(stats->maxSeverity));

         // Add source objects
         json_t *sources = json_array();
         stats->sourceObjects.forEach(
            [sources] (const uint32_t& id, uint32_t count) -> EnumerationCallbackResult
            {
               json_t *source = json_object();
               json_object_set_new(source, "objectId", json_integer(id));
               String name = GetObjectNameById(id);
               if (!name.isEmpty())
                  json_object_set_new(source, "objectName", json_string_t(name));
               json_object_set_new(source, "count", json_integer(count));
               json_array_append_new(sources, source);
               return _CONTINUE;
            });
         json_object_set_new(pattern, "sources", sources);

         json_array_append_new(newPatterns, pattern);
      }

      json_object_set_new(patternsJson, "newPatterns", newPatterns);
   }

   // Detect frequency (top patterns)
   bool detectFrequency = (!stricmp(patternType, "all") || !stricmp(patternType, "frequency"));
   if (detectFrequency)
   {
      json_t *topPatterns = json_array();

      // Collect and sort by count
      ObjectArray<PatternStats> allPatterns(256, 64, Ownership::False);

      patterns.forEach(
         [&allPatterns] (const TCHAR *key, PatternStats *stats) -> EnumerationCallbackResult
         {
            allPatterns.add(stats);
            return _CONTINUE;
         });

      allPatterns.sort(
         [] (const PatternStats **a, const PatternStats **b) -> int
         {
            return ((*b)->count > (*a)->count) ? 1 : (((*b)->count < (*a)->count) ? -1 : 0);
         });

      for (int i = 0; i < std::min(allPatterns.size(), 20); i++)
      {
         PatternStats *stats = allPatterns.get(i);
         json_t *pattern = json_object();
         json_object_set_new(pattern, "pattern", json_string_t(stats->pattern));
         json_object_set_new(pattern, "count", json_integer(stats->count));
         json_object_set_new(pattern, "firstSeen", json_integer(stats->firstSeen));
         json_object_set_new(pattern, "lastSeen", json_integer(stats->lastSeen));
         json_array_append_new(topPatterns, pattern);
      }

      json_object_set_new(patternsJson, "topPatterns", topPatterns);
   }

   json_object_set_new(output, "patterns", patternsJson);

   return JsonToString(output);
}
