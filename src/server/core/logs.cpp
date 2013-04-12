/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Defined logs
 */
static NXCORE_LOG s_logs[] =
{
	{ _T("AlarmLog"), _T("alarms"), _T("alarm_id"), SYSTEM_ACCESS_VIEW_AUDIT_LOG,
		{
			{ _T("alarm_id"), _T("Alarm ID"), LC_INTEGER },
			{ _T("alarm_state"), _T("State"), LC_ALARM_STATE },
			{ _T("hd_state"), _T("Helpdesk State"), LC_ALARM_HD_STATE },
			{ _T("source_object_id"), _T("Source"), LC_OBJECT_ID },
			{ _T("current_severity"), _T("Severity"), LC_SEVERITY },
			{ _T("original_severity"), _T("Original Severity"), LC_SEVERITY },
			{ _T("source_event_code"), _T("Event"), LC_EVENT_CODE },
			{ _T("message"), _T("Message"), LC_TEXT },
			{ _T("repeat_count"), _T("Repeat Count"), LC_INTEGER },
			{ _T("creation_time"), _T("Created"), LC_TIMESTAMP },
			{ _T("last_change_time"), _T("Last Changed"), LC_TIMESTAMP },
			{ _T("ack_by"), _T("Ack by"), LC_USER_ID },
			{ _T("resolved_by"), _T("Resolved by"), LC_USER_ID },
			{ _T("term_by"), _T("Terminated by"), LC_USER_ID },
			{ _T("alarm_key"), _T("Key"), LC_TEXT },
			{ NULL, NULL, 0 }
		}
	},
	{ _T("AuditLog"), _T("audit_log"), _T("record_id"), SYSTEM_ACCESS_VIEW_AUDIT_LOG,
		{
			{ _T("record_id"), _T("Record ID"), LC_INTEGER },
			{ _T("timestamp"), _T("Timestamp"), LC_TIMESTAMP },
			{ _T("subsystem"), _T("Subsystem"), LC_TEXT },
			{ _T("user_id"), _T("User"), LC_USER_ID },
			{ _T("workstation"), _T("Workstation"), LC_TEXT },
			{ _T("message"), _T("Message"), LC_TEXT },
			{ NULL, NULL, 0 }
		}
	},
	{ _T("EventLog"), _T("event_log"), _T("event_id"), SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
			{ _T("event_timestamp"), _T("Time"), LC_TIMESTAMP },
			{ _T("event_source"), _T("Source"), LC_OBJECT_ID },
			{ _T("event_code"), _T("Event"), LC_EVENT_CODE },
			{ _T("event_severity"), _T("Severity"), LC_SEVERITY },
			{ _T("event_message"), _T("Message"), LC_TEXT },
			{ NULL, NULL, 0 }
		}
	},
	{ _T("SnmpTrapLog"), _T("snmp_trap_log"), _T("trap_id"), SYSTEM_ACCESS_VIEW_TRAP_LOG,
		{
			{ _T("trap_timestamp"), _T("Time"), LC_TIMESTAMP },
			{ _T("ip_addr"), _T("Source IP"), LC_TEXT },
			{ _T("object_id"), _T("Object"), LC_OBJECT_ID },
			{ _T("trap_oid"), _T("Trap OID"), LC_TEXT },
			{ _T("trap_varlist"), _T("Varbinds"), LC_TEXT },
			{ NULL, NULL, 0 }
		}
	},
	{ _T("syslog"), _T("syslog"), _T("msg_id"), SYSTEM_ACCESS_VIEW_EVENT_LOG,
		{
			{ _T("msg_timestamp"), _T("Time"), LC_TIMESTAMP },
			{ _T("source_object_id"), _T("Source"), LC_OBJECT_ID },
			{ _T("facility"), _T("Facility"), LC_INTEGER },
			{ _T("severity"), _T("Severity"), LC_INTEGER },
			{ _T("hostname"), _T("Host"), LC_TEXT },
			{ _T("msg_tag"), _T("Tag"), LC_TEXT },
			{ _T("msg_text"), _T("Text"), LC_TEXT },
			{ NULL, NULL, 0 }
		}
	},
	{ NULL, NULL, 0 }
};

/**
 * Registered log handles
 */
struct LOG_HANDLE_REGISTRATION
{
	LogHandle *handle;
	DWORD sessionId;	
};
int s_regListSize = 0;
LOG_HANDLE_REGISTRATION *s_regList = NULL;
MUTEX s_regListMutex = INVALID_MUTEX_HANDLE;

/**
 * Init log access
 */
void InitLogAccess()
{
	s_regListMutex = MutexCreate();
}

/**
 * Register log handle
 */
static int RegisterLogHandle(LogHandle *handle, ClientSession *session)
{
	int i;

	MutexLock(s_regListMutex);

	for(i = 0; i < s_regListSize; i++)
		if (s_regList[i].handle == NULL)
			break;
	if (i == s_regListSize)
	{
		s_regListSize += 10;
		s_regList = (LOG_HANDLE_REGISTRATION *)realloc(s_regList, sizeof(LOG_HANDLE_REGISTRATION) * s_regListSize);
	}

	s_regList[i].handle = handle;
	s_regList[i].sessionId = session->getIndex();

	MutexUnlock(s_regListMutex);
	return i;
}

/**
 * Open log from given log set by name
 * 
 * @return log handle on success, -1 on error, -2 if log not found
 */
static int OpenLogInternal(NXCORE_LOG *logs, const TCHAR *name, ClientSession *session, DWORD *rcc)
{
	for(int i = 0; logs[i].name != NULL; i++)
	{	
		if (!_tcsicmp(name, logs[i].name))
		{
			if (session->checkSysAccessRights(logs[i].requiredAccess))
			{
				*rcc = RCC_SUCCESS;
				LogHandle *handle = new LogHandle(&logs[i]);
				return RegisterLogHandle(handle, session);
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
int OpenLog(const TCHAR *name, ClientSession *session, DWORD *rcc)
{
   int rc = OpenLogInternal(s_logs, name, session, rcc);
   if (rc != -2)
      return rc;

   // Try to find log definition in loaded modules
   for(DWORD i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].logs != NULL)
		{
         rc = OpenLogInternal(g_pModuleList[i].logs, name, session, rcc);
         if (rc != -2)
            return rc;
		}
	}

	*rcc = RCC_UNKNOWN_LOG_NAME;
	return -1;
}

/**
 * Close log
 */
DWORD CloseLog(ClientSession *session, int logHandle)
{
	DWORD rcc = RCC_INVALID_LOG_HANDLE;

	MutexLock(s_regListMutex);

	if ((logHandle >= 0) && (logHandle < s_regListSize) &&
	    (s_regList[logHandle].sessionId == session->getIndex()) &&
		 (s_regList[logHandle].handle != NULL))
	{
		s_regList[logHandle].handle->lock();
		s_regList[logHandle].handle->unlock();
		delete s_regList[logHandle].handle;
		s_regList[logHandle].handle = NULL;
		rcc = RCC_SUCCESS;
	}

	MutexUnlock(s_regListMutex);
	return rcc;
}

/**
 * Acqure log handle object
 * Caller must call LogHandle::unlock() when it finish work with acquired object
 */
LogHandle *AcquireLogHandleObject(ClientSession *session, int logHandle)
{
	LogHandle *object = NULL;

	MutexLock(s_regListMutex);

	if ((logHandle >= 0) && (logHandle < s_regListSize) &&
	    (s_regList[logHandle].sessionId == session->getIndex()) &&
		 (s_regList[logHandle].handle != NULL))
	{
		object = s_regList[logHandle].handle;
		object->lock();
	}

	MutexUnlock(s_regListMutex);
	return object;
}
