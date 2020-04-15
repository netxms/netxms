/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
 * Scheduled task handler ID for alarm summary email
 */
#define ALARM_SUMMARY_EMAIL_TASK_ID _T("Alarms.SendSummaryEmail")

/**
 * Alarm class
 */
class NXCORE_EXPORTABLE Alarm
{
private:
   UINT64 m_sourceEventId;   // Originating event ID
   UINT32 m_alarmId;         // Unique alarm ID
   UINT32 m_parentAlarmId;   // Parent (root cause) alarm ID
   TCHAR *m_rcaScriptName;   // Name of root cause analysis script
   time_t m_creationTime;    // Alarm creation time in UNIX time format
   time_t m_lastChangeTime;  // Alarm's last change time in UNIX time format
   uuid m_rule;              // GUID of EPP rule that generates this alarm
   UINT32 m_sourceObject;    // Source object ID
   int32_t m_zoneUIN;        // Zone UIN for source object
   UINT32 m_sourceEventCode; // Originating event code
   TCHAR *m_eventTags;       // Tags of originating event
   UINT32 m_dciId;           // related DCI ID
   BYTE m_currentSeverity;   // Alarm's current severity
   BYTE m_originalSeverity;  // Alarm's original severity
   BYTE m_state;             // Current state
   BYTE m_helpDeskState;     // State of alarm in helpdesk system
   UINT32 m_ackByUser;       // ID of user who was acknowledged this alarm (0 for system)
   UINT32 m_resolvedByUser;  // ID of user who was resolved this alarm (0 for system)
   UINT32 m_termByUser;      // ID of user who was terminated this alarm (0 for system)
   time_t m_ackTimeout;      // Sticky acknowledgment end time, 0 if acknowledged without timeout
   UINT32 m_repeatCount;
   UINT32 m_timeout;
   UINT32 m_timeoutEvent;
   TCHAR m_message[MAX_EVENT_MSG_LENGTH];
   TCHAR m_key[MAX_DB_STRING];
   TCHAR m_helpDeskRef[MAX_HELPDESK_REF_LEN];
   TCHAR *m_impact;
   UINT32 m_commentCount;     // Number of comments added to alarm
   IntegerArray<UINT64> *m_relatedEvents;
   IntegerArray<UINT32> *m_alarmCategoryList;
   UINT32 m_notificationCode; // notification code used when sending client notifications
   IntegerArray<UINT32> *m_subordinateAlarms;

   StringBuffer categoryListToString();

public:
   Alarm(Event *event, UINT32 parentAlarmId, const TCHAR *rcaScriptName, const uuid& rule, const TCHAR *message, const TCHAR *key, const TCHAR *impact,
            int state, int severity, UINT32 timeout, UINT32 timeoutEvent, UINT32 ackTimeout, IntegerArray<UINT32> *alarmCategoryList);
   Alarm(DB_HANDLE hdb, DB_RESULT hResult, int row);
   Alarm(const Alarm *src, bool copyEvents, UINT32 notificationCode = 0);
   ~Alarm();

   UINT64 getSourceEventId() const { return m_sourceEventId; }
   UINT32 getAlarmId() const { return m_alarmId; }
   UINT32 getParentAlarmId() const { return m_parentAlarmId; }
   const TCHAR *getRcaScriptName() const { return CHECK_NULL_EX(m_rcaScriptName); }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getLastChangeTime() const { return m_lastChangeTime; }
   const uuid& getRule() const { return m_rule; }
   UINT32 getSourceObject() const { return m_sourceObject; }
   UINT32 getSourceEventCode() const { return m_sourceEventCode; }
   const TCHAR *getEventTags() const { return m_eventTags; }
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
   const TCHAR *getImpact() const { return CHECK_NULL_EX(m_impact); }
   UINT32 getCommentCount() const { return m_commentCount; }
   UINT32 getNotificationCode() const { return m_notificationCode; }
   UINT64 getMemoryUsage() const;

   void fillMessage(NXCPMessage *msg) const;

   void createInDatabase();
   void updateInDatabase();

   void clearTimeout() { m_timeout = 0; }
   void onAckTimeoutExpiration() { m_ackTimeout = 0; m_state = ALARM_STATE_OUTSTANDING; }

   void addRelatedEvent(UINT64 eventId) { if (m_relatedEvents != NULL) m_relatedEvents->add(eventId); }
   bool isEventRelated(UINT64 eventId) const { return (m_relatedEvents != NULL) && m_relatedEvents->contains(eventId); }

   void updateFromEvent(Event *event, UINT32 parentAlarmId, const TCHAR *rcaScriptName, int state, int severity, UINT32 timeout,
            UINT32 timeoutEvent, UINT32 ackTimeout, const TCHAR *message, const TCHAR *impact, IntegerArray<UINT32> *alarmCategoryList);
   void updateParentAlarm(UINT32 parentAlarmId);
   UINT32 acknowledge(ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime, bool includeSubordinates);
   void resolve(UINT32 userId, Event *event, bool terminate, bool notify, bool includeSubordinates);
   UINT32 openHelpdeskIssue(TCHAR *hdref);
   void unlinkFromHelpdesk() { m_helpDeskState = ALARM_HELPDESK_IGNORED; m_helpDeskRef[0] = 0; }
   UINT32 updateAlarmComment(UINT32 *commentId, const TCHAR *text, UINT32 userId, bool syncWithHelpdesk);
   UINT32 deleteComment(UINT32 commentId);

   void addSubordinateAlarm(UINT32 alarmId);
   void removeSubordinateAlarm(UINT32 alarmId);

   bool checkCategoryAccess(ClientSession *session) const;
};

/**
 * Alarm category
 */
class AlarmCategory
{
private:
   UINT32 m_id;
   TCHAR *m_name;
   TCHAR *m_description;
   IntegerArray<UINT32> m_acl;

public:
   AlarmCategory(UINT32 id);
   AlarmCategory(DB_RESULT hResult, int row, IntegerArray<UINT32> *aclCache);
   AlarmCategory(UINT32 id, const TCHAR *name, const TCHAR *description);
   AlarmCategory(AlarmCategory* obj);
   ~AlarmCategory();

   UINT32 getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }

   bool checkAccess(UINT32 userId);

   void fillMessage(NXCPMessage *msg, UINT32 baseId) const;
   void modifyFromMessage(const NXCPMessage *msg);
   void updateDescription(const TCHAR *description) { MemFree(m_description); m_description = MemCopyString(description); }
   bool saveToDatabase() const;
};

/**
 * Alarm comments
 */
class AlarmComment
{
private:
   UINT32 m_id;
   time_t m_changeTime;
   UINT32 m_userId;
   TCHAR *m_text;

public:
   AlarmComment(UINT32 id, time_t changeTime, UINT32 userId, TCHAR *text);
   ~AlarmComment() { MemFree(m_text); }

   UINT32 getId() const { return m_id; }
   time_t getChangeTime() const { return m_changeTime; }
   UINT32 getUserId() const { return m_userId; }
   const TCHAR *getText() const { return m_text; }
};

/**
 * Functions
 */
bool InitAlarmManager();
void ShutdownAlarmManager();

void SendAlarmsToClient(UINT32 dwRqId, ClientSession *pSession);
void DeleteAlarmNotes(DB_HANDLE hdb, UINT32 alarmId);
void DeleteAlarmEvents(DB_HANDLE hdb, UINT32 alarmId);

UINT32 NXCORE_EXPORTABLE GetAlarm(UINT32 dwAlarmId, UINT32 userId, NXCPMessage *msg, ClientSession *session);
ObjectArray<Alarm> NXCORE_EXPORTABLE *GetAlarms(UINT32 objectId = 0, bool recursive = false);
Alarm NXCORE_EXPORTABLE *FindAlarmById(UINT32 alarmId);
UINT32 NXCORE_EXPORTABLE GetAlarmEvents(UINT32 dwAlarmId, UINT32 userId, NXCPMessage *msg, ClientSession *session);
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(uint32_t alarmId, bool alreadyLocked = false);
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(const TCHAR *hdref);
int GetMostCriticalStatusForObject(uint32_t objectId);
void GetAlarmStats(NXCPMessage *pMsg);
int GetAlarmCount();
UINT64 GetAlarmMemoryUsage();
Alarm NXCORE_EXPORTABLE *LoadAlarmFromDatabase(UINT32 alarmId);

UINT32 NXCORE_EXPORTABLE CreateNewAlarm(const uuid& rule, const TCHAR *message, const TCHAR *key, const TCHAR *impact, int state,
         int severity, UINT32 timeout, UINT32 timeoutEvent, UINT32 parentAlarmId, const TCHAR *rcaScriptName, Event *event,
         UINT32 ackTimeout, IntegerArray<UINT32> *alarmCategoryList, bool openHelpdeskIssue);
UINT32 NXCORE_EXPORTABLE AckAlarmById(UINT32 dwAlarmId, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime, bool includeSubordinates);
UINT32 NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, UINT32 acknowledgmentActionTime);
UINT32 NXCORE_EXPORTABLE ResolveAlarmById(UINT32 alarmId, ClientSession *session, bool terminate, bool includeSubordinates);
void NXCORE_EXPORTABLE ResolveAlarmsById(IntegerArray<UINT32> *alarmIds, IntegerArray<UINT32> *failIds,
         IntegerArray<UINT32> *failCodes, ClientSession *session, bool terminate, bool includeSubordinates);
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent);
void NXCORE_EXPORTABLE ResolveAlarmByDCObjectId(UINT32 dciId, bool terminate);
UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate);
void NXCORE_EXPORTABLE DeleteAlarm(UINT32 dwAlarmId, bool objectCleanup);

UINT32 AddAlarmComment(const TCHAR *hdref, const TCHAR *text, UINT32 userId);
UINT32 UpdateAlarmComment(UINT32 alarmId, UINT32 *noteId, const TCHAR *text, UINT32 userId, bool syncWithHelpdesk = true);
UINT32 DeleteAlarmCommentByID(UINT32 alarmId, UINT32 noteId);
UINT32 GetAlarmComments(UINT32 alarmId, NXCPMessage *msg);
ObjectArray<AlarmComment> *GetAlarmComments(UINT32 alarmId);

bool DeleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb);

UINT32 NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref);
UINT32 NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref);
void LoadHelpDeskLink();
UINT32 CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref);
UINT32 OpenHelpdeskIssue(UINT32 alarmId, ClientSession *session, TCHAR *hdref);
UINT32 AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text);
UINT32 GetHelpdeskIssueUrlFromAlarm(UINT32 alarmId, UINT32 userId, TCHAR *url, size_t size, ClientSession *session);
UINT32 GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);
UINT32 UnlinkHelpdeskIssueById(UINT32 dwAlarmId, ClientSession *session);
UINT32 UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, ClientSession *session);

/**
 * Alarm category functions
 */
void GetAlarmCategories(NXCPMessage *msg);
UINT32 UpdateAlarmCategory(const NXCPMessage *request, UINT32 *returnId);
UINT32 DeleteAlarmCategory(UINT32 id);
bool CheckAlarmCategoryAccess(UINT32 userId, UINT32 categoryId);
void LoadAlarmCategories();
AlarmCategory *GetAlarmCategory(UINT32 id);
UINT32 GetAndUpdateAlarmCategoryByName(const TCHAR *name, const TCHAR *description);
UINT32 CreateNewAlarmCategoryFromImport(const TCHAR *name, const TCHAR *description);

/**
 * Alarm summary emails
 */
void SendAlarmSummaryEmail(const shared_ptr<ScheduledTaskParameters>& parameters);
void EnableAlarmSummaryEmails();

#endif
