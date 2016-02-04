/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
									  _T("message=%s,resolved_by=%d,ack_timeout=%d,source_object_id=%d,")
									  _T("dci_id=%d WHERE alarm_id=%d"),
              pAlarm->state, pAlarm->ackByUser, pAlarm->termByUser,
              pAlarm->lastChangeTime, pAlarm->currentSeverity,
              pAlarm->repeatCount, pAlarm->helpDeskState,
			     (const TCHAR *)DBPrepareString(g_dbDriver, pAlarm->helpDeskRef),
              pAlarm->timeout, pAlarm->timeoutEvent,
			     (const TCHAR *)DBPrepareString(g_dbDriver, pAlarm->message),
				  pAlarm->resolvedByUser, pAlarm->ackTimeout, pAlarm->sourceObject,
				  pAlarm->dciId, pAlarm->alarmId);
   QueueSQLRequest(szQuery);

	if (pAlarm->state == ALARM_STATE_TERMINATED)
	{
		_sntprintf(szQuery, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%d"), (int)pAlarm->alarmId);
		QueueSQLRequest(szQuery);

		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DeleteAlarmNotes(hdb, pAlarm->alarmId);
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
void FillAlarmInfoMessage(NXCPMessage *pMsg, NXC_ALARM *pAlarm)
{
   pMsg->setField(VID_ALARM_ID, pAlarm->alarmId);
   pMsg->setField(VID_ACK_BY_USER, pAlarm->ackByUser);
   pMsg->setField(VID_RESOLVED_BY_USER, pAlarm->resolvedByUser);
   pMsg->setField(VID_TERMINATED_BY_USER, pAlarm->termByUser);
   pMsg->setField(VID_EVENT_CODE, pAlarm->sourceEventCode);
   pMsg->setField(VID_EVENT_ID, pAlarm->sourceEventId);
   pMsg->setField(VID_OBJECT_ID, pAlarm->sourceObject);
   pMsg->setField(VID_DCI_ID, pAlarm->dciId);
   pMsg->setField(VID_CREATION_TIME, pAlarm->creationTime);
   pMsg->setField(VID_LAST_CHANGE_TIME, pAlarm->lastChangeTime);
   pMsg->setField(VID_ALARM_KEY, pAlarm->key);
   pMsg->setField(VID_ALARM_MESSAGE, pAlarm->message);
   pMsg->setField(VID_STATE, (WORD)(pAlarm->state & ALARM_STATE_MASK));	// send only state to client, without flags
   pMsg->setField(VID_IS_STICKY, (WORD)((pAlarm->state & ALARM_STATE_STICKY) ? 1 : 0));
   pMsg->setField(VID_CURRENT_SEVERITY, (WORD)pAlarm->currentSeverity);
   pMsg->setField(VID_ORIGINAL_SEVERITY, (WORD)pAlarm->originalSeverity);
   pMsg->setField(VID_HELPDESK_STATE, (WORD)pAlarm->helpDeskState);
   pMsg->setField(VID_HELPDESK_REF, pAlarm->helpDeskRef);
   pMsg->setField(VID_REPEAT_COUNT, pAlarm->repeatCount);
   pMsg->setField(VID_ALARM_TIMEOUT, pAlarm->timeout);
   pMsg->setField(VID_ALARM_TIMEOUT_EVENT, pAlarm->timeoutEvent);
   pMsg->setField(VID_NUM_COMMENTS, pAlarm->noteCount);
   pMsg->setField(VID_TIMESTAMP, (UINT32)((pAlarm->ackTimeout != 0) ? (pAlarm->ackTimeout - time(NULL)) : 0));
}

/**
 * Fill NXCP message with event data from SQL query
 * Expected field order: event_id,event_code,event_name,severity,source_object_id,event_timestamp,message
 */
static void FillEventData(NXCPMessage *msg, UINT32 baseId, DB_RESULT hResult, int row, QWORD rootId)
{
	TCHAR buffer[MAX_EVENT_MSG_LENGTH];

	msg->setField(baseId, DBGetFieldUInt64(hResult, row, 0));
	msg->setField(baseId + 1, rootId);
	msg->setField(baseId + 2, DBGetFieldULong(hResult, row, 1));
	msg->setField(baseId + 3, DBGetField(hResult, row, 2, buffer, MAX_DB_STRING));
	msg->setField(baseId + 4, (WORD)DBGetFieldLong(hResult, row, 3));	// severity
	msg->setField(baseId + 5, DBGetFieldULong(hResult, row, 4));  // source object
	msg->setField(baseId + 6, DBGetFieldULong(hResult, row, 5));  // timestamp
	msg->setField(baseId + 7, DBGetField(hResult, row, 6, buffer, MAX_EVENT_MSG_LENGTH));
}

/**
 * Get events correlated to given event into NXCP message
 *
 * @return number of consumed variable identifiers
 */
static UINT32 GetCorrelatedEvents(QWORD eventId, NXCPMessage *msg, UINT32 baseId, DB_HANDLE hdb)
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
static void FillAlarmEventsMessage(NXCPMessage *msg, UINT32 alarmId)
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
			msg->setField(VID_NUM_ELEMENTS, (varId - VID_ELEMENT_LIST_BASE) / 10);
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
void NXCORE_EXPORTABLE CreateNewAlarm(TCHAR *pszMsg, TCHAR *pszKey, int state,
                                      int iSeverity, UINT32 timeout,
									           UINT32 timeoutEvent, Event *pEvent, UINT32 ackTimeout)
{
   TCHAR szQuery[2048];
   UINT32 alarmId = 0;
   BOOL bNewAlarm = TRUE;
   bool updateRelatedEvent = false;

   // Expand alarm's message and key
   TCHAR *pszExpMsg = pEvent->expandText(pszMsg);
   TCHAR *pszExpKey = pEvent->expandText(pszKey);

   // Check if we have a duplicate alarm
   if (((state & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED) && (*pszExpKey != 0))
   {
      MutexLock(m_mutex);

      for(int i = 0; i < m_alarmList->size(); i++)
      {
         NXC_ALARM *alarm = m_alarmList->get(i);
			if (!_tcscmp(pszExpKey, alarm->key))
         {
            alarm->repeatCount++;
            alarm->lastChangeTime = (UINT32)time(NULL);
            alarm->sourceObject = pEvent->getSourceId();
            alarm->dciId = pEvent->getDciId();
				if ((alarm->state & ALARM_STATE_STICKY) == 0)
					alarm->state = state;
            alarm->currentSeverity = iSeverity;
				alarm->timeout = timeout;
				alarm->timeoutEvent = timeoutEvent;
				alarm->ackTimeout = ackTimeout;
            nx_strncpy(alarm->message, pszExpMsg, MAX_EVENT_MSG_LENGTH);

            NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
            UpdateAlarmInDB(alarm);

            if(!alarm->connectedEvents.contains(pEvent->getId()))
            {
               alarmId = alarm->alarmId;		// needed for correct update of related events
               updateRelatedEvent = true;
               alarm->connectedEvents.add(pEvent->getId());
            }

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
      alarm->alarmId = alarmId;
      alarm->sourceEventId = pEvent->getId();
      alarm->sourceEventCode = pEvent->getCode();
      alarm->sourceObject = pEvent->getSourceId();
      alarm->dciId = pEvent->getDciId();
      alarm->creationTime = (UINT32)time(NULL);
      alarm->lastChangeTime = alarm->creationTime;
      alarm->state = state;
      alarm->originalSeverity = iSeverity;
      alarm->currentSeverity = iSeverity;
      alarm->repeatCount = 1;
      alarm->helpDeskState = ALARM_HELPDESK_IGNORED;
		alarm->timeout = timeout;
		alarm->timeoutEvent = timeoutEvent;
		alarm->noteCount = 0;
		alarm->ackTimeout = 0;
		alarm->connectedEvents.add(pEvent->getId());
      nx_strncpy(alarm->message, pszExpMsg, MAX_EVENT_MSG_LENGTH);
      nx_strncpy(alarm->key, pszExpKey, MAX_DB_STRING);

      // Add new alarm to active alarm list if needed
		if ((alarm->state & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
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
                 _T("hd_ref,repeat_count,term_by,timeout,timeout_event,source_event_id,")
                 _T("ack_timeout,dci_id) VALUES ")
                 _T("(%d,%d,%d,%d,%d,%s,%d,%d,%s,%d,%d,%d,%d,%s,%d,%d,%d,%d,") UINT64_FMT _T(",%d,%d)"),
              alarm->alarmId, alarm->creationTime, alarm->lastChangeTime,
				  alarm->sourceObject, alarm->sourceEventCode,
				  (const TCHAR *)DBPrepareString(g_dbDriver, alarm->message),
              alarm->originalSeverity, alarm->currentSeverity,
				  (const TCHAR *)DBPrepareString(g_dbDriver, alarm->key),
				  alarm->state, alarm->ackByUser, alarm->resolvedByUser, alarm->helpDeskState,
				  (const TCHAR *)DBPrepareString(g_dbDriver, alarm->helpDeskRef),
              alarm->repeatCount, alarm->termByUser, alarm->timeout,
				  alarm->timeoutEvent, alarm->sourceEventId, alarm->ackTimeout,
				  alarm->dciId);
      QueueSQLRequest(szQuery);
      updateRelatedEvent = true;

      // Notify connected clients about new alarm
      NotifyClients(NX_NOTIFY_NEW_ALARM, alarm);
   }

   // Update status of related object if needed
   if ((state & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
		UpdateObjectStatus(pEvent->getSourceId());

   if(updateRelatedEvent)
   {
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
   }

	free(pszExpMsg);
   free(pszExpKey);
}

/**
 * Do acknowledge
 */
static UINT32 DoAck(NXC_ALARM *alarm, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   if ((alarm->state & ALARM_STATE_MASK) != ALARM_STATE_OUTSTANDING)
      return RCC_ALARM_NOT_OUTSTANDING;

   if (session != NULL)
   {
      WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), alarm->sourceObject,
         _T("Acknowledged alarm %d (%s) on object %s"), alarm->alarmId, alarm->message,
         GetObjectName(alarm->sourceObject, _T("")));
   }

   UINT32 endTime = acknowledgmentActionTime != 0 ? (UINT32)time(NULL) + acknowledgmentActionTime : 0;
   alarm->ackTimeout = endTime;
   alarm->state = ALARM_STATE_ACKNOWLEDGED;
	if (sticky)
      alarm->state |= ALARM_STATE_STICKY;
   alarm->ackByUser = (session != NULL) ? session->getUserId() : 0;
   alarm->lastChangeTime = (UINT32)time(NULL);
   NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
   UpdateAlarmInDB(alarm);
   return RCC_SUCCESS;
}

/**
 * Acknowledge alarm with given ID
 */
UINT32 NXCORE_EXPORTABLE AckAlarmById(UINT32 alarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
      {
         dwRet = DoAck(alarm, session, sticky, acknowledgmentActionTime);
         dwObject = alarm->sourceObject;
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
      if (!_tcscmp(alarm->helpDeskRef, hdref))
      {
         dwRet = DoAck(alarm, session, sticky, acknowledgmentActionTime);
         dwObject = alarm->sourceObject;
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
UINT32 NXCORE_EXPORTABLE ResolveAlarmById(UINT32 alarmId, ClientSession *session, bool terminate)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
      {
         // If alarm is open in helpdesk, it cannot be terminated
         if (alarm->helpDeskState != ALARM_HELPDESK_OPEN)
         {
            dwObject = alarm->sourceObject;
            if (session != NULL)
            {
               WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), dwObject,
                  _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
                  alarmId, alarm->message, GetObjectName(dwObject, _T("")));
            }

				if (terminate)
               alarm->termByUser = (session != NULL) ? session->getUserId() : 0;
				else
               alarm->resolvedByUser = (session != NULL) ? session->getUserId() : 0;
            alarm->lastChangeTime = (UINT32)time(NULL);
				alarm->state = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
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
		if ((useRegexp ? RegexpMatch(alarm->key, pszKey, TRUE) : !_tcscmp(pszKey, alarm->key)) &&
         (alarm->helpDeskState != ALARM_HELPDESK_OPEN))
      {
         // Add alarm's source object to update list
         int j;
         for(j = 0; j < numObjects; j++)
         {
            if (pdwObjectList[j] == alarm->sourceObject)
               break;
         }
         if (j == numObjects)
         {
            pdwObjectList[numObjects++] = alarm->sourceObject;
         }

         // Resolve or terminate alarm
			alarm->state = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
         alarm->lastChangeTime = dwCurrTime;
			if (terminate)
				alarm->termByUser = 0;
			else
				alarm->resolvedByUser = 0;
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
            if(!alarm->connectedEvents.contains(pEvent->getId()))
            {
               alarm->connectedEvents.add(pEvent->getId());
               // Add record to alarm_events table if alarm is resolved
               TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
               const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, pEvent->getName(), valSeverity, valSource, valTimestamp, pEvent->getMessage() };
               _sntprintf(valAlarmId, 16, _T("%d"), (int)alarm->alarmId);
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
      if (!_tcscmp(alarm->helpDeskRef, hdref))
      {
         objectId = alarm->sourceObject;
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(), objectId,
               _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
               alarm->alarmId, alarm->message, GetObjectName(objectId, _T("")));
         }

			if (terminate)
            alarm->termByUser = (session != NULL) ? session->getUserId() : 0;
			else
            alarm->resolvedByUser = (session != NULL) ? session->getUserId() : 0;
         alarm->lastChangeTime = (UINT32)time(NULL);
			alarm->state = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
			alarm->ackTimeout = 0;
         if (alarm->helpDeskState != ALARM_HELPDESK_IGNORED)
            alarm->helpDeskState = ALARM_HELPDESK_CLOSED;
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
      if (alarm->alarmId == alarmId)
      {
         if (alarm->helpDeskState == ALARM_HELPDESK_IGNORED)
         {
            /* TODO: unlock alarm list before call */
            const TCHAR *nodeName = GetObjectName(alarm->sourceObject, _T("[unknown]"));
            int messageLen = (int)(_tcslen(nodeName) + _tcslen(alarm->message) + 32) * sizeof(TCHAR);
            TCHAR *message = (TCHAR *)malloc(messageLen);
            _sntprintf(message, messageLen, _T("%s: %s"), nodeName, alarm->message);
            rcc = CreateHelpdeskIssue(message, alarm->helpDeskRef);
            free(message);
            if (rcc == RCC_SUCCESS)
            {
               alarm->helpDeskState = ALARM_HELPDESK_OPEN;
			      NotifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
               UpdateAlarmInDB(alarm);
               nx_strncpy(hdref, alarm->helpDeskRef, MAX_HELPDESK_REF_LEN);
               DbgPrintf(5, _T("Helpdesk issue created for alarm %d, reference \"%s\""), alarm->alarmId, alarm->helpDeskRef);
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
      if (alarm->alarmId == alarmId)
      {
         if ((alarm->helpDeskState != ALARM_HELPDESK_IGNORED) && (alarm->helpDeskRef[0] != 0))
         {
            rcc = GetHelpdeskIssueUrl(alarm->helpDeskRef, url, size);
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
UINT32 UnlinkHelpdeskIssueById(UINT32 alarmId, ClientSession *session)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(),
               alarm->sourceObject, _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"),
               alarm->helpDeskRef, alarm->alarmId, alarm->message,
               GetObjectName(alarm->sourceObject, _T("")));
         }
         alarm->helpDeskState = ALARM_HELPDESK_IGNORED;
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
      if (!_tcscmp(alarm->helpDeskRef, hdref))
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), session->getId(),
               alarm->sourceObject, _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"),
               alarm->helpDeskRef, alarm->alarmId, alarm->message,
               GetObjectName(alarm->sourceObject, _T("")));
         }
         alarm->helpDeskState = ALARM_HELPDESK_IGNORED;
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
void NXCORE_EXPORTABLE DeleteAlarm(UINT32 alarmId, bool objectCleanup)
{
   DWORD dwObject;

   // Delete alarm from in-memory list
   if (!objectCleanup)  // otherwise already locked
      MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
      {
         dwObject = alarm->sourceObject;
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

      _sntprintf(szQuery, 256, _T("DELETE FROM alarms WHERE alarm_id=%d"), (int)alarmId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, 256, _T("DELETE FROM alarm_events WHERE alarm_id=%d"), (int)alarmId);
      QueueSQLRequest(szQuery);

	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	   DeleteAlarmNotes(hdb, alarmId);
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
		if (alarm->sourceObject == objectId)
      {
			DeleteAlarm(alarm->alarmId, true);
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
   NXCPMessage msg;
   msg.setCode(CMD_ALARM_DATA);
   msg.setId(dwRqId);

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      NetObj *pObject = FindObjectById(alarm->sourceObject);
      if (pObject != NULL)
      {
         if (pObject->checkAccessRights(dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            FillAlarmInfoMessage(&msg, alarm);
            pSession->sendMessage(&msg);
            msg.deleteAllFields();
         }
      }
   }
   MutexUnlock(m_mutex);

   // Send end-of-list indicator
   msg.setField(VID_ALARM_ID, (UINT32)0);
   pSession->sendMessage(&msg);
}

/**
 * Get alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
UINT32 NXCORE_EXPORTABLE GetAlarm(UINT32 alarmId, NXCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
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
UINT32 NXCORE_EXPORTABLE GetAlarmEvents(UINT32 alarmId, NXCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
      if (m_alarmList->get(i)->alarmId == alarmId)
      {
			dwRet = RCC_SUCCESS;
         break;
      }
   MutexUnlock(m_mutex);

	// we don't call FillAlarmEventsMessage from within loop
	// to prevent alarm list lock for a long time
	if (dwRet == RCC_SUCCESS)
		FillAlarmEventsMessage(msg, alarmId);

	return dwRet;
}

/**
 * Get source object for given alarm id
 */
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(UINT32 alarmId)
{
   UINT32 dwObjectId = 0;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
      {
         dwObjectId = alarm->sourceObject;
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
      if (!_tcscmp(alarm->helpDeskRef, hdref))
      {
         dwObjectId = alarm->sourceObject;
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
      if ((alarm->sourceObject == dwObjectId) &&
			 ((alarm->state & ALARM_STATE_MASK) < ALARM_STATE_RESOLVED) &&
          ((alarm->currentSeverity > iStatus) || (iStatus == STATUS_UNKNOWN)))
      {
         iStatus = (int)alarm->currentSeverity;
      }
   }
   MutexUnlock(m_mutex);
   return iStatus;
}

/**
 * Fill message with alarm stats
 */
void GetAlarmStats(NXCPMessage *pMsg)
{
   UINT32 dwCount[5];

   MutexLock(m_mutex);
   pMsg->setField(VID_NUM_ALARMS, m_alarmList->size());
   memset(dwCount, 0, sizeof(UINT32) * 5);
   for(int i = 0; i < m_alarmList->size(); i++)
      dwCount[m_alarmList->get(i)->currentSeverity]++;
   MutexUnlock(m_mutex);
   pMsg->setFieldFromInt32Array(VID_ALARMS_BY_SEVERITY, 5, dwCount);
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
			if ((alarm->timeout > 0) &&
				 ((alarm->state & ALARM_STATE_MASK) == ALARM_STATE_OUTSTANDING) &&
				 (((time_t)alarm->lastChangeTime + (time_t)alarm->timeout) < now))
			{
				DbgPrintf(5, _T("Alarm timeout: alarm_id=%u, last_change=%u, timeout=%u, now=%u"),
				          alarm->alarmId, alarm->lastChangeTime,
							 alarm->timeout, (UINT32)now);

				PostEvent(alarm->timeoutEvent, alarm->sourceObject, "dssd",
				          alarm->alarmId, alarm->message,
							 alarm->key, alarm->sourceEventCode);
				alarm->timeout = 0;	// Disable repeated timeout events
				UpdateAlarmInDB(alarm);
			}

			if ((alarm->ackTimeout != 0) &&
				 ((alarm->state & ALARM_STATE_STICKY) != 0) &&
				 (((time_t)alarm->ackTimeout <= now)))
			{
				DbgPrintf(5, _T("Alarm acknowledgment timeout: alarm_id=%u, timeout=%u, now=%u"),
				          alarm->alarmId, alarm->ackTimeout, (UINT32)now);

				PostEvent(alarm->timeoutEvent, alarm->sourceObject, "dssd",
				          alarm->alarmId, alarm->message,
							 alarm->key, alarm->sourceEventCode);
				alarm->ackTimeout = 0;	// Disable repeated timeout events
				alarm->state = ALARM_STATE_OUTSTANDING;
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
      if (IsValidNoteId(alarm->alarmId, noteId))
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
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, alarm->alarmId);
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
      if (syncWithHelpdesk && (alarm->helpDeskState == ALARM_HELPDESK_OPEN))
      {
         AddHelpdeskIssueComment(alarm->helpDeskRef, text);
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

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (!_tcscmp(alarm->helpDeskRef, hdref))
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

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *alarm = m_alarmList->get(i);
      if (alarm->alarmId == alarmId)
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
      if (alarm->alarmId == alarmId)
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
UINT32 GetAlarmComments(UINT32 alarmId, NXCPMessage *msg)
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
			msg->setField(VID_NUM_ELEMENTS, (UINT32)count);

			UINT32 varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < count; i++)
			{
				msg->setField(varId++, DBGetFieldULong(hResult, i, 0));
				msg->setField(varId++, alarmId);
				msg->setField(varId++, DBGetFieldULong(hResult, i, 1));
            UINT32 userId = DBGetFieldULong(hResult, i, 2);
				msg->setField(varId++, userId);

            TCHAR *text = DBGetField(hResult, i, 3, NULL, 0);
				msg->setField(varId++, CHECK_NULL_EX(text));
				safe_free(text);

            TCHAR userName[MAX_USER_NAME];
            if (ResolveUserId(userId, userName, MAX_USER_NAME))
            {
   				msg->setField(varId++, userName);
            }
            else
            {
               varId++;
            }

            varId += 4;
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
      if ((objectId == 0) || (alarm->sourceObject == objectId))
      {
         result->add((NXC_ALARM *)nx_memdup(alarm, sizeof(NXC_ALARM)));
      }
   }
   MutexUnlock(m_mutex);
   return result;
}

/**
 * NXSL extension: Find alarm by ID
 */
int F_FindAlarmById(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   UINT32 alarmId = argv[0]->getValueAsUInt32();
   NXC_ALARM *alarm = NULL;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *a = m_alarmList->get(i);
      if (a->alarmId == alarmId)
      {
         alarm = (NXC_ALARM *)nx_memdup(a, sizeof(NXC_ALARM));
         break;
      }
   }
   MutexUnlock(m_mutex);

   *result = (alarm != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslAlarmClass, alarm)) : new NXSL_Value();
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
   NXC_ALARM *alarm = NULL;

   MutexLock(m_mutex);
   for(int i = 0; i < m_alarmList->size(); i++)
   {
      NXC_ALARM *a = m_alarmList->get(i);
      if (!_tcscmp(a->key, key))
      {
         alarm = (NXC_ALARM *)nx_memdup(a, sizeof(NXC_ALARM));
         break;
      }
   }
   MutexUnlock(m_mutex);

   *result = (alarm != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslAlarmClass, alarm)) : new NXSL_Value();
   return 0;
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
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT alarm_id,source_object_id,")
                           _T("source_event_code,source_event_id,message,")
                           _T("original_severity,current_severity,")
                           _T("alarm_key,creation_time,last_change_time,")
                           _T("hd_state,hd_ref,ack_by,repeat_count,")
                           _T("alarm_state,timeout,timeout_event,resolved_by,")
                           _T("ack_timeout,dci_id FROM alarms WHERE alarm_state<>3"));
   if (hResult == NULL)
      return false;

   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      for(int i = 0; i < count; i++)
      {
         NXC_ALARM *alarm = new NXC_ALARM();
         alarm->alarmId = DBGetFieldULong(hResult, i, 0);
         alarm->sourceObject = DBGetFieldULong(hResult, i, 1);
         alarm->sourceEventCode = DBGetFieldULong(hResult, i, 2);
         alarm->sourceEventId = DBGetFieldUInt64(hResult, i, 3);
         DBGetField(hResult, i, 4, alarm->message, MAX_EVENT_MSG_LENGTH);
         alarm->originalSeverity = (BYTE)DBGetFieldLong(hResult, i, 5);
         alarm->currentSeverity = (BYTE)DBGetFieldLong(hResult, i, 6);
         DBGetField(hResult, i, 7, alarm->key, MAX_DB_STRING);
         alarm->creationTime = DBGetFieldULong(hResult, i, 8);
         alarm->lastChangeTime = DBGetFieldULong(hResult, i, 9);
         alarm->helpDeskState = (BYTE)DBGetFieldLong(hResult, i, 10);
         DBGetField(hResult, i, 11, alarm->helpDeskRef, MAX_HELPDESK_REF_LEN);
         alarm->ackByUser = DBGetFieldULong(hResult, i, 12);
         alarm->repeatCount = DBGetFieldULong(hResult, i, 13);
         alarm->state = (BYTE)DBGetFieldLong(hResult, i, 14);
         alarm->timeout = DBGetFieldULong(hResult, i, 15);
         alarm->timeoutEvent = DBGetFieldULong(hResult, i, 16);
			alarm->noteCount = GetNoteCount(hdb, alarm->alarmId);
         alarm->resolvedByUser = DBGetFieldULong(hResult, i, 17);
         alarm->ackTimeout = DBGetFieldULong(hResult, i, 18);
         alarm->dciId = DBGetFieldULong(hResult, i, 19);

         TCHAR query[256];
         _sntprintf(query, 256, _T("SELECT event_id FROM alarm_events WHERE alarm_id=%d"), (int)alarm->alarmId);
         DB_RESULT eventResult = DBSelect(hdb, query);
         if (eventResult == NULL)
         {
            DBFreeResult(hResult);
            return false;
         }
         int eventCount = DBGetNumRows(eventResult);
         for(int j = 0; j < eventCount; j++)
         {
            alarm->connectedEvents.add(DBGetFieldUInt64(eventResult, j, 0));
         }
         DBFreeResult(eventResult);
         m_alarmList->add(alarm);
      }
   }

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

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
