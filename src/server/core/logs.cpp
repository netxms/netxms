/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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

#include "nxcore.h"
#include <nxcore_logs.h>

/**
 * Defined logs
 */
static NXCORE_LOG s_logs[] =
{
   { L"AITaskLog", L"ai_task_execution_log", L"record_id", nullptr, SYSTEM_ACCESS_VIEW_EVENT_LOG,
      {
         { L"record_id", L"Record ID", LC_INTEGER, LCF_RECORD_ID },
         { L"execution_timestamp", L"Timestamp", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"task_id", L"Task", LC_INTEGER, 0 },
         { L"task_description", L"Task Description", LC_TEXT, 0 },
         { L"user_id", L"User", LC_USER_ID, 0 },
         { L"status", L"Status", LC_AI_TASK_STATUS, 0 },
         { L"iteration", L"Iteration", LC_INTEGER, 0 },
         { L"explanation", L"Explanation", LC_TEXT_DETAILS, 0 },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ L"AlarmLog", L"alarms", L"alarm_id", L"source_object_id", SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
			{ L"alarm_id", L"Alarm ID", LC_INTEGER, 0 },
			{ L"alarm_state", L"State", LC_ALARM_STATE, 0 },
			{ L"hd_state", L"Helpdesk State", LC_ALARM_HD_STATE, 0 },
			{ L"source_object_id", L"Source", LC_OBJECT_ID, 0 },
         { L"zone_uin", L"Zone", LC_ZONE_UIN, 0 },
         { L"dci_id", L"DCI", LC_INTEGER, 0 },
			{ L"current_severity", L"Severity", LC_SEVERITY, 0 },
			{ L"original_severity", L"Original Severity", LC_SEVERITY, 0 },
			{ L"source_event_code", L"Event", LC_EVENT_CODE, 0 },
			{ L"message", L"Message", LC_TEXT, 0 },
			{ L"repeat_count", L"Repeat Count", LC_INTEGER, 0 },
			{ L"creation_time", L"Created", LC_TIMESTAMP, 0 },
			{ L"last_change_time", L"Last Changed", LC_TIMESTAMP, 0 },
			{ L"ack_by", L"Ack by", LC_USER_ID, 0 },
			{ L"resolved_by", L"Resolved by", LC_USER_ID, 0 },
			{ L"term_by", L"Terminated by", LC_USER_ID, 0 },
         { L"rule_guid", L"Rule", LC_TEXT, 0 },
         { L"rule_description", L"Rule Description", LC_TEXT, 0 },
			{ L"alarm_key", L"Key", LC_TEXT, 0 },
         { L"event_tags", L"Event Tags", LC_TEXT, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
   { L"AssetChangeLog", L"asset_change_log", L"record_id", L"asset_id", SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG,
      {
         { L"record_id", L"Record ID", LC_INTEGER, LCF_RECORD_ID },
         { L"operation_timestamp", L"Timestamp", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"asset_id", L"Asset", LC_OBJECT_ID, 0 },
         { L"attribute_name", L"Attribute name", LC_TEXT, 0 },
         { L"operation", L"Operation", LC_ASSET_OPERATION, 0 },
         { L"old_value", L"Old value", LC_TEXT, 0 },
         { L"new_value", L"New value", LC_TEXT, 0 },
         { L"user_id", L"User", LC_USER_ID, 0 },
         { L"linked_object_id", L"Linked object", LC_OBJECT_ID, 0 },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ L"AuditLog", L"audit_log", L"record_id", L"object_id", SYSTEM_ACCESS_VIEW_AUDIT_LOG,
		{
			{ L"record_id", L"Record ID", LC_INTEGER, LCF_RECORD_ID },
			{ L"timestamp", L"Timestamp", LC_TIMESTAMP, 0 },
			{ L"subsystem", L"Subsystem", LC_TEXT, 0 },
         { L"object_id", L"Object", LC_OBJECT_ID, 0 },
			{ L"user_id", L"User", LC_USER_ID, 0 },
         { L"session_id", L"Session", LC_INTEGER, 0 },
			{ L"workstation", L"Workstation", LC_TEXT, 0 },
			{ L"message", L"Message", LC_TEXT, 0 },
         { L"old_value", L"Old value", LC_TEXT_DETAILS, 0 },
         { L"new_value", L"New value", LC_TEXT_DETAILS, 0 },
         { L"value_type", L"Value type", LC_TEXT_DETAILS, 0 },
         { L"hmac", L"HMAC", LC_TEXT_DETAILS, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
	{ L"EventLog", L"event_log", L"event_id", L"event_source", SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
         { L"event_id", L"ID", LC_INTEGER, 0 },
			{ L"event_timestamp", L"Time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"origin_timestamp", L"Origin time", LC_TIMESTAMP, 0 },
         { L"origin", L"Origin", LC_EVENT_ORIGIN, 0 },
			{ L"event_source", L"Source", LC_OBJECT_ID, 0 },
         { L"zone_uin", L"Zone", LC_ZONE_UIN, 0 },
         { L"dci_id", L"DCI", LC_INTEGER, 0 },
			{ L"event_code", L"Event", LC_EVENT_CODE, 0 },
			{ L"event_severity", L"Severity", LC_SEVERITY, 0 },
			{ L"event_message", L"Message", LC_TEXT, 0 },
			{ L"event_tags", L"Event tags", LC_TEXT, 0 },
         { L"root_event_id", L"Root ID", LC_INTEGER, 0 },
         { L"raw_data", L"Raw data", LC_JSON_DETAILS, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
   { L"MaintenanceJournal", L"maintenance_journal", L"record_id", L"object_id", 0,
      {
         { L"record_id", L"Record ID", LC_INTEGER, LCF_RECORD_ID },
         { L"object_id", L"Object", LC_OBJECT_ID, 0 },
         { L"author", L"Author", LC_USER_ID, 0 },
         { L"last_edited_by", L"Last edited dy", LC_USER_ID, 0 },
         { L"description", L"Description", LC_TEXT, 0 },
         { L"creation_time", L"Creation time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"modification_time", L"Modification time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { nullptr, nullptr, 0, 0 }
      }
   },
   { L"NotificationLog", L"notification_log", L"id", nullptr, SYSTEM_ACCESS_VIEW_EVENT_LOG,
      {
         { L"id", L"ID", LC_INTEGER, LCF_RECORD_ID },
         { L"notification_timestamp", L"Timestamp", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"notification_channel", L"Channel", LC_TEXT, 0 },
         { L"recipient", L"Recipient", LC_TEXT, 0 },
         { L"subject", L"Subject", LC_TEXT, 0 },
         { L"message", L"Message", LC_TEXT, 0 },
         { L"event_code", L"Event", LC_EVENT_CODE, 0 },
         { L"event_id", L"Event ID", LC_INTEGER, 0 },
         { L"rule_id", L"Rule ID", LC_TEXT, 0 },
         { L"success", L"Status", LC_COMPLETION_STATUS, LCF_CHAR_COLUMN },
         { nullptr, nullptr, 0, 0 }
      }
   },
   { L"PackageDeploymentLog", L"package_deployment_log", L"record_id", nullptr, SYSTEM_ACCESS_MANAGE_PACKAGES,
      {
         { L"record_id", L"ID", LC_INTEGER, LCF_RECORD_ID },
         { L"execution_time", L"Execution time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"completion_time", L"Completion time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"node_id", L"Node", LC_OBJECT_ID, 0 },
         { L"user_id", L"Node", LC_USER_ID, 0 },
         { L"status", L"Status", LC_DEPLOYMENT_STATUS, 0 },
         { L"failure_reason", L"Failure reason", LC_TEXT, 0 },
         { L"pkg_id", L"Package ID", LC_INTEGER, 0 },
         { L"pkg_type", L"Package type", LC_TEXT, 0 },
         { L"pkg_name", L"Package name", LC_TEXT, 0 },
         { L"version", L"Package version", LC_TEXT, 0 },
         { L"platform", L"Platform", LC_TEXT, 0 },
         { L"pkg_file", L"Package file", LC_TEXT, 0 },
         { L"command", L"Command", LC_TEXT, 0 },
         { L"description", L"Package description", LC_TEXT, 0 },
         { nullptr, nullptr, 0, 0 }
      }
   },
   { L"ServerActionExecutionLog", L"server_action_execution_log", L"id", nullptr, SYSTEM_ACCESS_VIEW_EVENT_LOG,
      {
         { L"id", L"ID", LC_INTEGER, LCF_RECORD_ID },
         { L"action_timestamp", L"Timestamp", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"action_id", L"Action ID", LC_INTEGER, 0 },
         { L"action_name", L"Action name", LC_TEXT, 0 },
         { L"channel_name", L"Channel name", LC_TEXT, 0 },
         { L"recipient", L"Recipient", LC_TEXT, 0 },
         { L"subject", L"Subject", LC_TEXT, 0 },
         { L"action_data", L"Action data", LC_TEXT, 0 },
         { L"event_id", L"Event ID", LC_INTEGER, 0 },
         { L"event_code", L"Event", LC_EVENT_CODE, 0 },
         { L"success", L"Status", LC_COMPLETION_STATUS, LCF_CHAR_COLUMN },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ L"SnmpTrapLog", L"snmp_trap_log", L"trap_id", L"object_id", SYSTEM_ACCESS_VIEW_TRAP_LOG,
		{
			{ L"trap_timestamp", L"Time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
			{ L"ip_addr", L"Source IP", LC_TEXT, 0 },
			{ L"object_id", L"Object", LC_OBJECT_ID, 0 },
         { L"zone_uin", L"Zone", LC_ZONE_UIN, 0 },
			{ L"trap_oid", L"Trap OID", LC_TEXT, 0 },
			{ L"trap_varlist", L"Varbinds", LC_TEXT, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
	{ L"syslog", L"syslog", L"msg_id", L"source_object_id", SYSTEM_ACCESS_VIEW_SYSLOG,
		{
			{ L"msg_timestamp", L"Time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
			{ L"source_object_id", L"Source", LC_OBJECT_ID, 0 },
         { L"zone_uin", L"Zone", LC_ZONE_UIN, 0 },
			{ L"facility", L"Facility", LC_INTEGER, 0 },
			{ L"severity", L"Severity", LC_INTEGER, 0 },
			{ L"hostname", L"Host", LC_TEXT, 0 },
			{ L"msg_tag", L"Tag", LC_TEXT, 0 },
			{ L"msg_text", L"Text", LC_TEXT, 0 },
         { nullptr, nullptr, 0, 0 }
		}
	},
   { L"WindowsEventLog", L"win_event_log", L"id", L"node_id", SYSTEM_ACCESS_VIEW_SYSLOG,
      {
         { L"id", L"ID", LC_INTEGER, LCF_RECORD_ID },
         { L"event_timestamp", L"Time", LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { L"origin_timestamp", L"Origin time", LC_TIMESTAMP, 0 },
         { L"node_id", L"Source", LC_OBJECT_ID, 0 },
         { L"zone_uin", L"Zone", LC_ZONE_UIN, 0 },
         { L"log_name", L"Log name", LC_TEXT, 0 },
         { L"event_source", L"Event source", LC_TEXT, 0 },
         { L"event_severity", L"Event severity", LC_INTEGER, 0 },
         { L"event_code", L"Event code", LC_INTEGER, 0 },
         { L"message", L"Message", LC_TEXT, 0 },
         { L"raw_data", L"Raw data", LC_TEXT_DETAILS, 0 },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ nullptr, nullptr, nullptr, nullptr, 0 }
};

/**
 * Registered log handles
 */
struct LogHandleRegistration
{
   LogHandleRegistration *next;
	shared_ptr<LogHandle> handle;
	session_id_t sessionId;
	int32_t id;

	LogHandleRegistration()
	{
	   next = nullptr;
	   sessionId = -1;
	   id = -1;
	}

   LogHandleRegistration(const shared_ptr<LogHandle>& h, session_id_t sid) : handle(h)
   {
      next = nullptr;
      sessionId = sid;
      id = -1;
   }
};
static LogHandleRegistration *s_regList = new LogHandleRegistration();
static Mutex s_regListMutex(MutexType::FAST);
static int32_t s_handleId = 0;

/**
 * Register log handle
 */
static int32_t RegisterLogHandle(const shared_ptr<LogHandle>& handle, ClientSession *session)
{
	LogHandleRegistration *r = new LogHandleRegistration(handle, session->getId());
	s_regListMutex.lock();
	r->next = s_regList->next;
	s_regList->next = r;
	r->id = s_handleId++;
	s_regListMutex.unlock();

   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, L"RegisterLogHandle: handle object %p registered as %d", handle.get(), r->id);
	return r->id;
}

/**
 * Open log from given log set by name
 *
 * @return log handle on success, -1 on error, -2 if log not found
 */
static int32_t OpenLogInternal(NXCORE_LOG *logs, const wchar_t *name, ClientSession *session, uint32_t *rcc)
{
	for(int i = 0; logs[i].name != nullptr; i++)
	{
		if (!wcsicmp(name, logs[i].name))
		{
			if (session->checkSystemAccessRights(logs[i].requiredAccess))
			{
				*rcc = RCC_SUCCESS;
				return RegisterLogHandle(make_shared<LogHandle>(&logs[i]), session);
			}
			else
			{
				*rcc = RCC_ACCESS_DENIED;
				return -1;
			}
		}
	}
   return -2;
}

/**
 * Open log by name
 */
int32_t OpenLog(const wchar_t *name, ClientSession *session, uint32_t *rcc)
{
   int32_t rc = OpenLogInternal(s_logs, name, session, rcc);
   if (rc != -2)
      return rc;

   // Try to find log definition in loaded modules
   ENUMERATE_MODULES(logs)
	{
      rc = OpenLogInternal(CURRENT_MODULE.logs, name, session, rcc);
      if (rc != -2)
         return rc;
	}

	*rcc = RCC_UNKNOWN_LOG_NAME;
	return -1;
}

/**
 * Close log
 */
uint32_t CloseLog(ClientSession *session, int32_t logHandle)
{
   uint32_t rcc = RCC_INVALID_LOG_HANDLE;
   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, L"CloseLog: close request from session %d for handle %d", session->getId(), logHandle);
	s_regListMutex.lock();
	for(LogHandleRegistration *r = s_regList; r->next != nullptr; r = r->next)
	{
	   LogHandleRegistration *n = r->next;
	   if ((n->id == logHandle) && (n->sessionId == session->getId()))
	   {
	      r->next = n->next;
	      delete n;
	      rcc = RCC_SUCCESS;
	      break;
	   }
	}
	s_regListMutex.unlock();
	return rcc;
}

/**
 * Close log
 */
void CloseAllLogsForSession(session_id_t sessionId)
{
   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, L"Closing all logs for session %d", sessionId);
   s_regListMutex.lock();
   for(LogHandleRegistration *r = s_regList; r->next != nullptr; r = r->next)
   {
      LogHandleRegistration *n = r->next;
      if (n->sessionId == sessionId)
      {
         r->next = n->next;
         delete n;
         if (r->next == nullptr)
            break;
      }
   }
   s_regListMutex.unlock();
}

/**
 * Acquire log handle object
 * Caller must call LogHandle::unlock() when it finish work with acquired object
 */
shared_ptr<LogHandle> AcquireLogHandleObject(ClientSession *session, int32_t logHandle)
{
	shared_ptr<LogHandle> object;

   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, L"AcquireLogHandleObject: request from session %d for handle %d", session->getId(), logHandle);
	s_regListMutex.lock();
   for(LogHandleRegistration *r = s_regList->next; r != nullptr; r = r->next)
   {
      if ((r->id == logHandle) && (r->sessionId == session->getId()))
      {
         object = r->handle;
         break;
      }
   }
	s_regListMutex.unlock();

   if (object != nullptr)
      object->lock();
	return object;
}
