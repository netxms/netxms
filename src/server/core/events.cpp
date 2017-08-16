/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Global variables
 */
Queue *g_pEventQueue = NULL;
EventPolicy *g_pEventPolicy = NULL;

/**
 * Static data
 */
static RefCountHashMap<UINT32, EventObject> m_eventObjects(true);
static RWLOCK m_rwlockTemplateAccess;

/**
 * Create event object from DB record
 */
EventObject::EventObject(DB_RESULT hResult, int row)
{
   m_code = DBGetFieldULong(hResult, row, 0);
   m_description = DBGetField(hResult, row, 1, NULL, 0);
   DBGetField(hResult, row, 2, m_name, MAX_EVENT_NAME);
   m_guid = DBGetFieldGUID(hResult, row, 3);
}

/**
 * Create event object from message
 */
EventObject::EventObject(NXCPMessage *msg)
{
   m_code = 0;
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   msg->getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_guid = uuid::generate();
}

/**
 * Event object destructor
 */
EventObject::~EventObject()
{
   free(m_description);
}

/**
 * Modify event object from message
 */
void EventObject::modifyFromMessage(NXCPMessage *msg)
{
   m_code = msg->getFieldAsUInt32(VID_EVENT_CODE);
   free(m_description);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
   msg->getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
}

/**
 * Fill message with event object data
 */
void EventObject::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   msg->setField(base + 1, m_code);
   msg->setField(base + 2, m_description);
   msg->setField(base + 3, m_name);
}

/**
 * Convert event object to JSON
 */
json_t *EventObject::toJson() const
{
   json_t *root = json_object();

   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "code", json_integer(m_code));
   json_object_set_new(root, "description", json_string_t(m_description));

   return root;
}

/**
 * Create event template from DB record
 */
EventTemplate::EventTemplate(DB_RESULT hResult, int row) : EventObject(hResult, row)
{
   m_severity = DBGetFieldLong(hResult, row, 4);
   m_flags = DBGetFieldLong(hResult, row, 5);
   m_messageTemplate = DBGetField(hResult, row, 6, NULL, 0);
}

/**
 * Create event template from message
 */
EventTemplate::EventTemplate(NXCPMessage *msg) : EventObject(msg)
{
   m_code = CreateUniqueId(IDG_EVENT);
   m_severity = msg->getFieldAsInt32(VID_SEVERITY);
   m_flags = msg->getFieldAsInt32(VID_FLAGS);
   m_messageTemplate = msg->getFieldAsString(VID_MESSAGE);
}

/**
 * Event template destructor
 */
EventTemplate::~EventTemplate()
{
   free(m_messageTemplate);
}

/**
 * Convert event template to JSON
 */
json_t *EventTemplate::toJson() const
{
   json_t *root = EventObject::toJson();
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "severity", json_integer(m_severity));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "message", json_string_t(m_messageTemplate));

   return root;
}

/**
 * Modify event template from message
 */
void EventTemplate::modifyFromMessage(NXCPMessage *msg)
{
   EventObject::modifyFromMessage(msg);
   m_severity = msg->getFieldAsInt32(VID_SEVERITY);
   m_flags = msg->getFieldAsInt32(VID_FLAGS);
   free(m_messageTemplate);
   m_messageTemplate = msg->getFieldAsString(VID_MESSAGE);
}

/**
 * Fill message with event template data
 */
void EventTemplate::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   EventObject::fillMessage(msg, base);
   msg->setField(base + 4, m_severity);
   msg->setField(base + 5, m_flags);
   msg->setField(base + 6, m_messageTemplate);
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
      hStmt = DBPrepare(hdb, _T("UPDATE event_cfg SET event_name=?,severity=?,flags=?,message=?,description=? WHERE event_code=?"));
   else
      hStmt = DBPrepare(hdb, _T("INSERT INTO event_cfg (event_name,severity,flags,message,description,event_code,guid) VALUES (?,?,?,?,?,?,?)"));

   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_severity);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_messageTemplate, DB_BIND_STATIC, MAX_EVENT_MSG_LENGTH - 1);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_code);

      if (!recordExists)
      {
         DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, uuid::generate());
      }
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return success;
}

/**
 * Create event group from message
 */
EventGroup::EventGroup(NXCPMessage *msg) : EventObject(msg)
{
   m_code = CreateUniqueId(IDG_EVENT_GROUP);
   m_eventCodeList = new IntegerArray<UINT32>;
   msg->getFieldAsInt32Array(VID_EVENT_LIST, m_eventCodeList);
}

/**
 * Create event group from DB
 */
EventGroup::EventGroup(DB_RESULT result, int row, IntegerArray<UINT32> *memberCache) : EventObject(result, row)
{
   m_eventCodeList = new IntegerArray<UINT32>(16, 16);
   int i = 0;
   while((i < memberCache->size()) && (memberCache->get(i) != m_code))
      i += 2;
   while((i < memberCache->size()) && (memberCache->get(i) == m_code))
   {
      m_eventCodeList->add(memberCache->get(i + 1));
      i += 2;
   }
}

/**
 * Event group destructor
 */
EventGroup::~EventGroup()
{
   free(m_eventCodeList);
}

/**
 * Check if event is a member of specific group
 */
static bool CheckGroupMembership(UINT32 eventCode, UINT32 groupId)
{
   if (!(groupId & GROUP_FLAG_BIT))
      return false;

   EventGroup *g = (EventGroup *)m_eventObjects.get(groupId);
   if (g != NULL)
   {
      for(int i = 0; i < g->getMemberCount(); i++)
      {
         if (eventCode == g->getMember(i))
            return true;
         if (CheckGroupMembership(eventCode, g->getMember(i)))
            return true;
      }
   }

   return false;
}
/**
 * Modify event template from message
 */
void EventGroup::modifyFromMessage(NXCPMessage *msg)
{
   EventObject::modifyFromMessage(msg);
   IntegerArray<UINT32> eventCodeList;
   msg->getFieldAsInt32Array(VID_EVENT_LIST, &eventCodeList);
   m_eventCodeList->clear();
   for(int i = 0; i < eventCodeList.size(); i++)
   {
      if (!m_eventCodeList->contains(eventCodeList.get(i)) &&
         !CheckGroupMembership(m_code, eventCodeList.get(i)) &&
         !CheckGroupMembership(eventCodeList.get(i), m_code))
      {
         m_eventCodeList->add(eventCodeList.get(i));
      }
   }
}

/**
 * Fill message with event group data
 */
void EventGroup::fillMessage(NXCPMessage *msg, UINT32 base) const
{
   EventObject::fillMessage(msg, base);
   msg->setFieldFromInt32Array(base + 4, m_eventCodeList);
}

/**
 * Save event group to database
 */
bool EventGroup::saveToDatabase() const
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   bool success = DBBegin(hdb);
   if (success)
   {
      DB_STATEMENT hStmt;
      bool recordExists = IsDatabaseRecordExist(hdb, _T("event_groups"), _T("id"), m_code);
      if (recordExists)
         hStmt = DBPrepare(hdb, _T("UPDATE event_groups SET name=?,description=? WHERE id=?"));
      else
         hStmt = DBPrepare(hdb, _T("INSERT INTO event_groups (name,description,id,guid) VALUES (?,?,?,?)"));

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_description, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_code);
         if (!recordExists)
            DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_guid);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("DELETE FROM event_group_members WHERE group_id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_code);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
         else
            success = false;
      }

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO event_group_members (group_id,event_code) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_code);
            for(int i = 0; i < m_eventCodeList->size() && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_eventCodeList->get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
      }

      if (success)
         success = DBCommit(hdb);
      else
         DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return success;
}

/**
 * Convert event group to JSON
 */
json_t *EventGroup::toJson() const
{
   json_t *root = EventObject::toJson();

   json_t *groupList =  json_array();
   for(int i = 0; i < m_eventCodeList->size(); i++)
      json_array_append_new(groupList, json_integer(m_eventCodeList->get(i)));
   json_object_set_new(root, "groups", groupList);

   return root;
}

/**
 * Check if event object is member of group (recursive)
 */
bool EventGroup::isMember(UINT32 eventCode)
{
   if (m_eventCodeList->contains(eventCode))
      return true;

   for(int i = 0; i < m_eventCodeList->size(); i++)
   {
      if (m_eventCodeList->get(i) & GROUP_FLAG_BIT)
      {
         EventGroup *g = (EventGroup*)FindEventObjectByCode(m_eventCodeList->get(i));
         if (g != NULL)
            return g->isMember(m_eventCodeList->get(i));
         return false;
      }
   }

   return false;
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
   m_sourceId = 0;
   m_dciId = 0;
   m_flags = 0;
   m_messageText = NULL;
   m_messageTemplate = NULL;
   m_timeStamp = 0;
	m_userTag = NULL;
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
   m_sourceId = src->m_sourceId;
   m_dciId = src->m_dciId;
   m_flags = src->m_flags;
   m_messageText = _tcsdup_ex(src->m_messageText);
   m_messageTemplate = _tcsdup_ex(src->m_messageTemplate);
   m_timeStamp = src->m_timeStamp;
	m_userTag = _tcsdup_ex(src->m_userTag);
	m_customMessage = _tcsdup_ex(src->m_customMessage);
	m_parameters.setOwner(true);
   for(int i = 0; i < src->m_parameters.size(); i++)
   {
      m_parameters.add(_tcsdup_ex((TCHAR *)src->m_parameters.get(i)));
   }
   m_parameterNames.addAll(&src->m_parameterNames);
}

/**
 * Construct event from template
 */
Event::Event(const EventTemplate *eventTemplate, UINT32 sourceId, UINT32 dciId, const TCHAR *userTag, const char *szFormat, const TCHAR **names, va_list args)
{
	_tcscpy(m_name, eventTemplate->getName());
   m_timeStamp = time(NULL);
   m_id = CreateUniqueEventId();
   m_rootId = 0;
   m_code = eventTemplate->getCode();
   m_severity = eventTemplate->getSeverity();
   m_flags = eventTemplate->getFlags();
   m_sourceId = sourceId;
   m_dciId = dciId;
   m_messageText = NULL;
	m_userTag = _tcsdup_ex(userTag);
   if ((m_userTag != NULL) && (_tcslen(m_userTag) >= MAX_USERTAG_LENGTH))
      m_userTag[MAX_USERTAG_LENGTH - 1] = 0;
	m_customMessage = NULL;
	m_parameters.setOwner(true);

   // Create parameters
   if (szFormat != NULL)
   {
		int count = (int)strlen(szFormat);
		TCHAR *buffer;

      for(int i = 0; i < count; i++)
      {
         switch(szFormat[i])
         {
            case 's':
               {
                  const TCHAR *s = va_arg(args, const TCHAR *);
					   m_parameters.add(_tcsdup_ex(s));
               }
               break;
            case 'm':	// multibyte string
               {
                  const char *s = va_arg(args, const char *);
#ifdef UNICODE
                  m_parameters.add((s != NULL) ? WideStringFromMBString(s) : _tcsdup(_T("")));
#else
					   m_parameters.add(strdup(CHECK_NULL_EX(s)));
#endif
               }
               break;
            case 'u':	// UNICODE string
               {
                  const WCHAR *s = va_arg(args, const WCHAR *);
#ifdef UNICODE
		   			m_parameters.add(wcsdup(CHECK_NULL_EX(s)));
#else
                  m_parameters.add((s != NULL) ? MBStringFromWideString(s) : strdup(""));
#endif
               }
               break;
            case 'd':
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(buffer, 16, _T("%d"), va_arg(args, LONG));
					m_parameters.add(buffer);
               break;
            case 'D':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(buffer, 32, INT64_FMT, va_arg(args, INT64));
					m_parameters.add(buffer);
               break;
            case 't':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               _sntprintf(buffer, 32, INT64_FMT, (INT64)va_arg(args, time_t));
					m_parameters.add(buffer);
               break;
            case 'x':
            case 'i':
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               _sntprintf(buffer, 16, _T("0x%08X"), va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
            case 'a':   // IPv4 address
               buffer = (TCHAR *)malloc(16 * sizeof(TCHAR));
               IpToStr(va_arg(args, UINT32), buffer);
					m_parameters.add(buffer);
               break;
            case 'A':   // InetAddress object
               buffer = (TCHAR *)malloc(64 * sizeof(TCHAR));
               (va_arg(args, InetAddress *))->toString(buffer);
					m_parameters.add(buffer);
               break;
            case 'h':
               buffer = (TCHAR *)malloc(32 * sizeof(TCHAR));
               MACToStr(va_arg(args, BYTE *), buffer);
					m_parameters.add(buffer);
               break;
            default:
               buffer = (TCHAR *)malloc(64 * sizeof(TCHAR));
               _sntprintf(buffer, 64, _T("BAD FORMAT \"%c\" [value = 0x%08X]"), szFormat[i], va_arg(args, UINT32));
					m_parameters.add(buffer);
               break;
         }
			m_parameterNames.add(((names != NULL) && (names[i] != NULL)) ? names[i] : _T(""));
      }
   }

   m_messageTemplate = _tcsdup(eventTemplate->getMessageTemplate());
}

/**
 * Destructor for event
 */
Event::~Event()
{
   free(m_messageText);
   free(m_messageTemplate);
	free(m_userTag);
	free(m_customMessage);
}

/**
 * Create message text from template
 */
void Event::expandMessageText()
{
   if (m_messageTemplate == NULL)
      return;

   if (m_messageText != NULL)
      free(m_messageText);
   m_messageText = expandText(m_messageTemplate);
}

/**
 * Substitute % macros in given text with actual values
 */
TCHAR *Event::expandText(const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey)
{
	return Event::expandText(this, m_sourceId, textTemplate, alarmMsg, alarmKey);
}

/**
 * Substitute % macros in given text with actual values
 */
TCHAR *Event::expandText(Event *event, UINT32 sourceObject, const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey)
{
   const TCHAR *pCurr;
   UINT32 dwPos, dwSize, dwParam;
	UINT32 sourceId = (event != NULL) ? event->getSourceId() : sourceObject;
   NetObj *pObject;
   struct tm *lt;
#if HAVE_LOCALTIME_R
   struct tm tmbuffer;
#endif
   TCHAR *pText, szBuffer[4], scriptName[256];
	int i;

	DbgPrintf(8, _T("Event::expandText(event=%p sourceObject=%d template='%s' alarmMsg='%s' alarmKey='%s')"),
	          event, (int)sourceObject, CHECK_NULL(textTemplate), CHECK_NULL(alarmMsg), CHECK_NULL(alarmKey));

   pObject = FindObjectById(sourceId);
   if (pObject == NULL)
   {
      pObject = FindObjectById(g_dwMgmtNode);
		if (pObject == NULL)
			pObject = g_pEntireNet;
   }
   dwSize = (UINT32)_tcslen(textTemplate) + 1;
   pText = (TCHAR *)malloc(dwSize * sizeof(TCHAR));
   for(pCurr = textTemplate, dwPos = 0; *pCurr != 0; pCurr++)
   {
      switch(*pCurr)
      {
         case '%':   // Metacharacter
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case '%':
                  pText[dwPos++] = '%';
                  break;
               case 'a':   // IP address of event source
                  dwSize += 64;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  GetObjectIpAddress(pObject).toString(&pText[dwPos]);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'A':   // Associated alarm message
                  if (alarmMsg != NULL)
                  {
                     dwSize += (UINT32)_tcslen(alarmMsg);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], alarmMsg);
                     dwPos += (UINT32)_tcslen(alarmMsg);
                  }
                  break;
               case 'c':   // Event code
                  dwSize += 16;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						_sntprintf(&pText[dwPos], 16, _T("%u"), (unsigned int)((event != NULL) ? event->m_code : 0));
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'g':   // Source object's GUID
                  dwSize += 36;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  pObject->getGuid().toString(&pText[dwPos]);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'i':   // Source object identifier in form 0xhhhhhhhh
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("0x%08X"), sourceId);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'I':   // Source object identifier in decimal form
                  dwSize += 10;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _sntprintf(&pText[dwPos], 11, _T("%u"), (unsigned int)sourceId);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'K':   // Associated alarm key
                  if (alarmKey != NULL)
                  {
                     dwSize += (UINT32)_tcslen(alarmKey);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], alarmKey);
                     dwPos += (UINT32)_tcslen(alarmKey);
                  }
                  break;
               case 'm':
                  if ((event != NULL) && (event->m_messageText != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_messageText);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_messageText);
                     dwPos += (UINT32)_tcslen(event->m_messageText);
                  }
                  break;
               case 'M':	// Custom message (usually set by matching script in EPP)
                  if ((event != NULL) && (event->m_customMessage != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_customMessage);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_customMessage);
                     dwPos += (UINT32)_tcslen(event->m_customMessage);
                  }
                  break;
               case 'n':   // Name of event source
                  dwSize += (UINT32)_tcslen(pObject->getName());
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], pObject->getName());
                  dwPos += (UINT32)_tcslen(pObject->getName());
                  break;
               case 'N':   // Event name
						if (event != NULL)
						{
							dwSize += (UINT32)_tcslen(event->m_name);
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_tcscpy(&pText[dwPos], event->m_name);
							dwPos += (UINT32)_tcslen(event->m_name);
						}
                  break;
               case 's':   // Severity code
						if (event != NULL)
						{
							dwSize += 3;
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_sntprintf(&pText[dwPos], 4, _T("%d"), (int)event->m_severity);
							dwPos = (UINT32)_tcslen(pText);
						}
                  break;
               case 'S':   // Severity text
						if (event != NULL)
						{
                     const TCHAR *statusText = GetStatusAsText(event->m_severity, false);
							dwSize += (UINT32)_tcslen(statusText);
							pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
							_tcscpy(&pText[dwPos], statusText);
							dwPos += (UINT32)_tcslen(statusText);
						}
                  break;
               case 't':   // Event's timestamp
                  dwSize += 32;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						if (event != NULL)
						{
#if HAVE_LOCALTIME_R
                     lt = localtime_r(&event->m_timeStamp, &tmbuffer);
#else
							lt = localtime(&event->m_timeStamp);
#endif
						}
						else
						{
							time_t t = time(NULL);
#if HAVE_LOCALTIME_R
                     lt = localtime_r(&t, &tmbuffer);
#else
							lt = localtime(&t);
#endif
						}
                  _tcsftime(&pText[dwPos], 32, _T("%d-%b-%Y %H:%M:%S"), lt);
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'T':   // Event's timestamp as number of seconds since epoch
                  dwSize += 16;
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
						_sntprintf(&pText[dwPos], 16, _T("%lu"), (unsigned long)((event != NULL) ? event->m_timeStamp : time(NULL)));
                  dwPos = (UINT32)_tcslen(pText);
                  break;
               case 'u':	// User tag
                  if ((event != NULL) && (event->m_userTag != NULL))
                  {
                     dwSize += (UINT32)_tcslen(event->m_userTag);
	                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                     _tcscpy(&pText[dwPos], event->m_userTag);
                     dwPos += (UINT32)_tcslen(event->m_userTag);
                  }
                  break;
               case 'v':   // NetXMS server version
                  dwSize += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
                  _tcscpy(&pText[dwPos], NETXMS_VERSION_STRING);
                  dwPos += (UINT32)_tcslen(NETXMS_VERSION_STRING);
                  break;
               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
						if (event != NULL)
						{
							szBuffer[0] = *pCurr;
							if (isdigit(*(pCurr + 1)))
							{
								pCurr++;
								szBuffer[1] = *pCurr;
								szBuffer[2] = 0;
							}
							else
							{
								szBuffer[1] = 0;
							}
							dwParam = _tcstoul(szBuffer, NULL, 10);
							if ((dwParam > 0) && (dwParam <= (UINT32)event->m_parameters.size()))
							{
								const TCHAR *value = (TCHAR *)event->m_parameters.get(dwParam - 1);
								if (value == NULL)
									value = _T("");
								dwSize += (UINT32)_tcslen(value);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], value);
								dwPos += (UINT32)_tcslen(value);
							}
						}
                  break;
					case '[':	// Script
						for(i = 0, pCurr++; (*pCurr != ']') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating ]
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;

							// Entry point can be given in form script/entry_point
							TCHAR *entryPoint = _tcschr(scriptName, _T('/'));
							if (entryPoint != NULL)
							{
								*entryPoint = 0;
								entryPoint++;
								StrStrip(entryPoint);
							}
							StrStrip(scriptName);

							NXSL_VM *vm = CreateServerScriptVM(scriptName);
							if (vm != NULL)
							{
								if (pObject->getObjectClass() == OBJECT_NODE)
									vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, pObject)));
								vm->setGlobalVariable(_T("$event"), (event != NULL) ? new NXSL_Value(new NXSL_Object(&g_nxslEventClass, event)) : new NXSL_Value);
								if (alarmMsg != NULL)
									vm->setGlobalVariable(_T("$alarmMessage"), new NXSL_Value(alarmMsg));
								if (alarmKey != NULL)
									vm->setGlobalVariable(_T("$alarmKey"), new NXSL_Value(alarmKey));

								if (vm->run(0, NULL, NULL, NULL, NULL, entryPoint))
								{
									NXSL_Value *result = vm->getResult();
									if (result != NULL)
									{
										const TCHAR *temp = result->getValueAsCString();
										if (temp != NULL)
										{
											dwSize += (UINT32)_tcslen(temp);
											pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
											_tcscpy(&pText[dwPos], temp);
											dwPos += (UINT32)_tcslen(temp);
											DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Script %s executed successfully"),
												(int)((event != NULL) ? event->m_code : 0), textTemplate, scriptName);
										}
									}
								}
								else
								{
									DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Script %s execution error: %s"),
												 (int)((event != NULL) ? event->m_code : 0), textTemplate, scriptName, vm->getErrorText());
									PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, vm->getErrorText(), 0);
								}
							}
							else
							{
								DbgPrintf(4, _T("Event::ExpandText(%d, \"%s\"): Cannot find script %s"),
									(int)((event != NULL) ? event->m_code : 0), textTemplate, scriptName);
							}
						}
						break;
					case '{':	// Custom attribute
						for(i = 0, pCurr++; (*pCurr != '}') && (*pCurr != 0) && (i < 255); pCurr++)
						{
							scriptName[i++] = *pCurr;
						}
						if (*pCurr == 0)	// no terminating }
						{
							pCurr--;
						}
						else
						{
							scriptName[i] = 0;
							StrStrip(scriptName);
							const TCHAR *temp = pObject->getCustomAttribute(scriptName);
							if (temp != NULL)
							{
								dwSize += (UINT32)_tcslen(temp);
								pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
								_tcscpy(&pText[dwPos], temp);
								dwPos += (UINT32)_tcslen(temp);
							}
						}
						break;
					case '(':	// input field - scan for closing ) for compatibility
						for(i = 0, pCurr++; (*pCurr != ')') && (*pCurr != 0) && (i < 255); pCurr++)
						{
						}
						if (*pCurr == 0)	// no terminating }
						{
							pCurr--;
						}
						break;
					case '<':	// Named parameter
						if (event != NULL)
						{
							for(i = 0, pCurr++; (*pCurr != '>') && (*pCurr != 0) && (i < 255); pCurr++)
							{
								scriptName[i++] = *pCurr;
							}
							if (*pCurr == 0)	// no terminating >
							{
								pCurr--;
							}
							else
							{
								scriptName[i] = 0;
								StrStrip(scriptName);
								int index = event->m_parameterNames.indexOfIgnoreCase(scriptName);
								if (index != -1)
								{
									const TCHAR *temp = (TCHAR *)event->m_parameters.get(index);
									if (temp != NULL)
									{
										dwSize += (UINT32)_tcslen(temp);
										pText = (TCHAR *)realloc(pText, dwSize * sizeof(TCHAR));
										_tcscpy(&pText[dwPos], temp);
										dwPos += (UINT32)_tcslen(temp);
									}
								}
							}
						}
						break;
               default:    // All other characters are invalid, ignore
                  break;
            }
            break;
         case '\\':  // Escape character
            pCurr++;
            if (*pCurr == 0)
            {
               pCurr--;
               break;   // Abnormal loop termination
            }
            switch(*pCurr)
            {
               case 't':
                  pText[dwPos++] = '\t';
                  break;
               case 'n':
                  pText[dwPos++] = 0x0D;
                  pText[dwPos++] = 0x0A;
                  break;
               default:
                  pText[dwPos++] = *pCurr;
                  break;
            }
            break;
         default:
            pText[dwPos++] = *pCurr;
            break;
      }
   }
   pText[dwPos] = 0;
   return pText;
}

/**
 * Add new parameter to event
 */
void Event::addParameter(const TCHAR *name, const TCHAR *value)
{
	m_parameters.add(_tcsdup(value));
	m_parameterNames.add(name);
}

/**
 * Set value of named parameter
 */
void Event::setNamedParameter(const TCHAR *name, const TCHAR *value)
{
	int index = m_parameterNames.indexOfIgnoreCase(name);
	if (index != -1)
	{
		m_parameters.replace(index, _tcsdup(value));
		m_parameterNames.replace(index, name);
	}
	else
	{
		m_parameters.add(_tcsdup(value));
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
		m_parameters.add(_tcsdup(_T("")));
		m_parameterNames.add(_T(""));
   }
   if (index < m_parameters.size())
   {
		m_parameters.replace(index, _tcsdup(value));
		m_parameterNames.replace(index, CHECK_NULL_EX(name));
   }
   else
   {
		m_parameters.add(_tcsdup(value));
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
	msg->setField(id++, (UINT32)m_timeStamp);
	msg->setField(id++, m_sourceId);
	msg->setField(id++, (WORD)m_severity);
	msg->setField(id++, CHECK_NULL_EX(m_messageText));
	msg->setField(id++, CHECK_NULL_EX(m_userTag));
	msg->setField(id++, (UINT32)m_parameters.size());
	for(int i = 0; i < m_parameters.size(); i++)
	   msg->setField(id++, (TCHAR *)m_parameters.get(i));
	msg->setField(id++, m_dciId);
}

/**
 * Create JSON object
 */
String Event::createJson()
{
   TCHAR buffer[64];

   String json = _T("{ \"id\":");
   json.append(m_id);
   json.append(_T(", \"code\":"));
   json.append(m_code);
   if (EventNameFromCode(m_code, buffer))
   {
      json.append(_T(", \"name\":\""));
      json.append(EscapeStringForJSON(buffer));
      json.append(_T('"'));
   }
   json.append(_T(", \"timestamp\":"));
   json.append((INT64)m_timeStamp);
   json.append(_T(", \"source\":"));
   json.append(m_sourceId);
   json.append(_T(", \"dci\":"));
   json.append(m_dciId);
   json.append(_T(", \"severity\":"));
   json.append(m_severity);
   json.append(_T(", \"tag\":\""));
   json.append(EscapeStringForJSON(m_userTag));
   json.append(_T("\", \"message\":\""));
   json.append(EscapeStringForJSON(m_messageText));
   json.append(_T("\", \"parameters\":["));
   for(int i = 0; i < m_parameters.size(); i++)
   {
      if (i > 0)
         json.append(_T(','));
      json.append(_T(" { \"name\":\""));
      json.append(EscapeStringForJSON(m_parameterNames.get(i)));
      json.append(_T("\", \"value\":\""));
      json.append(EscapeStringForJSON((TCHAR *)m_parameters.get(i)));
      json.append(_T("\" }"));
   }
   json.append(_T(" ] }"));
   return json;
}

/**
 * Load event configuration from database
 */
static bool LoadEvents()
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT event_code,description,event_name,guid,severity,flags,message FROM event_cfg"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         EventTemplate *t = new EventTemplate(hResult, i);
         m_eventObjects.set(t->getCode(), t);
         t->decRefCount();
      }
      DBFreeResult(hResult);

      IntegerArray<UINT32> memberCache(256, 256);
      hResult = DBSelect(hdb, _T("SELECT group_id,event_code FROM event_group_members ORDER BY group_id"));
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            memberCache.add(DBGetFieldULong(hResult, i, 0));
            memberCache.add(DBGetFieldULong(hResult, i, 1));
         }
         DBFreeResult(hResult);
      }

      hResult = DBSelect(hdb, _T("SELECT id,description,name,guid FROM event_groups"));
      if (hResult != NULL)
      {
         UINT32 numRows = DBGetNumRows(hResult);
         for(int i = 0; i < numRows; i++)
         {
            EventGroup *g = new EventGroup(hResult, i, &memberCache);
            m_eventObjects.set(g->getCode(), g);
            g->decRefCount();
         }
         success = true;
      }
   }
   else
   {
      nxlog_write(MSG_EVENT_LOAD_ERROR, EVENTLOG_ERROR_TYPE, NULL);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Inilialize event handling subsystem
 */
BOOL InitEventSubsystem()
{
   BOOL bSuccess;

   // Create object access mutex
   m_rwlockTemplateAccess = RWLockCreate();

   // Create event queue
   g_pEventQueue = new Queue;

   // Load events from database
   bSuccess = LoadEvents();

   // Create and initialize event processing policy
   if (bSuccess)
   {
      g_pEventPolicy = new EventPolicy;
      if (!g_pEventPolicy->loadFromDB())
      {
         bSuccess = FALSE;
         nxlog_write(MSG_EPP_LOAD_FAILED, EVENTLOG_ERROR_TYPE, NULL);
         delete g_pEventPolicy;
      }
   }

   return bSuccess;
}

/**
 * Shutdown event subsystem
 */
void ShutdownEventSubsystem()
{
   delete g_pEventQueue;
   delete g_pEventPolicy;
   RWLockDestroy(m_rwlockTemplateAccess);
}

/**
 * Reload event templates from database
 */
void ReloadEvents()
{
   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   m_eventObjects.clear();
   LoadEvents();
   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Delete event object from list
 */
void DeleteEventObjectFromList(UINT32 eventCode)
{
   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   m_eventObjects.remove(eventCode);
   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Post event to given event queue.
 *
 * @param queue event queue to post events to
 * @param eventCode Event code
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
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 * @param args event parameters
 */
static bool RealPostEvent(Queue *queue, UINT64 *eventId, UINT32 eventCode, UINT32 sourceId, UINT32 dciId,
                          const TCHAR *userTag, const char *format, const TCHAR **names, va_list args)
{
   EventTemplate *eventTemplate = NULL;
   bool success = false;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   eventTemplate = (EventTemplate *)m_eventObjects.get(eventCode);
   if (eventTemplate != NULL)
   {
      // Template found, create new event
      Event *evt = new Event(eventTemplate, sourceId, dciId, userTag, format, names, args);
      if (eventId != NULL)
         *eventId = evt->getId();

      // Add new event to queue
      queue->put(evt);

      eventTemplate->decRefCount();
      success = true;
   }

   RWLockUnlock(m_rwlockTemplateAccess);

   if (eventTemplate == NULL)
   {
      DbgPrintf(3, _T("RealPostEvent: event with code %d not defined"), eventCode);
   }
   return success;
}

/**
 * Post event to system event queue.
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
 *        h - MAC (hardware) address
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 */
bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, 0, NULL, format, NULL, args);
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
 *        h - MAC (hardware) address
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 */
bool NXCORE_EXPORTABLE PostDciEvent(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, dciId, NULL, format, NULL, args);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue and return ID of new event (0 in case of failure).
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
 *        h - MAC (hardware) address
 *        i - Object ID
 *        t - timestamp (time_t) as raw value (seconds since epoch)
 */
UINT64 NXCORE_EXPORTABLE PostEvent2(UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   UINT64 eventId;
   va_start(args, format);
   bool success = RealPostEvent(g_pEventQueue, &eventId, eventCode, sourceId, 0, NULL, format, NULL, args);
   va_end(args);
   return success ? eventId : 0;
}

/**
 * Post event to system event queue.
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
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, 0, NULL, format, names, args);
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
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, dciId, NULL, format, names, args);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue.
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
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 */
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, ...)
{
   va_list args;
   va_start(args, names);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, 0, userTag, format, names, args);
   va_end(args);
   return success;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
 * @param sourceId Event source object ID
 * @param parameters event parameters list
 */
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters)
{
   /*
   int count = parameters->size();
   if (count > 1023)
      count = 1023;

   char format[1024];
   memset(format, 's', count);
   format[count] = 0;

   const TCHAR *names[1024];
   const TCHAR *args[1024];
   for(int i = 0; i < count; i++)
   {
      names[i] = parameters->getKeyByIndex(i);
      args[i] = parameters->getValueByIndex(i);
   }

   return RealPostEvent(g_pEventQueue, eventCode, sourceId, NULL, format, names, args);
   */
   return false;
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
   /*
   int count = parameters->size();
   if (count > 1023)
      count = 1023;

   char format[1024];
   memset(format, 's', count);
   format[count] = 0;

   const TCHAR *names[1024];
   const TCHAR *args[1024];
   for(int i = 0; i < count; i++)
   {
      names[i] = parameters->getKeyByIndex(i);
      args[i] = parameters->getValueByIndex(i);
   }

   return RealPostEvent(g_pEventQueue, eventCode, sourceId, NULL, format, names, args);
   */
   return false;
}

/**
 * Post event to system event queue.
 *
 * @param eventCode Event code
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
 *        h - MAC (hardware) address
 *        i - Object ID
 * @param names names for parameters (NULL if parameters are unnamed)
 * @param args event parameters
 */
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(g_pEventQueue, NULL, eventCode, sourceId, 0, userTag, format, NULL, args);
   va_end(args);
   return success;
}

/**
 * Post event to given event queue.
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
 *        h - MAC (hardware) address
 *        i - Object ID
 */
bool NXCORE_EXPORTABLE PostEventEx(Queue *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...)
{
   va_list args;
   va_start(args, format);
   bool success = RealPostEvent(queue, NULL, eventCode, sourceId, 0, NULL, format, NULL, args);
   va_end(args);
   return success;
}

/**
 * Resend events from specific queue to system event queue
 */
void NXCORE_EXPORTABLE ResendEvents(Queue *queue)
{
   while(1)
   {
      void *pEvent = queue->get();
      if (pEvent == NULL)
         break;
      g_pEventQueue->put(pEvent);
   }
}

/**
 * Create NXMP record for event
 */
void CreateNXMPEventRecord(String &str, UINT32 eventCode)
{
   String strText, strDescr;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   // Find event template
   EventObject *o = m_eventObjects.get(eventCode);
   if (o != NULL)
   {
      str.appendFormattedString(_T("\t\t<event id=\"%d\">\n")
                                      _T("\t\t\t<name>%s</name>\n")
                                      _T("\t\t\t<code>%d</code>\n")
                                      _T("\t\t\t<description>%s</description>\n"),
                                      o->getCode(), (const TCHAR *)EscapeStringForXML2(o->getName()),
                                     (const TCHAR *)EscapeStringForXML2(o->getDescription()));
      if (eventCode & GROUP_FLAG_BIT)
      {
         str.appendFormattedString(_T("\t\t\t<members>\n"));
         for(int i = 0; i < ((EventGroup *) o)->getMemberCount(); i++)
         {
            str.appendFormattedString(_T("\t\t\t\t<code>%d</code>\n"),
                                     ((EventGroup *) o)->getMember(i));
         }
         str.appendFormattedString(_T("\t\t\t</members>\n"));
      }
      else
      {
         str.appendFormattedString(_T("\t\t\t<guid>%s</guid>\n")
                                _T("\t\t\t<severity>%d</severity>\n")
                                _T("\t\t\t<flags>%d</flags>\n")
                                _T("\t\t\t<message>%s</message>\n")
                                _T("\t\t</event>\n"),
                                (const TCHAR *)((EventTemplate *)o)->getGuid().toString(), o->getCode(),
                                ((EventTemplate *)o)->getSeverity(), ((EventTemplate *)o)->getFlags(),
                                (const TCHAR *)EscapeStringForXML2(((EventTemplate *)o)->getMessageTemplate()),
                                (const TCHAR *)EscapeStringForXML2(((EventTemplate *)o)->getDescription()));
      }
      o->decRefCount();
   }

   RWLockUnlock(m_rwlockTemplateAccess);
}

/**
 * Resolve event name
 */
bool EventNameFromCode(UINT32 eventCode, TCHAR *buffer)
{
   bool bRet = false;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);

   EventObject *o = m_eventObjects.get(eventCode);
   if (o != NULL)
   {
      _tcscpy(buffer, o->getName());
      o->decRefCount();
      bRet = true;
   }
   else
   {
      _tcscpy(buffer, _T("UNKNOWN_EVENT"));
   }

   RWLockUnlock(m_rwlockTemplateAccess);
   return bRet;
}

/**
 * Find event object by code - suitable for external call
 */
EventObject *FindEventObjectByCode(UINT32 eventCode)
{
   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);
   EventObject *o = m_eventObjects.get(eventCode);
   RWLockUnlock(m_rwlockTemplateAccess);
   return o;
}

/**
 * Find event object by name - suitable for external call
 */
EventObject *FindEventObjectByName(const TCHAR *name)
{
   EventObject *result = NULL;

   RWLockReadLock(m_rwlockTemplateAccess, INFINITE);
   Iterator<EventObject> *it = m_eventObjects.iterator();
   while(it->hasNext())
   {
      EventObject *o = it->next();
      if (!_tcscmp(o->getName(), name))
      {
         result = o;
         result->incRefCount();
         break;
      }
   }
   delete it;
   RWLockUnlock(m_rwlockTemplateAccess);
   return result;
}

/**
 * Translate event name to code
 * If event with given name does not exist, returns supplied default value
 */
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue)
{
   EventObject *o = FindEventObjectByName(name);
	if (o == NULL)
	   return defaultValue;
	UINT32 code = o->getCode();
	o->decRefCount();
	return code;
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
UINT32 UpdateEventObject(NXCPMessage *request, NXCPMessage *response, json_t **oldValue, json_t **newValue)
{
   TCHAR name[MAX_EVENT_NAME] = _T("");
   request->getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
   if (!IsValidObjectName(name, TRUE))
      return RCC_INVALID_OBJECT_NAME;

   EventObject *obj;
   UINT32 eventCode = request->getFieldAsUInt32(VID_EVENT_CODE);
   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);

   if (eventCode == 0)
   {
      if (request->getFieldAsBoolean(VID_IS_GROUP))
         obj = new EventGroup(request);
      else
         obj = new EventTemplate(request);
      m_eventObjects.set(obj->getCode(), obj);
      *oldValue = NULL;
   }
   else
   {
      obj = m_eventObjects.get(eventCode);
      if (obj == NULL)
      {
         RWLockUnlock(m_rwlockTemplateAccess);
         return RCC_INVALID_EVENT_CODE;
      }
      *oldValue = obj->toJson();
      obj->modifyFromMessage(request);
   }
   *newValue = obj->toJson();
   bool success = obj->saveToDatabase();
   if (success)
   {
      NXCPMessage nmsg;
      nmsg.setCode(CMD_EVENT_DB_UPDATE);
      nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_CHANGED);
      obj->fillMessage(&nmsg, VID_ELEMENT_LIST_BASE);
      EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);

      response->setField(VID_EVENT_CODE, obj->getCode());
   }

   RWLockUnlock(m_rwlockTemplateAccess);

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
UINT32 DeleteEventObject(UINT32 eventCode)
{
   UINT32 rcc = RCC_SUCCESS;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;

   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   if (eventCode & GROUP_FLAG_BIT)
   {
      if(m_eventObjects.contains(eventCode))
      {
         bool success = DBBegin(hdb);
         if (success)
         {
            hStmt = DBPrepare(hdb, _T("DELETE FROM event_groups WHERE id=?"));
            if (hStmt != NULL)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, eventCode);
               success = DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            else
               success = false;
            if (success)
            {
               hStmt = DBPrepare(hdb, _T("DELETE FROM event_group_members WHERE group_id=?"));
               if (hStmt != NULL)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, eventCode);
                  success = DBExecute(hStmt);
                  DBFreeStatement(hStmt);
               }
               else
                  success = false;
            }
            if (success)
               success = DBCommit(hdb);
            else
               DBRollback(hdb);
         }
         rcc = success ? RCC_SUCCESS : RCC_DB_FAILURE;
      }
   }
   else if (m_eventObjects.contains(eventCode))
   {
      hStmt = DBPrepare(hdb, _T("DELETE FROM event_cfg WHERE event_code=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, eventCode);
         rcc = DBExecute(hStmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
         DBFreeStatement(hStmt);
      }
   }
   else
      rcc = RCC_INVALID_EVENT_CODE;

   if (rcc == RCC_SUCCESS)
   {
      m_eventObjects.remove(eventCode);
      NXCPMessage nmsg;
      nmsg.setCode(CMD_EVENT_DB_UPDATE);
      nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_DELETED);
      nmsg.setField(VID_EVENT_CODE, eventCode);
      EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);
   }
   RWLockUnlock(m_rwlockTemplateAccess);

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get event configuration
 */
void GetEventConfiguration(NXCPMessage *msg)
{
   RWLockWriteLock(m_rwlockTemplateAccess, INFINITE);
   UINT32 base = VID_ELEMENT_LIST_BASE;
   msg->setField(VID_NUM_EVENTS, m_eventObjects.size());
   Iterator<EventObject> *it = m_eventObjects.iterator();
   while(it->hasNext())
   {
      EventObject *o = it->next();
      o->fillMessage(msg, base);
      base += 10;
   }
   RWLockUnlock(m_rwlockTemplateAccess);
}
