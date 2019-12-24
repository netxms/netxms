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
** File: events.cpp
**
**/

#include "nxcore.h"

/**
 * Event processing queue
 */
ObjectQueue<Event> g_eventQueue;

/**
 * Event processing policy
 */
EventPolicy *g_pEventPolicy = NULL;

/**
 * Static data
 */
static SharedHashMap<UINT32, EventTemplate> s_eventTemplates;
static SharedStringObjectMap<EventTemplate> s_eventNameIndex;
static RWLOCK s_eventTemplatesLock;

/**
 * Create event template from DB record
 */
EventTemplate::EventTemplate(DB_RESULT hResult, int row)
{
   m_code = DBGetFieldULong(hResult, row, 0);
   m_description = DBGetField(hResult, row, 1, NULL, 0);
   DBGetField(hResult, row, 2, m_name, MAX_EVENT_NAME);
   m_guid = DBGetFieldGUID(hResult, row, 3);
   m_severity = DBGetFieldLong(hResult, row, 4);
   m_flags = DBGetFieldLong(hResult, row, 5);
   m_messageTemplate = DBGetField(hResult, row, 6, NULL, 0);
   m_tags = DBGetField(hResult, row, 7, NULL, 0);
}

/**
 * Create event template from message
 */
EventTemplate::EventTemplate(NXCPMessage *msg)
{
   m_guid = uuid::generate();
   m_code = CreateUniqueId(IDG_EVENT);
   msg->getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_severity = msg->getFieldAsInt32(VID_SEVERITY);
   m_flags = msg->getFieldAsInt32(VID_FLAGS);
   m_messageTemplate = msg->getFieldAsString(VID_MESSAGE);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   m_tags = msg->getFieldAsString(VID_TAGS);
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

   if ((m_tags != NULL) && (*m_tags != 0))
   {
      StringList *tags = String(m_tags).split(_T(","));
      json_t *array = json_array();
      for(int i = 0; i < tags->size(); i++)
         json_array_append_new(array, json_string_t(tags->get(i)));
      json_object_set_new(root, "tags", array);
      delete tags;
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
void EventTemplate::modifyFromMessage(NXCPMessage *msg)
{
   m_code = msg->getFieldAsUInt32(VID_EVENT_CODE);
   msg->getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_severity = msg->getFieldAsInt32(VID_SEVERITY);
   m_flags = msg->getFieldAsInt32(VID_FLAGS);
   MemFree(m_messageTemplate);
   m_messageTemplate = msg->getFieldAsString(VID_MESSAGE);
   MemFree(m_description);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   MemFree(m_tags);
   m_tags = msg->getFieldAsString(VID_TAGS);
}

/**
 * Fill message with event template data
 */
void EventTemplate::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   msg->setField(base + 1, m_code);
   msg->setField(base + 2, m_description);
   msg->setField(base + 3, m_name);
   msg->setField(base + 4, m_severity);
   msg->setField(base + 5, m_flags);
   msg->setField(base + 6, m_messageTemplate);
   msg->setField(base + 7, m_tags);
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

   if (hStmt != NULL)
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
   m_messageText = NULL;
   m_messageTemplate = NULL;
   m_timestamp = 0;
   m_originTimestamp = 0;
	m_customMessage = NULL;
	m_parameters.setOwner(true);
}

/**
 * Copy constructor for event
 */
Event::Event(const Event *src)
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
	m_parameters.setOwner(true);
   for(int i = 0; i < src->m_parameters.size(); i++)
   {
      m_parameters.add(MemCopyString((TCHAR *)src->m_parameters.get(i)));
   }
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
   const char *name = NULL, *message = NULL;
   json_t *tags = NULL;
   if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:I, s:i, s:i, s:i, s:i, s:i, s:s, s:o}",
            "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
            "originTimestamp", &originTimestamp, "origin", &origin, "source", &event->m_sourceId,
            "zone", &event->m_zoneUIN, "dci", &event->m_dciId, "severity", &event->m_severity, "message", &message,
            (json_object_get(json, "tags") != NULL) ? "tags" : "tag", &tags) == -1)
   {
      if (json_unpack(json, "{s:I, s:I, s:i, s:s, s:I, s:i, s:i, s:i, s:i, s:s, s:o}",
               "id", &id, "rootId", &rootId, "code", &event->m_code, "name", &name, "timestamp", &timestamp,
               "source", &event->m_sourceId, "zone", &event->m_zoneUIN, "dci", &event->m_dciId,
               "severity", &event->m_severity, "message", &message,
               (json_object_get(json, "tags") != NULL) ? "tags" : "tag", &tags) == -1)
      {
         delete event;
         return NULL;   // Unpack failure
      }
      originTimestamp = timestamp;
   }

   event->m_id = id;
   event->m_rootId = rootId;
   event->m_origin = static_cast<EventOrigin>(origin);
   event->m_timestamp = timestamp;
   event->m_originTimestamp = originTimestamp;
   if (name != NULL)
   {
#ifdef UNICODE
      MultiByteToWideChar(CP_UTF8, 0, name, -1, event->m_name, MAX_EVENT_NAME);
#else
      utf8_to_mb(name, -1, event->m_name, MAX_EVENT_NAME);
#endif
   }
#ifdef UNICODE
   event->m_messageText = WideStringFromUTF8String(message);
#else
   event->m_messageText = MBStringFromUTF8String(message);
#endif
   if (tags != NULL)
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
   if (parameters != NULL)
   {
      char *value;
      size_t count = json_array_size(parameters);
      for(size_t i = 0; i < count; i++)
      {
         json_t *p = json_array_get(parameters, i);
         name = value = NULL;
         if (json_unpack(p, "{s:s, s:s}", "name", &name, "value", &value) != -1)
         {
#ifdef UNICODE
            event->m_parameters.add(WideStringFromUTF8String(CHECK_NULL_EX_A(value)));
            event->m_parameterNames.addPreallocated(WideStringFromUTF8String(CHECK_NULL_EX_A(name)));
#else
            event->m_parameters.add(MBStringFromUTF8String(CHECK_NULL_EX_A(value)));
            event->m_parameterNames.addPreallocated(MBStringFromUTF8String(CHECK_NULL_EX_A(name)));
#endif
         }
         else
         {
            event->m_parameters.add(MemCopyString(_T("")));
            event->m_parameterNames.add(_T(""));
         }
      }
   }

   return event;
}

/**
 * Construct event from template
 */
Event::Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId,
         UINT32 dciId, const TCHAR *tag, const char *format, const TCHAR **names, va_list args)
{
   init(eventTemplate, origin, originTimestamp, sourceId, dciId, tag);

   // Create parameters
   if (format != NULL)
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
					   m_parameters.add(MemCopyString(CHECK_NULL_EX(s)));
               }
               break;
            case 'm':	// multibyte string
               {
                  const char *s = va_arg(args, const char *);
#ifdef UNICODE
                  m_parameters.add((s != NULL) ? WideStringFromMBString(s) : MemCopyStringW(L""));
#else
					   m_parameters.add(MemCopyStringA(CHECK_NULL_EX_A(s)));
#endif
               }
               break;
            case 'u':	// UNICODE string
               {
                  const WCHAR *s = va_arg(args, const WCHAR *);
#ifdef UNICODE
		   			m_parameters.add(MemCopyStringW(CHECK_NULL_EX_W(s)));
#else
                  m_parameters.add((s != NULL) ? MBStringFromWideString(s) : MemCopyStringA(""));
#endif
               }
               break;
            case 'd':
               buffer = MemAllocString(16);
               _sntprintf(buffer, 16, _T("%d"), va_arg(args, LONG));
					m_parameters.add(buffer);
               break;
            case 'D':
               buffer = MemAllocString(32);
               _sntprintf(buffer, 32, INT64_FMT, va_arg(args, INT64));
					m_parameters.add(buffer);
               break;
            case 't':
               buffer = MemAllocString(32);
               _sntprintf(buffer, 32, INT64_FMT, (INT64)va_arg(args, time_t));
					m_parameters.add(buffer);
               break;
            case 'x':
            case 'i':
               buffer = MemAllocString(16);
               _sntprintf(buffer, 16, _T("0x%08X"), va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
            case 'a':   // IPv4 address
               buffer = MemAllocString(16);
               IpToStr(va_arg(args, UINT32), buffer);
					m_parameters.add(buffer);
               break;
            case 'A':   // InetAddress object
               buffer = MemAllocString(64);
               (va_arg(args, InetAddress *))->toString(buffer);
					m_parameters.add(buffer);
               break;
            case 'h':
               buffer = MemAllocString(64);
               MACToStr(va_arg(args, BYTE *), buffer);
					m_parameters.add(buffer);
               break;
            case 'H':
               buffer = MemAllocString(64);
               (va_arg(args, MacAddress *))->toString(buffer);
               m_parameters.add(buffer);
               break;
            case 'G':   // uuid object (GUID)
               buffer = MemAllocString(48);
               (va_arg(args, uuid *))->toString(buffer);
               m_parameters.add(buffer);
               break;
            default:
               buffer = MemAllocString(64);
               _sntprintf(buffer, 64, _T("BAD FORMAT \"%c\" [value = 0x%08X]"), format[i], va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
         }
			m_parameterNames.add(((names != NULL) && (names[i] != NULL)) ? names[i] : _T(""));
      }
   }
}

/**
 * Create event from template
 */
Event::Event(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId,
         UINT32 dciId, const TCHAR *tag, StringMap *args)
{
   init(eventTemplate, origin, originTimestamp, sourceId, dciId, tag);
   Iterator<std::pair<const TCHAR*, const TCHAR*>> *it = args->iterator();
   while(it->hasNext())
   {
      auto p = it->next();
      m_parameterNames.add(p->first);
      m_parameters.add(MemCopyString(p->second));
   }
   delete it;
}

/**
 * Common initialization code
 */
void Event::init(const EventTemplate *eventTemplate, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, UINT32 dciId, const TCHAR *tag)
{
   m_origin = origin;
   _tcscpy(m_name, eventTemplate->getName());
   m_timestamp = time(NULL);
   m_originTimestamp = (originTimestamp != 0) ? originTimestamp : m_timestamp;
   m_id = CreateUniqueEventId();
   m_rootId = 0;
   m_code = eventTemplate->getCode();
   m_severity = eventTemplate->getSeverity();
   m_flags = eventTemplate->getFlags();
   m_sourceId = sourceId;
   m_dciId = dciId;
   m_messageText = NULL;
   m_messageTemplate = MemCopyString(eventTemplate->getMessageTemplate());

   if ((eventTemplate->getTags() != NULL) && (eventTemplate->getTags()[0] != 0))
      m_tags.splitAndAdd(eventTemplate->getTags(), _T(","));
   if (tag != NULL)
      m_tags.add(tag);

   m_customMessage = NULL;
   m_parameters.setOwner(true);

   // Zone UIN
   NetObj *source = FindObjectById(sourceId);
   if (source != NULL)
   {
      switch(source->getObjectClass())
      {
         case OBJECT_NODE:
            m_zoneUIN = static_cast<Node*>(source)->getZoneUIN();
            break;
         case OBJECT_CLUSTER:
            m_zoneUIN = static_cast<Cluster*>(source)->getZoneUIN();
            break;
         case OBJECT_INTERFACE:
            m_zoneUIN = static_cast<Interface*>(source)->getZoneUIN();
            break;
         case OBJECT_SUBNET:
            m_zoneUIN = static_cast<Subnet*>(source)->getZoneUIN();
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
   if (m_messageTemplate == NULL)
      return;

   if (m_messageText != NULL)
      MemFree(m_messageText);
   m_messageText = MemCopyString(expandText(m_messageTemplate));
}

/**
 * Substitute % macros in given text with actual values
 */
StringBuffer Event::expandText(const TCHAR *textTemplate, const Alarm *alarm) const
{
   if (textTemplate == NULL)
      return StringBuffer();

   NetObj *obj = FindObjectById(m_sourceId);
   if (obj == NULL)
   {
     obj = FindObjectById(g_dwMgmtNode);
     if (obj == NULL)
        obj = g_pEntireNet;
   }

   return obj->expandText(textTemplate, alarm, this, NULL, NULL, NULL);
}

/**
 * Add new parameter to event
 */
void Event::addParameter(const TCHAR *name, const TCHAR *value)
{
	m_parameters.add(MemCopyString(value));
	m_parameterNames.add(name);
}

/**
 * Set value of named parameter
 */
void Event::setNamedParameter(const TCHAR *name, const TCHAR *value)
{
   if ((name == NULL) || (name[0] == 0))
      return;

	int index = m_parameterNames.indexOfIgnoreCase(name);
	if (index != -1)
	{
		m_parameters.replace(index, MemCopyString(value));
	}
	else
	{
		m_parameters.add(MemCopyString(value));
		m_parameterNames.add(name);
	}
}

/**
 * Set value (and optionally name) of parameter at given index.
 *
 * @param index 0-based parameter index
 * @param name parameter name (can be NULL)
 * @param value new value
 */
void Event::setParameter(int index, const TCHAR *name, const TCHAR *value)
{
   if (index < 0)
      return;

   int addup = index - m_parameters.size();
   for(int i = 0; i < addup; i++)
   {
		m_parameters.add(MemCopyString(_T("")));
		m_parameterNames.add(_T(""));
   }
   if (index < m_parameters.size())
   {
		m_parameters.replace(index, MemCopyString(value));
		m_parameterNames.replace(index, CHECK_NULL_EX(name));
   }
   else
   {
		m_parameters.add(MemCopyString(value));
		m_parameterNames.add(CHECK_NULL_EX(name));
   }
}

/**
 * Fill message with event data
 */
void Event::prepareMessage(NXCPMessage *msg) const
{
	msg->setField(VID_NUM_RECORDS, (UINT32)1);
	msg->setField(VID_RECORDS_ORDER, (WORD)RECORD_ORDER_NORMAL);

	UINT32 id = VID_EVENTLOG_MSG_BASE;
	msg->setField(id++, m_id);
	msg->setField(id++, m_code);
	msg->setField(id++, (UINT32)m_timestamp);
	msg->setField(id++, m_sourceId);
	msg->setField(id++, (WORD)m_severity);
	msg->setField(id++, CHECK_NULL_EX(m_messageText));
	msg->setField(id++, getTagsAsList());
	msg->setField(id++, (UINT32)m_parameters.size());
	for(int i = 0; i < m_parameters.size(); i++)
	   msg->setField(id++, (TCHAR *)m_parameters.get(i));
	msg->setField(id++, m_dciId);
}

/**
 * Get all tags as list
 */
StringBuffer Event::getTagsAsList() const
{
   StringBuffer list;
   if (!m_tags.isEmpty())
   {
      ConstIterator<const TCHAR> *it = m_tags.constIterator();
      while(it->hasNext())
      {
         if (!list.isEmpty())
            list.append(_T(','));
         list.append(it->next());
      }
      delete it;
   }
   return list;
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

   if (!m_tags.isEmpty())
   {
      json_t *tags = json_array();
      ConstIterator<const TCHAR> *it = m_tags.constIterator();
      while(it->hasNext())
      {
         json_array_append_new(tags, json_string_t(it->next()));
      }
      delete it;
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
      json_object_set_new(p, "value", json_string_t(static_cast<TCHAR*>(m_parameters.get(i))));
      json_array_append_new(parameters, p);
   }
   json_object_set_new(root, "parameters", parameters);

   return root;
}

/**
 * Load event from database
 */
Event *LoadEventFromDatabase(UINT64 eventId)
{
   if (eventId == 0)
      return NULL;

   Event *event = NULL;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT raw_data FROM event_log WHERE event_id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, eventId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            char *data = DBGetFieldUTF8(hResult, 0, 0, NULL, 0);
            if (data != NULL)
            {
               // Event data serialized with JSON_EMBED, so add { } for decoding
               char *pdata = MemAllocArray<char>(strlen(data) + 3);
               pdata[0] = '{';
               strcpy(&pdata[1], data);
               strcat(pdata, "}");
               json_t *json = json_loads(pdata, 0, NULL);
               if (json != NULL)
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
   if (hResult != NULL)
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
      nxlog_write(NXLOG_ERROR, _T("Unable to load event templates from database"));
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
   // Create object access mutex
   s_eventTemplatesLock = RWLockCreate();

   // Load events from database
   bool success = LoadEventConfiguration();

   // Create and initialize event processing policy
   if (success)
   {
      g_pEventPolicy = new EventPolicy;
      if (!g_pEventPolicy->loadFromDB())
      {
         success = FALSE;
         nxlog_write(NXLOG_ERROR, _T("Error loading event processing policy from database"));
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
   RWLockDestroy(s_eventTemplatesLock);
}

/**
 * Reload event templates from database
 */
void ReloadEvents()
{
   RWLockWriteLock(s_eventTemplatesLock, INFINITE);
   s_eventTemplates.clear();
   s_eventNameIndex.clear();
   LoadEventConfiguration();
   RWLockUnlock(s_eventTemplatesLock);
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param eventTag event's tag
 * @param namedArgs named arguments for event - if not NULL format and args will be ignored
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
 * @param names names for parameters (NULL if parameters are unnamed)
 * @param args event parameters
 * @param vm NXSL VM for transformation script
 */
static bool RealPostEvent(ObjectQueue<Event> *queue, UINT64 *eventId, UINT32 eventCode, EventOrigin origin,
         time_t originTimestamp, UINT32 sourceId, UINT32 dciId, const TCHAR *eventTag, StringMap *namedArgs,
         const char *format, const TCHAR **names, va_list args, NXSL_VM *vm)
{
   RWLockReadLock(s_eventTemplatesLock, INFINITE);
   shared_ptr<EventTemplate> eventTemplate = s_eventTemplates.getShared(eventCode);
   RWLockUnlock(s_eventTemplatesLock);

   bool success;
   if (eventTemplate != NULL)
   {
      // Template found, create new event
      Event *evt = (namedArgs != NULL) ?
               new Event(eventTemplate.get(), origin, originTimestamp, sourceId, dciId, eventTag, namedArgs) :
               new Event(eventTemplate.get(), origin, originTimestamp, sourceId, dciId, eventTag, format, names, args);
      if (eventId != NULL)
         *eventId = evt->getId();

      // Using transformation within PostEvent may cause deadlock if called from within locked object or DCI
      // Caller of PostEvent should make sure that it does not held object, object index, or DCI locks
      if (vm != NULL)
      {
         vm->setGlobalVariable("$event", vm->createValue(new NXSL_Object(vm, &g_nxslEventClass, evt, true)));
         if (!vm->run())
         {
            nxlog_debug(6, _T("RealPostEvent: Script execution error (%s)"), vm->getErrorText());
         }
      }

      // Add new event to queue
      queue->put(evt);

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
 */
bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
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
 */
bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const StringList& parameters)
{
   StringMap pmap;
   for(int i = 0; i < parameters.size(); i++)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("Parameter%d"), i + 1);
      pmap.set(name, parameters.get(i));
   }
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, NULL, &pmap, NULL, NULL, dummy, NULL);
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
 */
bool NXCORE_EXPORTABLE PostSystemEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
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
 */
bool NXCORE_EXPORTABLE PostDciEvent(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, dciId, NULL, NULL, format, NULL, args, NULL);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue and return ID of new event (0 in case of failure).
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
 */
UINT64 NXCORE_EXPORTABLE PostEvent2(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   UINT64 eventId;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, &eventId, eventCode, origin, originTimestamp, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
   va_end(args);
   return success ? eventId : 0;
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
 */
UINT64 NXCORE_EXPORTABLE PostSystemEvent2(UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   UINT64 eventId;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, &eventId, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
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
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, NULL, NULL, format, names, args, NULL);
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
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp, UINT32 sourceId, StringMap *parameters)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, NULL, parameters, NULL, NULL, dummy, NULL);
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
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostSystemEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, NULL, NULL, format, names, args, NULL);
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
bool NXCORE_EXPORTABLE PostSystemEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, NULL, parameters, NULL, NULL, dummy, NULL);
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
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, dciId, NULL, NULL, format, names, args, NULL);
   va_end(args);
   return success;
}

/**
 * Post DCI-related event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param dciId DCI ID
 * @param parameters event parameters list
 */
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, StringMap *parameters)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, dciId, NULL, parameters, NULL, NULL, dummy, NULL);
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
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, userTag, NULL, format, names, args, NULL);
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
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const TCHAR *tag, StringMap *parameters)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, tag, parameters, NULL, NULL, dummy, NULL);
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param origin event origin
 * @param originTimestamp event origin's timestamp
 * @param sourceId Event source object ID
 * @param userTag event's user tag
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
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const TCHAR *userTag, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, userTag, NULL, format, NULL, args, NULL);
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
 * @param userTag event's user tag
 * @param parameters event parameters
 */
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const TCHAR *userTag, const StringList& parameters)
{
   StringMap pmap;
   for(int i = 0; i < parameters.size(); i++)
   {
      TCHAR name[64];
      _sntprintf(name, 64, _T("Parameter%d"), i + 1);
      pmap.set(name, parameters.get(i));
   }
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, userTag, &pmap, NULL, NULL, dummy, NULL);
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
bool NXCORE_EXPORTABLE PostEventEx(ObjectQueue<Event> *queue, UINT32 eventCode, EventOrigin origin,
         time_t originTimestamp, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(queue, NULL, eventCode, origin, originTimestamp, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
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
bool NXCORE_EXPORTABLE PostSystemEventEx(ObjectQueue<Event> *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(queue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, NULL, NULL, format, NULL, args, NULL);
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
bool NXCORE_EXPORTABLE TransformAndPostEvent(UINT32 eventCode, EventOrigin origin, time_t originTimestamp,
         UINT32 sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, origin, originTimestamp, sourceId, 0, tag, parameters, NULL, NULL, dummy, vm);
}

/**
 * Create event with SYSTEM origin, run transformation script, and post to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param tag Event tag
 * @param parameters Named event parameters
 */
bool NXCORE_EXPORTABLE TransformAndPostSystemEvent(UINT32 eventCode, UINT32 sourceId, const TCHAR *tag, StringMap *parameters, NXSL_VM *vm)
{
   va_list dummy;
   return RealPostEvent(&g_eventQueue, NULL, eventCode, EventOrigin::SYSTEM, 0, sourceId, 0, tag, parameters, NULL, NULL, dummy, vm);
}

/**
 * Resend events from specific queue to system event queue
 */
void NXCORE_EXPORTABLE ResendEvents(ObjectQueue<Event> *queue)
{
   Event *e;
   while ((e = queue->get()) != NULL)
      g_eventQueue.put(e);
}

/**
 * Create export record for event template
 */
void CreateEventTemplateExportRecord(StringBuffer &str, UINT32 eventCode)
{
   String strText, strDescr;

   RWLockReadLock(s_eventTemplatesLock, INFINITE);

   // Find event template
   EventTemplate *e = s_eventTemplates.get(eventCode);
   if (e != NULL)
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

   RWLockUnlock(s_eventTemplatesLock);
}

/**
 * Resolve event name
 */
bool EventNameFromCode(UINT32 eventCode, TCHAR *buffer)
{
   bool bRet = false;

   RWLockReadLock(s_eventTemplatesLock, INFINITE);

   EventTemplate *e = s_eventTemplates.get(eventCode);
   if (e != NULL)
   {
      _tcscpy(buffer, e->getName());
      bRet = true;
   }
   else
   {
      _tcscpy(buffer, _T("UNKNOWN_EVENT"));
   }

   RWLockUnlock(s_eventTemplatesLock);
   return bRet;
}

/**
 * Find event template by code - suitable for external call
 */
shared_ptr<EventTemplate> FindEventTemplateByCode(UINT32 code)
{
   RWLockReadLock(s_eventTemplatesLock, INFINITE);
   shared_ptr<EventTemplate> e = s_eventTemplates.getShared(code);
   RWLockUnlock(s_eventTemplatesLock);
   return e;
}

/**
 * Find event template by name - suitable for external call
 */
shared_ptr<EventTemplate> FindEventTemplateByName(const TCHAR *name)
{
   RWLockReadLock(s_eventTemplatesLock, INFINITE);
   shared_ptr<EventTemplate> e = s_eventNameIndex.getShared(name);
   RWLockUnlock(s_eventTemplatesLock);
   return e;
}

/**
 * Translate event name to code
 * If event with given name does not exist, returns supplied default value
 */
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue)
{
   shared_ptr<EventTemplate> e = FindEventTemplateByName(name);
	return (e != NULL) ? e->getCode() : defaultValue;
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
static void SendEventDBChangeNotification(ClientSession *session, void *arg)
{
   if (session->isAuthenticated() &&
       (session->checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) ||
        session->checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) ||
        session->checkSysAccessRights(SYSTEM_ACCESS_EPP)))
      session->postMessage((NXCPMessage *)arg);
}

/**
 * Update or create new event object from request
 */
UINT32 UpdateEventTemplate(NXCPMessage *request, NXCPMessage *response, json_t **oldValue, json_t **newValue)
{
   TCHAR name[MAX_EVENT_NAME] = _T("");
   request->getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
   if (!IsValidObjectName(name, TRUE))
      return RCC_INVALID_OBJECT_NAME;

   shared_ptr<EventTemplate> e;
   UINT32 eventCode = request->getFieldAsUInt32(VID_EVENT_CODE);
   RWLockWriteLock(s_eventTemplatesLock, INFINITE);

   if (eventCode == 0)
   {
      e = make_shared<EventTemplate>(request);
      s_eventTemplates.set(e->getCode(), e);
      s_eventNameIndex.set(e->getName(), e);
      *oldValue = NULL;
   }
   else
   {
      e = s_eventTemplates.getShared(eventCode);
      if (e == NULL)
      {
         RWLockUnlock(s_eventTemplatesLock);
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

   RWLockUnlock(s_eventTemplatesLock);

   if (!success)
   {
      if (*oldValue != NULL)
      {
         json_decref(*oldValue);
         *oldValue = NULL;
      }
      json_decref(*newValue);
      *newValue = NULL;
      return RCC_DB_FAILURE;
   }

   return RCC_SUCCESS;
}

/**
 * Delete event template
 */
UINT32 DeleteEventTemplate(UINT32 eventCode)
{
   UINT32 rcc = RCC_SUCCESS;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;

   RWLockWriteLock(s_eventTemplatesLock, INFINITE);
   auto e = s_eventTemplates.get(eventCode);
   if (e != NULL)
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
   RWLockUnlock(s_eventTemplatesLock);

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get event configuration
 */
void GetEventConfiguration(NXCPMessage *msg)
{
   RWLockWriteLock(s_eventTemplatesLock, INFINITE);
   UINT32 base = VID_ELEMENT_LIST_BASE;
   msg->setField(VID_NUM_EVENTS, s_eventTemplates.size());
   auto it = s_eventTemplates.iterator();
   while(it->hasNext())
   {
      auto e = it->next();
      (*e)->fillMessage(msg, base);
      base += 10;
   }
   delete it;
   RWLockUnlock(s_eventTemplatesLock);
}
