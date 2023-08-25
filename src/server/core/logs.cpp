/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
	{ _T("AlarmLog"), _T("alarms"), _T("alarm_id"), _T("source_object_id"), SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
			{ _T("alarm_id"), _T("Alarm ID"), LC_INTEGER, 0 },
			{ _T("alarm_state"), _T("State"), LC_ALARM_STATE, 0 },
			{ _T("hd_state"), _T("Helpdesk State"), LC_ALARM_HD_STATE, 0 },
			{ _T("source_object_id"), _T("Source"), LC_OBJECT_ID, 0 },
         { _T("zone_uin"), _T("Zone"), LC_ZONE_UIN, 0 },
         { _T("dci_id"), _T("DCI"), LC_INTEGER, 0 },
			{ _T("current_severity"), _T("Severity"), LC_SEVERITY, 0 },
			{ _T("original_severity"), _T("Original Severity"), LC_SEVERITY, 0 },
			{ _T("source_event_code"), _T("Event"), LC_EVENT_CODE, 0 },
			{ _T("message"), _T("Message"), LC_TEXT, 0 },
			{ _T("repeat_count"), _T("Repeat Count"), LC_INTEGER, 0 },
			{ _T("creation_time"), _T("Created"), LC_TIMESTAMP, 0 },
			{ _T("last_change_time"), _T("Last Changed"), LC_TIMESTAMP, 0 },
			{ _T("ack_by"), _T("Ack by"), LC_USER_ID, 0 },
			{ _T("resolved_by"), _T("Resolved by"), LC_USER_ID, 0 },
			{ _T("term_by"), _T("Terminated by"), LC_USER_ID, 0 },
         { _T("rule_guid"), _T("Rule"), LC_TEXT, 0 },
         { _T("rule_description"), _T("Rule Description"), LC_TEXT, 0 },
			{ _T("alarm_key"), _T("Key"), LC_TEXT, 0 },
         { _T("event_tags"), _T("Event Tags"), LC_TEXT, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
   { _T("AssetChaneLog"), _T("asset_change_log"), _T("record_id"), _T("asset_id"), SYSTEM_ACCESS_VIEW_ASSET_CHANGE_LOG,
      {
         { _T("record_id"), _T("Record ID"), LC_INTEGER, 0 },
         { _T("operation_timestamp"), _T("Timestamp"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("asset_id"), _T("Asset"), LC_OBJECT_ID, 0 },
         { _T("attribute_name"), _T("Attribute name"), LC_TEXT, 0 },
         { _T("operation"), _T("Operation"), LC_ASSET_OPERATION, 0 },
         { _T("old_value"), _T("Old value"), LC_TEXT, 0 },
         { _T("new_value"), _T("New value"), LC_TEXT, 0 },
         { _T("user_id"), _T("User"), LC_USER_ID, 0 },
         { _T("linked_object_id"), _T("Linked object"), LC_OBJECT_ID, 0 },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ _T("AuditLog"), _T("audit_log"), _T("record_id"), _T("object_id"), SYSTEM_ACCESS_VIEW_AUDIT_LOG,
		{
			{ _T("record_id"), _T("Record ID"), LC_INTEGER, 0 },
			{ _T("timestamp"), _T("Timestamp"), LC_TIMESTAMP, 0 },
			{ _T("subsystem"), _T("Subsystem"), LC_TEXT, 0 },
         { _T("object_id"), _T("Object"), LC_OBJECT_ID, 0 },
			{ _T("user_id"), _T("User"), LC_USER_ID, 0 },
         { _T("session_id"), _T("Session"), LC_INTEGER, 0 },
			{ _T("workstation"), _T("Workstation"), LC_TEXT, 0 },
			{ _T("message"), _T("Message"), LC_TEXT, 0 },
         { _T("old_value"), _T("Old value"), LC_TEXT_DETAILS, 0 },
         { _T("new_value"), _T("New value"), LC_TEXT_DETAILS, 0 },
         { _T("value_type"), _T("Value type"), LC_TEXT_DETAILS, 0 },
         { _T("hmac"), _T("HMAC"), LC_TEXT_DETAILS, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
	{ _T("EventLog"), _T("event_log"), _T("event_id"), _T("event_source"), SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
         { _T("event_id"), _T("ID"), LC_INTEGER, 0 },
			{ _T("event_timestamp"), _T("Time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("origin_timestamp"), _T("Origin time"), LC_TIMESTAMP, 0 },
         { _T("origin"), _T("Origin"), LC_EVENT_ORIGIN, 0 },
			{ _T("event_source"), _T("Source"), LC_OBJECT_ID, 0 },
         { _T("zone_uin"), _T("Zone"), LC_ZONE_UIN, 0 },
         { _T("dci_id"), _T("DCI"), LC_INTEGER, 0 },
			{ _T("event_code"), _T("Event"), LC_EVENT_CODE, 0 },
			{ _T("event_severity"), _T("Severity"), LC_SEVERITY, 0 },
			{ _T("event_message"), _T("Message"), LC_TEXT, 0 },
			{ _T("event_tags"), _T("Event tags"), LC_TEXT, 0 },
         { _T("root_event_id"), _T("Root ID"), LC_INTEGER, 0 },
         { _T("raw_data"), _T("Raw data"), LC_JSON_DETAILS, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
   { _T("MaintenanceJournal"), _T("maintenance_journal"), _T("record_id"), _T("object_id"), 0,
      {
         { _T("record_id"), _T("Record ID"), LC_INTEGER, 0 },
         { _T("object_id"), _T("Object"), LC_OBJECT_ID, 0 },
         { _T("author"), _T("Author"), LC_USER_ID, 0 },
         { _T("last_edited_by"), _T("Last edited dy"), LC_USER_ID, 0 },
         { _T("description"), _T("Description"), LC_TEXT, 0 },
         { _T("creation_time"), _T("Creation time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("modification_time"), _T("Modification time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { nullptr, nullptr, 0, 0 }
      }
   },
   { _T("NotificationLog"), _T("notification_log"), _T("id"), nullptr, SYSTEM_ACCESS_VIEW_EVENT_LOG,
      {
         { _T("id"), _T("ID"), LC_INTEGER, 0 },
         { _T("notification_timestamp"), _T("Timestamp"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("notification_channel"), _T("Channel"), LC_TEXT, 0 },
         { _T("recipient"), _T("Recipient"), LC_TEXT, 0 },
         { _T("subject"), _T("Subject"), LC_TEXT, 0 },
         { _T("message"), _T("Message"), LC_TEXT, 0 },
         { _T("success"), _T("Status"), LC_COMPLETION_STATUS, LCF_CHAR_COLUMN },
         { nullptr, nullptr, 0, 0 }
      }
   },
   { _T("ServerActionExecutionLog"), _T("server_action_execution_log"), _T("id"), nullptr, SYSTEM_ACCESS_VIEW_EVENT_LOG,
      {
         { _T("id"), _T("ID"), LC_INTEGER, 0 },
         { _T("action_timestamp"), _T("Timestamp"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("action_id"), _T("Action ID"), LC_INTEGER, 0 },
         { _T("action_name"), _T("Action name"), LC_TEXT, 0 },
         { _T("channel_name"), _T("Channel name"), LC_TEXT, 0 },
         { _T("recipient"), _T("Recipient"), LC_TEXT, 0 },
         { _T("subject"), _T("Subject"), LC_TEXT, 0 },
         { _T("action_data"), _T("Action data"), LC_TEXT, 0 },
         { _T("event_id"), _T("Event ID"), LC_INTEGER, 0 },
         { _T("event_code"), _T("Event"), LC_EVENT_CODE, 0 },
         { _T("success"), _T("Status"), LC_COMPLETION_STATUS, LCF_CHAR_COLUMN },
         { nullptr, nullptr, 0, 0 }
      }
   },
	{ _T("SnmpTrapLog"), _T("snmp_trap_log"), _T("trap_id"), _T("object_id"), SYSTEM_ACCESS_VIEW_TRAP_LOG,
		{
			{ _T("trap_timestamp"), _T("Time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
			{ _T("ip_addr"), _T("Source IP"), LC_TEXT, 0 },
			{ _T("object_id"), _T("Object"), LC_OBJECT_ID, 0 },
         { _T("zone_uin"), _T("Zone"), LC_ZONE_UIN, 0 },
			{ _T("trap_oid"), _T("Trap OID"), LC_TEXT, 0 },
			{ _T("trap_varlist"), _T("Varbinds"), LC_TEXT, 0 },
			{ nullptr, nullptr, 0, 0 }
		}
	},
	{ _T("syslog"), _T("syslog"), _T("msg_id"), _T("source_object_id"), SYSTEM_ACCESS_VIEW_SYSLOG,
		{
			{ _T("msg_timestamp"), _T("Time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
			{ _T("source_object_id"), _T("Source"), LC_OBJECT_ID, 0 },
         { _T("zone_uin"), _T("Zone"), LC_ZONE_UIN, 0 },
			{ _T("facility"), _T("Facility"), LC_INTEGER, 0 },
			{ _T("severity"), _T("Severity"), LC_INTEGER, 0 },
			{ _T("hostname"), _T("Host"), LC_TEXT, 0 },
			{ _T("msg_tag"), _T("Tag"), LC_TEXT, 0 },
			{ _T("msg_text"), _T("Text"), LC_TEXT, 0 },
         { nullptr, nullptr, 0, 0 }
		}
	},
   { _T("WindowsEventLog"), _T("win_event_log"), _T("id"), _T("node_id"), SYSTEM_ACCESS_VIEW_SYSLOG,
      {
         { _T("id"), _T("ID"), LC_INTEGER, 0 },
         { _T("event_timestamp"), _T("Time"), LC_TIMESTAMP, LCF_TSDB_TIMESTAMPTZ },
         { _T("origin_timestamp"), _T("Origin time"), LC_TIMESTAMP, 0 },
         { _T("node_id"), _T("Source"), LC_OBJECT_ID, 0 },
         { _T("zone_uin"), _T("Zone"), LC_ZONE_UIN, 0 },
         { _T("log_name"), _T("Log name"), LC_TEXT, 0 },
         { _T("event_source"), _T("Event source"), LC_TEXT, 0 },
         { _T("event_severity"), _T("Event severity"), LC_INTEGER, 0 },
         { _T("event_code"), _T("Event code"), LC_INTEGER, 0 },
         { _T("message"), _T("Message"), LC_TEXT, 0 },
         { _T("raw_data"), _T("Raw data"), LC_TEXT_DETAILS, 0 },
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

   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, _T("RegisterLogHandle: handle object %p registered as %d"), handle.get(), r->id);
	return r->id;
}

/**
 * Open log from given log set by name
 *
 * @return log handle on success, -1 on error, -2 if log not found
 */
static int32_t OpenLogInternal(NXCORE_LOG *logs, const TCHAR *name, ClientSession *session, uint32_t *rcc)
{
	for(int i = 0; logs[i].name != nullptr; i++)
	{
		if (!_tcsicmp(name, logs[i].name))
		{
			if (session->checkSysAccessRights(logs[i].requiredAccess))
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
int32_t OpenLog(const TCHAR *name, ClientSession *session, uint32_t *rcc)
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
   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, _T("CloseLog: close request from session %d for handle %d"), session->getId(), logHandle);
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
   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, _T("Closing all logs for session %d"), sessionId);
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

   nxlog_debug_tag(DEBUG_TAG_LOGS, 6, _T("AcquireLogHandleObject: request from session %d for handle %d"), session->getId(), logHandle);
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
