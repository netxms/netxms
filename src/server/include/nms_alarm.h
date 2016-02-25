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
** File: nms_alarm.h
**
**/

#ifndef _nms_alarm_h_
#define _nms_alarm_h_

/**
 * Alarm class
 */
class NXCORE_EXPORTABLE Alarm
{
private:
   UINT64 m_sourceEventId;   // Originating event ID
   UINT32 m_alarmId;         // Unique alarm ID
   time_t m_creationTime;    // Alarm creation time in UNIX time format
   time_t m_lastChangeTime;  // Alarm's last change time in UNIX time format
   UINT32 m_sourceObject;    // Source object ID
   UINT32 m_sourceEventCode; // Originating event code
   UINT32 m_dciId;           // related DCI ID
   BYTE m_currentSeverity;   // Alarm's current severity
   BYTE m_originalSeverity;  // Alarm's original severity
   BYTE m_state;             // Current state
   BYTE m_helpDeskState;     // State of alarm in helpdesk system
   UINT32 m_ackByUser;       // ID of user who was acknowledged this alarm (0 for system)
   UINT32 m_resolvedByUser;  // ID of user who was resolved this alarm (0 for system)
   UINT32 m_termByUser;      // ID of user who was terminated this alarm (0 for system)
   time_t m_ackTimeout;      // Sticky acknowledgment end time. If acknowladgmant without timeout put 0
   UINT32 m_repeatCount;
   UINT32 m_timeout;
   UINT32 m_timeoutEvent;
   TCHAR m_message[MAX_EVENT_MSG_LENGTH];
   TCHAR m_key[MAX_DB_STRING];
   TCHAR m_helpDeskRef[MAX_HELPDESK_REF_LEN];
   UINT32 m_commentCount;     // Number of comments added to alarm
   IntegerArray<UINT64> *m_relatedEvents;

public:
   Alarm(Event *event, const TCHAR *message, const TCHAR *key, int state, int severity, UINT32 timeout, UINT32 timeoutEvent, UINT32 ackTimeout);
   Alarm(DB_HANDLE hdb, DB_RESULT hResult, int row);
   Alarm(const Alarm *src, bool copyEvents);
   ~Alarm();

   UINT64 getSourceEventId() const { return m_sourceEventId; }
   UINT32 getAlarmId() const { return m_alarmId; }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getLastChangeTime() const { return m_lastChangeTime; }
   UINT32 getSourceObject() const { return m_sourceObject; }
   UINT32 getSourceEventCode() const { return m_sourceEventCode; }
   UINT32 getDciId() const { return m_dciId; }
   BYTE getCurrentSeverity() const { return m_currentSeverity; }
   BYTE getOriginalSeverity() const { return m_originalSeverity; }
   BYTE getState() const { return m_state; }
   BYTE getHelpDeskState() const { return m_helpDeskState; }
   UINT32 getAckByUser() const { return m_ackByUser; }
   UINT32 getResolvedByUser() const { return m_resolvedByUser; }
   UINT32 getTermByUser() const { return m_termByUser; }
   time_t getAckTimeout() const { return m_ackTimeout; }
   UINT32 getRepeatCount() const { return m_repeatCount; }
   UINT32 getTimeout() const { return m_timeout; }
   UINT32 getTimeoutEvent() const { return m_timeoutEvent; }
   const TCHAR *getMessage() const { return m_message; }
   const TCHAR *getKey() const { return m_key; }
   const TCHAR *getHelpDeskRef() const { return m_helpDeskRef; }
   UINT32 getCommentCount() const { return m_commentCount; }

   void fillMessage(NXCPMessage *msg);

   void createInDatabase();
   void updateInDatabase();

   void clearTimeout() { m_timeout = 0; }
   void onAckTimeoutExpiration() { m_ackTimeout = 0; m_state = ALARM_STATE_OUTSTANDING; }

   void addRelatedEvent(UINT64 eventId) { if (m_relatedEvents != NULL) m_relatedEvents->add(eventId); }
   bool isEventRelated(UINT64 eventId) const { return (m_relatedEvents != NULL) && m_relatedEvents->contains(eventId); }

   void updateFromEvent(Event *event, int state, int severity, UINT32 timeout, UINT32 timeoutEvent, UINT32 ackTimeout, const TCHAR *message);
   UINT32 acknowledge(ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
   void resolve(UINT32 userId, Event *event, bool terminate);
   UINT32 openHelpdeskIssue(ClientSession *session, TCHAR *hdref);
   void unlinkFromHelpdesk() { m_helpDeskState = ALARM_HELPDESK_IGNORED; m_helpDeskRef[0] = 0; }
   UINT32 updateAlarmComment(UINT32 commentId, const TCHAR *text, UINT32 userId, bool syncWithHelpdesk);
   UINT32 deleteComment(UINT32 commentId);
};

/**
 * Functions
 */
bool InitAlarmManager();
void ShutdownAlarmManager();

void SendAlarmsToClient(UINT32 dwRqId, ClientSession *pSession);
void DeleteAlarmNotes(DB_HANDLE hdb, UINT32 alarmId);
void DeleteAlarmEvents(DB_HANDLE hdb, UINT32 alarmId);

UINT32 NXCORE_EXPORTABLE GetAlarm(UINT32 dwAlarmId, NXCPMessage *msg);
ObjectArray<Alarm> NXCORE_EXPORTABLE *GetAlarms(UINT32 objectId);
UINT32 NXCORE_EXPORTABLE GetAlarmEvents(UINT32 dwAlarmId, NXCPMessage *msg);
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(UINT32 dwAlarmId);
NetObj NXCORE_EXPORTABLE *GetAlarmSourceObject(const TCHAR *hdref);  
int GetMostCriticalStatusForObject(UINT32 dwObjectId);
void GetAlarmStats(NXCPMessage *pMsg);

void NXCORE_EXPORTABLE CreateNewAlarm(TCHAR *message, TCHAR *key, int state, int severity, UINT32 timeout,
									           UINT32 timeoutEvent, Event *event, UINT32 ackTimeout);
UINT32 NXCORE_EXPORTABLE AckAlarmById(UINT32 dwAlarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
UINT32 NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
UINT32 NXCORE_EXPORTABLE ResolveAlarmById(UINT32 dwAlarmId, ClientSession *session, bool terminate);
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent);
UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate);
void NXCORE_EXPORTABLE DeleteAlarm(UINT32 dwAlarmId, bool objectCleanup);

UINT32 AddAlarmComment(const TCHAR *hdref, const TCHAR *text, UINT32 userId);
UINT32 UpdateAlarmComment(UINT32 alarmId, UINT32 noteId, const TCHAR *text, UINT32 userId);
UINT32 DeleteAlarmCommentByID(UINT32 alarmId, UINT32 noteId);
UINT32 GetAlarmComments(UINT32 alarmId, NXCPMessage *msg);

bool DeleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb);

UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref);
UINT32 NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref);
void LoadHelpDeskLink();
UINT32 CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref);
UINT32 OpenHelpdeskIssue(UINT32 alarmId, ClientSession *session, TCHAR *hdref);
UINT32 AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text);
UINT32 GetHelpdeskIssueUrlFromAlarm(UINT32 alarmId, TCHAR *url, size_t size);
UINT32 GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);
UINT32 UnlinkHelpdeskIssueById(UINT32 dwAlarmId, ClientSession *session);
UINT32 UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, ClientSession *session);


#endif
