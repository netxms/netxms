/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: events.cpp
**
**/

#include "nxcore.h"

#ifdef _WIN32
#pragma warning(disable : 4700)
#endif

void GetSNMPTrapsEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences);
void GetSyslogEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences);
void GetWindowsEventLogEventReferences(uint32_t eventCode, ObjectArray<EventReference>* eventReferences);

/**
 * Event processing queue
 */
ObjectQueue<Event> g_eventQueue(4096, Ownership::True);

/**
 * Event processing policy
 */
EventPolicy *g_pEventPolicy = nullptr;

/**
 * Unique event ID
 */
static VolatileCounter64 s_eventId = 0;

/**
 * Getter for last event ID
 */
int64_t GetLastEventId()
{
   return s_eventId;
}

/**
 * Load last event ID
 */
void LoadLastEventId(DB_HANDLE hdb)
{
   int64_t id = ConfigReadInt64(_T("LastEventId"), 0);
   if (id > s_eventId)
      s_eventId = id;
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(event_id) FROM event_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_eventId = std::max(static_cast<int64_t>(s_eventId), DBGetFieldInt64(hResult, 0, 0));
      DBFreeResult(hResult);
   }
}

/**
 * Static data
 */
static SharedHashMap<UINT32, EventTemplate> s_eventTemplates;
static SharedStringObjectMap<EventTemplate> s_eventNameIndex;
static RWLock s_eventTemplatesLock;

#if VA_LIST_IS_POINTER
#define DUMMY_VA_LIST   nullptr
#else
static va_list s_dummy_va_list;
#define DUMMY_VA_LIST   s_dummy_va_list
#endif

/**
 * Create event template from DB record
 */
EventTemplate::EventTemplate(DB_RESULT hResult, int row)
{
   m_code = DBGetFieldULong(hResult, row, 0);
   m_description = DBGetField(hResult, row, 1, nullptr, 0);
   DBGetField(hResult, row, 2, m_name, MAX_EVENT_NAME);
   m_guid = DBGetFieldGUID(hResult, row, 3);
   m_severity = DBGetFieldLong(hResult, row, 4);
   m_flags = DBGetFieldLong(hResult, row, 5);
   m_messageTemplate = DBGetField(hResult, row, 6, nullptr, 0);
   m_tags = DBGetField(hResult, row, 7, nullptr, 0);
}

/**
 * Create event template from message
 */
EventTemplate::EventTemplate(const NXCPMessage& msg)
{
   m_guid = uuid::generate();
   m_code = CreateUniqueId(IDG_EVENT);
   msg.getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_severity = msg.getFieldAsInt32(VID_SEVERITY);
   m_flags = msg.getFieldAsInt32(VID_FLAGS);
   m_messageTemplate = msg.getFieldAsString(VID_MESSAGE);
   m_description = msg.getFieldAsString(VID_DESCRIPTION);
   m_tags = msg.getFieldAsString(VID_TAGS);
}

/**
 * Event template destructor
 */
EventTemplate::~EventTemplate()
{
   MemFree(m_messageTemplate);
   MemFree(m_description);
   MemFree(m_tags);
}

/**
 * Convert event template to JSON
 */
json_t *EventTemplate::toJson() const
{
   json_t *root = json_object();

   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "code", json_integer(m_code));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "severity", json_integer(m_severity));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "message", json_string_t(m_messageTemplate));

   if ((m_tags != nullptr) && (*m_tags != 0))
   {
      json_t *array = json_array();
      String::split(m_tags, _T(","), false,
         [array] (const String& s)
         {
            json_array_append_new(array, json_string_t(s));
         });
      json_object_set_new(root, "tags", array);
   }
   else
   {
      json_object_set_new(root, "tags", json_array());
   }

   return root;
}

/**
 * Modify event template from message
 */
void EventTemplate::modifyFromMessage(const NXCPMessage& msg)
{
   m_code = msg.getFieldAsUInt32(VID_EVENT_CODE);
   msg.getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_severity = msg.getFieldAsInt32(VID_SEVERITY);
   m_flags = msg.getFieldAsInt32(VID_FLAGS);
   MemFree(m_messageTemplate);
   m_messageTemplate = msg.getFieldAsString(VID_MESSAGE);
   MemFree(m_description);
   m_description = msg.getFieldAsString(VID_DESCRIPTION);
   MemFree(m_tags);
   m_tags = msg.getFieldAsString(VID_TAGS);
}

/**
 * Fill message with event template data
 */
void EventTemplate::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base + 1, m_code);
   msg->setField(base + 2, m_description);
   msg->setField(base + 3, m_name);
   msg->setField(base + 4, m_severity);
   msg->setField(base + 5, m_flags);
   msg->setField(base + 6, m_messageTemplate);
   msg->setField(base + 7, m_tags);
   msg->setField(base + 8, m_guid);
}

/**
 * Save event template to database
 */
bool EventTemplate::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;
   bool success = false;

   bool recordExists = IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), m_code);
   if (recordExists)
      hStmt = DBPrepare(hdb, _T("UPDATE event_cfg SET event_name=?,severity=?,flags=?,message=?,description=?,tags=? WHERE event_code=?"));
   else
      hStmt = DBPrepare(hdb, _T("INSERT INTO event_cfg (event_name,severity,flags,message,description,tags,event_code,guid) VALUES (?,?,?,?,?,?,?,?)"));

   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_severity);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_messageTemplate, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH - 1);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_tags, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_code);

      if (!recordExists)
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, uuid::generate());

      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return success;
}

/**
 * Default constructor for event
 */
Event::Event()
{
   m_id = 0;
	m_name[0] = 0;
   m_rootId = 0;
   m_code = 0;
   m_severity = SEVERITY_NORMAL;
   m_origin = EventOrigin::SYSTEM;
   m_sourceId = 0;
   m_zoneUIN = 0;
   m_dciId = 0;
   m_flags = 0;
   m_messageText = nullptr;
   m_messageTemplate = nullptr;
   m_timestamp = 0;
   m_originTimestamp = 0;
	m_customMessage = nullptr;
	m_queueTime = 0;
	m_queueBinding = nullptr;
}

/**
 * Copy constructor for event
 */
Event::Event(const Event *src) : m_lastAlarmKey(src->m_lastAlarmKey), m_lastAlarmMessage(src->m_lastAlarmMessage)
{
   m_id = src->m_id;
   _tcscpy(m_name, src->m_name);
   m_rootId = src->m_rootId;
   m_code = src->m_code;
   m_severity = src->m_severity;
   m_origin = src->m_origin;
   m_sourceId = src->m_sourceId;
   m_zoneUIN = src->m_zoneUIN;
   m_dciId = src->m_dciId;
   m_flags = src->m_flags;
   m_messageText = MemCopyString(src->m_messageText);
   m_messageTemplate = MemCopyString(src->m_messageTemplate);
   m_timestamp = src->m_timestamp;
   m_originTimestamp = src->m_originTimestamp;
   m_tags.addAll(src->m_tags);
	m_customMessage = MemCopyString(src->m_customMessage);
   m_queueTime = src->m_queueTime;
   m_queueBinding = src->m_queueBinding;
   m_parameters.addAll(src->m_parameters);
   m_parameterNames.addAll(&src->m_parameterNames);
}

/**
 * Create event object from serialized form
 */
Event *Event::createFromJson(json_t *json)
{
   Event *event = new Event();

   json_int_t id, rootId, timestamp, originTimestamp;
   int origin;
   const char *name = nullptr, *message = nullptr, *lastAlarmKey = nullptr, *lastAlarmMessage = nullptr;
   json_t *tags = nullptr;
   if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:I, s:i, s:i, s:i, s:i, s:i, s:s, s:s, s:s, s:o}",
            "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
            "originTimestamp", &originTimestamp, "origin", &origin, "source", &event->m_sourceId,
            "zone", &event->m_zoneUIN, "dci", &event->m_dciId, "severity", &event->m_severity,
            "message", &message, "lastAlarmKey", &lastAlarmKey, "lastAlarmMessage", &lastAlarmMessage,
            (json_object_get(json, "tags") != nullptr) ? "tags" : "tag", &tags) == -1)
   {
      if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:I, s:i, s:i, s:i, s:i, s:i, s:s, s:s, s:o}",
               "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
               "originTimestamp", &originTimestamp, "origin", &origin, "source", &event->m_sourceId,
               "zone", &event->m_zoneUIN, "dci", &event->m_dciId, "severity", &event->m_severity,
               "message", &message, "lastAlarmKey", &lastAlarmKey,
               (json_object_get(json, "tags") != nullptr) ? "tags" : "tag", &tags) == -1)
      {
         if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:I, s:i, s:i, s:i, s:i, s:i, s:s, s:o}",
                  "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
                  "originTimestamp", &originTimestamp, "origin", &origin, "source", &event->m_sourceId,
                  "zone", &event->m_zoneUIN, "dci", &event->m_dciId, "severity", &event->m_severity, "message", &message,
                  (json_object_get(json, "tags") != nullptr) ? "tags" : "tag", &tags) == -1)
         {
            if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:i, s:i, s:i, s:i, s:s, s:o}",
                     "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
                     "source", &event->m_sourceId, "zone", &event->m_zoneUIN, "dci", &event->m_dciId,
                     "severity", &event->m_severity, "message", &message,
                     (json_object_get(json, "tags") != nullptr) ? "tags" : "tag", &tags) == -1)
            {
               delete event;
               return nullptr;   // Unpack failure
            }
            originTimestamp = timestamp;
         }
      }
   }

   event->m_id = id;
   event->m_rootId = rootId;
   event->m_origin = static_cast<EventOrigin>(origin);
   event->m_timestamp = timestamp;
   event->m_originTimestamp = originTimestamp;
   if (name != nullptr)
   {
#ifdef UNICODE
      utf8_to_wchar(name, -1, event->m_name, MAX_EVENT_NAME);
#else
      utf8_to_mb(name, -1, event->m_name, MAX_EVENT_NAME);
#endif
   }
#ifdef UNICODE
   event->m_messageText = WideStringFromUTF8String(message);
#else
   event->m_messageText = MBStringFromUTF8String(message);
#endif
   if (lastAlarmKey != nullptr)
   {
#ifdef UNICODE
      WCHAR s[MAX_DB_STRING];
      utf8_to_wchar(lastAlarmKey, -1, s, MAX_DB_STRING);
#else
      char s[MAX_DB_STRING];
      utf8_to_mb(lastAlarmKey, -1, s, MAX_DB_STRING);
#endif
      event->m_lastAlarmKey = s;
   }
   if (lastAlarmMessage != nullptr)
   {
#ifdef UNICODE
      WCHAR s[MAX_EVENT_MSG_LENGTH];
      utf8_to_wchar(lastAlarmMessage, -1, s, MAX_EVENT_MSG_LENGTH);
#else
      char s[MAX_EVENT_MSG_LENGTH];
      utf8_to_mb(lastAlarmMessage, -1, s, MAX_EVENT_MSG_LENGTH);
#endif
      event->m_lastAlarmMessage = s;
   }
   if (tags != nullptr)
   {
      if (json_is_array(tags))
      {
         size_t count = json_array_size(tags);
         for(size_t i = 0; i < count; i++)
         {
            json_t *e = json_array_get(tags, i);
            if (json_is_string(e))
            {
#ifdef UNICODE
               event->m_tags.addPreallocated(WideStringFromUTF8String(json_string_value(e)));
#else
               event->m_tags.addPreallocated(MBStringFromUTF8String(json_string_value(e)));
#endif
            }
         }
      }
      else if (json_is_string(tags))
      {
#ifdef UNICODE
         event->m_tags.addPreallocated(WideStringFromUTF8String(json_string_value(tags)));
#else
         event->m_tags.addPreallocated(MBStringFromUTF8String(json_string_value(tags)));
#endif
      }
   }

   json_t *parameters = json_object_get(json, "parameters");
   if (parameters != nullptr)
   {
      char *value;
      size_t count = json_array_size(parameters);
      for(size_t i = 0; i < count; i++)
      {
         json_t *p = json_array_get(parameters, i);
         name = value = nullptr;
         if (json_unpack(p, "{s:s, s:s}", "name", &name, "value", &value) != -1)
         {
#ifdef UNICODE
            event->m_parameters.addPreallocated(WideStringFromUTF8String(CHECK_NULL_EX_A(value)));
            event->m_parameterNames.addPreallocated(WideStringFromUTF8String(CHECK_NULL_EX_A(name)));
#else
            event->m_parameters.addPreallocated(MBStringFromUTF8String(CHECK_NULL_EX_A(value)));
            event->m_parameterNames.addPreallocated(MBStringFromUTF8String(CHECK_NULL_EX_A(name)));
#endif
         }
         else
         {
            event->m_parameters.add(_T(""));
            event->m_parameterNames.add(_T(""));
         }
      }
   }

   return event;
}

/**
 * Construct event from template
 */
Event::Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId,
         uint32_t dciId, const char *format, const TCHAR **names, va_list args)
{
   init(eventTemplate, origin, originTimestamp, sourceId, dciId);

   // Create parameters
   if (format != nullptr)
   {
		int count = (int)strlen(format);
		TCHAR *buffer;

      for(int i = 0; i < count; i++)
      {
         switch(format[i])
         {
            case 's':
               {
                  const TCHAR *s = va_arg(args, const TCHAR *);
					   m_parameters.add(CHECK_NULL_EX(s));
               }
               break;
            case 'm':	// multibyte string
               {
                  const char *s = va_arg(args, const char *);
#ifdef UNICODE
                  m_parameters.addPreallocated((s != nullptr) ? WideStringFromMBString(s) : MemCopyStringW(L""));
#else
					   m_parameters.add(CHECK_NULL_EX_A(s));
#endif
               }
               break;
            case 'u':	// UNICODE string
               {
                  const WCHAR *s = va_arg(args, const WCHAR *);
#ifdef UNICODE
		   			m_parameters.add(CHECK_NULL_EX_W(s));
#else
                  m_parameters.addPreallocated((s != nullptr) ? MBStringFromWideString(s) : MemCopyStringA(""));
#endif
               }
               break;
            case 'd':
					m_parameters.add(va_arg(args, int32_t));
               break;
            case 'D':
               m_parameters.add(va_arg(args, int64_t));
               break;
            case 't':
               m_parameters.add(static_cast<uint64_t>(va_arg(args, time_t)));
               break;
            case 'x':
            case 'i':
               buffer = MemAllocString(16);
               _sntprintf(buffer, 16, _T("0x%08X"), va_arg(args, uint32_t));
					m_parameters.addPreallocated(buffer);
               break;
            case 'f':
               m_parameters.add(va_arg(args, double));
               break;
            case 'a':   // IPv4 address
					m_parameters.addPreallocated(IpToStr(va_arg(args, uint32_t), MemAllocString(16)));
               break;
            case 'A':
					m_parameters.add((va_arg(args, InetAddress *))->toString());
               break;
            case 'h':
               buffer = MemAllocString(64);
               MACToStr(va_arg(args, BYTE *), buffer);
					m_parameters.addPreallocated(buffer);
               break;
            case 'H':
               m_parameters.add((va_arg(args, MacAddress *))->toString());
               break;
            case 'G':   // uuid object (GUID)
               m_parameters.add((va_arg(args, uuid *))->toString());
               break;
            default:
               buffer = MemAllocString(64);
               _sntprintf(buffer, 64, _T("BAD FORMAT \"%c\" [value = 0x%08X]"), format[i], va_arg(args, UINT32));
					m_parameters.addPreallocated(buffer);
               break;
         }
			m_parameterNames.add(((names != nullptr) && (names[i] != nullptr)) ? names[i] : _T(""));
      }
   }
}

/**
 * Create event from template
 */
Event::Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, uint32_t dciId, const StringMap& args)
{
   init(eventTemplate, origin, originTimestamp, sourceId, dciId);
   auto it = args.begin();
   while(it.hasNext())
   {
      auto p = it.next();
      m_parameterNames.add(p->key);
      m_parameters.add(p->value);
   }
}

/**
 * Common initialization code
 */
void Event::init(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, uint32_t dciId)
{
   initFromTemplate(eventTemplate);
   m_origin = origin;
   m_originTimestamp = (originTimestamp != 0) ? originTimestamp : m_timestamp;
   m_dciId = dciId;
   m_queueTime = 0;
   m_queueBinding = nullptr;
   m_messageText = nullptr;
   m_rootId = 0;
   m_customMessage = nullptr;
   setSource(sourceId);
}

/**
 * Set event source (intended to be called only by constructor or event builder)
 */
void Event::setSource(uint32_t sourceId)
{
   m_sourceId = sourceId;

   shared_ptr<NetObj> source = FindObjectById(sourceId);
   if (source != nullptr)
   {
      switch(source->getObjectClass())
      {
         case OBJECT_NODE:
            m_zoneUIN = static_cast<Node*>(source.get())->getZoneUIN();
            break;
         case OBJECT_CLUSTER:
            m_zoneUIN = static_cast<Cluster*>(source.get())->getZoneUIN();
            break;
         case OBJECT_INTERFACE:
            m_zoneUIN = static_cast<Interface*>(source.get())->getZoneUIN();
            break;
         case OBJECT_SUBNET:
            m_zoneUIN = static_cast<Subnet*>(source.get())->getZoneUIN();
            break;
         default:
            m_zoneUIN = 0;
            break;
      }
   }
   else
   {
      m_zoneUIN = 0;
   }
}

/**
 * Common initialization code
 */
void Event::initFromTemplate(const EventTemplate *eventTemplate)
{
   _tcscpy(m_name, eventTemplate->getName());
   m_timestamp = time(nullptr);
   m_id = InterlockedIncrement64(&s_eventId);
   m_code = eventTemplate->getCode();
   m_severity = eventTemplate->getSeverity();
   m_flags = eventTemplate->getFlags();
   m_messageTemplate = MemCopyString(eventTemplate->getMessageTemplate());

   if ((eventTemplate->getTags() != nullptr) && (eventTemplate->getTags()[0] != 0))
      m_tags.splitAndAdd(eventTemplate->getTags(), _T(","));
}

/**
 * Destructor for event
 */
Event::~Event()
{
   MemFree(m_messageText);
   MemFree(m_messageTemplate);
	MemFree(m_customMessage);
}

/**
 * Create message text from template
 */
void Event::expandMessageText()
{
   if (m_messageTemplate == nullptr)
      return;

   if (m_messageText != nullptr)
      MemFree(m_messageText);
   m_messageText = MemCopyString(expandText(m_messageTemplate));
}

/**
 * Substitute % macros in given text with actual values
 */
StringBuffer Event::expandText(const TCHAR *textTemplate, const Alarm *alarm) const
{
   if (textTemplate == nullptr)
      return StringBuffer();

   shared_ptr<NetObj> object = FindObjectById(m_sourceId);
   if (object == nullptr)
   {
     object = FindObjectById(g_dwMgmtNode);
     if (object == nullptr)
        object = g_entireNetwork;
   }

   return object->expandText(textTemplate, alarm, this, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, nullptr);
}

/**
 * Add new parameter to event
 */
void Event::addParameter(const TCHAR *name, const TCHAR *value)
{
	m_parameters.add(value);
	m_parameterNames.add(name);
}

/**
 * Set value of named parameter
 */
void Event::setNamedParameter(const TCHAR *name, const TCHAR *value)
{
   if ((name == nullptr) || (name[0] == 0))
      return;

	int index = m_parameterNames.indexOfIgnoreCase(name);
	if (index != -1)
	{
		m_parameters.replace(index, value);
	}
	else
	{
		m_parameters.add(value);
		m_parameterNames.add(name);
	}
}

/**
 * Set value (and optionally name) of parameter at given index.
 *
 * @param index 0-based parameter index
 * @param name parameter name (can be nullptr)
 * @param value new value
 */
void Event::setParameter(int index, const TCHAR *name, const TCHAR *value)
{
   if (index < 0)
      return;

   int addup = index - m_parameters.size();
   for(int i = 0; i < addup; i++)
   {
		m_parameters.add(_T(""));
		m_parameterNames.add(_T(""));
   }
   if (index < m_parameters.size())
   {
		m_parameters.replace(index,value);
		if (name != nullptr)
		   m_parameterNames.replace(index, name);
   }
   else
   {
		m_parameters.add(value);
		m_parameterNames.add(CHECK_NULL_EX(name));
   }
}

/**
 * Fill message with event data
 */
void Event::prepareMessage(NXCPMessage *msg) const
{
	msg->setField(VID_NUM_RECORDS, static_cast<uint32_t>(1));
	msg->setField(VID_RECORDS_ORDER, static_cast<uint16_t>(RECORD_ORDER_NORMAL));

	uint32_t fieldId = VID_EVENTLOG_MSG_BASE;
	msg->setField(fieldId++, m_id);
	msg->setField(fieldId++, m_code);
	msg->setField(fieldId++, (UINT32)m_timestamp);
	msg->setField(fieldId++, m_sourceId);
	msg->setField(fieldId++, (WORD)m_severity);
	msg->setField(fieldId++, CHECK_NULL_EX(m_messageText));
	msg->setField(fieldId++, getTagsAsList());
	msg->setField(fieldId++, (UINT32)m_parameters.size());
	for(int i = 0; i < m_parameters.size(); i++)
	   msg->setField(fieldId++, m_parameters.get(i));
	msg->setField(fieldId++, m_dciId);
}

/**
 * Get all tags as list
 */
StringBuffer Event::getTagsAsList() const
{
   StringBuffer list;
   getTagsAsList(&list);
   return list;
}

/**
 * Get all tags as list
 */
void Event::getTagsAsList(StringBuffer *sb) const
{
   if (m_tags.isEmpty())
      return;

   auto it = m_tags.begin();
   sb->append(it.next());
   while(it.hasNext())
   {
      sb->append(_T(','));
      sb->append(it.next());
   }
}

/**
 * Convert event to JSON representation
 */
json_t *Event::toJson()
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "rootId", json_integer(m_rootId));
   json_object_set_new(root, "code", json_integer(m_code));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "timestamp", json_integer(m_timestamp));
   json_object_set_new(root, "originTimestamp", json_integer(m_originTimestamp));
   json_object_set_new(root, "origin", json_integer(static_cast<int>(m_origin)));
   json_object_set_new(root, "source", json_integer(m_sourceId));
   json_object_set_new(root, "zone", json_integer(m_zoneUIN));
   json_object_set_new(root, "dci", json_integer(m_dciId));
   json_object_set_new(root, "severity", json_integer(m_severity));
   json_object_set_new(root, "message", json_string_t(m_messageText));
   json_object_set_new(root, "lastAlarmKey", json_string_t(m_lastAlarmKey));
   json_object_set_new(root, "lastAlarmMessage", json_string_t(m_lastAlarmMessage));

   if (!m_tags.isEmpty())
   {
      json_t *tags = json_array();
      auto it = m_tags.begin();
      while(it.hasNext())
      {
         json_array_append_new(tags, json_string_t(it.next()));
      }
      json_object_set_new(root, "tags", tags);
   }
   else
   {
      json_object_set_new(root, "tags", json_null());
   }

   json_t *parameters = json_array();
   for(int i = 0; i < m_parameters.size(); i++)
   {
      json_t *p = json_object();
      json_object_set_new(p, "name", json_string_t(m_parameterNames.get(i)));
      json_object_set_new(p, "value", json_string_t(m_parameters.get(i)));
      json_array_append_new(parameters, p);
   }
   json_object_set_new(root, "parameters", parameters);

   return root;
}

/**
 * Load event from database
 */
Event *LoadEventFromDatabase(uint64_t eventId)
{
   if (eventId == 0)
      return nullptr;

   Event *event = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT raw_data FROM event_log WHERE event_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, eventId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            char *data = DBGetFieldUTF8(hResult, 0, 0, nullptr, 0);
            if (data != nullptr)
            {
               char *pdata = nullptr;
               if (data[0] != '{')
               {
                  // Event data serialized with JSON_EMBED, so add { } for decoding
                  pdata = MemAllocArray<char>(strlen(data) + 3);
                  pdata[0] = '{';
                  strcpy(&pdata[1], data);
                  strcat(pdata, "}");
               }
               json_t *json = json_loads((pdata != nullptr) ? pdata : data, 0, nullptr);
               if (json != nullptr)
               {
                  event = Event::createFromJson(json);
                  json_decref(json);
               }
               MemFree(pdata);
               MemFree(data);
            }
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return event;
}

/**
 * Load event configuration from database
 */
static bool LoadEventConfiguration()
{
   bool success;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT event_code,description,event_name,guid,severity,flags,message,tags FROM event_cfg"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         auto t = make_shared<EventTemplate>(hResult, i);
         s_eventTemplates.set(t->getCode(), t);
         s_eventNameIndex.set(t->getName(), t);
      }
      DBFreeResult(hResult);
      success = true;
   }
   else
   {
      nxlog_write_tag(NXLOG_ERROR, _T("event.db"), _T("Unable to load event templates from database"));
      success = false;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Inilialize event handling subsystem
 */
bool InitEventSubsystem()
{
   // Load events from database
   bool success = LoadEventConfiguration();

   // Create and initialize event processing policy
   if (success)
   {
      g_pEventPolicy = new EventPolicy;
      if (!g_pEventPolicy->loadFromDB())
      {
         success = FALSE;
         nxlog_write_tag(NXLOG_ERROR, _T("event.db"), _T("Error loading event processing policy from database"));
         delete g_pEventPolicy;
      }
   }

   return success;
}

/**
 * Shutdown event subsystem
 */
void ShutdownEventSubsystem()
{
   delete g_pEventPolicy;
}

/**
 * Reload event templates from database
 */
void ReloadEvents()
{
   s_eventTemplatesLock.writeLock();
   s_eventTemplates.clear();
   s_eventNameIndex.clear();
   LoadEventConfiguration();
   s_eventTemplatesLock.unlock();
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param eventTag event's tag (can be nullptr)
 * @param eventTags event's tags (use if multiple tags need to be added, can be nullptr)
 * @param namedArgs named arguments for event - if not nullptr format and args will be ignored
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 * @param names names for parameters (nullptr if parameters are unnamed)
 * @param args event parameters
 * @param vm NXSL VM for transformation script
 */
static bool RealPostEvent(ObjectQueue<Event> *queue, uint64_t *eventId, uint32_t eventCode, EventOrigin origin,
         time_t originTimestamp, uint32_t sourceId, uint32_t dciId, const TCHAR *eventTag, const StringSet *eventTags,
         const StringMap *namedArgs, const char *format, const TCHAR **names, va_list args, NXSL_VM *vm)
{
   // Check that source object exists
   if ((sourceId == 0) || (FindObjectById(sourceId) == nullptr))
   {
      nxlog_debug_tag(_T("event.proc"), 3, _T("RealPostEvent: invalid event source object ID %u for event with code %u and origin %d"),
            sourceId, eventCode, (int)origin);
      return false;
   }

   s_eventTemplatesLock.readLock();
   shared_ptr<EventTemplate> eventTemplate = s_eventTemplates.getShared(eventCode);
   s_eventTemplatesLock.unlock();

   bool success;
   if (eventTemplate != nullptr)
   {
      // Template found, create new event
      Event *event = (namedArgs != nullptr) ?
               new Event(eventTemplate.get(), origin, originTimestamp, sourceId, dciId, *namedArgs) :
               new Event(eventTemplate.get(), origin, originTimestamp, sourceId, dciId, format, names, args);
      if (eventId != nullptr)
         *eventId = event->getId();
      if (eventTag != nullptr)
         event->addTag(eventTag);
      if (eventTags != nullptr)
      {
         eventTags->forEach([](const TCHAR *tag, void *event) { static_cast<Event*>(event)->addTag(tag); return true; }, event);
      }

      // Using transformation within PostEvent may cause deadlock if called from within locked object or DCI
      // Caller of PostEvent should make sure that it does not held object, object index, or DCI locks
      if (vm != nullptr)
      {
         vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));
         if (!vm->run())
         {
            nxlog_debug(6, _T("RealPostEvent: Script execution error (%s)"), vm->getErrorText());
         }
      }

      // Add new event to queue
      queue->put(event);

      success = true;
   }
   else
   {
      nxlog_debug_tag(_T("event.proc"), 3, _T("RealPostEvent: event with code %u not defined"), eventCode);
      success = false;
   }
   return success;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 *        f - floating point number (double)
 */
bool NXCORE_EXPORTABLE PostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 *        f - floating point number (double)
 */
bool NXCORE_EXPORTABLE PostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, const StringList& parameters)
{
   StringMap pmap;
   for(int i = 0; i < parameters.size(); i++)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("Parameter%d"), i + 1);
      pmap.set(name, parameters.get(i));
   }
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, nullptr, &pmap, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post event to system event queue with origin SYSTEM.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 *        f - floating point number (double)
 */
bool NXCORE_EXPORTABLE PostSystemEvent(uint32_t eventCode, uint32_t sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post DCI-related event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param dciId DCI ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 *        f - floating point number (double)
 */
bool NXCORE_EXPORTABLE PostDciEvent(uint32_t eventCode, uint32_t sourceId, uint32_t dciId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, dciId, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event with origin SYSTEM to system event queue and return ID of new event (0 in case of failure).
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 *        f - floating point number (double)
 */
uint64_t NXCORE_EXPORTABLE PostSystemEvent2(uint32_t eventCode, uint32_t sourceId, const char *format, ...)
{
   va_list args;
   uint64_t eventId;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, &eventId, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success ? eventId : 0;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 * @param names names for parameters (nullptr if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostEventWithNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp,
         uint32_t sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, nullptr, nullptr, format, names, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param parameters event parameters list
 */
bool NXCORE_EXPORTABLE PostEventWithNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp, uint32_t sourceId, StringMap *parameters)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, nullptr, parameters, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post event with SYSTEM origin to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 * @param names names for parameters (nullptr if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostSystemEventWithNames(uint32_t eventCode, uint32_t sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, nullptr, nullptr, nullptr, format, names, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event with SYSTEM origin to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param parameters event parameters list
 */
bool NXCORE_EXPORTABLE PostSystemEventWithNames(uint32_t eventCode, uint32_t sourceId, StringMap *parameters)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, nullptr, nullptr, parameters, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post DCI-related event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param dciId DCI ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 * @param names names for parameters (nullptr if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostDciEventWithNames(uint32_t eventCode, uint32_t sourceId, uint32_t dciId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, dciId, nullptr, nullptr, nullptr, format, names, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param tag Event tag
 * @param parameters Named event parameters
 */
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp,
         uint32_t sourceId, const TCHAR *tag, const StringMap *parameters)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, tag, nullptr, parameters, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param tag Event tag
 * @param parameters Named event parameters
 */
bool NXCORE_EXPORTABLE PostEventWithTagsAndNames(uint32_t eventCode, EventOrigin origin, time_t originTimestamp,
         uint32_t sourceId, const StringSet *tags, const StringMap *parameters)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, tags, parameters, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param userTag event's user tag
 * @param parameters event parameters
 */
bool NXCORE_EXPORTABLE PostEventWithTag(uint32_t eventCode, EventOrigin origin, time_t originTimestamp,
         uint32_t sourceId, const TCHAR *userTag, const StringList& parameters)
{
   StringMap pmap;
   for(int i = 0; i < parameters.size(); i++)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("Parameter%d"), i + 1);
      pmap.set(name, parameters.get(i));
   }
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, userTag, nullptr, &pmap, nullptr, nullptr, DUMMY_VA_LIST, nullptr);
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 */
bool NXCORE_EXPORTABLE PostEventEx(ObjectQueue<Event> *queue, uint32_t eventCode, EventOrigin origin,
         time_t originTimestamp, uint32_t sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(queue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Post event with SYSTEM origin to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param format Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        m - Multibyte string
 *        u - UNICODE string
 *        d - Decimal integer
 *        D - 64-bit decimal integer
 *        x - Hex integer
 *        a - IPv4 address
 *        A - InetAddress object
 *        h - MAC (hardware) address as byte array
 *        H - MAC (hardware) address as MacAddress object
 *        G - uuid object (GUID)
 *        i - Object ID
 */
bool NXCORE_EXPORTABLE PostSystemEventEx(ObjectQueue<Event> *queue, uint32_t eventCode, uint32_t sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(queue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, nullptr, nullptr, nullptr, format, nullptr, args, nullptr);
   va_end(args);
   return success;
}

/**
 * Create event, run transformation script, and post to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param tag Event tag
 * @param parameters Named event parameters
 */
bool NXCORE_EXPORTABLE TransformAndPostEvent(uint32_t eventCode, EventOrigin origin, time_t originTimestamp,
         uint32_t sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, origin, originTimestamp, sourceId, 0, tag, nullptr, parameters, nullptr, nullptr, DUMMY_VA_LIST, vm);
}

/**
 * Create event with SYSTEM origin, run transformation script, and post to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param tag Event tag
 * @param parameters Named event parameters
 */
bool NXCORE_EXPORTABLE TransformAndPostSystemEvent(uint32_t eventCode, uint32_t sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm)
{
   return RealPostEvent(&g_eventQueue, nullptr, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, tag, nullptr, parameters, nullptr, nullptr, DUMMY_VA_LIST, vm);
}

/**
 * Add event parameters to builder from NXCP message (message does not contain parameter names)
 */
EventBuilder& EventBuilder::params(const NXCPMessage& msg, uint32_t baseId, uint32_t countId)
{
   int count = msg.getFieldAsInt32(countId);
   TCHAR name[32] = _T("parameter");
   for(int i = 0; i < count; i++)
   {
      IntegerToString(i + 1, &name[9]);
      m_event->m_parameterNames.add(name);
   }
   m_event->m_parameters.addAllFromMessage(msg, baseId, countId);
   return *this;
}

/**
 * Add event parameters to builder from string map
 */
EventBuilder& EventBuilder::params(const StringMap& map)
{
   auto it = map.begin();
   while(it.hasNext())
   {
      auto p = it.next();
      m_event->m_parameterNames.add(p->key);
      m_event->m_parameters.add(p->value);
   }
   return *this;
}

/**
 * Post built event
 */
bool EventBuilder::post(ObjectQueue<Event> *queue, std::function<void (Event*)> callback)
{
   // Check that source object exists
   if ((m_event->m_sourceId == 0) || (FindObjectById(m_event->m_sourceId) == nullptr))
   {
      nxlog_debug_tag(_T("event.proc"), 3, _T("EventBuilder::post: invalid event source object ID %u for event with code %u and origin %d"),
            m_event->m_sourceId, m_eventCode, (int)m_event->m_origin);
      return false;
   }

   s_eventTemplatesLock.readLock();
   shared_ptr<EventTemplate> eventTemplate = s_eventTemplates.getShared(m_eventCode);
   s_eventTemplatesLock.unlock();

   if (eventTemplate == nullptr)
   {
      nxlog_debug_tag(_T("event.proc"), 3, _T("EventBuilder::post: event with code %u not defined"), m_eventCode);
      return false;
   }

   m_event->initFromTemplate(eventTemplate.get());

   if (m_event->m_originTimestamp == 0)
      m_event->m_originTimestamp = m_event->m_timestamp;

   // Using transformation within PostEvent may cause deadlock if called from within locked object or DCI
   // Caller of PostEvent should make sure that it does not held object, object index, or DCI locks
   if (m_vm != nullptr)
   {
      m_vm->setGlobalVariable("$event", m_vm->createValue(m_vm->createObject(&g_nxslEventClass, m_event, true)));
      if (!m_vm->run())
      {
         nxlog_debug_tag(_T("event.proc"), 6, _T("EventBuilder::post: Script execution error (%s)"), m_vm->getErrorText());
      }
   }

   // Execute pre-send callback
   if (callback != nullptr)
      m_event->m_callback = callback;

   // Add new event to m_queue
   if (queue == nullptr)
   {
      g_eventQueue.put(m_event);
   }
   else
   {
      queue->put(m_event);
   }

   m_event = nullptr;
   return true;
}

/**
 * Integer formats for EventBuilder
 */
const TCHAR EventBuilder::OBJECT_ID_FORMAT[8] = _T("0x%08X");
const TCHAR EventBuilder::HEX_32BIT_FORMAT[8] = _T("0x%08X");
const TCHAR EventBuilder::HEX_64BIT_FORMAT[8] = _T("0x%16X");

/**
 * Resend events from specific m_queue to system event m_queue
 */
void NXCORE_EXPORTABLE ResendEvents(ObjectQueue<Event> *queue)
{
   Event *e;
   while ((e = queue->get()) != nullptr)
      g_eventQueue.put(e);
}

/**
 * Create export record for event template
 */
void CreateEventTemplateExportRecord(StringBuffer &str, uint32_t eventCode)
{
   String strText, strDescr;

   s_eventTemplatesLock.readLock();

   // Find event template
   EventTemplate *e = s_eventTemplates.get(eventCode);
   if (e != nullptr)
   {
      str.append(_T("\t\t<event id=\""));
      str.append(e->getCode());
      str.append(_T("\">\n\t\t\t<guid>"));
      str.append(e->getGuid().toString());
      str.append(_T("</guid>\n\t\t\t<name>"));
      str.append(EscapeStringForXML2(e->getName()));
      str.append(_T("</name>\n\t\t\t<code>"));
      str.append(e->getCode());
      str.append(_T("</code>\n\t\t\t<description>"));
      str.append(EscapeStringForXML2(e->getDescription()));
      str.append(_T("</description>\n\t\t\t<severity>"));
      str.append(e->getSeverity());
      str.append(_T("</severity>\n\t\t\t<flags>"));
      str.append(e->getFlags());
      str.append(_T("</flags>\n\t\t\t<message>"));
      str.append(EscapeStringForXML2(e->getMessageTemplate()));
      str.append(_T("</message>\n\t\t\t<tags>"));
      str.append(EscapeStringForXML2(e->getTags()));
      str.append(_T("</tags>\n\t\t</event>\n"));
   }

   s_eventTemplatesLock.unlock();
}

/**
 * Resolve event name
 */
bool EventNameFromCode(uint32_t eventCode, TCHAR *buffer)
{
   bool bRet = false;

   s_eventTemplatesLock.readLock();

   EventTemplate *e = s_eventTemplates.get(eventCode);
   if (e != nullptr)
   {
      _tcscpy(buffer, e->getName());
      bRet = true;
   }
   else
   {
      _tcscpy(buffer, _T("UNKNOWN_EVENT"));
   }

   s_eventTemplatesLock.unlock();
   return bRet;
}

/**
 * Find event template by code - suitable for external call
 */
shared_ptr<EventTemplate> FindEventTemplateByCode(uint32_t code)
{
   s_eventTemplatesLock.readLock();
   shared_ptr<EventTemplate> e = s_eventTemplates.getShared(code);
   s_eventTemplatesLock.unlock();
   return e;
}

/**
 * Find event template by name - suitable for external call
 */
shared_ptr<EventTemplate> FindEventTemplateByName(const TCHAR *name)
{
   s_eventTemplatesLock.readLock();
   shared_ptr<EventTemplate> e = s_eventNameIndex.getShared(name);
   s_eventTemplatesLock.unlock();
   return e;
}

/**
 * Translate event name to code
 * If event with given name does not exist, returns supplied default value
 */
uint32_t NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, uint32_t defaultValue)
{
   shared_ptr<EventTemplate> e = FindEventTemplateByName(name);
	return (e != nullptr) ? e->getCode() : defaultValue;
}

/**
 * Get status as text
 */
const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps)
{
   static const TCHAR *statusText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"), _T("CRITICAL"), _T("UNKNOWN"), _T("UNMANAGED"), _T("DISABLED"), _T("TESTING") };
   static const TCHAR *statusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"), _T("Critical"), _T("Unknown"), _T("Unmanaged"), _T("Disabled"), _T("Testing") };

   if (allCaps)
   {
      return ((status >= STATUS_NORMAL) && (status <= STATUS_TESTING)) ? statusText[status] : _T("INTERNAL ERROR");
   }
   else
   {
      return ((status >= STATUS_NORMAL) && (status <= STATUS_TESTING)) ? statusTextSmall[status] : _T("INTERNAL ERROR");
   }
}

/**
 * Callback for sending event configuration change notifications
 */
static void SendEventDBChangeNotification(ClientSession *session, NXCPMessage *msg)
{
   if (session->isAuthenticated() &&
       (session->checkSystemAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) ||
        session->checkSystemAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) ||
        session->checkSystemAccessRights(SYSTEM_ACCESS_EPP)))
      session->postMessage(msg);
}

/**
 * Update or create new event object from request
 */
uint32_t UpdateEventTemplate(const NXCPMessage& request, NXCPMessage *response, json_t **oldValue, json_t **newValue)
{
   TCHAR name[MAX_EVENT_NAME] = _T("");
   request.getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
   if (!IsValidObjectName(name, TRUE))
      return RCC_INVALID_OBJECT_NAME;

   uint32_t eventCode = request.getFieldAsUInt32(VID_EVENT_CODE);
   shared_ptr<EventTemplate> et = FindEventTemplateByName(name);
   if ((et != nullptr) && (et->getCode() != eventCode))
      return RCC_NAME_ALEARDY_EXISTS;

   s_eventTemplatesLock.writeLock();

   shared_ptr<EventTemplate> e;
   if (eventCode == 0)
   {
      e = make_shared<EventTemplate>(request);
      s_eventTemplates.set(e->getCode(), e);
      s_eventNameIndex.set(e->getName(), e);
      *oldValue = nullptr;
   }
   else
   {
      e = s_eventTemplates.getShared(eventCode);
      if (e == nullptr)
      {
         s_eventTemplatesLock.unlock();
         return RCC_INVALID_EVENT_CODE;
      }
      *oldValue = e->toJson();
      s_eventNameIndex.remove(e->getName());
      e->modifyFromMessage(request);
      s_eventNameIndex.set(e->getName(), e);
   }
   *newValue = e->toJson();
   bool success = e->saveToDatabase();
   if (success)
   {
      NXCPMessage nmsg;
      nmsg.setCode(CMD_EVENT_DB_UPDATE);
      nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_CHANGED);
      e->fillMessage(&nmsg, VID_ELEMENT_LIST_BASE);
      EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);

      response->setField(VID_EVENT_CODE, e->getCode());
   }

   s_eventTemplatesLock.unlock();

   if (!success)
   {
      if (*oldValue != nullptr)
      {
         json_decref(*oldValue);
         *oldValue = nullptr;
      }
      json_decref(*newValue);
      *newValue = nullptr;
      return RCC_DB_FAILURE;
   }

   return RCC_SUCCESS;
}

/**
 * Delete event template
 */
uint32_t DeleteEventTemplate(uint32_t eventCode)
{
   uint32_t rcc = RCC_SUCCESS;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   s_eventTemplatesLock.writeLock();
   auto e = s_eventTemplates.get(eventCode);
   if (e != nullptr)
   {
      rcc = ExecuteQueryOnObject(hdb, eventCode, _T("DELETE FROM event_cfg WHERE event_code=?")) ? RCC_SUCCESS : RCC_DB_FAILURE;
   }
   else
   {
      rcc = RCC_INVALID_EVENT_CODE;
   }

   if (rcc == RCC_SUCCESS)
   {
      s_eventNameIndex.remove(e->getName());
      s_eventTemplates.remove(eventCode);
      NXCPMessage nmsg;
      nmsg.setCode(CMD_EVENT_DB_UPDATE);
      nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_DELETED);
      nmsg.setField(VID_EVENT_CODE, eventCode);
      EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);
   }
   s_eventTemplatesLock.unlock();

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get event configuration
 */
void GetEventConfiguration(NXCPMessage *msg)
{
   s_eventTemplatesLock.writeLock();
   uint32_t base = VID_ELEMENT_LIST_BASE;
   msg->setField(VID_NUM_EVENTS, s_eventTemplates.size());
   auto it = s_eventTemplates.begin();
   while(it.hasNext())
   {
      it.next()->fillMessage(msg, base);
      base += 10;
   }
   s_eventTemplatesLock.unlock();
}

/**
 * Get information about all objects that are using the specified event.
 */
unique_ptr<ObjectArray<EventReference>> GetAllEventReferences(uint32_t eventCode)
{
   ObjectArray<EventReference>* eventReferences = new ObjectArray<EventReference>(0, 64, Ownership::True);

   // Agent Policies + DCI from Templates
   unique_ptr<SharedObjectArray<NetObj>> templates = g_idxObjectById.getObjects([](NetObj* object, void* context) -> bool { return object->getObjectClass() == OBJECT_TEMPLATE; }, nullptr);
   for (int i = 0; i < templates->size(); i++)
   {
      Template* t = static_cast<Template*>(templates->get(i));
      t->getEventReferences(eventCode, eventReferences);
   }

   // Conditions
   unique_ptr<SharedObjectArray<NetObj>> conditions = g_idxConditionById.getObjects();
   for (int i = 0; i < conditions->size(); i++)
   {
      ConditionObject* c = static_cast<ConditionObject*>(conditions->get(i));
      if (c->isUsingEvent(eventCode))
      {
         eventReferences->add(new EventReference(EventReferenceType::CONDITION, c->getId(),  c->getGuid(), c->getName()));
      }
   }

   // DCI from DCTarget
   unique_ptr<SharedObjectArray<NetObj>> dct = g_idxObjectById.getObjects([](NetObj* object, void* context) -> bool { return object->isDataCollectionTarget(); }, nullptr);
   for (int i = 0; i < dct->size(); i++)
   {
      DataCollectionTarget* target = static_cast<DataCollectionTarget*>(dct->get(i));
      target->getEventReferences(eventCode, eventReferences);
   }

   g_pEventPolicy->getEventReferences(eventCode, eventReferences);
   GetSNMPTrapsEventReferences(eventCode, eventReferences);
   GetSyslogEventReferences(eventCode, eventReferences);
   GetWindowsEventLogEventReferences(eventCode, eventReferences);

   return unique_ptr<ObjectArray<EventReference>>(eventReferences);
}
