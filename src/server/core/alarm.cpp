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
AlarmManager NXCORE_EXPORTABLE g_alarmMgr;

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
		(g_nDBSyntax == DB_SYNTAX_ORACLE) ?
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
	switch(g_nDBSyntax)
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
 * Resolve alarm by helpdesk reference
 */
UINT32 ResolveAlarmByHDRef(const TCHAR *hdref)
{
   return g_alarmMgr.resolveByHDRef(hdref, NULL, false);
}

/**
 * Terminate alarm by helpdesk reference
 */
UINT32 TerminateAlarmByHDRef(const TCHAR *hdref)
{
   return g_alarmMgr.resolveByHDRef(hdref, NULL, true);
}

/**
 * Alarm manager constructor
 */
AlarmManager::AlarmManager()
{
   m_numAlarms = 0;
   m_pAlarmList = NULL;
   m_mutex = MutexCreate();
	m_condShutdown = ConditionCreate(FALSE);
	m_hWatchdogThread = INVALID_THREAD_HANDLE;
}

/**
 * Alarm manager destructor
 */
AlarmManager::~AlarmManager()
{
   safe_free(m_pAlarmList);
   MutexDestroy(m_mutex);
	ConditionSet(m_condShutdown);
	ThreadJoin(m_hWatchdogThread);
}

/**
 * Watchdog thread starter
 */
static THREAD_RESULT THREAD_CALL WatchdogThreadStarter(void *pArg)
{
	((AlarmManager *)pArg)->watchdogThread();
	return THREAD_OK;
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
 * Initialize alarm manager at system startup
 */
BOOL AlarmManager::init()
{
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
      return FALSE;

   m_numAlarms = DBGetNumRows(hResult);
   if (m_numAlarms > 0)
   {
      m_pAlarmList = (NXC_ALARM *)malloc(sizeof(NXC_ALARM) * m_numAlarms);
      memset(m_pAlarmList, 0, sizeof(NXC_ALARM) * m_numAlarms);
      for(int i = 0; i < m_numAlarms; i++)
      {
         m_pAlarmList[i].dwAlarmId = DBGetFieldULong(hResult, i, 0);
         m_pAlarmList[i].dwSourceObject = DBGetFieldULong(hResult, i, 1);
         m_pAlarmList[i].dwSourceEventCode = DBGetFieldULong(hResult, i, 2);
         m_pAlarmList[i].qwSourceEventId = DBGetFieldUInt64(hResult, i, 3);
         DBGetField(hResult, i, 4, m_pAlarmList[i].szMessage, MAX_EVENT_MSG_LENGTH);
         m_pAlarmList[i].nOriginalSeverity = (BYTE)DBGetFieldLong(hResult, i, 5);
         m_pAlarmList[i].nCurrentSeverity = (BYTE)DBGetFieldLong(hResult, i, 6);
         DBGetField(hResult, i, 7, m_pAlarmList[i].szKey, MAX_DB_STRING);
         m_pAlarmList[i].dwCreationTime = DBGetFieldULong(hResult, i, 8);
         m_pAlarmList[i].dwLastChangeTime = DBGetFieldULong(hResult, i, 9);
         m_pAlarmList[i].nHelpDeskState = (BYTE)DBGetFieldLong(hResult, i, 10);
         DBGetField(hResult, i, 11, m_pAlarmList[i].szHelpDeskRef, MAX_HELPDESK_REF_LEN);
         m_pAlarmList[i].dwAckByUser = DBGetFieldULong(hResult, i, 12);
         m_pAlarmList[i].dwRepeatCount = DBGetFieldULong(hResult, i, 13);
         m_pAlarmList[i].nState = (BYTE)DBGetFieldLong(hResult, i, 14);
         m_pAlarmList[i].dwTimeout = DBGetFieldULong(hResult, i, 15);
         m_pAlarmList[i].dwTimeoutEvent = DBGetFieldULong(hResult, i, 16);
			m_pAlarmList[i].noteCount = GetNoteCount(g_hCoreDB, m_pAlarmList[i].dwAlarmId);
         m_pAlarmList[i].dwResolvedByUser = DBGetFieldULong(hResult, i, 17);
         m_pAlarmList[i].ackTimeout = DBGetFieldULong(hResult, i, 18);
      }
   }

   DBFreeResult(hResult);

	m_hWatchdogThread = ThreadCreateEx(WatchdogThreadStarter, 0, this);
   return TRUE;
}

/**
 * Create new alarm
 */
void AlarmManager::newAlarm(TCHAR *pszMsg, TCHAR *pszKey, int nState,
                            int iSeverity, UINT32 dwTimeout,
									 UINT32 dwTimeoutEvent, Event *pEvent, UINT32 ackTimeout)
{
   NXC_ALARM alarm;
   TCHAR *pszExpMsg, *pszExpKey, szQuery[2048];
   BOOL bNewAlarm = TRUE;

   // Expand alarm's message and key
   pszExpMsg = pEvent->expandText(pszMsg);
   pszExpKey = pEvent->expandText(pszKey);

   // Check if we have a duplicate alarm
   if (((nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED) && (*pszExpKey != 0))
   {
      lock();

      for(int i = 0; i < m_numAlarms; i++)
			if (!_tcscmp(pszExpKey, m_pAlarmList[i].szKey))
         {
            m_pAlarmList[i].dwRepeatCount++;
            m_pAlarmList[i].dwLastChangeTime = (UINT32)time(NULL);
            m_pAlarmList[i].dwSourceObject = pEvent->getSourceId();
				if ((m_pAlarmList[i].nState & ALARM_STATE_STICKY) == 0)
					m_pAlarmList[i].nState = nState;
            m_pAlarmList[i].nCurrentSeverity = iSeverity;
				m_pAlarmList[i].dwTimeout = dwTimeout;
				m_pAlarmList[i].dwTimeoutEvent = dwTimeoutEvent;
				m_pAlarmList[i].ackTimeout = ackTimeout;
            nx_strncpy(m_pAlarmList[i].szMessage, pszExpMsg, MAX_EVENT_MSG_LENGTH);

            notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
            updateAlarmInDB(&m_pAlarmList[i]);

				alarm.dwAlarmId = m_pAlarmList[i].dwAlarmId;		// needed for correct update of related events

            bNewAlarm = FALSE;
            break;
         }

      unlock();
   }

   if (bNewAlarm)
   {
      // Create new alarm structure
      memset(&alarm, 0, sizeof(NXC_ALARM));
      alarm.dwAlarmId = CreateUniqueId(IDG_ALARM);
      alarm.qwSourceEventId = pEvent->getId();
      alarm.dwSourceEventCode = pEvent->getCode();
      alarm.dwSourceObject = pEvent->getSourceId();
      alarm.dwCreationTime = (UINT32)time(NULL);
      alarm.dwLastChangeTime = alarm.dwCreationTime;
      alarm.nState = nState;
      alarm.nOriginalSeverity = iSeverity;
      alarm.nCurrentSeverity = iSeverity;
      alarm.dwRepeatCount = 1;
      alarm.nHelpDeskState = ALARM_HELPDESK_IGNORED;
		alarm.dwTimeout = dwTimeout;
		alarm.dwTimeoutEvent = dwTimeoutEvent;
		alarm.noteCount = 0;
		alarm.ackTimeout = 0;
      nx_strncpy(alarm.szMessage, pszExpMsg, MAX_EVENT_MSG_LENGTH);
      nx_strncpy(alarm.szKey, pszExpKey, MAX_DB_STRING);

      // Add new alarm to active alarm list if needed
		if ((alarm.nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
      {
         lock();

			DbgPrintf(7, _T("AlarmManager: adding new active alarm, current alarm count %d"), (int)m_numAlarms);
         m_numAlarms++;
         m_pAlarmList = (NXC_ALARM *)realloc(m_pAlarmList, sizeof(NXC_ALARM) * m_numAlarms);
			memcpy(&m_pAlarmList[m_numAlarms - 1], &alarm, sizeof(NXC_ALARM));

         unlock();
      }

      // Save alarm to database
      _sntprintf(szQuery, 2048,
			        _T("INSERT INTO alarms (alarm_id,creation_time,last_change_time,")
                 _T("source_object_id,source_event_code,message,original_severity,")
                 _T("current_severity,alarm_key,alarm_state,ack_by,resolved_by,hd_state,")
                 _T("hd_ref,repeat_count,term_by,timeout,timeout_event,source_event_id, ack_timeout) VALUES ")
                 _T("(%d,%d,%d,%d,%d,%s,%d,%d,%s,%d,%d,%d,%d,%s,%d,%d,%d,%d,") UINT64_FMT _T(",%d)"),
              alarm.dwAlarmId, alarm.dwCreationTime, alarm.dwLastChangeTime,
				  alarm.dwSourceObject, alarm.dwSourceEventCode,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm.szMessage),
              alarm.nOriginalSeverity, alarm.nCurrentSeverity,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm.szKey),
				  alarm.nState, alarm.dwAckByUser, alarm.dwResolvedByUser, alarm.nHelpDeskState,
				  (const TCHAR *)DBPrepareString(g_hCoreDB, alarm.szHelpDeskRef),
              alarm.dwRepeatCount, alarm.dwTermByUser, alarm.dwTimeout,
				  alarm.dwTimeoutEvent, alarm.qwSourceEventId, alarm.ackTimeout);
      QueueSQLRequest(szQuery);

      // Notify connected clients about new alarm
      notifyClients(NX_NOTIFY_NEW_ALARM, &alarm);
   }

   // Update status of related object if needed
   if ((nState & ALARM_STATE_MASK) != ALARM_STATE_TERMINATED)
		updateObjectStatus(pEvent->getSourceId());

	// Add record to alarm_events table
	TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
	const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, pEvent->getName(), valSeverity, valSource, valTimestamp, pEvent->getMessage() };
	_sntprintf(valAlarmId, 16, _T("%d"), (int)alarm.dwAlarmId);
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
UINT32 AlarmManager::doAck(NXC_ALARM *alarm, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   if ((alarm->nState & ALARM_STATE_MASK) != ALARM_STATE_OUTSTANDING)
      return RCC_ALARM_NOT_OUTSTANDING;

   WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), alarm->dwSourceObject,
      _T("Acknowledged alarm %d (%s) on object %s"), alarm->dwAlarmId, alarm->szMessage,
      GetObjectName(alarm->dwSourceObject, _T("")));

   UINT32 endTime = acknowledgmentActionTime != 0 ? (UINT32)time(NULL) + acknowledgmentActionTime : 0;
   alarm->ackTimeout = endTime;
   alarm->nState = ALARM_STATE_ACKNOWLEDGED;
	if (sticky)
      alarm->nState |= ALARM_STATE_STICKY;
   alarm->dwAckByUser = session->getUserId();
   alarm->dwLastChangeTime = (UINT32)time(NULL);
   notifyClients(NX_NOTIFY_ALARM_CHANGED, alarm);
   updateAlarmInDB(alarm);
   return RCC_SUCCESS;
}

/**
 * Acknowledge alarm with given ID
 */
UINT32 AlarmManager::ackById(UINT32 dwAlarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwRet = doAck(&m_pAlarmList[i], session, sticky, acknowledgmentActionTime);
         dwObject = m_pAlarmList[i].dwSourceObject;
         break;
      }
   unlock();

   if (dwRet == RCC_SUCCESS)
      updateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Acknowledge alarm with given helpdesk reference
 */
UINT32 AlarmManager::ackByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (!_tcscmp(m_pAlarmList[i].szHelpDeskRef, hdref))
      {
         dwRet = doAck(&m_pAlarmList[i], session, sticky, acknowledgmentActionTime);
         dwObject = m_pAlarmList[i].dwSourceObject;
         break;
      }
   unlock();

   if (dwRet == RCC_SUCCESS)
      updateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Resolve and possibly terminate alarm with given ID
 * Should return RCC which can be sent to client
 */
UINT32 AlarmManager::resolveById(UINT32 dwAlarmId, ClientSession *session, bool terminate)
{
   UINT32 dwObject, dwRet = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         // If alarm is open in helpdesk, it cannot be terminated
         if (m_pAlarmList[i].nHelpDeskState != ALARM_HELPDESK_OPEN)
         {
            dwObject = m_pAlarmList[i].dwSourceObject;
            if (session != NULL)
            {
               WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), dwObject,
                  _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
                  dwAlarmId, m_pAlarmList[i].szMessage, GetObjectName(dwObject, _T("")));
            }

				if (terminate)
               m_pAlarmList[i].dwTermByUser = (session != NULL) ? session->getUserId() : 0;
				else
               m_pAlarmList[i].dwResolvedByUser = (session != NULL) ? session->getUserId() : 0;
            m_pAlarmList[i].dwLastChangeTime = (UINT32)time(NULL);
				m_pAlarmList[i].nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
				m_pAlarmList[i].ackTimeout = 0;
				notifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
            updateAlarmInDB(&m_pAlarmList[i]);
				if (terminate)
				{
					m_numAlarms--;
					memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_numAlarms - i));
				}
            dwRet = RCC_SUCCESS;
         }
         else
         {
            dwRet = RCC_ALARM_OPEN_IN_HELPDESK;
         }
         break;
      }
   unlock();

   if (dwRet == RCC_SUCCESS)
      updateObjectStatus(dwObject);
   return dwRet;
}

/**
 * Resolve and possibly terminate all alarms with given key
 */
void AlarmManager::resolveByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent)
{
   UINT32 *pdwObjectList = (UINT32 *)malloc(sizeof(UINT32) * m_numAlarms);

   lock();
   DWORD dwCurrTime = (UINT32)time(NULL);
   int numObjects = 0;
   for(int i = 0; i < m_numAlarms; i++)
		if ((useRegexp ? RegexpMatch(m_pAlarmList[i].szKey, pszKey, TRUE) : !_tcscmp(pszKey, m_pAlarmList[i].szKey)) &&
         (m_pAlarmList[i].nHelpDeskState != ALARM_HELPDESK_OPEN))
      {
         // Add alarm's source object to update list
         int j;
         for(j = 0; j < numObjects; j++)
         {
            if (pdwObjectList[j] == m_pAlarmList[i].dwSourceObject)
               break;
         }
         if (j == numObjects)
         {
            pdwObjectList[numObjects++] = m_pAlarmList[i].dwSourceObject;
         }

         // Resolve or terminate alarm
			m_pAlarmList[i].nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
         m_pAlarmList[i].dwLastChangeTime = dwCurrTime;
			if (terminate)
				m_pAlarmList[i].dwTermByUser = 0;
			else
				m_pAlarmList[i].dwResolvedByUser = 0;
         m_pAlarmList[i].ackTimeout = 0;
			notifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
         updateAlarmInDB(&m_pAlarmList[i]);
			if (terminate)
			{
				m_numAlarms--;
				memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_numAlarms - i));
				i--;
			}
         else
         {
	         // Add record to alarm_events table if alarm is resolved
	         TCHAR valAlarmId[16], valEventId[32], valEventCode[16], valSeverity[16], valSource[16], valTimestamp[16];
	         const TCHAR *values[8] = { valAlarmId, valEventId, valEventCode, pEvent->getName(), valSeverity, valSource, valTimestamp, pEvent->getMessage() };
	         _sntprintf(valAlarmId, 16, _T("%d"), (int)m_pAlarmList[i].dwAlarmId);
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
   unlock();

   // Update status of objects
   for(int i = 0; i < numObjects; i++)
      updateObjectStatus(pdwObjectList[i]);
   free(pdwObjectList);
}

/**
 * Resolve and possibly terminate alarm with given helpdesk reference.
 * Auitomatically change alarm's helpdesk state to "closed"
 */
UINT32 AlarmManager::resolveByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate)
{
   UINT32 objectId = 0;
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (!_tcscmp(m_pAlarmList[i].szHelpDeskRef, hdref))
      {
         objectId = m_pAlarmList[i].dwSourceObject;
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), objectId,
               _T("%s alarm %d (%s) on object %s"), terminate ? _T("Terminated") : _T("Resolved"),
               m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szMessage, GetObjectName(objectId, _T("")));
         }

			if (terminate)
            m_pAlarmList[i].dwTermByUser = (session != NULL) ? session->getUserId() : 0;
			else
            m_pAlarmList[i].dwResolvedByUser = (session != NULL) ? session->getUserId() : 0;
         m_pAlarmList[i].dwLastChangeTime = (UINT32)time(NULL);
			m_pAlarmList[i].nState = terminate ? ALARM_STATE_TERMINATED : ALARM_STATE_RESOLVED;
			m_pAlarmList[i].ackTimeout = 0;
         if (m_pAlarmList[i].nHelpDeskState != ALARM_HELPDESK_IGNORED)
            m_pAlarmList[i].nHelpDeskState = ALARM_HELPDESK_CLOSED;
			notifyClients(terminate ? NX_NOTIFY_ALARM_TERMINATED : NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
         updateAlarmInDB(&m_pAlarmList[i]);
			if (terminate)
			{
				m_numAlarms--;
				memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_numAlarms - i));
			}
         DbgPrintf(5, _T("Alarm with helpdesk reference \"%s\" %s"), hdref, terminate ? _T("terminated") : _T("resolved"));
         rcc = RCC_SUCCESS;
         break;
      }
   unlock();

   if (objectId != 0)
      updateObjectStatus(objectId);
   return rcc;
}

/**
 * Open issue in helpdesk system
 */
UINT32 AlarmManager::openHelpdeskIssue(UINT32 alarmId, ClientSession *session, TCHAR *hdref)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;
   *hdref = 0;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == alarmId)
      {
         if (m_pAlarmList[i].nHelpDeskState == ALARM_HELPDESK_IGNORED)
         {
            /* TODO: unlock alarm list before call */
            const TCHAR *nodeName = GetObjectName(m_pAlarmList[i].dwSourceObject, _T("[unknown]"));
            int messageLen = (_tcslen(nodeName) + _tcslen(m_pAlarmList[i].szMessage) + 32) * sizeof(TCHAR);
            TCHAR *message = (TCHAR *)malloc(messageLen);
            _sntprintf(message, messageLen, _T("%s: %s"), nodeName, m_pAlarmList[i].szMessage);
            rcc = CreateHelpdeskIssue(message, m_pAlarmList[i].szHelpDeskRef);
            free(message);
            if (rcc == RCC_SUCCESS)
            {
               m_pAlarmList[i].nHelpDeskState = ALARM_HELPDESK_OPEN;
			      notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
               updateAlarmInDB(&m_pAlarmList[i]);
               nx_strncpy(hdref, m_pAlarmList[i].szHelpDeskRef, MAX_HELPDESK_REF_LEN);
               DbgPrintf(5, _T("Helpdesk issue created for alarm %d, reference \"%s\""), m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szHelpDeskRef);
            }
         }
         else
         {
            rcc = RCC_OUT_OF_STATE_REQUEST;
         }
         break;
      }
   unlock();
   return rcc;
}

/**
 * Get helpdesk issue URL for given alarm
 */
UINT32 AlarmManager::getHelpdeskIssueUrl(UINT32 alarmId, TCHAR *url, size_t size)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == alarmId)
      {
         if ((m_pAlarmList[i].nHelpDeskState != ALARM_HELPDESK_IGNORED) && (m_pAlarmList[i].szHelpDeskRef[0] != 0))
         {
            rcc = GetHelpdeskIssueUrl(m_pAlarmList[i].szHelpDeskRef, url, size);
         }
         else
         {
            rcc = RCC_OUT_OF_STATE_REQUEST;
         }
         break;
      }
   unlock();
   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
UINT32 AlarmManager::unlinkIssueById(UINT32 dwAlarmId, ClientSession *session)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), m_pAlarmList[i].dwSourceObject,
               _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"), m_pAlarmList[i].szHelpDeskRef,
               m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szMessage, 
               GetObjectName(m_pAlarmList[i].dwSourceObject, _T("")));
         }
         m_pAlarmList[i].nHelpDeskState = ALARM_HELPDESK_IGNORED;
			notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
         updateAlarmInDB(&m_pAlarmList[i]);
         rcc = RCC_SUCCESS;
         break;
      }
   unlock();

   return rcc;
}

/**
 * Unlink helpdesk issue from alarm
 */
UINT32 AlarmManager::unlinkIssueByHDRef(const TCHAR *hdref, ClientSession *session)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (!_tcscmp(m_pAlarmList[i].szHelpDeskRef, hdref))
      {
         if (session != NULL)
         {
            WriteAuditLog(AUDIT_OBJECTS, TRUE, session->getUserId(), session->getWorkstation(), m_pAlarmList[i].dwSourceObject,
               _T("Helpdesk issue %s unlinked from alarm %d (%s) on object %s"), m_pAlarmList[i].szHelpDeskRef,
               m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szMessage, 
               GetObjectName(m_pAlarmList[i].dwSourceObject, _T("")));
         }
         m_pAlarmList[i].nHelpDeskState = ALARM_HELPDESK_IGNORED;
			notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
         updateAlarmInDB(&m_pAlarmList[i]);
         rcc = RCC_SUCCESS;
         break;
      }
   unlock();

   return rcc;
}

/**
 * Delete alarm with given ID
 */
void AlarmManager::deleteAlarm(UINT32 dwAlarmId, bool objectCleanup)
{
   DWORD dwObject;

   // Delete alarm from in-memory list
   if (!objectCleanup)  // otherwise already locked
      lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObject = m_pAlarmList[i].dwSourceObject;
         notifyClients(NX_NOTIFY_ALARM_DELETED, &m_pAlarmList[i]);
         m_numAlarms--;
         memmove(&m_pAlarmList[i], &m_pAlarmList[i + 1], sizeof(NXC_ALARM) * (m_numAlarms - i));
         break;
      }
   if (!objectCleanup)
      unlock();

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

      updateObjectStatus(dwObject);
   }
}


/**
 * Delete all alarms of given object. Intended to be called only
 * on final stage of object deletion.
 */
bool AlarmManager::deleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb)
{
	lock();

	// go through from end because m_numAlarms is decremented by deleteAlarm()
	for(int i = m_numAlarms - 1; i >= 0; i--)
   {
		if (m_pAlarmList[i].dwSourceObject == objectId)
      {
			deleteAlarm(m_pAlarmList[i].dwAlarmId, true);
      }
	}

	unlock();

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
 * Update alarm information in database
 */
void AlarmManager::updateAlarmInDB(NXC_ALARM *pAlarm)
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
 * Callback for client session enumeration
 */
void AlarmManager::sendAlarmNotification(ClientSession *pSession, void *pArg)
{
   pSession->onAlarmUpdate(((AlarmManager *)pArg)->m_dwNotifyCode,
                           ((AlarmManager *)pArg)->m_pNotifyAlarmInfo);
}

/**
 * Notify connected clients about changes
 */
void AlarmManager::notifyClients(UINT32 dwCode, NXC_ALARM *pAlarm)
{
	// Notify modules
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfAlarmChangeHook != NULL)
			g_pModuleList[i].pfAlarmChangeHook(dwCode, pAlarm);
	}

   m_dwNotifyCode = dwCode;
   m_pNotifyAlarmInfo = pAlarm;
   EnumerateClientSessions(sendAlarmNotification, this);
}

/**
 * Send all alarms to client
 */
void AlarmManager::sendAlarmsToClient(UINT32 dwRqId, ClientSession *pSession)
{
   DWORD dwUserId = pSession->getUserId();

   // Prepare message
   CSCPMessage msg;
   msg.SetCode(CMD_ALARM_DATA);
   msg.SetId(dwRqId);

   lock();
   for(int i = 0; i < m_numAlarms; i++)
   {
      NetObj *pObject = FindObjectById(m_pAlarmList[i].dwSourceObject);
      if (pObject != NULL)
      {
         if (pObject->checkAccessRights(dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            FillAlarmInfoMessage(&msg, &m_pAlarmList[i]);
            pSession->sendMessage(&msg);
            msg.deleteAllVariables();
         }
      }
   }
   unlock();

   // Send end-of-list indicator
   msg.SetVariable(VID_ALARM_ID, (UINT32)0);
   pSession->sendMessage(&msg);
}

/**
 * Get alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
UINT32 AlarmManager::getAlarm(UINT32 dwAlarmId, CSCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
			FillAlarmInfoMessage(msg, &m_pAlarmList[i]);
			dwRet = RCC_SUCCESS;
         break;
      }
   unlock();

	return dwRet;
}

/**
 * Get all related events for alarm with given ID into NXCP message
 * Should return RCC that can be sent to client
 */
UINT32 AlarmManager::getAlarmEvents(UINT32 dwAlarmId, CSCPMessage *msg)
{
   UINT32 dwRet = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
			dwRet = RCC_SUCCESS;
         break;
      }
   unlock();

	// we don't call FillAlarmEventsMessage from within loop
	// to prevent alarm list lock for a long time
	if (dwRet == RCC_SUCCESS)
		FillAlarmEventsMessage(msg, dwAlarmId);

	return dwRet;
}

/**
 * Get source object for given alarm id
 */
NetObj *AlarmManager::getAlarmSourceObject(UINT32 dwAlarmId)
{
   UINT32 dwObjectId = 0;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == dwAlarmId)
      {
         dwObjectId = m_pAlarmList[i].dwSourceObject;
         break;
      }
   unlock();
   return (dwObjectId != 0) ? FindObjectById(dwObjectId) : NULL;
}

/**
 * Get source object for given alarm helpdesk reference
 */
NetObj *AlarmManager::getAlarmSourceObject(const TCHAR *hdref)
{
   UINT32 dwObjectId = 0;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (!_tcscmp(m_pAlarmList[i].szHelpDeskRef, hdref))
      {
         dwObjectId = m_pAlarmList[i].dwSourceObject;
         break;
      }
   unlock();
   return (dwObjectId != 0) ? FindObjectById(dwObjectId) : NULL;
}

/**
 * Get most critical status among active alarms for given object
 * Will return STATUS_UNKNOWN if there are no active alarms
 */
int AlarmManager::getMostCriticalStatusForObject(UINT32 dwObjectId)
{
   int iStatus = STATUS_UNKNOWN;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
   {
      if ((m_pAlarmList[i].dwSourceObject == dwObjectId) &&
			 ((m_pAlarmList[i].nState & ALARM_STATE_MASK) < ALARM_STATE_RESOLVED) &&
          ((m_pAlarmList[i].nCurrentSeverity > iStatus) || (iStatus == STATUS_UNKNOWN)))
      {
         iStatus = (int)m_pAlarmList[i].nCurrentSeverity;
      }
   }
   unlock();
   return iStatus;
}

/**
 * Update object status after alarm acknowledgement or deletion
 */
void AlarmManager::updateObjectStatus(UINT32 dwObjectId)
{
   NetObj *pObject;

   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
      pObject->calculateCompoundStatus();
}

/**
 * Fill message with alarm stats
 */
void AlarmManager::getAlarmStats(CSCPMessage *pMsg)
{
   UINT32 dwCount[5];

   lock();
   pMsg->SetVariable(VID_NUM_ALARMS, m_numAlarms);
   memset(dwCount, 0, sizeof(UINT32) * 5);
   for(int i = 0; i < m_numAlarms; i++)
      dwCount[m_pAlarmList[i].nCurrentSeverity]++;
   unlock();
   pMsg->setFieldInt32Array(VID_ALARMS_BY_SEVERITY, 5, dwCount);
}

/**
 * Watchdog thread
 */
void AlarmManager::watchdogThread()
{
	while(1)
	{
		if (ConditionWait(m_condShutdown, 1000))
			break;

		lock();
		time_t now = time(NULL);
	   for(int i = 0; i < m_numAlarms; i++)
		{
			if ((m_pAlarmList[i].dwTimeout > 0) &&
				 ((m_pAlarmList[i].nState & ALARM_STATE_MASK) == ALARM_STATE_OUTSTANDING) &&
				 (((time_t)m_pAlarmList[i].dwLastChangeTime + (time_t)m_pAlarmList[i].dwTimeout) < now))
			{
				DbgPrintf(5, _T("Alarm timeout: alarm_id=%u, last_change=%u, timeout=%u, now=%u"),
				          m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].dwLastChangeTime,
							 m_pAlarmList[i].dwTimeout, (UINT32)now);

				PostEvent(m_pAlarmList[i].dwTimeoutEvent, m_pAlarmList[i].dwSourceObject, "dssd",
				          m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szMessage,
							 m_pAlarmList[i].szKey, m_pAlarmList[i].dwSourceEventCode);
				m_pAlarmList[i].dwTimeout = 0;	// Disable repeated timeout events
				updateAlarmInDB(&m_pAlarmList[i]);
			}

			if ((m_pAlarmList[i].ackTimeout != 0) &&
				 ((m_pAlarmList[i].nState & ALARM_STATE_STICKY) != 0) &&
				 (((time_t)m_pAlarmList[i].ackTimeout <= now)))
			{
				DbgPrintf(5, _T("Alarm aknowledgment timeout: alarm_id=%u, timeout=%u, now=%u"),
				          m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].ackTimeout, (UINT32)now);

				PostEvent(m_pAlarmList[i].dwTimeoutEvent, m_pAlarmList[i].dwSourceObject, "dssd",
				          m_pAlarmList[i].dwAlarmId, m_pAlarmList[i].szMessage,
							 m_pAlarmList[i].szKey, m_pAlarmList[i].dwSourceEventCode);
				m_pAlarmList[i].ackTimeout = 0;	// Disable repeated timeout events
				m_pAlarmList[i].nState = ALARM_STATE_OUTSTANDING;
				updateAlarmInDB(&m_pAlarmList[i]);
				notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
			}
		}
		unlock();
	}
}

/**
 * Check if givel alram/note id pair is valid
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
UINT32 AlarmManager::updateAlarmComment(UINT32 alarmId, UINT32 noteId, const TCHAR *text, UINT32 userId)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;
   bool newNote = false;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == alarmId)
      {
			if (noteId != 0)
			{
				if (IsValidNoteId(alarmId, noteId))
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
					DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, alarmId);
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
               m_pAlarmList[i].noteCount++;
				notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
            if (m_pAlarmList[i].nHelpDeskState == ALARM_HELPDESK_OPEN)
            {
               AddHelpdeskIssueComment(m_pAlarmList[i].szHelpDeskRef, text);
            }
			}
         break;
      }
   unlock();

   return rcc;
}

/**
 * Delete comment
 */
UINT32 AlarmManager::deleteAlarmCommentByID(UINT32 alarmId, UINT32 noteId)
{
   UINT32 rcc = RCC_INVALID_ALARM_ID;

   lock();
   for(int i = 0; i < m_numAlarms; i++)
      if (m_pAlarmList[i].dwAlarmId == alarmId)
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
				m_pAlarmList[i].noteCount--;
				notifyClients(NX_NOTIFY_ALARM_CHANGED, &m_pAlarmList[i]);
			}
         break;
      }
   unlock();

   return rcc;
}

/**
 * Get alarm's comments
 */
UINT32 AlarmManager::getAlarmComments(UINT32 alarmId, CSCPMessage *msg)
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
ObjectArray<NXC_ALARM> *AlarmManager::getAlarms(UINT32 objectId)
{
   ObjectArray<NXC_ALARM> *result = new ObjectArray<NXC_ALARM>(16, 16, true);

   lock();
   for(int i = 0; i < m_numAlarms; i++)
   {
      if ((objectId == 0) || (m_pAlarmList[i].dwSourceObject == objectId))
      {
         NXC_ALARM *a = new NXC_ALARM;
         memcpy(a, &m_pAlarmList[i], sizeof(NXC_ALARM));
         result->add(a);
      }
   }
   unlock();
   return result;
}
