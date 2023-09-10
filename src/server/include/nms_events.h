/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
   REMOTE_SERVER = 6,
   WINDOWS_EVENT = 7
};

/**
 * Script execution contexts
 */
#define SCRIPT_CONTEXT_ACTION       _T("Action")
#define SCRIPT_CONTEXT_AGENT_CFG    _T("Agent configuration")
#define SCRIPT_CONTEXT_ALARM        _T("Alarm")
#define SCRIPT_CONTEXT_AUTOBIND     _T("Autobind")
#define SCRIPT_CONTEXT_BIZSVC       _T("Business service")
#define SCRIPT_CONTEXT_CLIENT       _T("Client")
#define SCRIPT_CONTEXT_DCI          _T("DCI")
#define SCRIPT_CONTEXT_EVENT_PROC   _T("Event processing")
#define SCRIPT_CONTEXT_LDAP         _T("LDAP")
#define SCRIPT_CONTEXT_LOGIN        _T("Login")
#define SCRIPT_CONTEXT_NETMAP       _T("Network map")
#define SCRIPT_CONTEXT_OBJECT       _T("Object")
#define SCRIPT_CONTEXT_OBJECT_QUERY _T("Object query")
#define SCRIPT_CONTEXT_SNMP_TRAP    _T("SNMP Trap")
#define SCRIPT_CONTEXT_TUNNEL       _T("Tunnel")

/**
 * Event template
 */
class EventTemplate
{
private:
   uuid m_guid;
   uint32_t m_code;
   TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_tags;
   int m_severity;
   uint32_t m_flags;
   TCHAR *m_messageTemplate;
   TCHAR *m_description;

public:
   EventTemplate(DB_RESULT hResult, int row);
   EventTemplate(const NXCPMessage& msg);
   ~EventTemplate();

   const uuid& getGuid() const { return m_guid; }
   uint32_t getCode() const { return m_code; }
   const TCHAR *getName() const { return m_name; }
   int getSeverity() const { return m_severity; }
   uint32_t getFlags() const { return m_flags; }
   const TCHAR *getMessageTemplate() const { return m_messageTemplate; }
   const TCHAR *getDescription() const { return m_description; }
   const TCHAR *getTags() const { return m_tags; }

   void modifyFromMessage(const NXCPMessage& msg);
   void fillMessage(NXCPMessage *msg, uint32_t base) const;
   bool saveToDatabase() const;

   json_t *toJson() const;
};

/**
 * Event queue binding (used for parallel event processing)
 */
struct EventQueueBinding;

/**
 * Event
 */
class NXCORE_EXPORTABLE Event
{
private:
   uint64_t m_id;
   uint64_t m_rootId;    // Root event id
   uint32_t m_code;
   int m_severity;
   uint32_t m_flags;
   EventOrigin m_origin;
   uint32_t m_sourceId;
   int32_t m_zoneUIN;
   uint32_t m_dciId;
	TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_messageText;
   TCHAR *m_messageTemplate;
   time_t m_timestamp;
   time_t m_originTimestamp;
   StringSet m_tags;
	TCHAR *m_customMessage;
	Array m_parameters;
	StringList m_parameterNames;
	MutableString m_lastAlarmKey;
	MutableString m_lastAlarmMessage;
	int64_t m_queueTime;
	EventQueueBinding *m_queueBinding;

	void init(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, uint32_t dciId);

public:
   Event();
   Event(const Event *src);
   Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId,
            uint32_t dciId, const char *format, const TCHAR **names, va_list args);
   Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId,
            uint32_t dciId, const StringMap& args);
   ~Event();

   uint64_t getId() const { return m_id; }
   uint32_t getCode() const { return m_code; }
   uint32_t getSeverity() const { return m_severity; }
   uint32_t getFlags() const { return m_flags; }
   EventOrigin getOrigin() const { return m_origin; }
   uint32_t getSourceId() const { return m_sourceId; }
   int32_t getZoneUIN() const { return m_zoneUIN; }
   uint32_t getDciId() const { return m_dciId; }
	const TCHAR *getName() const { return m_name; }
   const TCHAR *getMessage() const { return m_messageText; }
   StringBuffer getTagsAsList() const;
   void getTagsAsList(StringBuffer *sb) const;
   time_t getTimestamp() const { return m_timestamp; }
   time_t getOriginTimestamp() const { return m_originTimestamp; }
   const TCHAR *getLastAlarmKey() const { return m_lastAlarmKey.cstr(); }
   const TCHAR *getLastAlarmMessage() const { return m_lastAlarmMessage.cstr(); }
   const Array *getParameterList() const { return &m_parameters; }
   const StringList *getParameterNames() const { return &m_parameterNames; }

   void setLogWriteFlag(bool flag) { if (flag) m_flags |= EF_LOG; else m_flags &= ~EF_LOG; }
   void setSeverity(int severity) { m_severity = severity; }
   void setLastAlarmKey(const TCHAR *key) { m_lastAlarmKey = key; }
   void setLastAlarmMessage(const TCHAR *message) { m_lastAlarmMessage = message; }

   int64_t getQueueTime() const { return m_queueTime; }
   void setQueueTime(int64_t t) { m_queueTime = t; }

   EventQueueBinding *getQueueBinding() const { return m_queueBinding; }
   void setQueueBinding(EventQueueBinding *b) { m_queueBinding = b; }

   uint64_t getRootId() const { return m_rootId; }
   void setRootId(uint64_t id) { m_rootId = id; }

   void prepareMessage(NXCPMessage *msg) const;

   void expandMessageText();
   StringBuffer expandText(const TCHAR *textTemplate, const Alarm *alarm = nullptr) const;
   void setMessage(const TCHAR *text) { MemFree(m_messageText); m_messageText = MemCopyString(text); }

   bool hasTag(const TCHAR *tag) const { return m_tags.contains(tag); }
   void addTag(const TCHAR *tag) { if (*tag != 0) m_tags.add(tag); }
   void removeTag(const TCHAR *tag) { m_tags.remove(tag); }

   int getParametersCount() const { return m_parameters.size(); }
   const TCHAR *getParameter(int index, const TCHAR *defaultValue = nullptr) const
   {
      const TCHAR *v = static_cast<TCHAR*>(m_parameters.get(index));
      return (v != nullptr) ? v : defaultValue;
   }
   const TCHAR *getParameterName(int index) const { return m_parameterNames.get(index); }
   int32_t getParameterAsInt32(int index, int32_t defaultValue = 0) const
   {
      const TCHAR *v = static_cast<const TCHAR*>(m_parameters.get(index));
      return (v != nullptr) ? _tcstol(v, nullptr, 0) : defaultValue;
   }
   uint32_t getParameterAsUInt32(int index, uint32_t defaultValue = 0) const
   {
      const TCHAR *v = static_cast<const TCHAR*>(m_parameters.get(index));
      return (v != nullptr) ? _tcstoul(v, nullptr, 0) : defaultValue;
   }
   int64_t getParameterAsInt64(int index, int64_t defaultValue = 0) const
   {
      const TCHAR *v = static_cast<const TCHAR*>(m_parameters.get(index));
      return (v != nullptr) ? _tcstoll(v, nullptr, 0) : defaultValue;
   }
   uint64_t getParameterAsUInt64(int index, uint64_t defaultValue = 0) const
   {
      const TCHAR *v = static_cast<const TCHAR*>(m_parameters.get(index));
      return (v != nullptr) ? _tcstoull(v, nullptr, 0) : defaultValue;
   }

	const TCHAR *getNamedParameter(const TCHAR *name, const TCHAR *defaultValue = nullptr) const { return getParameter(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
   int32_t getNamedParameterAsInt32(const TCHAR *name, int32_t defaultValue = 0) const { return getParameterAsInt32(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
	uint32_t getNamedParameterAsUInt32(const TCHAR *name, uint32_t defaultValue = 0) const { return getParameterAsUInt32(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
   int64_t getNamedParameterAsInt64(const TCHAR *name, int64_t defaultValue = 0) const { return getParameterAsInt64(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }
   uint64_t getNamedParameterAsUInt64(const TCHAR *name, uint64_t defaultValue = 0) const { return getParameterAsUInt64(m_parameterNames.indexOfIgnoreCase(name), defaultValue); }

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
   uint32_t actionId;
   TCHAR *timerDelay;
   TCHAR *snoozeTime;
   TCHAR *timerKey;
   TCHAR *blockingTimerKey;

   ActionExecutionConfiguration(uint32_t i, TCHAR *d, TCHAR *st, TCHAR *k, TCHAR *bk)
   {
      actionId = i;
      timerDelay = d;
      snoozeTime = st;
      timerKey = k;
      blockingTimerKey = bk;
   }

   ~ActionExecutionConfiguration()
   {
      MemFree(timerDelay);
      MemFree(snoozeTime);
      MemFree(timerKey);
      MemFree(blockingTimerKey);
   }
};

/**
 * Event policy rule
 */
class EPRule
{
private:
   uint32_t m_id;
   uuid m_guid;
   uint32_t m_flags;
   IntegerArray<uint32_t> m_sources;
   IntegerArray<uint32_t> m_events;
   ObjectArray<ActionExecutionConfiguration> m_actions;
   StringList m_timerCancellations;
   TCHAR *m_comments;
   TCHAR *m_scriptSource;
   NXSL_Program *m_script;

   TCHAR *m_alarmMessage;
   TCHAR *m_alarmImpact;
   int m_alarmSeverity;
   TCHAR *m_alarmKey;
   uint32_t m_alarmTimeout;
   uint32_t m_alarmTimeoutEvent;
	IntegerArray<uint32_t> m_alarmCategoryList;
	TCHAR *m_rcaScriptName;    // Name of library script used for root cause analysis

	StringMap m_pstorageSetActions;
	StringList m_pstorageDeleteActions;

   bool matchSource(const shared_ptr<NetObj>& object) const;
   bool matchEvent(uint32_t eventCode) const;
   bool matchSeverity(uint32_t severity) const;
   bool matchScript(Event *event) const;

   uint32_t generateAlarm(Event *event) const;

public:
   EPRule(uint32_t id);
   EPRule(DB_RESULT hResult, int row);
   EPRule(const NXCPMessage& msg);
   EPRule(const ConfigEntry& config);
   ~EPRule();

   uint32_t getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   void setId(uint32_t newId) { m_id = newId; }
   bool loadFromDB(DB_HANDLE hdb);
	bool saveToDB(DB_HANDLE hdb) const;
   bool processEvent(Event *event) const;
   void createMessage(NXCPMessage *msg) const;
   void createExportRecord(StringBuffer &xml) const;
   void createOrderingExportRecord(StringBuffer &xml) const;
   json_t *toJson() const;

   bool isActionInUse(uint32_t actionId) const;
   bool isCategoryInUse(uint32_t categoryId) const { return m_alarmCategoryList.contains(categoryId); }

   bool isUsingEvent(uint32_t eventCode) const { return m_events.contains(eventCode); }
   const TCHAR* getComments() { return m_comments; }
};

/**
 * Event reference type enum for EventReference
 */
enum class EventReferenceType
{
   AGENT_POLICY = 0,
   DCI = 1,
   EP_RULE = 2,
   SNMP_TRAP = 3,
   CONDITION = 4,
   SYSLOG = 5,
   WIN_EVENT_LOG = 6
};

/**
 * Information about an object that is using the event
 */
class EventReference
{
private:
   EventReferenceType m_type;
   uint32_t m_id;
   uuid m_guid;
   String m_name;
   uint32_t m_ownerId;
   uuid m_ownerGuid;
   String m_ownerName;
   String m_description;

public:
   /**
    * Create new event reference.
    * @param type Reference type
    * @param id Object id within it's type
    * @param guid Object guid
    * @param ownerId Object owner id within it's type, if one exists (0 if not)
    * @param ownerGuid Object owner guid, if one exists (0 if not)
    * @param ownerName Object owner name
    * @param description Description
    */
   EventReference(EventReferenceType type, uint32_t id, const uuid& guid, const TCHAR *name, uint32_t ownerId, const uuid& ownerGuid, const TCHAR *ownerName, const TCHAR *description = nullptr)
      : m_guid(guid), m_name(name), m_ownerGuid(ownerGuid), m_ownerName(ownerName), m_description(description)
   {
      m_type = type;
      m_id = id;
      m_ownerId = ownerId;
   }

   /**
    * Create new event reference.
    * @param type Reference type
    * @param id Object id within it's type
    * @param guid Object guid
    * @param description Description
    */
   EventReference(EventReferenceType type, uint32_t id, const uuid& guid, const TCHAR *name, const TCHAR *description = nullptr)
      : m_guid(guid), m_name(name), m_description(description)
   {
      m_type = type;
      m_id = id;
      m_ownerId = 0;
   }

   /**
    * Create new event reference.
    * @param type Reference type
    * @param description Description
    */
   EventReference(EventReferenceType type, const TCHAR *description = nullptr) : m_description(description)
   {
      m_type = type;
      m_id = 0;
      m_ownerId = 0;
   }

   /**
    * Fill NXCPMessage.
    * @param msg message to fill
    * @param baseId base field ID
    */
   void fillMessage(NXCPMessage* msg, uint32_t baseId) const
   {
      msg->setField(baseId, static_cast<int16_t>(m_type));
      msg->setField(baseId + 1, m_id);
      msg->setField(baseId + 2, m_guid);
      msg->setField(baseId + 3, m_name);
      msg->setField(baseId + 4, m_ownerId);
      msg->setField(baseId + 5, m_ownerGuid);
      msg->setField(baseId + 6, m_ownerName);
      msg->setField(baseId + 7, m_description);
   }
};

/**
 * Event policy
 */
class EventPolicy
{
private:
   ObjectArray<EPRule> m_rules;
   RWLock m_rwlock;

   void readLock() const { m_rwlock.readLock(); }
   void writeLock() { m_rwlock.writeLock(); }
   void unlock() const { m_rwlock.unlock(); }
   int findRuleIndexByGuid(const uuid& guid, int shift = 0) const;

public:
   EventPolicy() : m_rules(128, 128, Ownership::True) { }

   uint32_t getNumRules() const { return m_rules.size(); }
   bool loadFromDB();
   bool saveToDB() const;
   void processEvent(Event *pEvent);
   void sendToClient(ClientSession *session, uint32_t requestId) const;
   void replacePolicy(uint32_t numRules, EPRule **ruleList);
   void exportRule(StringBuffer& xml, const uuid& guid) const;
   void exportRuleOrgering(StringBuffer& xml) const;
   void importRule(EPRule *rule, bool overwrite, ObjectArray<uuid> *ruleOrdering);
   json_t *toJson() const;

   bool isActionInUse(uint32_t actionId) const;
   bool isCategoryInUse(uint32_t categoryId) const;

   void getEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences) const;
};

/**
 * Stats for event processing thread
 */
struct EventProcessingThreadStats
{
   uint64_t processedEvents;
   uint32_t averageWaitTime;
   uint32_t maxWaitTime;
   uint32_t queueSize;
   uint32_t bindings;
};

/**
 * Functions
 */
bool InitEventSubsystem();
void ShutdownEventSubsystem();
void ReloadEvents();
uint32_t UpdateEventTemplate(const NXCPMessage& request, NXCPMessage *response, json_t **oldValue, json_t **newValue);
uint32_t DeleteEventTemplate(uint32_t eventCode);
void GetEventConfiguration(NXCPMessage *msg);
void CreateEventTemplateExportRecord(StringBuffer &str, uint32_t eventCode);

void CorrelateEvent(Event *event);
Event *LoadEventFromDatabase(uint64_t eventId);
Event *FindEventInLoggerQueue(uint64_t eventId);
StructArray<EventProcessingThreadStats> *GetEventProcessingThreadStats();

bool EventNameFromCode(UINT32 eventCode, TCHAR *buffer);
uint32_t NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, uint32_t defaultValue = 0);
shared_ptr<EventTemplate> FindEventTemplateByCode(uint32_t code);
shared_ptr<EventTemplate> FindEventTemplateByName(const TCHAR *name);

bool NXCORE_EXPORTABLE PostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const StringList& parameters);
bool NXCORE_EXPORTABLE PostSystemEvent(uint32_t eventCode, uint32_t sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostDciEvent(uint32_t eventCode, uint32_t sourceId, uint32_t dciId, const char *format, ...);
uint64_t NXCORE_EXPORTABLE PostEvent2(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const char *format, ...);
uint64_t NXCORE_EXPORTABLE PostSystemEvent2(uint32_t eventCode, uint32_t sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostSystemEventWithNames(uint32_t eventCode, uint32_t sourceId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostSystemEventWithNames(uint32_t eventCode, uint32_t sourceId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostDciEventWithNames(uint32_t eventCode, uint32_t sourceId, uint32_t dciId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostDciEventWithNames(uint32_t eventCode, uint32_t sourceId, uint32_t dciId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const TCHAR *tag, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const TCHAR *tag, StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTagsAndNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const StringSet *tags, const StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTag(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const TCHAR *tag, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventWithTag(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const TCHAR *tag, const StringList& parameters);
bool NXCORE_EXPORTABLE PostEventEx(ObjectQueue<Event> *queue, uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostSystemEventEx(ObjectQueue<Event> *queue, uint32_t eventCode, uint32_t sourceId, const char *format, ...);
void NXCORE_EXPORTABLE ResendEvents(ObjectQueue<Event> *queue);
bool NXCORE_EXPORTABLE TransformAndPostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm);
bool NXCORE_EXPORTABLE TransformAndPostSystemEvent(uint32_t eventCode, uint32_t sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm);

const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps);

/**
 * Global variables
 */
extern ObjectQueue<Event> g_eventQueue;
extern EventPolicy *g_pEventPolicy;
extern VolatileCounter64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
