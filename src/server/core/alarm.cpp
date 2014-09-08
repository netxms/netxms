/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

/**
 * Global instance of alarm manager
 */
static ObjectArray<NXC_ALARM> *m_alarmList;
static MUTEX m_mutex = INVALID_MUTEX_HANDLE;
static UINT32 m_dwNotifyCode;
static NXC_ALARM *m_pNotifyAlarmInfo;
static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;
static THREAD m_hWatchdogThread = INVALID_THREAD_HANDLE;

/**
 * Client notification data
 */
struct CLIENT_NOTIFICATION_DATA
{
   UINT32 code;
   NXC_ALARM *alarm;
};

/**
 * Callback for client session enumeration
 */
static void SendAlarmNotification(ClientSession *session, void *arg)
{
   session->onAlarmUpdate(((CLIENT_NOTIFICATION_DATA *)arg)->code,
                          ((CLIENT_NOTIFICATION_DATA *)arg)->alarm);
}

/**
 * Notify connected clients about changes
 */
static void NotifyClients(UINT32 code, NXC_ALARM *alarm)
{
   CALL_ALL_MODULES(pfAlarmChangeHook, (code, alarm));

   CLIENT_NOTIFICATION_DATA data;
   data.code = code;
   data.alarm = alarm;
   EnumerateClientSessions(SendAlarmNotification, &data);
}

/**
 * Update alarm information in database
 */
static void UpdateAlarmInDB(NXC_ALARM *pAlarm)
{
   TCHAR szQuery[4096];

   _sntprintf(szQuery, 4096, _T("UPDATE alarms SET alarm_state=%d,ack_by=%d,term_by=%d,")
                             _T("last_change_time=%d,current_severity=%d,repeat_count=%d,")
                             _T("hd_state=%d,hd_ref=%s,timeout=%d,timeout_event=%d,")
									  _T("message=%s,resolved_by=%d, ack_timeout=%d WHERE alarm_id=%d"),
              pAlarm->nState, pAlarm->dwAckByUser, pAlarm->dwTermByUser,
              pAlarm->dwLastChangeTime, pAlarm->nCurrentSeverity,
              pAlarm->dwRepeatCount, pAlarm->nHelpDeskState,
			     (const TCHAR *)DBPrepareString(g_hCoreDB, pAlarm->szHelpDeskRef),
              pAlarm->dwTimeout, pAlarm->dwTimeoutEvent,
			     (const TCHAR *)DBPrepareString(g_hCoreDB, pAlarm->szMessage),
				  pAlarm->dwResolvedByUser, pAlarm->ackTimeout, pAlarm->dwAlarmId);
   QueueSQLRequest(szQuery);

	if (pAlarm->nState == ALARM_STATE_TERMINATED)
	{
		_sntprintf(szQuery, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%d"), (int)pAlarm->dwAlarmId);
		QueueSQLRequest(szQuery);
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DeleteAlarmNotes(hdb, pAlarm->dwAlarmId);
      DBConnectionPoolReleaseConnection(hdb);
	}
}

/**
 * Update object status after alarm acknowledgement or deletion
 */
static void UpdateObjectStatus(UINT32 objectId)
{
   NetObj *object = FindObjectById(objectId);
   if (object != NULL)
      object->calculateCompoundStatus();
}

/**
 * Fill NXCP message with alarm data
 */
void FillAlarmInfoMessage(CSCPMessage *pMsg, NXC_ALARM *pAlarm)
{
   pMsg->SetVariable(VID_ALARM_ID, pAlarm->dwAlarmId);
   pMsg->SetVariable(VID_ACK_BY_USER, pAlarm->dwAckByUser);
   pMsg->SetVariable(VID_RESOLVED_BY_USER, pAlarm->dwResolvedByUser);
   pMsg->SetVariable(VID_TERMINATED_BY_USER, pAlarm->dwTermByUser);
   pMsg->SetVariable(VID_EVENT_CODE, pAlarm->dwSourceEventCode);
   pMsg->SetVariable(VID_EVENT_ID, pAlarm->qwSourceEventId);
   pMsg->SetVariable(VID_OBJECT_ID, pAlarm->dwSourceObject);
   pMsg->SetVariable(VID_CREATION_TIME, pAlarm->dwCreationTime);
   pMsg->SetVariable(VID_LAST_CHANGE_TIME, pAlarm->dwLastChangeTime);
   pMsg->SetVariable(VID_ALARM_KEY, pAlarm->szKey);
   pMsg->SetVariable(VID_ALARM_MESSAGE, pAlarm->szMessage);
   pMsg->SetVariable(VID_STATE, (WORD)(pAlarm->nState & ALARM_STATE_MASK));	// send only state to client, without flags
   pMsg->SetVariable(VID_IS_STICKY, (WORD)((pAlarm->nState & ALARM_STATE_STICKY) ? 1 : 0));
   pMsg->SetVariable(VID_CURRENT_SEVERITY, (WORD)pAlarm->nCurrentSeverity);
   pMsg->SetVariable(VID_ORIGINAL_SEVERITY, (WORD)pAlarm->nOriginalSeverity);
   pMsg->SetVariable(VID_HELPDESK_STATE, (WORD)pAlarm->nHelpDeskState);
   pMsg->SetVariable(VID_HELPDESK_REF, pAlarm->szHelpDeskRef);
   pMsg->SetVariable(VID_REPEAT_COUNT, pAlarm->dwRepeatCount);
   pMsg->SetVariable(VID_ALARM_TIMEOUT, pAlarm->dwTimeout);
   pMsg->SetVariable(VID_ALARM_TIMEOUT_EVENT, pAlarm->dwTimeoutEvent);
   pMsg->SetVariable(VID_NUM_COMMENTS, pAlarm->noteCount);
   pMsg->SetVariable(VID_TIMESTAMP, (UINT32)((pAlarm->ackTimeout != 0) ? (pAlarm->ackTimeout - time(NULL)) : 0));
}

/**
 * Fill NXCP message with event data from SQL query
 * Expected field order: event_id,event_code,event_name,severity,source_object_id,event_timestamp,message
 */
static void FillEventData(CSCPMessage *msg, UINT32 baseId, DB_RESULT hResult, int row, QWORD rootId)
{
	TCHAR buffer[MAX_EVENT_MSG_LENGTH];

	msg->SetVariable(baseId, DBGetFieldUInt64(hResult, row, 0));
	msg->SetVariable(baseId + 1, rootId);
	msg->SetVariable(baseId + 2, DBGetFieldULong(hResult, row, 1));
	msg->SetVariable(baseId + 3, DBGetField(hResult, row, 2, buffer, MAX_DB_STRING));
	msg->SetVariable(baseId + 4, (WORD)DBGetFieldLong(hResult, row, 3));	// severity
	msg->SetVariable(baseId + 5, DBGetFieldULong(hResult, row, 4));  // source object
	msg->SetVariable(baseId + 6, DBGetFieldULong(hResult, row, 5));  // timestamp
	msg->SetVariable(baseId + 7, DBGetField(hResult, row, 6, buffer, MAX_EVENT_MSG_LENGTH));
}

/**
 * Get events correlated to given event into NXCP message
 *
 * @return number of consumed variable identifiers
 */
static UINT32 GetCorrelatedEvents(QWORD eventId, CSCPMessage *msg, UINT32 baseId, DB_HANDLE hdb)
{
	UINT32 varId = baseId;
	DB_STATEMENT hStmt = DBPrepare(hdb,
		(g_dbSyntax == DB_SYNTAX_ORACLE) ?
			_T("SELECT e.event_id,e.event_code,c.event_name,e.event_severity,e.event_source,e.event_timestamp,e.event_message ")
			_T("FROM event_log e,event_cfg c WHERE zero_to_null(e.root_event_id)=? AND c.event_code=e.event_code")
		:
			_T("SELECT e.event_id,e.event_code,c.event_name,e.event_severity,e.event_source,e.event_timestamp,e.event_message ")
			_T("FROM event_log e,event_cfg c WHERE e.root_event_id=? AND c.event_code=e.event_code"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, eventId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				FillEventData(msg, varId, hResult, i, eventId);
				varId += 10;
				QWORD eventId = DBGetFieldUInt64(hResult, i, 0);
				varId += GetCorrelatedEvents(eventId, msg, varId, hdb);
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}
	return varId - baseId;
}

/**
 * Fill NXCP message with alarm's related events
 */
static void FillAlarmEventsMessage(CSCPMessage *msg, UINT32 alarmId)
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
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			UINT32 varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < count; i++)
			{
				FillEventData(msg, varId, hResult, i, 0);
				varId += 10;
				QWORD eventId = DBGetFieldUInt64(hResult, i, 0);
				varId += GetCorrelatedEvents(eventId, msg, varId, hdb);
			}
			DBFreeResult(hResult);
			msg->SetVariable(VID_NUM_ELEMENTS, (varId - VID_ELEMENT_LIST_BASE) / 10);
		}
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get number of notes for alarm
 */
static UINT32 GetNoteCount(DB_HANDLE hdb, UINT32 alarmId)
{
	UINT32 value = 0;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT count(*) FROM alarm_notes WHERE alarm_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
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
 * Create new alarm
 */
void NXCORE_EXPORTABLE CreateNewAlarm(TCHAR *pszMsg, TCHAR *pszKey, int nState,
                                      int iSeverity, UINT32 dwTimeout,
									           UINT32 dwTimeoutEvent, Event *pEvent, UINT32 ackTimeout)
{
   TCHAR szQuery[2048];
   UINT32 alarmId = 0;
   BOOL bNewAlarm = TRUE;

   // Expand alarm's message and key
   TCHAR *pszExpMsg = pEvent->expandText(pszMsg);
   TCHAR *pszExpKey = pEvent->expandText(pszKey);

   // Check if we have a duplicate alarm
   if (((nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED) && (*pszExpKey != 0))
   {
      MutexLock(m_mutex);

      for(int i = 0; i < m_alarmList->size(); i++)
      {
         NXC_ALARM *alarm = m_alarmList->get(i);
			if (!_tcscmp(pszExpKey, alarm->szKey))
         {
            alarm->dwRepeatCount++;
            alarm->dwLastChangeTime = (UINT32)time(NULL);
            alarm->dwSourceObject = pEvent->getSourceId();
				if ((alarm->nState & ALARM_STATE_STICKY) == 0)
					alarm->nState = nState;
            alarm->nCurrentSeverity = iSeverity;
				alarm->dwTimeout = dwTimeout;
				alarm->dwTimeoutEvent = dwTimeoutEvent;
				alarm->ackTimeout = ackTimeout;
            nx_strncpy(alarm->szMessage, pszExpMsg, MAX_EVENT_MSG_LENGTH);

            NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
            UpdateAlarmInDB(alarm);

				alarmId = alarm->dwAlarmId;		// needed for correct update of related events

            bNewAlarm = FALSE;
            break;
         }
      }

      MutexUnlock(m_mutex);
   }

   if (bNewAlarm)
   {
      // Create new alarm structure
      NXC_ALARM *alarm = new NXC_ALARM();
      alarmId = CreateUniqueId(IDG_ALARM);
      alarm->dwAlarmId = alarmId;
      alarm->qwSourceEventId = pEvent->getId();
      alarm->dwSourceEventCode = pEvent->getCode();
      alarm->dwSourceObject = pEvent->getSourceId();
      alarm->dwCreationTime = (UINT32)time(NULL);
      alarm->dwLastChangeTime = alarm->dwCreationTime;
      alarm->nState = nState;
      alarm->nOriginalSeverity = iSeverity;
      alarm->nCurrentSeverity = iSeverity;
      alarm->dwRepeatCount = 1;
      alarm->nHelpDeskState = ALARM_HELPDESK_IGNORED;
		alarm->dwTimeout = dwTimeout;
		alarm->dwTimeoutEvent = dwTimeoutEvent;
		alarm->noteCount = 0;
		alarm->ackTimeout = 0;
      nx_strncpy(alarm->szMessage, pszExpMsg, MAX_EVENT_MSG_LENGTH);
      nx_strncpy(alarm->szKey, pszExpKey, MAX_DB_STRING);

      // Add new alarm to active alarm list if needed
		if ((alarm->nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
      {
         MutexLock(m_mutex);
         DbgPrintf(7, _T("AlarmManager: adding new active alarm, current alarm count %d"), m_alarmList->size());
         m_alarmList->add(alarm);
         MutexUnlock(m_mutex);
      }

      // Save alarm to database
      _sntprintf(szQuery, 2048,
			        _T("INSERT INTO alarms (alarm_id,creation_time,last_change_time,")
                 _T("source_object_id,source_event_code,message,original_severity,")
                 _T("current_severity,alarm_key,alarm_state,ack_by,resolved_by,hd_state,")
                 _T("hd_ref,repeat_count,term_by,timeout,timeout_event,source_event_id, ack_timeout) VALUES ")
                 _T("(%d,%d,%d,%d,%d,%s,%d,%d,%s,%d,%d,%d,%d,%s,%d,%d,%d,%d,") UINT64_FMT _T(",%d)"),
              alarm->dwAlarmId, alarm->dwCreationTime, alarm->dwLastChangeTime,
				  alarm->dwSourceObject, alarm->dwSourceEventCode,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm->szMessage),
              alarm->nOriginalSeverity, alarm->nCurrentSeverity,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm->szKey),
				  alarm->nState, alarm->dwAckByUser, alarm->dwResolvedByUser, alarm->nHelpDeskState,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm->szHelpDeskRef),
              alarm->dwRepeatCount, alarm->dwTermByUser, alarm->dwTimeout,
				  alarm->dwTimeoutEvent, alarm->qwSourceEventId, alarm->ackTimeout);
      QueueSQLRequest(szQuery);

      // Notify connected clients about new alarm
      NotifyClients(NX_NOTIFY_NEW_ALARM, alarm);
   }

   // Update status of related object if needed
   if ((nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
		UpdateObjectStatus(pEvent->getSourceId());

	// Add record to alarm_events table
	TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
	const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, pEvent->getName(), valSeverity, valSource, valTimestamp, pEvent->getMessage() };
	_sntprintf(valAlarmId, 16, _T("%d"), (int)alarmId);
	_sntprintf(valEventId, 32, UINT64_FMT, pEvent->getId());
	_sntprintf(valEventCode, 16, _T("%d"), (int)pEvent->getCode());
	_sntprintf(valSeverity, 16, _T("%d"), (int)pEvent->getSeverity());
	_sntprintf(valSource, 16, _T("%d"), pEvent->getSourceId());
	_sntprintf(valTimestamp, 16, _T("%u"), (UINT32)pEvent->getTimeStamp());
	static int sqlTypes[8] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_BIGINT, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };
	QueueSQLRequest(_T("INSERT INTO alarm_events (alarm_id,event_id,event_code,event_name,severity,source_object_id,event_timestamp,message) VALUES (?,?,?,?,?,?,?,?)"),
	                8, sqlTypes, values);

	free(pszExpMsg);
   free(pszExpKey);
}

/**
 * Do acknowledge
 */
static UINT32 DoAck(NXC_ALARM *alarm, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   if ((alarm->nState & ALARM_STATE_MASK) != ALARM_STATE_OUTSTANDING)
      return RCC_ALARM_NOT_OUTSTANDING;

   WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), alarm->dwSourceObject,
      _T("Acknowledged alarm %d (%s) on object %s"), alarm->dwAlarmId, alarm->szMessage,
      GetObjectName(alarm->dwSourceObject, _T("")));

   UINT32 endTime = acknowledgmentActionTime != 0 ? (UINT32)time(NULL) + acknowledgmentActionTime : 0;
   alarm->ackTimeout = endTime;
   alarm->nState = ALARM_STATE_ACKNOWLEDGED;
	if (sticky)
      alarm->nState |= ALARM_STATE_STICKY;
   alarm->dwAckByUser = session->getUserId();
   alarm->dwLastChangeTime = (UINT32)time(NULL);
   NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
   UpdateAlarmInDB(alarm);
   return RCC_SUCCESS;
}

/**
 * Acknowledge alarm with given ID
 */
UINT32 NXCORE_EXPORTABLE AckAlarmById(UINT32 dwAlarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
         dwRet = DoAck(alarm, session, sticky, acknowledgmentActionTime);
         dwObject = alarm->dwSourceObject;
         break;
      }
   }
   MutexUnlock(m_mutex);

   if (dwRet == RCC_SUCCESS)
      UpdateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Acknowledge alarm with given helpdesk reference
 */
UINT32 NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->szHelpDeskRef, hdref))
      {
         dwRet = DoAck(alarm, session, sticky, acknowledgmentActionTime);
         dwObject = alarm->dwSourceObject;
         break;
      }
   }
   MutexUnlock(m_mutex);

   if (dwRet == RCC_SUCCESS)
      UpdateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Resolve and possibly terminate alarm with given ID
 * Should return RCC which can be sent to client
 */
UINT32 NXCORE_EXPORTABLE ResolveAlarmById(UINT32 dwAlarmId, ClientSession *session, bool terminate)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
         // If alarm is open in helpdesk, it cannot be terminated
         if (alarm->nHelpDeskState != ALARM_HELPDESK_OPEN)
         {
            dwObject = alarm->dwSourceObject;
            if (session != NULL)
            {
               WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), dwObject,
                  _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
                  dwAlarmId, alarm->szMessage, GetObjectName(dwObject, _T("")));
            }

				if (terminate)
               alarm->dwTermByUser = (session != NULL) ? session->getUserId() : 0;
				else
               alarm->dwResolvedByUser = (session != NULL) ? session->getUserId() : 0;
            alarm->dwLastChangeTime = (UINT32)time(NULL);
				alarm->nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
				alarm->ackTimeout = 0;
				NotifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, alarm);
            UpdateAlarmInDB(alarm);
				if (terminate)
				{
               m_alarmList->remove(i);
				}
            dwRet = RCC_SUCCESS;
         }
         else
         {
            dwRet = RCC_ALARM_OPEN_IN_HELPDESK;
         }
         break;
      }
   }
   MutexUnlock(m_mutex);

   if (dwRet == RCC_SUCCESS)
      UpdateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Resolve and possibly terminate all alarms with given key
 */
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent)
{
   UINT32 *pdwObjectList = (UINT32 *)malloc(sizeof(UINT32) * m_alarmList->size());

   MutexLock(m_mutex);
   DWORD dwCurrTime = (UINT32)time(NULL);
   int numObjects = 0;
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
		if ((useRegexp ? RegexpMatch(alarm->szKey, pszKey, TRUE) : !_tcscmp(pszKey, alarm->szKey)) &&
         (alarm->nHelpDeskState != ALARM_HELPDESK_OPEN))
      {
         // Add alarm's source object to update list
         int j;
         for(j = 0; j < numObjects; j++)
         {
            if (pdwObjectList[j] == alarm->dwSourceObject)
               break;
         }
         if (j == numObjects)
         {
            pdwObjectList[numObjects++] = alarm->dwSourceObject;
         }

         // Resolve or terminate alarm
			alarm->nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
         alarm->dwLastChangeTime = dwCurrTime;
			if (terminate)
				alarm->dwTermByUser = 0;
			else
				alarm->dwResolvedByUser = 0;
         alarm->ackTimeout = 0;
			NotifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, alarm);
         UpdateAlarmInDB(alarm);
			if (terminate)
			{
            m_alarmList->remove(i);
				i--;
			}
         else
         {
	         // Add record to alarm_events table if alarm is resolved
	         TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
	         const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, pEvent->getName(), valSeverity, valSource, valTimestamp, pEvent->getMessage() };
	         _sntprintf(valAlarmId, 16, _T("%d"), (int)alarm->dwAlarmId);
	         _sntprintf(valEventId, 32, UINT64_FMT, pEvent->getId());
	         _sntprintf(valEventCode, 16, _T("%d"), (int)pEvent->getCode());
	         _sntprintf(valSeverity, 16, _T("%d"), (int)pEvent->getSeverity());
	         _sntprintf(valSource, 16, _T("%d"), pEvent->getSourceId());
	         _sntprintf(valTimestamp, 16, _T("%u"), (UINT32)pEvent->getTimeStamp());
	         static int sqlTypes[8] = { DB_SQLTYPE_INTEGER, DB_SQLTYPE_BIGINT, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_INTEGER, DB_SQLTYPE_VARCHAR };
	         QueueSQLRequest(_T("INSERT INTO alarm_events (alarm_id,event_id,event_code,event_name,severity,source_object_id,event_timestamp,message) VALUES (?,?,?,?,?,?,?,?)"),
	                         8, sqlTypes, values);
         }
      }
   }
   MutexUnlock(m_mutex);

   // Update status of objects
   for(int i = 0; i < numObjects; i++)
      UpdateObjectStatus(pdwObjectList[i]);
   free(pdwObjectList);
}

/**
 * Resolve and possibly terminate alarm with given helpdesk reference.
 * Auitomatically change alarm's helpdesk state to "closed"
 */
UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate)
{
   UINT32 objectId = 0;
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->szHelpDeskRef, hdref))
      {
         objectId = alarm->dwSourceObject;
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), objectId,
               _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
               alarm->dwAlarmId, alarm->szMessage, GetObjectName(objectId, _T("")));
         }

			if (terminate)
            alarm->dwTermByUser = (session != NULL) ? session->getUserId() : 0;
			else
            alarm->dwResolvedByUser = (session != NULL) ? session->getUserId() : 0;
         alarm->dwLastChangeTime = (UINT32)time(NULL);
			alarm->nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
			alarm->ackTimeout = 0;
         if (alarm->nHelpDeskState != ALARM_HELPDESK_IGNORED)
            alarm->nHelpDeskState = ALARM_HELPDESK_CLOSED;
			NotifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, alarm);
         UpdateAlarmInDB(alarm);
			if (terminate)
			{
            m_alarmList->remove(i);
			}
         DbgPrintf(5, _T("Alarm with helpdesk reference \"%s\" %s"), hdref, terminate ? _T("terminated") : _T("resolved"));
         rcc = RCC_SUCCESS;
         break;
      }
   }
   MutexUnlock(m_mutex);

   if (objectId != 0)
      UpdateObjectStatus(objectId);
   return rcc;
}

/**
 * Resolve alarm by helpdesk reference
 */
UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref)
{
   return ResolveAlarmByHDRef(hdref, NULL, false);
}

/**
 * Terminate alarm by helpdesk reference
 */
UINT32 NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref)
{
   return ResolveAlarmByHDRef(hdref, NULL, true);
}

/**
 * Open issue in helpdesk system
 */
UINT32 OpenHelpdeskIssue(UINT32 alarmId, ClientSession *session, TCHAR *hdref)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;
   *hdref = 0;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == alarmId)
      {
         if (alarm->nHelpDeskState == ALARM_HELPDESK_IGNORED)
         {
            /* TODO: unlock alarm list before call */
            const TCHAR *nodeName = GetObjectName(alarm->dwSourceObject, _T("[unknown]"));
            int messageLen = (int)(_tcslen(nodeName) + _tcslen(alarm->szMessage) + 32) * sizeof(TCHAR);
            TCHAR *message = (TCHAR *)malloc(messageLen);
            _sntprintf(message, messageLen, _T("%s: %s"), nodeName, alarm->szMessage);
            rcc = CreateHelpdeskIssue(message, alarm->szHelpDeskRef);
            free(message);
            if (rcc == RCC_SUCCESS)
            {
               alarm->nHelpDeskState = ALARM_HELPDESK_OPEN;
			      NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
               UpdateAlarmInDB(alarm);
               nx_strncpy(hdref, alarm->szHelpDeskRef, MAX_HELPDESK_REF_LEN);
               DbgPrintf(5, _T("Helpdesk issue created for alarm %d, reference \"%s\""), alarm->dwAlarmId, alarm->szHelpDeskRef);
            }
         }
         else
         {
            rcc = RCC_OUT_OF_STATE_REQUEST;
         }
         break;
      }
   }
   MutexUnlock(m_mutex);
   return rcc;
}

/**
 * Get helpdesk issue URL for given alarm
 */
UINT32 GetHelpdeskIssueUrlFromAlarm(UINT32 alarmId, TCHAR *url, size_t size)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == alarmId)
      {
         if ((alarm->nHelpDeskState != ALARM_HELPDESK_IGNORED) && (alarm->szHelpDeskRef[0] != 0))
         {
            rcc = GetHelpdeskIssueUrl(alarm->szHelpDeskRef, url, size);
         }
         else
         {
            rcc = RCC_OUT_OF_STATE_REQUEST;
         }
         break;
      }
   }
   MutexUnlock(m_mutex);
   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
UINT32 UnlinkHelpdeskIssueById(UINT32 dwAlarmId, ClientSession *session)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), 
               alarm->dwSourceObject, _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"), 
               alarm->szHelpDeskRef, alarm->dwAlarmId, alarm->szMessage, 
               GetObjectName(alarm->dwSourceObject, _T("")));
         }
         alarm->nHelpDeskState = ALARM_HELPDESK_IGNORED;
			NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
         UpdateAlarmInDB(alarm);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   MutexUnlock(m_mutex);

   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
UINT32 UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, ClientSession *session)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->szHelpDeskRef, hdref))
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(),
               alarm->dwSourceObject, _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"),
               alarm->szHelpDeskRef, alarm->dwAlarmId, alarm->szMessage, 
               GetObjectName(alarm->dwSourceObject, _T("")));
         }
         alarm->nHelpDeskState = ALARM_HELPDESK_IGNORED;
			NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
         UpdateAlarmInDB(alarm);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   MutexUnlock(m_mutex);

   return rcc;
}

/**
 * Delete alarm with given ID
 */
void NXCORE_EXPORTABLE DeleteAlarm(UINT32 dwAlarmId, bool objectCleanup)
{
   DWORD dwObject;

   // Delete alarm from in-memory list
   if (!objectCleanup)  // otherwise already locked
      MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
         dwObject = alarm->dwSourceObject;
         NotifyClients(NX_NOTIFY_ALARM_DELETED, alarm);
         m_alarmList->remove(i);
         break;
      }
   }
   if (!objectCleanup)
      MutexUnlock(m_mutex);

   // Delete from database
   if (!objectCleanup)
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, 256, _T("DELETE FROM alarms WHERE alarm_id=%d"), (int)dwAlarmId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%d"), (int)dwAlarmId);
      QueueSQLRequest(szQuery);

	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	   DeleteAlarmNotes(hdb, dwAlarmId);
	   DBConnectionPoolReleaseConnection(hdb);

      UpdateObjectStatus(dwObject);
   }
}

/**
 * Delete all alarms of given object. Intended to be called only
 * on final stage of object deletion.
 */
bool DeleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb)
{
	MutexLock(m_mutex);

	// go through from end because m_alarmList->size() is decremented by DeleteAlarm()
	for(int i = m_alarmList->size() - 1; i >= 0; i--)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
		if (alarm->dwSourceObject == objectId)
      {
			DeleteAlarm(alarm->dwAlarmId, true);
      }
	}

	MutexUnlock(m_mutex);

   // Delete all object alarms from database
   bool success = false;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT alarm_id FROM alarms WHERE source_object_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
         success = true;
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
         {
            UINT32 alarmId = DBGetFieldULong(hResult, i, 0);
				DeleteAlarmNotes(hdb, alarmId);
            DeleteAlarmEvents(hdb, alarmId);
         }
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

   if (success)
   {
	   hStmt = DBPrepare(hdb, _T("DELETE FROM alarms WHERE source_object_id=?"));
	   if (hStmt != NULL)
	   {
		   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
         success = DBExecute(hStmt) ? true : false;
         DBFreeStatement(hStmt);
      }
   }
   return success;
}

/**
 * Send all alarms to client
 */
void SendAlarmsToClient(UINT32 dwRqId, ClientSession *pSession)
{
   DWORD dwUserId = pSession->getUserId();

   // Prepare message
   CSCPMessage msg;
   msg.SetCode(CMD_ALARM_DATA);
   msg.SetId(dwRqId);

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      NetObj *pObject = FindObjectById(alarm->dwSourceObject);
      if (pObject != NULL)
      {
         if (pObject->checkAccessRights(dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            FillAlarmInfoMessage(&msg, alarm);
            pSession->sendMessage(&msg);
            msg.deleteAllVariables();
         }
      }
   }
   MutexUnlock(m_mutex);

   // Send end-of-list indicator
   msg.SetVariable(VID_ALARM_ID, (UINT32)0);
   pSession->sendMessage(&msg);
}

/**
 * Get alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
UINT32 NXCORE_EXPORTABLE GetAlarm(UINT32 dwAlarmId, CSCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
			FillAlarmInfoMessage(msg, alarm);
			dwRet = RCC_SUCCESS;
         break;
      }
   }
   MutexUnlock(m_mutex);

	return dwRet;
}

/**
 * Get all related events for alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
UINT32 NXCORE_EXPORTABLE GetAlarmEvents(UINT32 dwAlarmId, CSCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
      if (m_alarmList->get(i)->dwAlarmId == dwAlarmId)
      {
			dwRet = RCC_SUCCESS;
         break;
      }
   MutexUnlock(m_mutex);

	// we don't call FillAlarmEventsMessage from within loop
	// to prevent alarm list lock for a long time
	if (dwRet == RCC_SUCCESS)
		FillAlarmEventsMessage(msg, dwAlarmId);

	return dwRet;
}

/**
 * Get source object for given alarm id
 */
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(UINT32 dwAlarmId)
{
   UINT32 dwObjectId = 0;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == dwAlarmId)
      {
         dwObjectId = alarm->dwSourceObject;
         break;
      }
   }
   MutexUnlock(m_mutex);
   return (dwObjectId != 0) ? FindObjectById(dwObjectId) : NULL;
}

/**
 * Get source object for given alarm helpdesk reference
 */
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(const TCHAR *hdref)
{
   UINT32 dwObjectId = 0;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->szHelpDeskRef, hdref))
      {
         dwObjectId = alarm->dwSourceObject;
         break;
      }
   }
   MutexUnlock(m_mutex);
   return (dwObjectId != 0) ? FindObjectById(dwObjectId) : NULL;
}

/**
 * Get most critical status among active alarms for given object
 * Will return STATUS_UNKNOWN if there are no active alarms
 */
int GetMostCriticalStatusForObject(UINT32 dwObjectId)
{
   int iStatus = STATUS_UNKNOWN;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if ((alarm->dwSourceObject == dwObjectId) &&
			 ((alarm->nState & ALARM_STATE_MASK) < ALARM_STATE_RESOLVED) &&
          ((alarm->nCurrentSeverity > iStatus) || (iStatus == STATUS_UNKNOWN)))
      {
         iStatus = (int)alarm->nCurrentSeverity;
      }
   }
   MutexUnlock(m_mutex);
   return iStatus;
}

/**
 * Fill message with alarm stats
 */
void GetAlarmStats(CSCPMessage *pMsg)
{
   UINT32 dwCount[5];

   MutexLock(m_mutex);
   pMsg->SetVariable(VID_NUM_ALARMS, m_alarmList->size());
   memset(dwCount, 0, sizeof(UINT32) * 5);
   for(int i = 0; i < m_alarmList->size(); i++)
      dwCount[m_alarmList->get(i)->nCurrentSeverity]++;
   MutexUnlock(m_mutex);
   pMsg->setFieldInt32Array(VID_ALARMS_BY_SEVERITY, 5, dwCount);
}

/**
 * Watchdog thread
 */
static THREAD_RESULT THREAD_CALL WatchdogThread(void *arg)
{
	while(1)
	{
		if (ConditionWait(m_condShutdown, 1000))
			break;

		MutexLock(m_mutex);
		time_t now = time(NULL);
	   for(int i = 0; i < m_alarmList->size(); i++)
		{
         NXC_ALARM *alarm = m_alarmList->get(i);
			if ((alarm->dwTimeout > 0) &&
				 ((alarm->nState & ALARM_STATE_MASK) == ALARM_STATE_OUTSTANDING) &&
				 (((time_t)alarm->dwLastChangeTime + (time_t)alarm->dwTimeout) < now))
			{
				DbgPrintf(5, _T("Alarm timeout: alarm_id=%u, last_change=%u, timeout=%u, now=%u"),
				          alarm->dwAlarmId, alarm->dwLastChangeTime,
							 alarm->dwTimeout, (UINT32)now);

				PostEvent(alarm->dwTimeoutEvent, alarm->dwSourceObject, "dssd",
				          alarm->dwAlarmId, alarm->szMessage,
							 alarm->szKey, alarm->dwSourceEventCode);
				alarm->dwTimeout = 0;	// Disable repeated timeout events
				UpdateAlarmInDB(alarm);
			}

			if ((alarm->ackTimeout != 0) &&
				 ((alarm->nState & ALARM_STATE_STICKY) != 0) &&
				 (((time_t)alarm->ackTimeout <= now)))
			{
				DbgPrintf(5, _T("Alarm acknowledgment timeout: alarm_id=%u, timeout=%u, now=%u"),
				          alarm->dwAlarmId, alarm->ackTimeout, (UINT32)now);

				PostEvent(alarm->dwTimeoutEvent, alarm->dwSourceObject, "dssd",
				          alarm->dwAlarmId, alarm->szMessage,
							 alarm->szKey, alarm->dwSourceEventCode);
				alarm->ackTimeout = 0;	// Disable repeated timeout events
				alarm->nState = ALARM_STATE_OUTSTANDING;
				UpdateAlarmInDB(alarm);
				NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
			}
		}
		MutexUnlock(m_mutex);
	}
   return THREAD_OK;
}

/**
 * Check if given alram/note id pair is valid
 */
static bool IsValidNoteId(UINT32 alarmId, UINT32 noteId)
{
	bool isValid = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT note_id FROM alarm_notes WHERE alarm_id=? AND note_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, noteId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
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
 */
static UINT32 DoUpdateAlarmComment(NXC_ALARM *alarm, UINT32 noteId, const TCHAR *text, UINT32 userId, bool syncWithHelpdesk)
{
   bool newNote = false;
   UINT32 rcc;

	if (noteId != 0)
	{
      if (IsValidNoteId(alarm->dwAlarmId, noteId))
		{
			DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
			DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE alarm_notes SET change_time=?,user_id=?,note_text=? WHERE note_id=?"));
			if (hStmt != NULL)
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
				DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, userId);
				DBBind(hStmt, 3, DB_SQLTYPE_TEXT, text, DB_BIND_STATIC);
				DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, noteId);
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
		noteId = CreateUniqueId(IDG_ALARM_NOTE);
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO alarm_notes (note_id,alarm_id,change_time,user_id,note_text) VALUES (?,?,?,?,?)"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, noteId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, alarm->dwAlarmId);
			DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
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
      if(newNote)
         alarm->noteCount++;
		NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
      if (syncWithHelpdesk && (alarm->nHelpDeskState == ALARM_HELPDESK_OPEN))
      {
         AddHelpdeskIssueComment(alarm->szHelpDeskRef, text);
      }
	}

   return rcc;
}

/**
 * Add alarm's comment by helpdesk reference
 */
UINT32 AddAlarmComment(const TCHAR *hdref, const TCHAR *text, UINT32 userId)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;
   bool newNote = false;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->szHelpDeskRef, hdref))
      {
         rcc = DoUpdateAlarmComment(alarm, 0, text, userId, false);
         break;
      }
   }
   MutexUnlock(m_mutex);

   return rcc;
}

/**
 * Update alarm's comment
 */
UINT32 UpdateAlarmComment(UINT32 alarmId, UINT32 noteId, const TCHAR *text, UINT32 userId)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;
   bool newNote = false;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == alarmId)
      {
         rcc = DoUpdateAlarmComment(alarm, noteId, text, userId, true);
         break;
      }
   }
   MutexUnlock(m_mutex);

   return rcc;
}

/**
 * Delete comment
 */
UINT32 DeleteAlarmCommentByID(UINT32 alarmId, UINT32 noteId)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->dwAlarmId == alarmId)
      {
         if (IsValidNoteId(alarmId, noteId))
         {
            DB_HANDLE db = DBConnectionPoolAcquireConnection();
            DB_STATEMENT stmt = DBPrepare(db, _T("DELETE FROM alarm_notes WHERE note_id=?"));
            if (stmt != NULL)
            {
               DBBind(stmt, 1, DB_SQLTYPE_INTEGER, noteId);
               rcc = DBExecute(stmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
               DBFreeStatement(stmt);
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }
            DBConnectionPoolReleaseConnection(db);
         }
         else
         {
            rcc = RCC_INVALID_ALARM_NOTE_ID;
         }
         if (rcc == RCC_SUCCESS)
			{
				alarm->noteCount--;
				NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
			}
         break;
      }
   }
   MutexUnlock(m_mutex);

   return rcc;
}

/**
 * Get alarm's comments
 */
UINT32 GetAlarmComments(UINT32 alarmId, CSCPMessage *msg)
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	UINT32 rcc = RCC_DB_FAILURE;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT note_id,change_time,user_id,note_text FROM alarm_notes WHERE alarm_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, alarmId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			msg->SetVariable(VID_NUM_ELEMENTS, (UINT32)count);

			UINT32 varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < count; i++)
			{
				msg->SetVariable(varId++, DBGetFieldULong(hResult, i, 0));
				msg->SetVariable(varId++, alarmId);
				msg->SetVariable(varId++, DBGetFieldULong(hResult, i, 1));
				msg->SetVariable(varId++, DBGetFieldULong(hResult, i, 2));
				TCHAR *text = DBGetField(hResult, i, 3, NULL, 0);
				msg->SetVariable(varId++, CHECK_NULL_EX(text));
				safe_free(text);
				varId += 5;
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
 * @return array of active alarms for given object
 */
ObjectArray<NXC_ALARM> NXCORE_EXPORTABLE *GetAlarms(UINT32 objectId)
{
   ObjectArray<NXC_ALARM> *result = new ObjectArray<NXC_ALARM>(16, 16, true);

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if ((objectId == 0) || (alarm->dwSourceObject == objectId))
      {
         result->add((NXC_ALARM *)nx_memdup(alarm, sizeof(NXC_ALARM)));
      }
   }
   MutexUnlock(m_mutex);
   return result;
}

/**
 * Initialize alarm manager at system startup
 */
bool InitAlarmManager()
{
   m_alarmList = new ObjectArray<NXC_ALARM>(64, 64, true);
   m_mutex = MutexCreate();
	m_condShutdown = ConditionCreate(FALSE);
	m_hWatchdogThread = INVALID_THREAD_HANDLE;

   DB_RESULT hResult;

   // Load active alarms into memory
   hResult = DBSelect(g_hCoreDB, _T("SELECT alarm_id,source_object_id,")
                                 _T("source_event_code,source_event_id,message,")
                                 _T("original_severity,current_severity,")
                                 _T("alarm_key,creation_time,last_change_time,")
                                 _T("hd_state,hd_ref,ack_by,repeat_count,")
                                 _T("alarm_state,timeout,timeout_event,resolved_by,")
                                 _T("ack_timeout FROM alarms WHERE alarm_state<>3"));
   if (hResult == NULL)
      return false;

   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      for(int i = 0; i < count; i++)
      {
         NXC_ALARM *alarm = new NXC_ALARM();
         alarm->dwAlarmId = DBGetFieldULong(hResult, i, 0);
         alarm->dwSourceObject = DBGetFieldULong(hResult, i, 1);
         alarm->dwSourceEventCode = DBGetFieldULong(hResult, i, 2);
         alarm->qwSourceEventId = DBGetFieldUInt64(hResult, i, 3);
         DBGetField(hResult, i, 4, alarm->szMessage, MAX_EVENT_MSG_LENGTH);
         alarm->nOriginalSeverity = (BYTE)DBGetFieldLong(hResult, i, 5);
         alarm->nCurrentSeverity = (BYTE)DBGetFieldLong(hResult, i, 6);
         DBGetField(hResult, i, 7, alarm->szKey, MAX_DB_STRING);
         alarm->dwCreationTime = DBGetFieldULong(hResult, i, 8);
         alarm->dwLastChangeTime = DBGetFieldULong(hResult, i, 9);
         alarm->nHelpDeskState = (BYTE)DBGetFieldLong(hResult, i, 10);
         DBGetField(hResult, i, 11, alarm->szHelpDeskRef, MAX_HELPDESK_REF_LEN);
         alarm->dwAckByUser = DBGetFieldULong(hResult, i, 12);
         alarm->dwRepeatCount = DBGetFieldULong(hResult, i, 13);
         alarm->nState = (BYTE)DBGetFieldLong(hResult, i, 14);
         alarm->dwTimeout = DBGetFieldULong(hResult, i, 15);
         alarm->dwTimeoutEvent = DBGetFieldULong(hResult, i, 16);
			alarm->noteCount = GetNoteCount(g_hCoreDB, alarm->dwAlarmId);
         alarm->dwResolvedByUser = DBGetFieldULong(hResult, i, 17);
         alarm->ackTimeout = DBGetFieldULong(hResult, i, 18);
         m_alarmList->add(alarm);
      }
   }

   DBFreeResult(hResult);

	m_hWatchdogThread = ThreadCreateEx(WatchdogThread, 0, NULL);
   return true;
}

/**
 * Alarm manager destructor
 */
void ShutdownAlarmManager()
{
	ConditionSet(m_condShutdown);
	ThreadJoin(m_hWatchdogThread);
   MutexDestroy(m_mutex);
   delete m_alarmList;
}
