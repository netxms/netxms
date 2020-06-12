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
   uint64_t m_sourceEventId;   // Originating event ID
   uint32_t m_alarmId;         // Unique alarm ID
   uint32_t m_parentAlarmId;   // Parent (root cause) alarm ID
   TCHAR *m_rcaScriptName;   // Name of root cause analysis script
   time_t m_creationTime;    // Alarm creation time in UNIX time format
   time_t m_lastChangeTime;  // Alarm's last change time in UNIX time format
   uuid m_rule;              // GUID of EPP rule that generates this alarm
   uint32_t m_sourceObject;    // Source object ID
   int32_t m_zoneUIN;        // Zone UIN for source object
   uint32_t m_sourceEventCode; // Originating event code
   TCHAR *m_eventTags;       // Tags of originating event
   uint32_t m_dciId;           // related DCI ID
   BYTE m_currentSeverity;   // Alarm's current severity
   BYTE m_originalSeverity;  // Alarm's original severity
   BYTE m_state;             // Current state
   BYTE m_helpDeskState;     // State of alarm in helpdesk system
   uint32_t m_ackByUser;       // ID of user who was acknowledged this alarm (0 for system)
   uint32_t m_resolvedByUser;  // ID of user who was resolved this alarm (0 for system)
   uint32_t m_termByUser;      // ID of user who was terminated this alarm (0 for system)
   time_t m_ackTimeout;      // Sticky acknowledgment end time, 0 if acknowledged without timeout
   uint32_t m_repeatCount;
   uint32_t m_timeout;
   uint32_t m_timeoutEvent;
   TCHAR m_message[MAX_EVENT_MSG_LENGTH];
   TCHAR m_key[MAX_DB_STRING];
   TCHAR m_helpDeskRef[MAX_HELPDESK_REF_LEN];
   TCHAR *m_impact;
   uint32_t m_commentCount;     // Number of comments added to alarm
   IntegerArray<uint64_t> *m_relatedEvents;
   IntegerArray<uint32_t> *m_alarmCategoryList;
   uint32_t m_notificationCode; // notification code used when sending client notifications
   IntegerArray<uint32_t> *m_subordinateAlarms;

   StringBuffer categoryListToString();

   void executeHookScript();

public:
   Alarm(Event *event, uint32_t parentAlarmId, const TCHAR *rcaScriptName, const uuid& rule, const TCHAR *message, const TCHAR *key, const TCHAR *impact,
            int state, int severity, uint32_t timeout, uint32_t timeoutEvent, uint32_t ackTimeout, IntegerArray<uint32_t> *alarmCategoryList);
   Alarm(DB_HANDLE hdb, DB_RESULT hResult, int row);
   Alarm(const Alarm *src, bool copyEvents, uint32_t notificationCode = 0);
   ~Alarm();

   uint64_t getSourceEventId() const { return m_sourceEventId; }
   uint32_t getAlarmId() const { return m_alarmId; }
   uint32_t getParentAlarmId() const { return m_parentAlarmId; }
   const TCHAR *getRcaScriptName() const { return CHECK_NULL_EX(m_rcaScriptName); }
   time_t getCreationTime() const { return m_creationTime; }
   time_t getLastChangeTime() const { return m_lastChangeTime; }
   const uuid& getRule() const { return m_rule; }
   uint32_t getSourceObject() const { return m_sourceObject; }
   uint32_t getSourceEventCode() const { return m_sourceEventCode; }
   const TCHAR *getEventTags() const { return m_eventTags; }
   uint32_t getDciId() const { return m_dciId; }
   BYTE getCurrentSeverity() const { return m_currentSeverity; }
   BYTE getOriginalSeverity() const { return m_originalSeverity; }
   BYTE getState() const { return m_state; }
   BYTE getHelpDeskState() const { return m_helpDeskState; }
   uint32_t getAckByUser() const { return m_ackByUser; }
   uint32_t getResolvedByUser() const { return m_resolvedByUser; }
   uint32_t getTermByUser() const { return m_termByUser; }
   time_t getAckTimeout() const { return m_ackTimeout; }
   uint32_t getRepeatCount() const { return m_repeatCount; }
   uint32_t getTimeout() const { return m_timeout; }
   uint32_t getTimeoutEvent() const { return m_timeoutEvent; }
   const TCHAR *getMessage() const { return m_message; }
   const TCHAR *getKey() const { return m_key; }
   const TCHAR *getHelpDeskRef() const { return m_helpDeskRef; }
   const TCHAR *getImpact() const { return CHECK_NULL_EX(m_impact); }
   uint32_t getCommentCount() const { return m_commentCount; }
   uint32_t getNotificationCode() const { return m_notificationCode; }
   uint64_t getMemoryUsage() const;

   void fillMessage(NXCPMessage *msg) const;

   void createInDatabase();
   void updateInDatabase();

   void clearTimeout() { m_timeout = 0; }
   void onAckTimeoutExpiration() { m_ackTimeout = 0; m_state = ALARM_STATE_OUTSTANDING; }

   void addRelatedEvent(UINT64 eventId) { if (m_relatedEvents != NULL) m_relatedEvents->add(eventId); }
   bool isEventRelated(UINT64 eventId) const { return (m_relatedEvents != NULL) && m_relatedEvents->contains(eventId); }

   void updateFromEvent(Event *event, uint32_t parentAlarmId, const TCHAR *rcaScriptName, int state, int severity, uint32_t timeout,
            uint32_t timeoutEvent, uint32_t ackTimeout, const TCHAR *message, const TCHAR *impact, IntegerArray<uint32_t> *alarmCategoryList);
   void updateParentAlarm(UINT32 parentAlarmId);
   uint32_t acknowledge(ClientSession *session, bool sticky, uint32_t acknowledgmentActionTime, bool includeSubordinates);
   void resolve(uint32_t userId, Event *event, bool terminate, bool notify, bool includeSubordinates);
   uint32_t openHelpdeskIssue(TCHAR *hdref);
   void unlinkFromHelpdesk() { m_helpDeskState = ALARM_HELPDESK_IGNORED; m_helpDeskRef[0] = 0; }
   uint32_t updateAlarmComment(uint32_t *commentId, const TCHAR *text, uint32_t userId, bool syncWithHelpdesk);
   uint32_t deleteComment(uint32_t commentId);

   void addSubordinateAlarm(uint32_t alarmId);
   void removeSubordinateAlarm(uint32_t alarmId);

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
   uint32_t m_id;
   time_t m_changeTime;
   uint32_t m_userId;
   TCHAR *m_text;

public:
   AlarmComment(uint32_t id, time_t changeTime, uint32_t userId, TCHAR *text);
   ~AlarmComment() { MemFree(m_text); }

   uint32_t getId() const { return m_id; }
   time_t getChangeTime() const { return m_changeTime; }
   uint32_t getUserId() const { return m_userId; }
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

uint32_t NXCORE_EXPORTABLE GetAlarm(uint32_t alarmId, uint32_t userId, NXCPMessage *msg, ClientSession *session);
ObjectArray<Alarm> NXCORE_EXPORTABLE *GetAlarms(uint32_t objectId = 0, bool recursive = false);
Alarm NXCORE_EXPORTABLE *FindAlarmById(UINT32 alarmId);
uint32_t NXCORE_EXPORTABLE GetAlarmEvents(uint32_t alarmId, uint32_t userId, NXCPMessage *msg, ClientSession *session);
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(uint32_t alarmId, bool alreadyLocked = false);
shared_ptr<NetObj> NXCORE_EXPORTABLE GetAlarmSourceObject(const TCHAR *hdref);
int GetMostCriticalStatusForObject(uint32_t objectId);
void GetAlarmStats(NXCPMessage *pMsg);
int GetAlarmCount();
uint64_t GetAlarmMemoryUsage();
Alarm NXCORE_EXPORTABLE *LoadAlarmFromDatabase(UINT32 alarmId);

uint32_t NXCORE_EXPORTABLE CreateNewAlarm(const uuid& rule, const TCHAR *message, const TCHAR *key, const TCHAR *impact, int state,
         int severity, uint32_t timeout, uint32_t timeoutEvent, uint32_t parentAlarmId, const TCHAR *rcaScriptName, Event *event,
         uint32_t ackTimeout, IntegerArray<uint32_t> *alarmCategoryList, bool openHelpdeskIssue);
uint32_t NXCORE_EXPORTABLE AckAlarmById(uint32_t dwAlarmId, ClientSession *session, bool sticky, uint32_t acknowledgmentActionTime, bool includeSubordinates);
uint32_t NXCORE_EXPORTABLE AckAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool sticky, uint32_t acknowledgmentActionTime);
uint32_t NXCORE_EXPORTABLE ResolveAlarmById(uint32_t alarmId, ClientSession *session, bool terminate, bool includeSubordinates);
void NXCORE_EXPORTABLE ResolveAlarmsById(IntegerArray<UINT32> *alarmIds, IntegerArray<UINT32> *failIds,
         IntegerArray<UINT32> *failCodes, ClientSession *session, bool terminate, bool includeSubordinates);
void NXCORE_EXPORTABLE ResolveAlarmByKey(const TCHAR *pszKey, bool useRegexp, bool terminate, Event *pEvent);
void NXCORE_EXPORTABLE ResolveAlarmByDCObjectId(uint32_t dciId, bool terminate);
uint32_t NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref, ClientSession *session, bool terminate);
uint32_t NXCORE_EXPORTABLE ResolveAlarmByHDRef(const TCHAR *hdref);
uint32_t NXCORE_EXPORTABLE TerminateAlarmByHDRef(const TCHAR *hdref);
void NXCORE_EXPORTABLE DeleteAlarm(uint32_t alarmId, bool objectCleanup);

uint32_t AddAlarmComment(const TCHAR *hdref, const TCHAR *text, uint32_t userId);
uint32_t UpdateAlarmComment(uint32_t alarmId, uint32_t *noteId, const TCHAR *text, uint32_t userId, bool syncWithHelpdesk = true);
uint32_t DeleteAlarmCommentByID(uint32_t alarmId, uint32_t noteId);
uint32_t GetAlarmComments(uint32_t alarmId, NXCPMessage *msg);
ObjectArray<AlarmComment> *GetAlarmComments(uint32_t alarmId);

bool DeleteObjectAlarms(UINT32 objectId, DB_HANDLE hdb);

void LoadHelpDeskLink();
uint32_t CreateHelpdeskIssue(const TCHAR *description, TCHAR *hdref);
uint32_t OpenHelpdeskIssue(uint32_t alarmId, ClientSession *session, TCHAR *hdref);
uint32_t AddHelpdeskIssueComment(const TCHAR *hdref, const TCHAR *text);
uint32_t GetHelpdeskIssueUrlFromAlarm(uint32_t alarmId, uint32_t userId, TCHAR *url, size_t size, ClientSession *session);
uint32_t GetHelpdeskIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);
uint32_t UnlinkHelpdeskIssueById(uint32_t alarmId, ClientSession *session);
uint32_t UnlinkHelpdeskIssueByHDRef(const TCHAR *hdref, ClientSession *session);

/**
 * Alarm category functions
 */
void GetAlarmCategories(NXCPMessage *msg);
uint32_t UpdateAlarmCategory(const NXCPMessage *request, uint32_t *returnId);
uint32_t DeleteAlarmCategory(uint32_t id);
bool CheckAlarmCategoryAccess(uint32_t userId, uint32_t categoryId);
void LoadAlarmCategories();
AlarmCategory *GetAlarmCategory(uint32_t id);
uint32_t GetAndUpdateAlarmCategoryByName(const TCHAR *name, const TCHAR *description);
uint32_t CreateNewAlarmCategoryFromImport(const TCHAR *name, const TCHAR *description);

/**
 * Alarm summary emails
 */
void SendAlarmSummaryEmail(const shared_ptr<ScheduledTaskParameters>& parameters);
void EnableAlarmSummaryEmails();

#endif
