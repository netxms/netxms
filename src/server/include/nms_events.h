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
** File: nms_events.h
**
**/

#ifndef _nms_events_h_
#define _nms_events_h_

#include <nxevent.h>
#include <jansson.h>

#include <nxcore_schedule.h>

/**
 * Event origins
 */
enum class EventOrigin
{
   SYSTEM = 0,
   AGENT = 1,
   CLIENT = 2,
   SYSLOG = 3,
   SNMP = 4,
   NXSL = 5,
   REMOTE_SERVER = 6
};

/**
 * Event template
 */
class EventTemplate : public RefCountObject
{
private:
   uuid m_guid;
   UINT32 m_code;
   TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_tags;
   int m_severity;
   UINT32 m_flags;
   TCHAR *m_messageTemplate;
   TCHAR *m_description;

protected:
   virtual ~EventTemplate();

public:
   EventTemplate(DB_RESULT hResult, int row);
   EventTemplate(NXCPMessage *msg);

   const uuid& getGuid() const { return m_guid; }
   UINT32 getCode() const { return m_code; }
   const TCHAR *getName() const { return m_name; }
   int getSeverity() const { return m_severity; }
   UINT32 getFlags() const { return m_flags; }
   const TCHAR *getMessageTemplate() const { return m_messageTemplate; }
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getTags() const { return m_tags; }

   void modifyFromMessage(NXCPMessage *msg);
   void fillMessage(NXCPMessage *msg, UINT32 base) const;
   bool saveToDatabase() const;

   json_t *toJson() const;
};

/**
 * Event
 */
class NXCORE_EXPORTABLE Event
{
private:
   UINT64 m_id;
   UINT64 m_rootId;    // Root event id
   UINT32 m_code;
   int m_severity;
   UINT32 m_flags;
   EventOrigin m_origin;
   UINT32 m_sourceId;
   UINT32 m_zoneUIN;
   UINT32 m_dciId;
	TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_messageText;
   TCHAR *m_messageTemplate;
   time_t m_timestamp;
   time_t m_originTimestamp;
   StringSet m_tags;
	TCHAR *m_customMessage;
	Array m_parameters;
	StringList m_parameterNames;

	void init(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, UINT32 dciId, const TCHAR *tag);

public:
   Event();
   Event(const Event *src);
   Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId,
            UINT32 dciId, const TCHAR *tag, const char *format, const TCHAR **names, va_list args);
   Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId,
            UINT32 dciId, const TCHAR *tag, StringMap *args);
   ~Event();

   UINT64 getId() const { return m_id; }
   UINT32 getCode() const { return m_code; }
   UINT32 getSeverity() const { return m_severity; }
   UINT32 getFlags() const { return m_flags; }
   EventOrigin getOrigin() const { return m_origin; }
   UINT32 getSourceId() const { return m_sourceId; }
   UINT32 getZoneUIN() const { return m_zoneUIN; }
   UINT32 getDciId() const { return m_dciId; }
	const TCHAR *getName() const { return m_name; }
   const TCHAR *getMessage() const { return m_messageText; }
   StringBuffer getTagsAsList() const;
   time_t getTimestamp() const { return m_timestamp; }
   time_t getOriginTimestamp() const { return m_originTimestamp; }
   const Array *getParameterList() const { return &m_parameters; }
   const StringList *getParameterNames() const { return &m_parameterNames; }

   void setSeverity(int severity) { m_severity = severity; }

   UINT64 getRootId() const { return m_rootId; }
   void setRootId(UINT64 id) { m_rootId = id; }

   void prepareMessage(NXCPMessage *msg) const;

   void expandMessageText();
   StringBuffer expandText(const TCHAR *textTemplate, const Alarm *alarm = NULL) const;
   void setMessage(const TCHAR *text) { MemFree(m_messageText); m_messageText = MemCopyString(text); }

   bool hasTag(const TCHAR *tag) const { return m_tags.contains(tag); }
   void addTag(const TCHAR *tag) { m_tags.add(tag); }
   void removeTag(const TCHAR *tag) { m_tags.remove(tag); }

   int getParametersCount() const { return m_parameters.size(); }
   const TCHAR *getParameter(int index, const TCHAR *defaultValue = NULL) const
   {
      const TCHAR *v = static_cast<TCHAR*>(m_parameters.get(index));
      return (v != NULL) ? v : defaultValue;
   }
   const TCHAR *getParameterName(int index) const { return m_parameterNames.get(index); }
   UINT32 getParameterAsULong(int index, UINT32 defaultValue = 0) const
   {
      const TCHAR *v = static_cast<TCHAR*>(m_parameters.get(index));
      return (v != NULL) ? _tcstoul(v, NULL, 0) : defaultValue;
   }
   UINT64 getParameterAsUInt64(int index, UINT64 defaultValue = 0) const
   {
      const TCHAR *v = static_cast<TCHAR*>(m_parameters.get(index));
      return (v != NULL) ? _tcstoull(v, NULL, 0) : defaultValue;
   }

	const TCHAR *getNamedParameter(const TCHAR *name, const TCHAR *defaultValue = NULL) const { return getParameter(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
   UINT32 getNamedParameterAsULong(const TCHAR *name, UINT32 defaultValue = 0) const { return getParameterAsULong(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
   UINT64 getNamedParameterAsUInt64(const TCHAR *name, UINT64 defaultValue = 0) const { return getParameterAsUInt64(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }

	void addParameter(const TCHAR *name, const TCHAR *value);
	void setNamedParameter(const TCHAR *name, const TCHAR *value);
	void setParameter(int index, const TCHAR *name, const TCHAR *value);

   const TCHAR *getCustomMessage() const { return CHECK_NULL_EX(m_customMessage); }
   void setCustomMessage(const TCHAR *message) { MemFree(m_customMessage); m_customMessage = MemCopyString(message); }

   json_t *toJson();
   static Event *createFromJson(json_t *json);
};

/**
 * Transient data for scheduled action execution
 */
class ActionExecutionTransientData : public ScheduledTaskTransientData
{
private:
   Event *m_event;
   Alarm *m_alarm;

public:
   ActionExecutionTransientData(const Event *e, const Alarm *a);
   virtual ~ActionExecutionTransientData();

   const Event *getEvent() const { return m_event; }
   const Alarm *getAlarm() const { return m_alarm; }
};

/**
 * Defines for type of persistent storage action
 */
 #define PSTORAGE_SET      1
 #define PSTORAGE_DELETE   2

/**
 * Action execution
 */
struct ActionExecutionConfiguration
{
   UINT32 actionId;
   UINT32 timerDelay;
   TCHAR *timerKey;

   ActionExecutionConfiguration(UINT32 i, UINT32 d, TCHAR *k)
   {
      actionId = i;
      timerDelay = d;
      timerKey = k;
   }

   ~ActionExecutionConfiguration()
   {
      MemFree(timerKey);
   }
};

/**
 * Event policy rule
 */
class EPRule
{
private:
   UINT32 m_id;
   uuid m_guid;
   UINT32 m_flags;
   IntegerArray<UINT32> m_sources;
   IntegerArray<UINT32> m_events;
   ObjectArray<ActionExecutionConfiguration> m_actions;
   StringList m_timerCancellations;
   TCHAR *m_comments;
   TCHAR *m_scriptSource;
   NXSL_VM *m_script;

   TCHAR *m_alarmMessage;
   TCHAR *m_alarmImpact;
   int m_alarmSeverity;
   TCHAR *m_alarmKey;
	UINT32 m_alarmTimeout;
	UINT32 m_alarmTimeoutEvent;
	IntegerArray<UINT32> m_alarmCategoryList;
	TCHAR *m_rcaScriptName;    // Name of library script used for root cause analysis

	StringMap m_pstorageSetActions;
	StringList m_pstorageDeleteActions;

   bool matchSource(UINT32 objectId);
   bool matchEvent(UINT32 eventCode);
   bool matchSeverity(UINT32 severity);
   bool matchScript(Event *event);

   UINT32 generateAlarm(Event *event);

public:
   EPRule(UINT32 id);
   EPRule(DB_RESULT hResult, int row);
   EPRule(NXCPMessage *msg);
   EPRule(ConfigEntry *config);
   ~EPRule();

   UINT32 getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   void setId(UINT32 newId) { m_id = newId; }
   bool loadFromDB(DB_HANDLE hdb);
	bool saveToDB(DB_HANDLE hdb);
   bool processEvent(Event *pEvent);
   void createMessage(NXCPMessage *pMsg);
   void createExportRecord(StringBuffer &xml) const;
   void createOrderingExportRecord(StringBuffer &xml) const;
   json_t *toJson() const;

   bool isActionInUse(UINT32 actionId) const;
   bool isCategoryInUse(UINT32 categoryId) const { return m_alarmCategoryList.contains(categoryId); }
};

/**
 * Event policy
 */
class EventPolicy
{
private:
   ObjectArray<EPRule> m_rules;
   RWLOCK m_rwlock;

   void readLock() const { RWLockReadLock(m_rwlock, INFINITE); }
   void writeLock() { RWLockWriteLock(m_rwlock, INFINITE); }
   void unlock() const { RWLockUnlock(m_rwlock); }
   int findRuleIndexByGuid(const uuid& guid, int shift = 0) const;

public:
   EventPolicy();
   ~EventPolicy();

   UINT32 getNumRules() const { return m_rules.size(); }
   bool loadFromDB();
   bool saveToDB() const;
   void processEvent(Event *pEvent);
   void sendToClient(ClientSession *pSession, UINT32 dwRqId) const;
   void replacePolicy(UINT32 dwNumRules, EPRule **ppRuleList);
   void exportRule(StringBuffer& xml, const uuid& guid) const;
   void exportRuleOrgering(StringBuffer& xml) const;
   void importRule(EPRule *rule, bool overwrite, ObjectArray<uuid> *ruleOrdering);
   void removeRuleCategory (UINT32 categoryId);
   json_t *toJson() const;

   bool isActionInUse(UINT32 actionId) const;
   bool isCategoryInUse(UINT32 categoryId) const;
};

/**
 * Functions
 */
bool InitEventSubsystem();
void ShutdownEventSubsystem();
void ReloadEvents();
UINT32 UpdateEventTemplate(NXCPMessage *request, NXCPMessage *response, json_t **oldValue, json_t **newValue);
UINT32 DeleteEventTemplate(UINT32 eventCode);
void GetEventConfiguration(NXCPMessage *msg);
void CreateEventTemplateExportRecord(StringBuffer &str, UINT32 eventCode);

void CorrelateEvent(Event *pEvent);
Event *LoadEventFromDatabase(UINT64 eventId);
Event *FindEventInLoggerQueue(UINT64 eventId);

bool EventNameFromCode(UINT32 eventCode, TCHAR *buffer);
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue = 0);
EventTemplate *FindEventTemplateByCode(UINT32 code);
EventTemplate *FindEventTemplateByName(const TCHAR *name);

bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const StringList& parameters);
bool NXCORE_EXPORTABLE PostSystemEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostDciEvent(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, ...);
UINT64 NXCORE_EXPORTABLE PostEvent2(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, ...);
UINT64 NXCORE_EXPORTABLE PostSystemEvent2(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostSystemEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostSystemEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const TCHAR *tag, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const TCHAR *tag, StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const TCHAR *tag, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const TCHAR *tag, const StringList& parameters);
bool NXCORE_EXPORTABLE PostEventEx(ObjectQueue<Event> *queue, UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostSystemEventEx(ObjectQueue<Event> *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...);
void NXCORE_EXPORTABLE ResendEvents(ObjectQueue<Event> *queue);
bool NXCORE_EXPORTABLE TransformAndPostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm);
bool NXCORE_EXPORTABLE TransformAndPostSystemEvent(UINT32 eventCode, UINT32 sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm);

const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps);

/**
 * Global variables
 */
extern ObjectQueue<Event> g_eventQueue;
extern EventPolicy *g_pEventPolicy;
extern UINT64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
